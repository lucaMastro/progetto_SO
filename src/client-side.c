#include <sys/socket.h>       /*  soceet definitions        */
#include <sys/types.h>        /*  socket types              */
#include <arpa/inet.h>        /*  inet (3) funtions         */
#include <unistd.h>           /*  misc. UNIX functions      */

#include "../lib/helper.h"           /*  our own helper functions  */
#include "../lib/helper-client.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <netdb.h>
#include <errno.h>
#define fflush(stdin) while(getchar() != '\n')

int ParseCmdLine(int argc, char *argv[], char **szAddress, char **szPort);
char *my_usrname;
int sock_ds;


void handler_sigpipe(){
	printf("\ncatched SIGPIPE\nJust exiting\n");
	exit(EXIT_SUCCESS);
}


int main(int argc, char *argv[]){

	int ret, sock_ds;
        struct sockaddr_in server_addr;
        int server_len;
 	char *port, *addr; 
	struct hostent *he;

	signal(SIGPIPE, handler_sigpipe);
	signal(SIGINT, SIG_IGN);

	my_usrname = malloc(sizeof(char) * MAX_USR_LEN);
	if (my_usrname == NULL){
		perror("error initializing usrname");
		exit(EXIT_FAILURE);
	}

	ParseCmdLine(argc, argv, &addr, &port);

	he = NULL;
        
	sock_ds = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_ds == -1){
                perror("error");
                printf("errore creazione socket\n");
                exit(-1);
        }
	
        bzero((char*) &server_addr, sizeof(server_addr));
 
 	server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(strtol(port, &port, 0));
	
	if (inet_aton(addr, &server_addr.sin_addr) == -1){
		printf("ip non valido. provo a risolverlo...\n");
		if ((he = gethostbyname(argv[1])) == NULL){
			printf("fallito.\n");
  			exit(EXIT_FAILURE);
		}
		printf("riuscito.\n\n");
		server_addr.sin_addr = *((struct in_addr *)he->h_addr);
   	 }
	

	server_len = sizeof(server_addr);

        ret = connect(sock_ds, (struct sockaddr*) &server_addr, server_len);
        if (ret == -1){
                perror("error");
                printf("errore connect\n");
                exit(-1);
        }
	
	printf("\e[1;1H\e[2J");	
	printf("Connessione al server riuscita.\nPremi INVIO per continuare.\n");
	fflush(stdin);

reg_log:
	usr_registration_login(sock_ds, &my_usrname);
	
	printf("\e[1;1H\e[2J");	
	ret = usr_menu(sock_ds, my_usrname);
	if (ret == 0){
		bzero(my_usrname, MAX_USR_LEN);
		goto reg_log;
	}
}



int ParseCmdLine(int argc, char *argv[], char **szAddress, char **szPort) {

    int n = 1;

    while ( n < argc )
	{
		if ( !strncmp(argv[n], "-a", 2) || !strncmp(argv[n], "-A", 2) )
		{
		    *szAddress = argv[++n];
		}
		else 
			if ( !strncmp(argv[n], "-p", 2) || !strncmp(argv[n], "-P", 2) )
			{
			    *szPort = argv[++n];
			}
			else
				if ( !strncmp(argv[n], "-h", 2) || !strncmp(argv[n], "-H", 2) )
				{
		    		printf("Sintassi:\n\n");
			    	printf("    client -a (indirizzo remoto) -p (porta remota) [-h].\n\n");
			    	exit(EXIT_SUCCESS);
				}
		++n;
    }
	if (argc==1)
	{
	    printf("Sintassi:\n\n");
		printf("    client -a (indirizzo remoto) -p (porta remota) [-h].\n\n");
	    exit(EXIT_SUCCESS);
	}
    return 0;
}


