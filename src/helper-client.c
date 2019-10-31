#include "helper.h"
#include "helper-client.h"
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define fflush(stdin) while(getchar() != '\n')
#define audit printf("ok\n")

int leggi_messaggi(int sock_ds, char *my_usrname, int flag){ //if flag == 1, only new messages will be print
        int found, again = 1, isnew, len_send, len_obj, len_text, pos, isfirst = 1, wb;
        char *sender, *object, *text;
        message *mex;
        if ((mex = malloc(sizeof(mex))) == NULL)
                error(83);

        if ((mex -> usr_destination = malloc(sizeof(char) *MAX_USR_LEN)) == NULL)
                error(86);

        mex -> usr_sender = malloc(sizeof(char) *MAX_USR_LEN);
        mex -> object = malloc(sizeof(char) *MAX_OBJ_LEN);
        mex -> text = malloc(sizeof(char) *MAX_MESS_LEN);
        mex -> is_new = malloc(sizeof(int));
        mex -> position = malloc(sizeof(int));

start:
        read_int(sock_ds, &found, 95);
        while(found && again){
                isfirst = 0;

                /*READING IS_NEW*/
                read_int(sock_ds, &isnew, 101);
                if (!isnew && flag)
                        goto start;

                /*READING SENDER*/
                read_string(sock_ds, &sender, 108);
                /*READING OBJECT*/
                read_string(sock_ds, &object, 112);
                /*READING TEXT*/
                read_string(sock_ds, &text, 116);
                /*READING POSITION*/
                read_int(sock_ds, &pos, 119);
                mex -> usr_sender = sender;
                mex -> object = object;
                mex -> text = text;
                mex -> usr_destination = my_usrname;
                *(mex -> is_new) = isnew;
                *(mex -> position) = pos;

                /*PRINTING MESSAGE*/
                stampa_messaggio(mex);

                /*CHECKING IF USR WANT TO WRITE BACK TO LAST READ MESS: only if
                 *                                                               strlen("RE: <object>") <= MAX_OBJ_LEN          */
                if (strlen(object) <= MAX_OBJ_LEN - 4){
                        printf("vuoi rispondere al messaggio? (1 = si, qualsiasi altro tasto = no)\n");

                        if (scanf("%d", &wb) == -1 && errno != EINTR)
                                error(128);
                        fflush(stdin);
                        write_int(sock_ds, wb, 154);
                        if (wb == 1)
                                write_back(sock_ds, object, my_usrname, sender);
                }

usr_will:
                /*CHECKING USR'S WILL*/
                printf("cercare un altro messaggio? (1 = si, 0 = no)\npuoi anche inserire un numero negativo per cancellare l'ultimo messaggio letto.\n");

                if (scanf("%d", &again) == -1 && errno != EINTR)
                        error(140);

                fflush(stdin);

                if (again > 1){
                        printf("valore non accettabile. riprova.\n\n");
                        goto usr_will;
                }
                //send user' will
                write_int(sock_ds, again, 183);

                if (again < 0){
                        //must eliminate mess:
                        cancella_messaggio(sock_ds, pos);
                        goto usr_will;
                }

                else if (again == 1) // || again == 0){
                        //updating found
                        read_int(sock_ds, &found, 156);
        }

        if (isfirst && !flag)
                printf("non ci sono messaggi per te\n");
        else if (isfirst && flag)
                printf("non ci sono nuovi messaggi per te\n");
        else{//sono entrato nel while
                if (flag){//solo messaggi new
                        if (again)
                                printf("non ci sono altri nuovi mess.\n");
                }
                else{
                        if (again)
                                printf("non ci sono altri messaggi\n");
                }
        }
        free(mex);
        return 0;

}

