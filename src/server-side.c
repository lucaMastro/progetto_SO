#include <sys/socket.h>       /*  socket definitions        */
#include <sys/types.h>        /*  socket types              */
#include <arpa/inet.h>        /*  inet (3) funtions         */
#include <unistd.h>           /*  misc. UNIX functions      */

#include "helper.h"           /*  our own helper functions  */
#include "helper-server.h"
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <sys/ipc.h>

#define MAX_LINES 1000

int ParseCmdLine(int argc, char *argv[], char **szPort);

message **message_list;

void handler_sigint(){
	//free(message_list);
	exit(EXIT_SUCCESS);
}

void handler_sigpipe(){
	printf("\ncatched SIGPIPE\njust exiting\n");
	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]){
	//porta come 1 parametro. aggiustare parsing
	int sock_ds, acc_sock, ret, on = 1, i;
	char *port, *client_usrname;
	struct sockaddr_in server_addr, client_addr;
	int server_len, client_len, port_num, operation;
	ssize_t read_ch;
	int *position; //puntatore alla posizione minima in cui salvare un messaggio
	int *last; //puntatore alla prossima posizione in cui memorizzare un messaggio.
	message *new_mess;
       
	int my_messages[MAX_NUM_MEX] = { [0 ... MAX_NUM_MEX - 1] = -1}; //bitmask
	int my_new_messages[MAX_NUM_MEX] = { [0 ... MAX_NUM_MEX - 1] = -1}; //bitmask
	int my_send_messages[MAX_NUM_MEX] = { [0 ... MAX_NUM_MEX -1] = 0};
	int *server; //bitmask for messages of server. it must be shared
	int sem_write; //write sem's: synchronizing write of mess
	

	printf("\e[1;1H\e[2J");	
	ParseCmdLine(argc, argv, &port);
	port_num = strtol(port, &port, 0); 
	

	//INITIALIZING PARAMS
	position = (int *) mmap(NULL, sizeof(position), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
	if (position == (int*) -1){
		perror("error initializing mex list");
		exit(EXIT_FAILURE);		
	}

	*position = 0;

	server = (int *) mmap(NULL, sizeof(int) * MAX_NUM_MEX, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
	if (position == (int*) -1){
		perror("error initializing mex list");
		exit(EXIT_FAILURE);		
	}
	for (i = 0; i < MAX_NUM_MEX; i++)
		server[i] = 0;

	last = (int *) mmap(NULL, sizeof(position), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
	if (last == (int*) -1){
		perror("error initializing mex list");
		exit(EXIT_FAILURE);		
	}
	*last = 0;
	
	sem_write = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666 );
	if (sem_write == -1){
		perror("error initializing semaphore");
		exit(EXIT_FAILURE);
	}
	if (semctl(sem_write, 0, SETVAL, 1) == -1){
		perror("error secmtl");
		exit(EXIT_FAILURE);
	}



	message_list = (message**) mmap(NULL, sizeof(*message_list) * MAX_NUM_MEX, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
	if (message_list == (message**) -1){
		perror("error initializing mex list");
		exit(EXIT_FAILURE);
	}

	inizializza_server(message_list);
	printf("struttura per gestione dei messaggi inizializzata\n");
	
	sock_ds = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_ds == -1){
		perror("error");
		printf("errore creazione socket\n");
		exit(-1);
	}

	if (setsockopt(sock_ds, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1){
		perror("impossibile settare opzione");
		exit(EXIT_FAILURE);		
	}
	
	bzero((char*) &server_addr, sizeof(server_addr));
	
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port_num);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_len = sizeof(server_addr);
	
	ret = bind(sock_ds, (struct sockaddr*) &server_addr, server_len);
	if (ret == -1){
		perror("error binding server address");
		exit(-1);
	}
	
	ret = listen(sock_ds, LISTENQ); 
	if (ret == -1){
		perror("error listen");
		exit(-1);
	}	
	printf("Server in ascolto sulla porta %d.\n", port_num);
	
	//make a new hidden folder, if not exist
	if (mkdir(".db", S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO) == -1 && errno != EEXIST){

	//if (mkdir("db", 0040000) == -1 && errno != EEXIST){
		perror("error creating < db >");
		exit(EXIT_FAILURE);
	}
	

new_accept:
	acc_sock = accept(sock_ds, (struct sockaddr*) &client_addr, &client_len); 
//	printf("connessione avvenuta da parte dell'indirizzo %d\n", client_addr.sin_addr.s_addr);
	printf("\nconnessione avvenuta\n");

	if ( !fork() ){ //fork == 0, im a child;
		
		if (close(sock_ds) == -1){
			perror("error in closing child socket");
			exit(-1);
		}
	
/*		printf("\n\nRECEIVING TEST MESS:\n");
		test_server_func(acc_sock);	*/
	
reg_log:
		managing_usr_registration_login(acc_sock, &client_usrname);	
		printf("\n\nlogin effettuato da: %s\n\n", client_usrname);

		ret = managing_usr_menu(acc_sock, message_list, position, last, client_usrname, my_messages, my_new_messages, server, sem_write, client_usrname);
		if (ret == 0)
			goto reg_log;
	}
	else{
		if (close(acc_sock) == -1){
			perror("error in closing parent socket");
			exit(-1);
		}
		goto new_accept;
	}
}



int ParseCmdLine(int argc, char *argv[], char **szPort)
{
    int n = 1;

    while ( n < argc )
	{
		if ( !strncmp(argv[n], "-p", 2) || !strncmp(argv[n], "-P", 2) )
			*szPort = argv[++n];
		else 
			if ( !strncmp(argv[n], "-h", 2) || !strncmp(argv[n], "-H", 2) )
			{
				printf("Sintassi:\n\n");
	    			printf("    server -p (porta) [-h]\n\n");
				exit(EXIT_SUCCESS);
			}
		++n;
    }
	if (argc==1){
	       	printf("Sintassi:\n\n");
		printf("    server -p (porta) [-h]\n\n");
	  	exit(EXIT_SUCCESS);
	}
    return 0;
}


