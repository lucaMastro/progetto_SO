#define _XOPEN_SOURCE
#define _GNU_SOURCE 
#include "../lib/helper.h"
#include "../lib/helper-client.h"
//#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <fcntl.h>
//#include <sys/mman.h>
//#include <sys/ipc.h>
//#include <sys/sem.h>
#include <signal.h>
#include <crypt.h>

#define fflush(stdin) while(getchar() != '\n')
#define audit printf("ok\n")

int handler_sock;


void random_salt(char salt[3]){
        /* aggiorna salt, passato come parametro, inserendo 2 valori casuali tra
         * quelli accettabili per il salt nella funzione crypt_r.
         *
         * intervallo di valori per il salt: [a-zA-Z0-9./]
         * a-z: 26
         * A-z: 26
         * 0-9: 10
         * ./ :  2 
         * totale: 64 */
        char values[64 + 1];
        int i, random_value;

        bzero(salt, 3);
        for (i = 0; i < 26; i++){
                values[i] = (char) (i + 97);
                values[i + 26] = (char) (i + 65); //maiuscole 
        }
        for (i = 52; i < 62; i++)
                values[i] = (char) (i - 4);
        values[62] = '.';
        values[63] = '/';

        for (i = 0; i < 2; i++){
                random_value = rand() % 64;
                salt[i] = values[random_value];
        }
}




void not_accepted_code(int scan_ret, int *codice_da_modificare, int valore_inaccettabile){
	/* per questioni di modularità e riusabilità del codice, creo questa funzione che, ogni volta che c'è una scanf per scegliere tra operazioni disponibili, 
	 * verifica se il valore di ritorno della scanf (passato come primo parametro) è uguale a 0. in quel caso aggiorno il valore della variabile intera puntata
	 * da codice_da_modificare, settandolo a "valore_inaccettabile". il secondo parametro deve essere lo stesso puntatore passato nello scanf: infati dopo ogni 
	 * scanf su interi, si farà un check sul valore inserito. qualora venisse inserito un carattere invece che un intero, il controllo "fallirà": verrà stampato
	 * "codice non accettabile" o simili. */

	if (!scan_ret)
		*codice_da_modificare = valore_inaccettabile;

}

void get_file_db(int sock_ds){

	/* funzione che non memorizza la lista degli utenti registrati, ma la 
	 * stampa solamente in fase di invio messaggi. */
	char *buffer;
	int found, is_first = 1;

	printf("[");
	while (1){
		/* UPDATING IF FOUND */
		read_int(sock_ds, &found, 194);
		if (!found)
			break;

		if (!is_first)
			printf(", ");

		read_string(sock_ds, &buffer, 197);

		printf("%s", buffer);
		is_first = 0;
		
		free(buffer); //in the while, cause each read_string does a malloc but doesnt free
	}
	printf("]");
	return;
}

void handler(int signo){
	int n = MAX_NUM_MEX + 1;
	printf("\n");
	write_int(handler_sock, n, 23);
	exit(EXIT_SUCCESS);
}


