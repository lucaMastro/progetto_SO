#include "../lib/helper.h"
#include "../lib/helper-client.h"
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
#include <signal.h>

#define fflush(stdin) while(getchar() != '\n')
#define audit printf("ok\n")

int handler_sock;

void not_accepted_code(int scan_ret, int *codice_da_modificare, int valore_inaccettabile){
	/* per questioni di modularità e riusabilità del codice, creo questa funzione che, ogni volta che c'è una scanf per scegliere tra operazioni disponibili, 
	 * ferifica se il valore di ritorno della scanf (passato come primo parametro) è uguale a 0. in quel caso aggiorno il valore della variabile intera puntata
	 * da codice_da_modificare, settandolo a "valore_inaccettabile". il secondo parametro deve essere lo stesso puntatore passato nello scanf: infati dopo ogni 
	 * scanf su interi, si farà un check sul valore inserito. qualora venisse inserito un carattere invece che un intero, il controllo "fallirà": verrà stampato
	 * "codice non accettabile" o simili. */

	if (!scan_ret)
		*codice_da_modificare = valore_inaccettabile;

}

void get_file_db(int sock_ds){

	char *buffer, *token;
	int found, is_first = 1;

	buffer = (char*) malloc(sizeof(char) * (MAX_USR_LEN +1));
	if (buffer == NULL)
		error(195);

	printf("[");
	while (1){
		/* UPDATING IF FOUND */
		read_int(sock_ds, &found, 194);
		if (!found)
			break;

		if (!is_first)
			printf(", ");

		bzero(buffer, MAX_USR_LEN + 1);
		read_string(sock_ds, &buffer, 197);

		printf("%s", buffer);
		is_first = 0;
	}
	printf("]");

	free(buffer);
	return;
}

void handler(int signo){
	int n = MAX_NUM_MEX + 1;
	printf("\n");
	write_int(handler_sock, n, 23);
	exit(EXIT_SUCCESS);
}


