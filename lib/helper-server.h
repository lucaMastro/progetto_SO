message **inizializza_server();
int ricevi_messaggio(int sock_ds, message **mess_list, int *position, int *last, int *server, int sem_write, int *my_mex, int *my_new_mex, char *client_usr, int flag); 

int gestore_letture(int acc_sock, message **mess_list, int *last, char *usr, int flag, int *my_mex, int *my_new_mex, int *server, int sem_write, int *position);
int managing_usr_menu(int acc_sock, message **message_list, int *position, int *last, char *usr, int *my_mex, int *my_new_mex, int *server, int sem_write);
int managing_usr_registration_login(int acc_sock, char **usr); 
void close_server(int acc_sock, char *usr, int flag);
int update_system_state(int *my_mex, int *my_new_mex, message **mex_list, char *usr, int position, int *server);
void update_position(int *position, int *server, int last);
void stampa_bitmask(int *bitmask, int last);
int gestore_eliminazioni(int acc_sock, char *usr, message **mex_list, int *my_mex, int *my_new_mex, int *server, int sem_write, int *position, int *last);
void update_last(int *server, int *last);
int delete_user(int acc_sock, char *usr, message **mex_list, int *server, int *my_mex, int *last, int *position, int sem_write);
int check_destination(char **usr_destination, char **dest);
void server_test(int acc_sock, message **mex_list, int *position, int semid);
int mng_cambio_pass(int acc_sock, char *my_usr);
void log_out(char *usr);