int leggi_messaggi(int sock_ds, char *my_usrname, int flag){ 
	/* se flag == 2, tutti i messaggi dell'utente verranno stampati;
	 * se flag == 1, verranno stampati solo i nuovi messaggi;
	 * se flag == 2, verrà stampato solo il messaggio nella posizione
	 * indicata dall'utente. */

        message *mex;
        int found, again = 1, isnew, isfirst = 1, wb, can_i_wb, op, minimal_code, maximal_code = 3, leave = 0, pos, code, scan_ret; 
	/* minimal e maximal code sono parametri che servono a staibilire 
	 * l'intervallo di valori validi disponibili all'utente. in particolare:
	 * 
	 * minimal code: è un parametro che serve a stabilire se è possibile o 
	 * meno rispondere a un messaggio letto;
	 *
	 * maximal code: è un parametro che serve a discriminare il valore massimo
	 * tra quelli possibili nel set di operazioni possibili dopo aver letto 
	 * un messaggio. serve solo nel caso di flag == 2. infatti è il caso in cui
	 * va bloccata la ricerca di un nuovo messaggio*/

     	if ((mex = (message*) malloc(sizeof(message))) == NULL)
                error(24);
	
	if (flag == 2){
		maximal_code = 2;
		minimal_code = 0;
		goto get_code;
	}
	
start:
        
	read_int(sock_ds, &found, 95);

	if (found) //im gonna start the while
		isfirst = 0;

	while(found && again){
		if (leave)
			break;
		
		minimal_code = 0;
		can_i_wb = 1;

		/* non eseguito se la funzione è chiamata con flag == 2 
		 * (si vuole leggere un solo mes fornendone il codice) */
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
				printf("Operazione annullata con successo.\n");
				code = -1;
				write_int(sock_ds, code, 72);
				goto exit_lab;	
			}
			else if (code > MAX_NUM_MEX){
				printf("Codice non valido. Premi un tasto e riprova: ");
				fflush(stdin);
				goto get_code;
			}
			else{//codice valido
				write_int(sock_ds, code, 72);
				//read if users has a mex in `code` position
				read_int(sock_ds, &found, 124);
				if (found){
					printf("Codice accettato.\n");
					get_mex(sock_ds, mex, 1);
				}
				else{
					printf("Non hai messaggi associati a quel codice. premi un tasto per terminare: ");
					fflush(stdin);
					goto exit_lab;
				}
			}
		}
		
		printf("..............................Il messaggio ricevuto è:................................\n");
                stampa_messaggio(mex);
	        printf("......................................................................................\n");	
	        printf(" ______ ________ ________ _____Operazioni Disponibili_____ ________ ________ ______ _\n");
	  	printf("|                                                                                    |\n");

               	/*CHECKING IF USR CAN WRITE BACK TO LAST READ MESS */

/*		printf("(strlen(mex -> object)) <= (MAX_OBJ_LEN - 4)) == condizione\n");
		printf("%ld <= %d == %d\n", strlen(mex -> object), (MAX_OBJ_LEN - 4), (strlen(mex -> object)) <= (MAX_OBJ_LEN - 4));*/

                if ( ((int)(strlen(mex -> object)) <= (MAX_OBJ_LEN - 4)) && !(mex -> is_sender_deleted))
	        	printf("|   OPERAZIONE 0 : Rispondere al messaggio visualizzato				     |\n");
		else{
        		printf("|   OPERAZIONE 0 : Rispondere al messaggio visualizzato (NON DISPONIBILE)	     |\n");
			can_i_wb = 0;
			minimal_code = 1;
		}

        	printf("|   OPERAZIONE 1 : Cancellare il messaggio visualizzato				     |\n");
		if (flag != 2){
			printf("|   OPERAZIONE 2 : Cercare un altro messaggio					     |\n");
        		printf("|   OPERAZIONE 3 : Interrompere la ricerca e tornare al menu principale		     |\n");
		}
		else{
        		printf("|   OPERAZIONE 2 : Tornare al menu principale					     |\n");
		}
		        printf("|____ ________ ________ ________ ________ ________ ________ ________ ________ _____ _|\n\n");
			printf("\nQuale operazione vuoi svolgere?\n");