int leggi_messaggi(int sock_ds, char *my_usrname, int flag){ //if flag == 1, only new messages will be print, if flag == 2, just print the mex in code position
        int found, again = 1, isnew, isfirst = 1, wb, can_i_wb, op, minimal_code, leave = 0, pos, code, scan_ret; 
        char *sender, *object, *text;
        message *mex;

     	if ((mex = (message*) malloc(sizeof(message))) == NULL)
                error(24);
//	printf("sizeof mex: %ld\nsizeof message: %ld\n", sizeof(mex), sizeof(message));
	
	if (flag == 2)
		goto get_code;
	
start:
        
	read_int(sock_ds, &found, 95);

	if (found) //im gonna start the while
		isfirst = 0;

	while(found && again){
		if (leave)
			break;
		
		minimal_code = 0;
		can_i_wb = 1;

		get_mex(sock_ds, mex, 1); 

get_code:
		/*PRINTING MESSAGE*/
        	printf("\e[1;1H\e[2J");

	        printf("......................................................................................\n");
        	printf("......................................................................................\n");
	        printf("................__ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __..............\n");
        	printf("...............|                                                        |.............\n");
	        printf("...............|              GESTORE LETTURE DEI MESSAGGI              |.............\n");
        	printf("...............|__ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __|.............\n");
	        printf("......................................................................................\n");
        	printf("......................................................................................\n");
		if (flag == 2){
        		printf(" ______ ________ ________ _____Operazioni Disponibili_____ ________ ________ ______ _\n");
		        printf("|                                                                                    |\n");
		        printf("|   *Inserisci il codice del messaggio da leggere				     |\n");
	        	printf("|   *Inserisci un numero negativo per annullare e tornare al menù principale	     |\n");
		        printf("|____ ________ ________ ________ ________ ________ ________ ________ ________ _____ _|\n\n");
			if ((scan_ret = scanf("%d", &code)) == -1 && errno != EINTR)
				error(105);
			fflush(stdin);
			not_accepted_code(scan_ret, &code, MAX_NUM_MEX + 2);

			if (code < 0){
				printf("operazione annullata con successo.\n");
				code = -1;
				write_int(sock_ds, code, 72);
				goto exit_lab;	
			}
			else if (code > MAX_NUM_MEX){
				printf("codice non valido. premi un tasto e riprova: ");
				fflush(stdin);
				goto get_code;
			}
			else{//codice valido
				write_int(sock_ds, code, 72);
				//read if users has a mex in `code` position
				read_int(sock_ds, &found, 124);
				if (found){
					printf("codice accettato.\n");
					get_mex(sock_ds, mex, 1);
				}
				else{
					printf("non hai messaggi associati a quel codice. premi un tasto per terminare: ");
					fflush(stdin);
					goto exit_lab;
				}
			}
		}
		
		printf("..............................Il messaggio ricevuto è:................................\n");
		//printf("%p, len = %d\n", mex, strlen( mex -> object));
                stampa_messaggio(mex);
	        printf("......................................................................................\n");	
		if (flag != 2){
	        	printf(" ______ ________ ________ _____Operazioni Disponibili_____ ________ ________ ______ _\n");
		        printf("|                                                                                    |\n");

                	/*CHECKING IF USR CAN WRITE BACK TO LAST READ MESS */

	                if (strlen(mex -> object) <= MAX_OBJ_LEN - 4 && !(mex -> is_sender_deleted))
		        	printf("|   OPERAZIONE 0 : Rispondere al messaggio visualizzato				     |\n");
			else{
	        		printf("|   OPERAZIONE 0 : Rispondere al messaggio visualizzato (NON DISPONIBILE)	     |\n");
				can_i_wb = 0;
				minimal_code = 1;
			}

	        	printf("|   OPERAZIONE 1 : Cancellare il messaggio visualizzato				     |\n");
		        printf("|   OPERAZIONE 2 : Cercare un altro messaggio					     |\n");
        		printf("|   OPERAZIONE 3 : Interrompere la ricerca e tornare al menu principale		     |\n");
		        printf("|____ ________ ________ ________ ________ ________ ________ ________ ________ _____ _|\n\n");
			printf("\nQuale operazione vuoi svolgere?\n");

usr_will:
                	/*CHECKING USR'S WILL*/

	                if ((scan_ret = scanf("%d", &op)) == -1 && errno != EINTR)
        	                error(140);
                	fflush(stdin);
			not_accepted_code(scan_ret, &op, 4);

			if (op < minimal_code || op > 3){
				printf("codice non valido. riprovare\n");
				goto usr_will;
			}
			/*SENDING USR'S WILL*/
			write_int(sock_ds, op, 116);

			switch (op){
				case 0:
					if (can_i_wb)				
                        	        	write_back(sock_ds, mex -> object, my_usrname, mex -> usr_sender);
/*					else{//non dovrebbe essere necessario
					printf("operazione non disponibile. riprovare\n");
					goto usr_will;
					}*/
					printf("premi un tasto per continuare la ricerca:");
					fflush(stdin);
					break;
				case 1:	
		                        cancella_messaggio(sock_ds, mex -> position);
					printf("premi un tasto per continuare la ricerca: ");
					fflush(stdin);
					break;
	/*			case 2:
					break;*/
				case 3:
					leave = 1;
					break;
				default:
					break;
                	}
			if (!leave){ //updating found
				read_int(sock_ds, &found, 156);
			}
		}
		else
			goto exit_lab;
	}
	
        if (!leave){ //eseguito solo con flag == 0 o flag == 1.
		if (isfirst && !flag)
        	        printf("non ci sono messaggi per te\n");
	        else if (isfirst && flag)
        	        printf("non ci sono nuovi messaggi per te\n");
	        else{//sono entrato nel while
        	        if (flag){//solo messaggi new
                	        if (again)
                        	        printf("\nnon ci sono altri nuovi mess.\n");
	                }
        	        else{
                	        if (again)
                        	        printf("\nnon ci sono altri messaggi\n");
	                }
        	}
	}
exit_lab:
	free(mex);
        return 0;

}

