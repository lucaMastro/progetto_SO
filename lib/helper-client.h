int leggi_messaggi(int sock_ds, char *my_usrname, int flag);
void invia_messaggio(int sock_ds, char *sender);
int usr_menu(int sock_ds, char *my_usrname);
void usr_registration_login(int sock_ds, char **usr);
void close_client(int sock_ds);
int cancella_messaggio(int sock_ds, int code);
int write_back(int sock_ds, char *object, char *my_usr, char *usr_dest );
int delete_me(int sock_ds);
void client_test(int sock_ds);
void cambia_pass(int sock_ds);