usr_will:
                	/*CHECKING USR'S WILL*/

	                if ((scan_ret = scanf("%d", &op)) == -1 && errno != EINTR){
        	         	free(mex);
			 	error(140);
			}
                	fflush(stdin);
			not_accepted_code(scan_ret, &op, 4);

			if (op < minimal_code || op > maximal_code){
				printf("Codice non valido. Riprovare\n\n");
				goto usr_will;
			}

			/* se flag == 2, allora il codice di uscita (mostrato 
			 * all'utente) è 2. settando op = 3, non devo più dividere
			 * i casi flag == 2 dagli altri, potendo quindi 
			 * riutilizzare la stessa struttura di controllo. */
			if (flag == 2 && op == 2)
				op = 3;

			/*SENDING USR'S WILL*/
			write_int(sock_ds, op, 116);

			switch (op){
				case 0:
					if (can_i_wb)				
                        	        	write_back(sock_ds, mex -> object, my_usrname, mex -> usr_sender);
					if (flag != 2){
						printf("Premi un tasto per continuare la ricerca:");
						fflush(stdin);
					}
					break;
				case 1:	
		                        cancella_messaggio(sock_ds, mex -> position);
					if (flag != 2){
						printf("Premi un tasto per continuare la ricerca: ");
						fflush(stdin);
					}
					break;
				case 3:
					leave = 1;
					break;
				default: //case 2: devo solo continuare il ciclo
					break;
                	}
			if (flag == 2)
				leave = 1; //evita i successivi blocchi if

			if (!leave){ //updating found
				read_int(sock_ds, &found, 156);
			}
        	        free(mex -> usr_sender);
        	        free(mex -> usr_destination);
        	        free(mex -> object);
        	        free(mex -> text);
		}
	
	/* GESTIONE DEL CORRETTO MESSAGGIO DA STAMPARE IN BASE AI VARI CASI */
        if (!leave){ //eseguito solo con flag == 0 o flag == 1.
		if (isfirst && !flag)
        	        printf("Non ci sono messaggi per te\n");
	        else if (isfirst && flag)
        	        printf("Non ci sono nuovi messaggi per te\n");
	        else{//sono entrato nel while
        	        if (flag){//solo messaggi new
                	        if (again)
                        	        printf("\nNon ci sono altri nuovi mess.\n");
	                }
        	        else{
                	        if (again)
                        	        printf("\nNon ci sono altri messaggi\n");
	                }
        	}
	}

exit_lab:
	free(mex);
        return 0;

}

int operazioni_disponibili_invio(int mode){
	/* funzione per riorganizzazione del codice. chiamata solo durante 
	 * l'invio di un messaggio, in caso di errori sui parametri forniti 
	 * dall'utente. */

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
			printf("Operazione non valida. Premi un tasto per riprovare: ");
			fflush(stdin);
			goto retry;
		}
		return retry;
}