int operazioni_disponibili_invio(int mode){
	int retry, scan_ret;	
retry:
        	printf(" ______ ________ ________ _____Operazioni Disponibili_____ ________ ________ ______ _\n");
	        printf("|                                                                                    |\n");
	        printf("|   0. Riprovare a inserire il destinatario	    				     |\n");
		if (mode >= 1)
			printf("|   1. Riprovare a inserire l'oggetto	 	 				     |\n");
		if (mode == 2)
		       	printf("|   2. Riprovare a inserire il messaggio	    				     |\n");

	        printf("|   %d. Anullare e tornare al menù principale					     |\n", mode + 1);
	        printf("|____ ________ ________ ________ ________ ________ ________ ________ ________ _____ _|\n\n");
		if ((scan_ret = scanf("%d", &retry)) == -1 && errno != EINTR)
			error(220);
		
		not_accepted_code(scan_ret, &retry, mode + 2);
		if (retry < 0 || retry > mode + 1){
			printf("operazione non valida. premi un tasto per riprovare\n");
			fflush(stdin);
			goto retry;
		}
		return retry;
}

void invia_messaggio(int sock_ds, char *sender){
        char *destination, *obj, *mes;
        int len_dest, len_send, len_obj, len_mess, ret, retry;
	message *mex;

	        printf("\e[1;1H\e[2J");
        	printf(".....................................................................................\n");
	        printf(".....................................................................................\n");
        	printf("................__ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ _..............\n");
	        printf("...............|                                                       |.............\n");
		printf("...............|                 GESTORE INVIO MESSAGGI                |.............\n");
	        printf("...............|__ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ _|.............\n");
        	printf(".....................................................................................\n");
	        printf(".....................................................................................\n");
        	printf(".....................................................................................\n\n");
		printf("\t\tUtenti a cui è possibile inviare un messaggio:\n");
		get_file_db(sock_ds);
		printf("\n");
        	printf(".....................................................................................\n");
	        printf(".....................................................................................\n\n");
	
	if ((mex = (message*) malloc(sizeof(message))) == NULL)
		error(183);

	mex -> usr_sender = sender;
get_dest:
        //GETTING DATA AND THEIR LEN
        printf("username (max %d caratteri):\t", MAX_USR_LEN);
        if (scanf("%ms", &(mex -> usr_destination)) == -1 && errno != EINTR)
                error(470);

        fflush(stdin);


	//CHECK SULLA LUNGHEZZA
	if (strlen(mex -> usr_destination) > MAX_USR_LEN){
		printf("username destinatario troppo lungo.\n");
		retry = operazioni_disponibili_invio(0);	
		write_int(sock_ds, retry, 221);
		
		if (retry)
			return;
		else
			goto get_dest;
	}
	retry = -1;
	write_int(sock_ds, retry, 221);


        //invio destinatario
        write_string(sock_ds, mex -> usr_destination, 633);

        //reading response if destination exists:
        read_int(sock_ds, &ret, 539);

        if (!ret){
                printf("destinatario non esiste.\n\n"); 

		retry = operazioni_disponibili_invio(0);
		write_int(sock_ds, retry, 221);
		
		if (retry)
			return;
		else
			goto get_dest;
        }
	retry = -1;
	write_int(sock_ds, retry, 221);

get_obj:
        printf("\n");
	printf("object (max %d caratteri):\t", MAX_OBJ_LEN);
        if (scanf(" %m[^\n]", &(mex -> object)) == -1 && errno != EINTR)
                error(485);

        fflush(stdin);

        len_obj = strlen(mex -> object);
        if (len_obj > MAX_OBJ_LEN){
                printf("oggetto inserito troppo lungo.\n");

		retry = operazioni_disponibili_invio(1);
		write_int(sock_ds, retry, 221);
		
		switch (retry){
			case 0:
				goto get_dest;
			case 1:
				goto get_obj;
			case 2:
				return;
		}	
        }
	retry = -1;
	write_int(sock_ds, retry, 221);

get_mess:
        printf("\n");
	printf("text: (max %d caratteri):\t", MAX_MESS_LEN);
        if (scanf(" %m[^\n]", &(mex -> text)) == -1 && errno != EINTR)
                error(497);
        fflush(stdin);

        len_mess = strlen(mex -> text);
        if (len_mess > MAX_MESS_LEN){
                printf("messaggio inserito troppo lungo.\n");
		retry = operazioni_disponibili_invio(2);
		write_int(sock_ds, retry, 221);
		
		switch (retry){
			case 0:
				goto get_dest;
			case 1:
				goto get_obj;
			case 2:
				goto get_mess;
			case 3:
				return;
		}	
        }
	retry = -1;
	write_int(sock_ds, retry, 221);

	/*READING IF CAN WRITE*/
        read_int(sock_ds, &ret, 567);
	if (ret < 0)	
		printf("operazione temporaneamente fuori servizio. riprovare più tardi\n\n");
/*	elseV
		printf("messaggio ricevuto correttamente\n\n\n");*/

	else{
	        //SENDING DATA
		send_mex(sock_ds, mex, 0);
	        printf("\n\ninvio del messaggio avvenuto con successo.\n");
	}
	free(mex);
}