void invia_messaggio(int acc_sock, char *sender){
        char *destination, *obj, *mes;
        int len_dest, len_send, len_obj, len_mess, ret;

restart:
        //GETTING DATA AND THEIR LEN
        printf("inserisci l'username del destinatario (max %d caratteri):\n", MAX_USR_LEN);
        if (scanf("%ms", &destination) == -1 && errno != EINTR)
                error(470);

        fflush(stdin);

        len_dest = strlen(destination);
        if (len_dest > MAX_USR_LEN){
                printf("usrname destinatario inserito troppo lungo. per favore riprova.\n\n");
                goto restart;
        }

        //its len was checked at login/registration act
        len_send = strlen(sender);

        printf("inserisci l'oggetto del messaggio (max %d caratteri):\n", MAX_OBJ_LEN);
        if (scanf("%m[^\n]", &obj) == -1 && errno != EINTR)
                error(485);

        fflush(stdin);

        len_obj = strlen(obj);
        if (len_obj > MAX_OBJ_LEN){
                printf("oggetto inserito troppo lungo. per favore riprova.\n\n");
                goto restart;
        }

        printf("inserisci il testo del messaggio: (max %d caratteri):\n", MAX_MESS_LEN);
        if (scanf("%m[^\n]", &mes) == -1 && errno != EINTR)
                error(497);

        fflush(stdin);

        len_mess = strlen(mes);
        if (len_mess > MAX_MESS_LEN){
                printf("messaggio inserito troppo lungo. per favore riprova.\n\n");
                goto restart;
        }

        //SENDING DATA

        //invio destinatario
        write_string(acc_sock, destination, 633);

        //reading response if destination exists:
        read_int(acc_sock, &ret, 539);

        if (!ret){
                printf("destinatario non esiste. per favore riprova:\n\n");
                goto restart;
        }

        //invio mio usrname
        write_string(acc_sock, sender, 646);

        //invio oggetto
        write_string(acc_sock, obj, 649);

        //invio mess
        write_string(acc_sock, mes, 652);

        printf("\n\ninvio del messaggio avvenuto con successo. attendo conferma ricezione...\n");

        read_int(acc_sock, &ret, 567);

        if (ret < 0)
                printf("ricezione fallita. riprovare.\n\n\n");
        else
                printf("messaggio ricevuto correttamente\n\n\n");

}

int usr_menu(int sock_ds, char *my_usrname){

        int operation, new_mex_avaiable, check_upd = 1, code = -1;

select_operation:

        printf("\n\nlogin effettuato come: %s\n\n", my_usrname);

        if (check_upd){
                //Reading if there are new mex

                read_int(sock_ds, &new_mex_avaiable, 1079);
                check_upd = 0;
        }

        if (new_mex_avaiable)
                printf("esistono messaggi non letti\n\n");
        else
                printf("non esistono messaggi non letti\n\n");

        printf(".....................................................................................\n");
        printf(".....................................................................................\n");
        printf("..............................|      MENU UTENTE      |..............................\n");
        printf(".....................................................................................\n");
        printf(".....................................................................................\n\n");

        printf(" ______ ________ ________ _____Operazioni Disponibili_____ ________ ________ ______ _\n");
        printf("|                                                                                    |\n");
        printf("|                                                                    [LOG OUT: 0]    |\n");
        printf("|                                                                                    |\n");
        printf("|   OPERAZIONE 1 : Lettura di tutti i messaggi ricevuti dall'utente                  |\n");
        printf("|   OPERAZIONE 2 : Lettura dei soli messaggi non letti                               |\n");
        printf("|   OPERAZIONE 3 : Spedizione di un nuovo messaggio a uno qualunque degli utenti     |\n|\t\t   del sistema                                                       |\n");
        printf("|   OPERAZIONE 4 : Cancellare messaggi ricevuti dall'utente                          |\n");
        printf("|   OPERAZIONE 5 : Aggiornare lo stato del sistema (cerca se ci sono nuovi messaggi) |\n");
        printf("|   OPERAZIONE 6 : Chiudere l'applicazione                                           |\n");
        printf("|   OPERAZIONE 7 : Cancellare l'account                                              |\n");
        printf("|   OPERAZIONE 8 : Cambia password                                                   |\n");
        printf("|____ ________ ________ ________ ________ ________ ________ ________ ________ _____ _|\n\n");

        printf("quale operazione vuoi svolgere?\n");

        if (scanf("%d", &operation) == -1 && errno != EINTR)
                error(995);

        fflush(stdin);

        if (operation > 8 || operation < 0){
                printf("operazione %d non valida\n", operation);
                printf("premi un tasto per riprovare:");
                fflush(stdin);
                printf("\e[1;1H\e[2J");
                goto select_operation;
        }

        //invio il segnale per preparare il server alla gestione dell'operazione:
        write_int(sock_ds, operation, 1162);

        printf("operazione accettata.\n\n");
        switch (operation){
                case 0:
                        printf("tornerai alla schermata di login fra 3 secondi. arrivederci :)\n");
                        sleep(3);
                        return 0;
                        break;
                case 1:
                        leggi_messaggi(sock_ds, my_usrname, 0);
                        check_upd = 1;
//                      fflush(stdin);
                        break;
                case 2:
                        leggi_messaggi(sock_ds, my_usrname, 1);
                        check_upd = 1;
                        break;

                case 3:
                        invia_messaggio(sock_ds, my_usrname);
                        break;
                case 4:
                        cancella_messaggio(sock_ds, code);
//                      check_upd = 1;
                        break;
                case 5:
                        read_int(sock_ds, &new_mex_avaiable, 1147);
                        printf("aggiornamento avvenuto con successo.\n");
                        break;
                case 6:
                        free(my_usrname);
                        close_client(sock_ds);
                case 7:
                        delete_me(sock_ds);
                        free(my_usrname);
                        close_client(sock_ds);
                case 8:
                        cambia_pass(sock_ds);
                        break;
                default:
                        break;
        }
        printf("premi INVIO per continuare: ");
        fflush(stdin);
        printf("\e[1;1H\e[2J");

        goto select_operation;

}

