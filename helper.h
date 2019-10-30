//#include <unistd.h>             /*  for ssize_t data type  */

#define MAX_NUM_MEX 1024 //memorizzerò al massimo 1024 messaggi
#define LISTENQ        (1024)   /*  Backlog for listen()   */
#define MAX_USR_LEN 20
#define MAX_PW_LEN 10
#define MAX_OBJ_LEN 20
#define MAX_MESS_LEN 100

typedef struct MESSAGE message;

/*  Function declarations  */

//server side:
void inizializza_server(message **mex_list);
int ricevi_messaggio(int sock_ds, message **mess_list, int *position, int *last, int *server, int sem_write, int *my_mex, int *my_new_mex, char *client_usr, int flag); 

int gestore_letture(int acc_sock, message **mess_list, int *last, char *usr, int flag, int *my_mex, int *my_new_mex, int *server, int sem_write, int *position);
int managing_usr_menu(int acc_sock, message **message_list, int *position, int *last, char *usr, int *my_mex, int *my_new_mex, int *server, int sem_write, char *client_usrname);
void managing_usr_registration_login(int acc_sock, char **usr); 
void close_server(int acc_sock, char *usr);
int update_system_state(int *my_mex, int *my_new_mex, message **mex_list, char *usr, int position, int *server);
void update_position(int *position, int *server, int last);
void stampa_bitmask(int *bitmask, int last);
int gestore_eliminazioni(int acc_sock, char *usr, message **mex_list, int *my_mex, int *my_new_mex, int *server, int sem_write, int *position, int *last);
void update_last(int *server, int *last);
int delete_user(int acc_sock, char *usr, message **mex_list, int *server, int *my_mex, int *last, int *position, int sem_write);
int check_destination(char **usr_destination, char **dest);
void server_test(int acc_sock, message **mex_list, int *position, int semid);
void mng_cambio_pass(int acc_sock, char *my_usr);

//client side:
int leggi_messaggi(int sock_ds, char *my_usrname, int flag);
void invia_messaggio(int sock_ds, char *sender);
int usr_menu(int sock_ds, char *my_usrname);
void usr_registration_login(int sock_ds, char **usr);
void log_out(char *usr);
void close_client(int sock_ds);
int cancella_messaggio(int sock_ds, int code);
int write_back(int sock_ds, char *object, char *my_usr, char *usr_dest );
int delete_me(int sock_ds);
void client_test(int sock_ds);
void cambia_pass(int sock_ds);

//both side:
void stampa_messaggio(message *mess);
void read_int(int sock, int *num, int line);
void write_int(int sock, int num, int line);
void read_string(int sock, char **string, int line);
void write_string(int sock, char *string, int line);
void read_mex(int sock, message **mex_list, int *position, int semid);
void write_mex(int sock, message *mex);