int usr_menu(int sock_ds, char *my_usrname){

        int operation, new_mex_avaiable, check_upd = 1, code = -1, ret, scan_ret;

	signal(SIGINT, handler);
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
        printf("|   OPERAZIONE 3 : Lettura di un messaggio in base al codice inserito                |\n");
        printf("|   OPERAZIONE 4 : Spedizione di un nuovo messaggio a uno qualunque degli utenti     |\n|\t\t   del sistema                                                       |\n");
        printf("|   OPERAZIONE 5 : Cancellare messaggi ricevuti dall'utente                          |\n");
        printf("|   OPERAZIONE 6 : Aggiornare lo stato del sistema (cerca se ci sono nuovi messaggi) |\n");
        printf("|   OPERAZIONE 7 : Chiudere l'applicazione                                           |\n");
        printf("|   OPERAZIONE 8 : Cancellare l'account                                              |\n");
        printf("|   OPERAZIONE 9 : Cambia password                                                   |\n");
        printf("|____ ________ ________ ________ ________ ________ ________ ________ ________ _____ _|\n\n");

        printf("quale operazione vuoi svolgere?\n");

        if ((scan_ret = scanf("%d", &operation)) == -1 && errno != EINTR)
                error(995);
	not_accepted_code(scan_ret, &operation, 10);

        fflush(stdin);

        if (operation > 9 || operation < 0){
                printf("operazione non valida\n");
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
                        //sleep(3);
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
			leggi_messaggi(sock_ds, my_usrname, 2);
			check_upd = 1;
			break;
                case 4:
                        invia_messaggio(sock_ds, my_usrname);
                        break;
                case 5:
                        cancella_messaggio(sock_ds, code);
//                      check_upd = 1;
                        break;
                case 6:
                        read_int(sock_ds, &new_mex_avaiable, 1147);
                        printf("aggiornamento avvenuto con successo.\n");
                        break;
                case 7:
                        free(my_usrname);
                        close_client(sock_ds);
                case 8:
                        delete_me(sock_ds);
                        free(my_usrname);
                        close_client(sock_ds);
                case 9:
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

        int ret, operation, len, retry, scan_ret;
        char *pw;

	handler_sock = sock_ds;
	signal(SIGINT, handler);
portal:
	operation = 3;
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

        if ((scan_ret = scanf("%d", &operation)) == -1 && errno != EINTR)
                error(732);
        fflush(stdin);
	not_accepted_code(scan_ret, &operation, 3);

        if (operation < 0 || operation > 2){
                printf("operazione non valida. premi un tasto per riprovare: ");
                fflush(stdin);
                goto portal;
        }


        //sending the selected operation:
        write_int(sock_ds, operation, 410);

        if (operation == 0){
                free(*usr);
                close_client(sock_ds);
        }

        //same structure for registration and login
        //change menu view
        printf("\e[1;1H\e[2J");

        printf(".....................................................................................\n");
        printf(".....................................................................................\n");
        printf("................__ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ _..............\n");
        printf("...............|                                                       |.............\n");
        if (operation == 1)
                printf("...............|                 GESTORE REGISTRAZIONE                 |.............\n");
        else
                printf("...............|                     GESTORE LOGIN                     |.............\n");
        printf("...............|__ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ _|.............\n");
        printf(".....................................................................................\n");
        printf(".....................................................................................\n");
        printf(".....................................................................................\n\n");

get_usr:
        retry = 0;
        printf("inserisci username (max %d caratteri):\n", MAX_USR_LEN);
        if (scanf("%s", *usr) == -1 && errno != EINTR)
                error(756);

        fflush(stdin);

        len = strlen(*usr);
        if (len > MAX_USR_LEN){

                printf("\n...........................Errore: username troppo lungo.............................\n");
                printf(" ______ ________ ________ _____Operazioni Disponibili_____ ________ ________ ______ _\n");
                printf("|                                                                                    |\n");
                printf("|   1. Inserire un nuovo username                                                    |\n");
                printf("|   2. Annullare e tornare al menu iniziale                                          |\n");
                printf("|____ ________ ________ ________ ________ ________ ________ ________ ________ _____ _|\n\n");
                printf("quale operazione vuoi svolgere?\n");
get_op:
                if ((scan_ret = scanf("%d", &retry)) == -1 && errno != EINTR)
                        error(732);

                fflush(stdin);
		not_accepted_code(scan_ret, &retry, 3);

                switch (retry){
                        case 1:
                                write_int(sock_ds, retry, 463);
                                goto get_usr;
                                break;
                        case 2:
                                write_int(sock_ds, retry, 467);
                                goto portal;
                                break;
                        default:
                                printf("operazione non valida. premi un tasto per riprovare: ");
                                fflush(stdin);
                                goto get_op;
                                break;
                }
        }

        write_int(sock_ds, retry, 463);
        write_string(sock_ds, *usr, 433);

get_pw:
        retry = 0;
	printf("inserisci password (max %d caratteri):\n", MAX_PW_LEN);

#ifndef CRYPT
        if (scanf("%ms", &pw) == -1 && errno != EINTR)
                error(772);
        fflush(stdin);
#else
	pw = getpass("(per uscire con CTRL+C, premerlo e dare invio)\n");
	if (strstr(pw, "\003") != NULL){ //è presente un ctrl+c
		pid_t my_pid = getpid(); //got my pid
		kill(my_pid, SIGINT);
	}
#endif

        len = strlen(pw);
        if (len > MAX_PW_LEN){
                printf("\n...........................Errore: password troppo lunga.............................\n");
                printf(" ______ ________ ________ _____Operazioni Disponibili_____ ________ ________ ______ _\n");
                printf("|                                                                                    |\n");
                printf("|   1. Inserire una nuova password                                                   |\n");
                printf("|   2. Annullare e tornare al menu iniziale                                          |\n");
                printf("|____ ________ ________ ________ ________ ________ ________ ________ ________ _____ _|\n\n");
                printf("quale operazione vuoi svolgere?\n");
get_op1:
                if ((scan_ret = scanf("%d", &retry)) == -1 && errno != EINTR)
                        error(732);
                fflush(stdin);
		not_accepted_code(scan_ret, &retry, 3);

                switch (retry){

                        case 1:
                                write_int(sock_ds, retry, 503);
                               goto get_pw;
                        case 2:
                                write_int(sock_ds, retry, 507);
                                goto portal;
                        default:
                                printf("operazione non valida. premi un tasto per riprovare: ");
                                fflush(stdin);
                                goto get_op1;
                }
        }
//      fflush(stdin);

        write_int(sock_ds, retry, 522);
        write_string(sock_ds, pw, 523);

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
        /*if (!sleep(3)){
                printf("\e[1;1H\e[2J");
        }*/
	exit(EXIT_SUCCESS);
}

int cancella_messaggio(int sock_ds, int mode){//mode < 0 quando è chiamata separatamente a leggi_messaggi
        int is_mine, ret = 0, again, fine, code, scan_ret;
	
        /*SCRIVO MODE*/
        write_int(sock_ds, mode, 326);

        if (mode >= 0)
                code = mode;

        else{ //mode < 0
               // printf("dammi il codice del messaggio da eliminare.\nNOTA: se non hai messaggi associati al codice inserito, l'operazione verrà interrotta.\n(inserire un numero negativo per annullare)\n");

		another_code:
        	printf("\e[1;1H\e[2J");

	        printf(".....................................................................................\n");
        	printf(".....................................................................................\n");
	        printf("................__ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ _..............\n");
        	printf("...............|                                                       |.............\n");
	        printf("...............|           GESTORE ELIMINAZIONE DEI MESSAGGI           |.............\n");
        	printf("...............|__ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ _|.............\n");
	        printf(".....................................................................................\n");
        	printf(".....................................................................................\n");
	        printf(".....................................................................................\n");	
        	printf(" ______ ________ ________ _____Operazioni Disponibili_____ ________ ________ ______ _\n");
	        printf("|                                                                                    |\n");
	        printf("|   *Inserisci il codice del messaggio da eliminare				     |\n");
	        printf("|   *Inserisci un numero negativo per annullare e tornare al menù principale	     |\n");
	        printf("|____ ________ ________ ________ ________ ________ ________ ________ ________ _____ _|\n\n");
		
                if ((scan_ret = scanf("%d", &code)) == -1 && errno != EINTR)
                        error(308);
                fflush(stdin);
		not_accepted_code(scan_ret, &code, MAX_NUM_MEX + 2); 

		if (code > MAX_NUM_MEX)
			goto not_acc; //just writing that "the code isnt accepted", no writing why.

                if (code < 0){
			code = -1; //SET THE CODE = -1: I FIX THE BUG IF USERS INSERT -999999
                	write_int(sock_ds, (int) code, 328);
                        printf("operazione annullata con successo\n");
                        goto exit_lab;
                }
                /*SCRIVO CODE*/
                write_int(sock_ds, code, 328);
        }
	/*READ IF THE CODE IS ACCEPTED*/
        read_int(sock_ds, &is_mine, 332);
        if (is_mine == 1){ //&& code <= MAX_NUM_MEX){ CHECK GIÀ FATTO
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
		not_acc:
 /*               if (mode >= 0)	//NON È POSSIBILE CHE SI VERIFICHI: UNA VOLTA CANCELLATO UN MESS, SI PROCEDE NELLA RICERCA IN "GESTORE_LETTURE"
                        printf("messaggio già eliminato\n");*/ 	
                //else{
		//
		//
		//eseguito sempre e solo con mode < 0
                        printf("Errore: non hai messaggi ricevuti associati al codice inserito. Premi un tasto e riprova.\n");
			fflush(stdin);
                        goto another_code;
                //}
        }

        /*CHECKING USR'S WILL. eseguire solo se mode < 0*/
        if (mode < 0){ //IN QUESTO MODO BLOCCO ELIMINAZIONI MULTIPLE DURANTE LA "GESTORE_LETTURE"
usr_will:
	        printf(".....................................................................................\n");	
        	printf(" ______ ________ ________ __Altre Operazioni Disponibili__ ________ ________ _____ _\n");
	        printf("|                                                                                   |\n");
        	printf("|   OPERAZIONE 0 : Interrompere le eliminazioni e tornare al menu principale	    |\n");
	        printf("|   OPERAZIONE 1 : Eliminare un altro messaggio					    |\n");
	        printf("|____ ________ ________ ________ ________ ________ ________ ________ ________ ____ _|\n\n");
		printf("\nQuale operazione vuoi svolgere?\n");
                if ((scan_ret = scanf("%d", &again)) == -1 && errno != EINTR)
                        error(348);
		not_accepted_code(scan_ret, &again, 2);

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



int operazioni_disponibili_wb(){
	int retry, scan_ret;	
retry:
        	printf(" ______ ________ ________ _____Operazioni Disponibili_____ ________ ________ ______ _\n");
	        printf("|                                                                                    |\n");
		printf("|   0. Riprovare a inserire il messaggio	    				     |\n");

	        printf("|   1. Anullare e tornare al menù principale					     |\n");
	        printf("|____ ________ ________ ________ ________ ________ ________ ________ ________ _____ _|\n\n");
		if ((scan_ret = scanf("%d", &retry)) == -1 && errno != EINTR)
			error(220);
		not_accepted_code(scan_ret, &retry, 2);

		if (retry < 0 || retry > 1){
			printf("operazione non valida. premi un tasto per riprovare\n");
			fflush(stdin);
			goto retry;
		}
		return retry;
}

int write_back(int sock_ds, char *object, char *my_usr, char *usr_dest ){
        char *text, *re_obj;
        int len = strlen(object), ret, retry;
	message *mex;

	//invio destinatario
        write_string(sock_ds, usr_dest, 1296);
        //reading response if destination exists:
        read_int(sock_ds, &ret, 1299);
        if (!ret){
                printf("destinatario non più esistente.");
                return 0;
        }

	if ((mex = (message*) malloc(sizeof(message))) == NULL)
		error(680);

	mex -> usr_sender = my_usr;
	mex -> usr_destination = usr_dest;
        /*CREATING THE STRING:  RE: <object>    */
        if ((mex -> object = (char*) malloc(sizeof(char) * (len + 5))) == NULL)
                error(1226);
        bzero(mex -> object, len + 5);

        sprintf(mex -> object, "RE: %s", object);

get_mex:
        /*GETTING THE TEXT FROM USR*/
        printf("inserisci il messaggio:\n");
        if (scanf(" %m[^\n]", &(mex -> text)) == -1 && errno != EINTR)
                error(1235);

        fflush(stdin);

	if (strlen(mex -> text) > MAX_MESS_LEN){
                printf("messaggio inserito troppo lungo.\n");
		retry = operazioni_disponibili_wb();	
		write_int(sock_ds, retry, 221);

		if (retry)
			return 1;
		else
			goto get_mex;
	}
	retry = -1;
	write_int(sock_ds, retry, 221);
	
	/*READING IF THE MEX CAN BE STORED*/

        read_int(sock_ds, &ret, 567);
	if (ret < 0)	
		printf("operazione temporaneamente fuori servizio. riprovare più tardi\n\n");

	else{
	        //SENDING DATA
		send_mex(sock_ds, mex, 0);
	        printf("\n\ninvio del messaggio avvenuto con successo.\n");
	}

	free(mex);
        return 1;
}

int delete_me(int sock_ds){//, char *usr){
        /*client-side version of elimination*/
        int ok;

        read_int(sock_ds, &ok, 1347);
        if (ok == 1)
                printf("account eliminato con successo. ");
        else
                printf("impossibile eliminare account.\n");

        return ok;

}

void cambia_pass(int sock_ds){
        int ret;
        char *new_pw;
	
        printf("inserisci la nuova password:\n");
#ifndef CRYPT
        if (scanf("%ms", &new_pw) == -1 && errno != EINTR)
                error(1423);
	fflush(stdin);
#else
	new_pw = getpass("(per uscire con CTRL+C, premerlo e dare invio)\n");
	if (strstr(new_pw, "\003") != NULL){ //è presente un ctrl+c
		pid_t my_pid = getpid(); //got my pid
		kill(my_pid, SIGINT);
	}
#endif
        write_string(sock_ds, new_pw, 1530);
        read_int(sock_ds, &ret, 1519);

        if (ret)
                printf("password cambiata con successo.\n");
        free(new_pw);
}