void usr_registration_login(int sock_ds, char **usr){
//      printf("not implemented yet :)\n");

        int ret, operation, len;
        char *pw;

portal:
        printf("\e[1;1H\e[2J");

        printf("......................................................................................\n");
        printf("......................................................................................\n");
        printf("................__ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __..............\n");
        printf("...............|                                                        |.............\n");
        printf("...............|      PORTALE DI ACCESSO: SISTEMA SCAMBIO MESSAGGI      |.............\n");
        printf("...............|__ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __|.............\n");
        printf("......................................................................................\n");
        printf("......................................................................................\n");
        printf(".........................   Benvenuto! Cosa vuoi fare?   .............................\n");
        printf("......................................................................................\n");
        printf("......................................................................................\n");
        printf("..................__ ___ __...........__ ___ __...........__ ___ __...................\n");
        printf(".................|         |.........|         |.........|         |..................\n");
        printf(".................|   EXIT  |.........| SIGN IN |.........|  LOGIN  |..................\n");
        printf(".................|   [0]   |.........|   [1]   |.........|   [2]   |..................\n");
        printf(".................|__ ___ __|.........|__ ___ __|.........|__ ___ __|..................\n");
        printf("......................................................................................\n");
        printf("......................................................................................\n");
        printf("......................................................................................\n\n\n");

        if (scanf("%d", &operation) == -1 && errno != EINTR)
                error(732);

        fflush(stdin);

        if (operation < 0 || operation > 2){
                printf("operazione non valida. riprova:\n\n");
                goto portal;
                exit(EXIT_FAILURE);
        }


        //sending the selected operation:
        if (write(sock_ds, &operation, sizeof(operation)) == -1)
                error(745);

        if (operation == 0){
                free(*usr);
                close_client(sock_ds);
        }

        //same structure for registration and login
get_usr:
        printf("inserisci username (max %d caratteri):\n", MAX_USR_LEN);
        if (scanf("%s", *usr) == -1 && errno != EINTR)
                error(756);

        fflush(stdin);

        len = strlen(*usr);
        if (len > MAX_USR_LEN){
                printf("usrname too long. try again:\n");
                goto get_usr;
        }
        if (write(sock_ds, &len, sizeof(len)) == -1)
                error(765);

        if (write(sock_ds, *usr, len) == -1)
                error(768);
pw_get:
        printf("inserisci password (max %d caratteri):\n", MAX_PW_LEN);
        if (scanf("%ms", &pw) == -1 && errno != EINTR)
                error(772);

        len = strlen(pw);
        if (len > MAX_PW_LEN){
                printf("password too long. try again:\n");
                goto pw_get;
        }
        fflush(stdin);

        if (write(sock_ds, &len, sizeof(len)) == -1)
                error(784);

        if (write(sock_ds, pw, len) == -1)
                error(785);

        //reading response:
        read_int(sock_ds, &ret, 879);

        switch (ret){
                case 0: //it was allright
                        if (operation == 1){ //it was a registration:
                                printf("registrazione avvenuta.\npremi un tasto per continuare: ");
                                fflush(stdin);
                                goto portal;
                                break;
                        }
                        else
                                return;

                case 1:
                        //file già esistente:
                        printf("username già presente. per favore riprova.\npremi INVIO per continuare.\n");
                        while(getchar() != '\n') {};
                        goto portal;
                        break;
                case 2:
                        //uncorrect pw or username in login
                        printf("username o password errati. per favore riprova (premi INVIO per continuare).");
                        while(getchar() != '\n') {};
                        goto portal;
                        break;
                case 3:
                        //usr already logged
                        printf("username già loggato. per favore riprova (premi INVIO per continuare).\n");
                        while(getchar() != '\n') {};
                        goto portal;
                        break;
                case 4:
                        printf("password o usrname troppo lunghi, per favore riprova.\npremi INVIO per continuare\n");
                        //while(getchar() != '\n') {};
                        fflush(stdin);
                        goto portal;
                        break;

                default:
                        exit(EXIT_FAILURE);
                        break;
        }
}

void close_client(int sock_ds){

        close(sock_ds);
        printf("l'applicazione si chiuderà in 3 secondi. arrivederci :)\n");
        if (!sleep(3)){
                printf("\e[1;1H\e[2J");
                exit(EXIT_SUCCESS);
        }
}

