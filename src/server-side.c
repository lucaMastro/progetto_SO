#include <sys/socket.h>       /*  socket definitions        */
#include <sys/types.h>        /*  socket types              */
#include <arpa/inet.h>        /*  inet (3) funtions         */
#include <unistd.h>           /*  misc. UNIX functions      */

#include "../lib/helper.h"           /*  our own helper functions  */
#include "../lib/helper-server.h"
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <pthread.h>
#include <fcntl.h>

#define MAX_LINES 1000

int ParseCmdLine(int argc, char *argv[], char **szPort);
void *thread_func(void *sock_ds);


/*VARIABILI GLOBALI E TLS*/

message **message_list;
int position = 0; //puntatore alla posizione minima in cui salvare un messaggio
int fileid, last = 0; //puntatore alla prossima posizione in cui memorizzare un messaggio.
int sem_write; //write sem's: synchronizing write of mess, deleting
int server[MAX_NUM_MEX] = { [0 ... MAX_NUM_MEX - 1] = 0}; //bitmask for messages of server. it must be shared

//__thread int acc_sock;
__thread int my_messages[MAX_NUM_MEX] = { [0 ... MAX_NUM_MEX - 1] = -1}; //bitmask
__thread int my_new_messages[MAX_NUM_MEX] = { [0 ... MAX_NUM_MEX - 1] = -1}; //bitmask

/***********************************************************************************************************/



void handler_sigint(){
	int i;
	printf("last %d\n", last);
//	while(getchar() != "\n");
	for (i = 0; i < MAX_NUM_MEX; i++){
		if (i < last){
			printf("gone into codi\n");
			free(message_list[i] -> usr_destination);
			free(message_list[i] -> usr_sender);
			free(message_list[i] -> object);
			free(message_list[i] -> text);
		}
		free(message_list[i]);
	}
	printf("ok\n");
	free(message_list);
	exit(EXIT_SUCCESS);
}

void handler_sigpipe(){
	printf("\ncatched SIGPIPE\njust exiting\n");
	exit(EXIT_SUCCESS);
}


int main(int argc, char *argv[]){
	//porta come 1 parametro. aggiustare parsing
	int sock_ds, *acc_sock, ret, on = 1, i;
	char *port, *client_usrname;
	struct sockaddr_in server_addr, client_addr;
	int server_len, client_len, port_num, operation;
	ssize_t read_ch;
	struct sembuf sops;
	pthread_t tid;

	printf("\e[1;1H\e[2J");	
	ParseCmdLine(argc, argv, &port);
	port_num = strtol(port, &port, 0); 
	

	signal(SIGINT, handler_sigint);

	//INITIALIZING PARAMS
	
	sem_write = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666 );
	if (sem_write == -1){
		perror("error initializing semaphore");
		exit(EXIT_FAILURE);
	}
	if (semctl(sem_write, 0, SETVAL, 1) == -1){
		perror("error secmtl");
		exit(EXIT_FAILURE);
	}
	/*message_list = malloc(sizeof(message) * MAX_NUM_MEX);
	if (message_list == NULL){
		perror("error initializing mex list");
		exit(EXIT_FAILURE);
	}*/

	message_list = inizializza_server();
	printf("struttura per gestione dei messaggi inizializzata\n");
	
	sock_ds = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_ds == -1){
		perror("error");
		printf("errore creazione socket\n");
		exit(-1);
	}

	//FORSE NON SERVE
	if (setsockopt(sock_ds, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1){
		perror("impossibile settare opzione");
		exit(EXIT_FAILURE);		
	}//end
	
	bzero((char*) &server_addr, sizeof(server_addr));
	
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port_num);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_len = sizeof(server_addr);
	
	client_len = sizeof(struct sockaddr_in);

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
	//creating the list of users
        fileid = open(".db/list.txt", O_CREAT|O_TRUNC, 0666);
        if (fileid == -1)
                error(142);
        else
                close(fileid);
	

	sops.sem_flg = 0;
	while(1){
		
		acc_sock = (int *) malloc(sizeof(int));
		if (acc_sock == NULL)
			error(169);

		printf("main in attesa di accept....\n\n");
		*acc_sock = accept(sock_ds, (struct sockaddr*) &client_addr, &client_len); 
		if (acc_sock < 0){
			perror("error accepting\n");
			exit(EXIT_FAILURE);
		} 
	
//                printf("\n");
		printf("\nconnessione avvenuta da parte dell'indirizzo %s\n", inet_ntoa(client_addr.sin_addr));
	//	printf("\nconnessione avvenuta\n");
	
		if (pthread_create(&tid, NULL, thread_func, (void*) acc_sock)){
			perror("impossibile creare thread");
			exit(EXIT_FAILURE);
		}
		if (pthread_detach(tid)){
			perror("error detatching");
			exit(EXIT_FAILURE);
		}
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

void *thread_func(void *sock_ds){
			
	char *client_usrname;
	struct sembuf sops;
	int acc_sock, is_usr_freed;


	acc_sock = *((int *)sock_ds);	

	
/*	if ((client_usrname = malloc(sizeof(char) * (MAX_USR_LEN + 1))) == NULL){
		perror("impossibile allocare usrname");
		exit(EXIT_FAILURE);
	}*/

	while(1){	
		//bzero(client_usrname, MAX_USR_LEN + 1);
		is_usr_freed = 0;
                printf("....................................................................................\n");
                printf("...............................USR_REGISTRATION_LOGIN...............................\n");
		if (!managing_usr_registration_login(acc_sock, &client_usrname))
			break;	

		is_usr_freed = 1;
		if (managing_usr_menu(acc_sock, message_list, &position, &last, client_usrname, my_messages, my_new_messages, server, sem_write))
			break;
//                printf("....................................................................................\n");
	}
//	printf("fine\n");
//	pthread_exit((void*) 0);
	free(sock_ds);
}