void invia_messaggio(int sock_ds, char *sender){
        int ret, retry;
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
        printf("Username (max %d caratteri):\t", MAX_USR_LEN);
        if (scanf("%ms", &(mex -> usr_destination)) == -1 && errno != EINTR)
                error(470);

        fflush(stdin);


	//CHECK SULLA LUNGHEZZA
	if (strlen(mex -> usr_destination) > MAX_USR_LEN){
		printf("Username destinatario troppo lungo.\n");
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
                printf("Destinatario non esiste.\n\n"); 
		free(mex -> usr_destination);

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
	printf("Object (max %d caratteri):\t", MAX_OBJ_LEN);
        if (scanf(" %m[^\n]", &(mex -> object)) == -1 && errno != EINTR)
                error(485);

        fflush(stdin);

        if (strlen(mex -> object) > MAX_OBJ_LEN){
                printf("Oggetto inserito troppo lungo.\n");	
		free(mex -> object);

		retry = operazioni_disponibili_invio(1);
		write_int(sock_ds, retry, 221);
		
		switch (retry){
			case 0:
				free(mex -> usr_destination);
				goto get_dest;
			case 1:
				free(mex -> object);
				goto get_obj;
			case 2:
				free(mex -> usr_destination);
				free(mex -> object);
				return;
		}	
        }
	retry = -1;
	write_int(sock_ds, retry, 221);

get_mess:
        printf("\n");
	printf("Text: (max %d caratteri):\t", MAX_MESS_LEN);
        if (scanf(" %m[^\n]", &(mex -> text)) == -1 && errno != EINTR)
                error(497);
        fflush(stdin);

        if (strlen(mex -> text) > MAX_MESS_LEN){
                printf("Messaggio inserito troppo lungo.\n");
		retry = operazioni_disponibili_invio(2);
		write_int(sock_ds, retry, 221);
		
		free(mex -> text);
		switch (retry){
			case 0:
				free(mex -> usr_destination);
				free(mex -> object);
				goto get_dest;
			case 1:
				free(mex -> object);
				goto get_obj;
			case 2:
				goto get_mess;
			case 3:
				free(mex -> usr_destination);
				free(mex -> object);
				return;
		}	
        }
	retry = -1;
	write_int(sock_ds, retry, 221);

	/*READING IF CAN WRITE*/
        read_int(sock_ds, &ret, 567);
	if (ret < 0)	
		printf("Operazione temporaneamente fuori servizio. Riprovare più tardi\n\n");
	else{
	        //SENDING DATA
		send_mex(sock_ds, mex, 0);
	        printf("\n\nInvio del messaggio avvenuto con successo.\n");
	}
	free(mex -> usr_destination);
	free(mex -> object);
	free(mex -> text);
	free(mex);
}



int usr_menu(int sock_ds, char *my_usrname){

        int operation, new_mex_avaiable, check_upd = 1, code = -1, ret, scan_ret;
	signal(SIGINT, handler);

select_operation:
        printf("\n\nLogin effettuato come: %s.\n\n", my_usrname);

        if (check_upd){
                //Reading if there are new mex
                read_int(sock_ds, &new_mex_avaiable, 1079);
                check_upd = 0;
        }

        if (new_mex_avaiable)
                printf("Esistono messaggi non letti.\n\n");
        else
                printf("Non esistono messaggi non letti.\n\n");

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

	printf("Quale operazione vuoi svolgere?\n");

        if ((scan_ret = scanf("%d", &operation)) == -1 && errno != EINTR)
                error(995);
	not_accepted_code(scan_ret, &operation, 10);

        fflush(stdin);

        if (operation > 9 || operation < 0){
                printf("Operazione non valida.\nPremi un tasto per riprovare: ");
                fflush(stdin);
                printf("\e[1;1H\e[2J");
                goto select_operation;
        }

        //invio il segnale per preparare il server alla gestione dell'operazione:
        write_int(sock_ds, operation, 1162);

        printf("Operazione accettata.\n\n");
        switch (operation){
                case 0:
                        printf("Tornerai alla schermata di login fra 3 secondi. Arrivederci :)\n");
                        //sleep(3);
                        return 0;
                        break;
                case 1:
                        leggi_messaggi(sock_ds, my_usrname, 0);
                        check_upd = 1;
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
                        read_int(sock_ds, &new_mex_avaiable, 502);//leggo per aggiornamenti
                        break;
                case 6:
                        read_int(sock_ds, &new_mex_avaiable, 1147);
                        printf("Aggiornamento avvenuto con successo.\n");
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
        printf("\nPremi INVIO per continuare: ");
        fflush(stdin);
        printf("\e[1;1H\e[2J");

        goto select_operation;

}

void usr_registration_login(int sock_ds, char **usr){
        int ret, operation, len, retry, scan_ret;
        char *pw, *salt;
#ifdef CRYPT
	char *crypted;
	struct crypt_data data;
	data.initialized = 0;
#endif

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
                printf("Operazione non valida. Premi un tasto per riprovare: ");
                fflush(stdin);
                goto portal;
        }


        //sending the selected operation:
        write_int(sock_ds, operation, 410);

        if (operation == 0){
                free(*usr);
                close_client(sock_ds);
        }

        //same basical structure for registration and login
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
        printf("Inserisci username (max %d caratteri):\n", MAX_USR_LEN);
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
                printf("Quale operazione vuoi svolgere?\n");
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
                                printf("Operazione non valida. Premi un tasto per riprovare: ");
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
                printf("Quale operazione vuoi svolgere?\n");
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
                                printf("Operazione non valida. Premi un tasto per riprovare: ");
                                fflush(stdin);
                                goto get_op1;
                }
        }

        write_int(sock_ds, retry, 522);

#ifdef CRYPT
	if (operation == 2){//login procedure. read salt
		read_int(sock_ds, &ret, 712); //check if usr exist
		if (ret == 2)
			goto check_ret;
		read_string(sock_ds, &salt, 715);
	}
	else{ //registrazione: devo generare un salt:
		if ((salt = malloc(sizeof(char) * 3)) == NULL)
			error(719);
		bzero(salt, 3);
		random_salt(salt);
	}
	crypted = crypt_r(pw, salt, &data);
	write_string(sock_ds, crypted, 524);