int cancella_messaggio(int sock_ds, int mode){//mode < 0 quando è chiamata separatamente a leggi_messaggi
        int is_mine, ret = 0, again, fine, code;

        /*SCRIVO MODE*/
        write_int(sock_ds, mode, 326);

        if (mode >= 0)
                code = mode;

        else{
                another_code:
                printf("dammi il codice del messaggio da eliminare.\nNOTA: se non hai messaggi associati al codice inserito, l'operazione verrà interrotta.\n(inserire un numero negativo per annullare)\n");
                if (scanf("%d", &code) == -1 && errno != EINTR)
                        error(308);

                fflush(stdin);

                /*SCRIVO CODE*/
                write_int(sock_ds, code, 328);
                if (code < 0){
                        printf("operazione annullata con successo\n");
                        goto exit_lab;
                }
        }

        /*READ IF THE CODE IS ACCEPTED*/
        read_int(sock_ds, &is_mine, 332);

        if (is_mine == 1){
                printf("codice accettato. attendi conferma eliminazione\n");

                /*READ IF ELIMINATION WAS OK*/
                read_int(sock_ds, &ret, 338);

                if (ret == 1)
                        printf("messaggio eliminato correttamente\n");
                else{
                        printf("errore nell'eliminazione del messaggio. termino.\n\n");
                        exit(EXIT_FAILURE);
                }
        }
        else{
                if (mode >= 0)
                        printf("messaggio già eliminato\n");
                else{
                        printf("errore: non hai messaggi ricevuti associati al codice inserito.per favore riporva.\n\n");
                        goto another_code;
                }
        }

        /*CHECKING USR'S WILL. eseguire solo se mode < 0*/
        if (mode < 0){
usr_will:
                printf("eliminare un altro messaggio? (0 = no, 1 = si)\n");
                if (scanf("%d", &again) == -1 && errno != EINTR)
                        error(348);

                fflush(stdin);
                if (again < 0 || again > 1){
                        printf("codice non valido. riprova.\n\n");
                        goto usr_will;
                }
                write_int(sock_ds, again, 361);
                if (again)
                        goto another_code;
        }
exit_lab:
        return ret;
}

int write_back(int sock_ds, char *object, char *my_usr, char *usr_dest ){

        char *text, *re_obj;
        int len = strlen(object), ret;

        //invio destinatario
        write_string(sock_ds, usr_dest, 1296);

        //reading response if destination exists:
        read_int(sock_ds, &ret, 1299);

        if (!ret){
                printf("destinatario non più esistente.");
                return 0;
        }

        //invio mio usrname
        write_string(sock_ds, my_usr, 1307);

        /*CREATING THE STRING:  RE: <object>    */
        if ((re_obj = malloc(sizeof(char) * (len + 4))) == NULL)
                error(1226);

        bzero(re_obj, len + 4);

        sprintf(re_obj, "RE: %s", object);

        /*GETTING THE TEXT FROM USR*/
        printf("inserisci il messaggio:\n");
        if (scanf("%m[^\n]", &text) == -1 && errno != EINTR)
                error(1235);

        fflush(stdin);

        /*SENDING MEX*/

        //invio oggetto
        write_string(sock_ds, re_obj, 1329);

        //invio text
        write_string(sock_ds, text, 1332);

        return 1;
}

int delete_me(int sock_ds){//, char *usr){
        /*client-side version of elimination*/
        int ok;

//      write_string(sock_ds, usr, 1343);

        read_int(sock_ds, &ok, 1347);
        if (ok == 1)
                printf("account eliminato con successo. ");
        else
                printf("impossibile eliminare account.\n");

        return ok;

}

/*
void client_test(int sock_ds){
        message *mex = malloc(sizeof(message*));
        mex -> usr_destination = malloc(sizeof(char) * MAX_USR_LEN);
        mex -> usr_sender = malloc(sizeof(char) * MAX_USR_LEN);
        mex -> object = malloc(sizeof(char) * MAX_OBJ_LEN);
        mex -> text = malloc(sizeof(char) * MAX_MESS_LEN);

        sprintf(mex -> usr_destination, "%s", "a");
        sprintf(mex -> usr_sender, "%s", "b");
        sprintf(mex -> object, "%s", "c");
        sprintf(mex -> text, "%s", "d");

//      printf("mex %p\n", mex);

//      stampa_messaggio(mex);
        write_mex(sock_ds, mex);
        free(mex);
}*/

void cambia_pass(int sock_ds){
        int ret;
        char *new_pw;

        printf("inserisci la nuova password:\n");
        if (scanf("%ms", &new_pw) == -1 && errno != EINTR)
                error(1423);

        write_string(sock_ds, new_pw, 1530);
        read_int(sock_ds, &ret, 1519);

        if (ret)
                printf("password cambiata con successo.\n");
        free(new_pw);
}


