//#include <unistd.h>             /*  for ssize_t data type  */

#define MAX_NUM_MEX 1024 //memorizzerò al massimo 1024 messaggi
#define LISTENQ        (1024)   /*  Backlog for listen()   */
#define MAX_USR_LEN 20
#define MAX_PW_LEN 10
#define MAX_OBJ_LEN 20
#define MAX_MESS_LEN 100


typedef struct MESSAGE{
        char *usr_destination;
       	char *usr_sender;
       	char *object;
        char *text;
        int *is_new;
	int *position; //message's position
} message;

void stampa_messaggio(message *mess);
void read_int(int sock, int *num, int line);
void write_int(int sock, int num, int line);
void read_string(int sock, char **string, int line);
void write_string(int sock, char *string, int line);
void read_mex(int sock, message **mex_list, int *position, int semid);
void write_mex(int sock, message *mex);
void error(int line);
void send_mex(int sock, message *mex);
int get_mex(int sock, message *mex); //store the sent mex in *mex