#else
        write_string(sock_ds, pw, 781);
#endif

        //reading response:
        read_int(sock_ds, &ret, 879);
	free(pw);
check_ret:
        switch (ret){
                case 0: //it was allright
                        if (operation == 1){ //it was a registration:
                                printf("Registrazione avvenuta.\nPremi INVIO per continuare: ");
                                fflush(stdin);
                                goto portal;
                                break;
                        }
                        else
                                return;

                case 1:
                        //file già esistente:
                        printf("Username già presente. Per favore riprova.\nPremi INVIO per continuare: ");
			fflush(stdin);
                        goto portal;
                        break;
                case 2:
                        //uncorrect pw or username in login
                        printf("Username o password errati. Per favore riprova.\nPremi INVIO per continuare: ");
			fflush(stdin);
                        goto portal;
                        break;
                case 3:
                        //usr already logged
                        printf("Username già loggato. Per favore riprova\nPremi INVIO per continuare: ");
			fflush(stdin);
                        goto portal;
                        break;
                case 4:
                        printf("Username o password troppo lunghi. Per favore riprova.\nPremi INVIO per continuare: ");
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
        printf("L'applicazione si chiuderà in 3 secondi. Arrivederci :)\n");
        /*if (!sleep(3)){
                printf("\e[1;1H\e[2J");
        }*/
	exit(EXIT_SUCCESS);
}

int cancella_messaggio(int sock_ds, int mode){//mode < 0 quando è chiamata separatamente a leggi_messaggi
        int is_mine, ret = 0, again, code, scan_ret, op;
	message *mex;

        /*SCRIVO MODE*/
        write_int(sock_ds, mode, 326);

        if (mode >= 0)
                code = mode;

        else{ //mode < 0, separatamente a gestore letture

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

		if (code > MAX_NUM_MEX){
			//goto not_acc; //just writing that "the code isnt accepted", no writing why.	
                        printf("Errore: non hai messaggi ricevuti associati al codice inserito. Premi un tasto e riprova.\n");
			fflush(stdin);
                        goto another_code;
		}

		else if (code < 0){
			code = -1; //SET THE CODE = -1: I FIX THE BUG IF USERS INSERT -999999
                	write_int(sock_ds, code, 328);
                        printf("Operazione annullata con successo\n");
                        goto exit_lab;
                }
		
		else
                	/*SCRIVO CODE*/
	                write_int(sock_ds, code, 328);
        }
	/*READ IF THE CODE IS ACCEPTED*/
        read_int(sock_ds, &is_mine, 332);
        if (is_mine == 1){ //&& code <= MAX_NUM_MEX){ CHECK GIÀ FATTO
		printf("Codice accettato.\n");
		
		if (mode < 0){
	     		if ((mex = (message*) malloc(sizeof(message))) == NULL)
        	        	error(24);
			get_mex(sock_ds, mex, 1);
	read_conf:
			printf(".......................Il messaggio che stai per eliminare è:.........................\n");
                	stampa_messaggio(mex);
		        printf("......................................................................................\n");	
		        printf(" ______ ________ ________ _____Operazioni Disponibili_____ ________ ________ ______ _\n");
			printf("|                                                                                    |\n");
		        printf("|   OPERAZIONE 0 : Annulla l'eliminazione del messaggio				     |\n");
		        printf("|   OPERAZIONE 1 : Conferma l'eliminazione del messaggio			     |\n");
			printf("|____ ________ ________ ________ ________ ________ ________ ________ ________ _____ _|\n\n");
			printf("\nQuale operazione vuoi svolgere?\n");

        	        free(mex -> usr_sender);
	        	free(mex -> usr_destination);
        		free(mex -> object);
        		free(mex -> text);	
			free(mex);
	               
			if ((scan_ret = scanf("%d", &op)) == -1 && errno != EINTR){
				error(140);
			}
			fflush(stdin);
			not_accepted_code(scan_ret, &op, 2);
			if (op < 0 || op > 1){
				printf("Codice non valido. Riprovare\n");
				goto read_conf;
			}
			write_int(sock_ds, op, 854);
			if (!op)
				goto exit_lab;
		}
		
		printf("Attendi conferma eliminazione...\n");
                /*READ IF ELIMINATION WAS OK*/
                read_int(sock_ds, &ret, 338);

                if (ret == 1)
                        printf("Messaggio eliminato correttamente\n\n");
                else{
                        printf("Errore nell'eliminazione del messaggio. Termino.\n\n");
                        exit(EXIT_FAILURE);
                }
        }
        else //eseguito sempre e solo con mode < 0
	 	printf("Errore: non hai messaggi ricevuti associati al codice inserito.\n");
                        

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
                        printf("Codice non valido. Riprova.\n\n");
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
	/* funzione inserita per modularità e chiamata solo da write_back. */
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
			printf("Operazione non valida. Premi un tasto per riprovare: ");
			fflush(stdin);
			goto retry;
		}
		return retry;
}

int write_back(int sock_ds, char *object, char *my_usr, char *usr_dest ){
        int len = strlen(object), ret, retry;
	message *mex;

	//invio destinatario
        write_string(sock_ds, usr_dest, 1296);

        //reading response if destination exists:
        read_int(sock_ds, &ret, 1299);
        if (!ret){
                printf("Destinatario non più esistente.\n");
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
        printf("Inserisci il messaggio (max %d caratteri):\n", MAX_MESS_LEN);
        if (scanf(" %m[^\n]", &(mex -> text)) == -1 && errno != EINTR)
                error(1235);
        fflush(stdin);

	if (strlen(mex -> text) > MAX_MESS_LEN){
                printf("Messaggio inserito troppo lungo.\n");
		retry = operazioni_disponibili_wb();	
		write_int(sock_ds, retry, 221);

		if (retry)
			return 1;
		else{
			free(mex -> text);
			goto get_mex;
		}
	}
	retry = -1;
	write_int(sock_ds, retry, 221);
	
	/*READING IF THE MEX CAN BE STORED*/
        read_int(sock_ds, &ret, 567);
	if (ret < 0)	
		printf("Operazione temporaneamente fuori servizio. Riprovare più tardi.\n\n");

	else{
	        //SENDING DATA
		send_mex(sock_ds, mex, 0);
	        printf("\n\nInvio del messaggio avvenuto con successo.\n");
	}

	free(mex -> text);
	free(mex -> object);
	free(mex);
        return 1;
}

int delete_me(int sock_ds){
        /*client-side version of elimination: just waiting for a confirm*/

	int ok;

	printf("Attendo conferma eliminazione.\n");
        read_int(sock_ds, &ok, 1347);
        if (ok == 1)
                printf("Account eliminato con successo. ");
        else
                printf("Impossibile eliminare account.\n");
        return ok;

}

void cambia_pass(int sock_ds){
        int ret;
        char *new_pw;
	
        printf("Inserisci la nuova password:\n");
#ifndef CRYPT
        if (scanf("%ms", &new_pw) == -1 && errno != EINTR)
                error(1423);
	fflush(stdin);
        write_string(sock_ds, new_pw, 1530);
#else
	char *crypted;
	char salt[3];
	struct crypt_data data;
	data.initialized = 0;

	new_pw = getpass("(per uscire con CTRL+C, premerlo e dare invio)\n");
	if (strstr(new_pw, "\003") != NULL){ //è presente un ctrl+c
		pid_t my_pid = getpid(); //got my pid
		kill(my_pid, SIGINT);
	}
	random_salt(salt);
	crypted = crypt_r(new_pw, salt, &data);
        write_string(sock_ds, crypted, 1530);
#endif
        read_int(sock_ds, &ret, 1519);

        if (ret)
                printf("Password cambiata con successo.\n");
	else{
		printf("Si è verificato un errore. Chiudo.\n");
		exit(EXIT_FAILURE);
	}
        free(new_pw);
}

