#include "helper.h"
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

typedef struct MESSAGE{
        char *usr_destination;
       	char *usr_sender;
       	char *object;
        char *text;
        int *is_new;
	int *position; //message's position
} message;


void inizializza_server(message **mex_list){ //sequenza di messaggi
	int i;

	for (i = 0; i < MAX_NUM_MEX; i++){
//		mex = mex_list[i];	
		//sharing mex
		if ((mex_list[i] = (message*) mmap(NULL, (sizeof(message)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) == (message*) -1){
			perror("error mapping");
			exit(EXIT_FAILURE);
		}
			//sharing usr_destination
		if ((mex_list[i] -> usr_destination = (char*) mmap(NULL, (sizeof(char) * MAX_USR_LEN), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) == (char*) -1){
			perror("error mapping");
			exit(EXIT_FAILURE);
		}	bzero(mex_list[i] -> usr_destination, MAX_USR_LEN);

		//sharing usr_sender
		if ((mex_list[i] -> usr_sender = (char*) mmap(NULL, (sizeof(char) * MAX_USR_LEN), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) == (char*) -1){
			perror("error mapping");
			exit(EXIT_FAILURE);
		}	bzero(mex_list[i] -> usr_sender, MAX_USR_LEN);

		//sharing object
		if ((mex_list[i] -> object = (char*) mmap(NULL, (sizeof(char) * MAX_OBJ_LEN), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) == (char*) -1){
			perror("error mapping");
			exit(EXIT_FAILURE);
		}	bzero(mex_list[i] -> object, MAX_OBJ_LEN);

		//sharing text
		if ((mex_list[i]-> text = (char*) mmap(NULL, (sizeof(char) * MAX_MESS_LEN), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) == (char*) -1){
			perror("error mapping");
			exit(EXIT_FAILURE);
		}	bzero(mex_list[i] -> text, MAX_MESS_LEN);
	
		//sharing is_new 
		if ((mex_list[i]-> is_new = (int*) mmap(NULL, (sizeof(int)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) == (int*) -1){
			perror("error mapping");
			exit(EXIT_FAILURE);
		}
		//sharing position 
		if ((mex_list[i]-> position = (int*) mmap(NULL, (sizeof(int*)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) == (int*) -1){
			perror("error mapping");
			exit(EXIT_FAILURE);
		}
		*(mex_list[i]-> position) = i;
	}
	return;
}


int leggi_messaggi(int sock_ds, char *my_usrname, int flag){ //if flag == 1, only new messages will be print
	int found, again = 1, isnew, len_send, len_obj, len_text, pos, isfirst = 1, wb;
	char *sender, *object, *text;
	message *mex;
	if ((mex = malloc(sizeof(mex))) == NULL){
		perror("error line 86");
		exit(EXIT_FAILURE);
	}
	
	audit;
	if ((mex -> usr_destination = malloc(sizeof(char) *MAX_USR_LEN)) == NULL){
		perror("error line 91");
		exit(EXIT_FAILURE);
	};
	printf("mex -> usr_dest: %p\n", mex -> usr_destination);

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
		 * 								 strlen("RE: <object>") <= MAX_OBJ_LEN		*/
		if (strlen(object) <= MAX_OBJ_LEN - 4){
			printf("vuoi rispondere al messaggio? (1 = si, qualsiasi altro tasto = no)\n");	

			if (scanf("%d", &wb) == -1 && errno != EINTR){
				perror("error");
				exit(EXIT_FAILURE);
			}
			fflush(stdin);
			write_int(sock_ds, wb, 154);
			if (wb == 1)
				write_back(sock_ds, object, my_usrname, sender);
		}

usr_will:	
		/*CHECKING USR'S WILL*/
		printf("cercare un altro messaggio? (1 = si, 0 = no)\npuoi anche inserire un numero negativo per cancellare l'ultimo messaggio letto.\n");
		
		if (scanf("%d", &again) == -1 && errno != EINTR){
			perror("error9");
			exit(EXIT_FAILURE);
		}
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



int check_destination(char **usr_destination){
        int len = strlen(*usr_destination) + 8, fileid;
        char *destination_file;

        if ((destination_file = malloc(sizeof(char) * len)) == NULL){
                perror("error checking existing dest");
                exit(EXIT_FAILURE);
        }
        bzero(destination_file, len);
        sprintf(destination_file, ".db/%s.txt", *usr_destination);

        if ((fileid = open(destination_file, O_RDONLY)) == -1){
                if (errno == ENOENT){
                //file doesnt exist
                return 0;
                }
                else{
                        perror("error checkin esisting dest");
                        exit(EXIT_FAILURE);
                }
        }

        if (close(fileid) == -1){
                perror("damn error closing dest");
                exit(EXIT_FAILURE);
        }
        free(destination_file);

        return 1;
}



int ricevi_messaggio(int acc_sock, message **mess_list, int *position, int *last, int *server, int sem_write, int *my_mex, int *my_new_mex, char *client_usr, int flag){ //flag == 1 means its a write back
	char *usr_destination, *usr_sender, *object, *text, *destination_file;
	int i, len, fileid, exist;
	struct sembuf sops;

	sops.sem_flg = 0;
	sops.sem_num = 0;
	sops.sem_op = -1;

start:

	/*	READING USR_DESTINATION	*/
	read_string(acc_sock, &usr_destination, 199);
	len = strlen(usr_destination);

	/*	CHECKING IF DESTINATION EXISTS AND SENDING RESPONSE	*/
	exist = check_destination(&usr_destination);	
	/*	SENDING IF EXISTS	 */
	write_int(acc_sock, exist, 269);

	if (!exist){
		if (!flag)
			goto start;
		else
			return 1;
	}

	/*	READING USR_SENDER	*/
	read_string(acc_sock, &usr_sender, 231);
	
	/*	READING OBJECT	*/
	read_string(acc_sock, &object, 235);

	/*	READING TEXT	*/
	read_string(acc_sock, &text, 238);

	/*	TRY TO GET CONTROL	*/
	if (semop(sem_write, &sops, 1) == -1){
		perror("error decrementing sem");
		exit(EXIT_FAILURE);
	}		

	message *mex = mess_list[*position];

	sprintf((mex -> usr_destination), "%s", usr_destination);
	sprintf((mex ->usr_sender), "%s", usr_sender);
	sprintf((mex -> object), "%s", object);
	sprintf((mex -> text), "%s", text);
	*(mex -> is_new) = 1;
//	*(mex -> position) = *position;
/*
	printf("dest = %s\n", (mex -> usr_destination));
	printf("send = %s\n", (mex ->usr_sender));
	printf("obj = %s\n", (mex -> object));
	printf("text = %s\n", (mex -> text));
	printf("new = %d\n", *(mex -> is_new));
	printf("mes_pos = %d\n", *(mex -> position));
	printf("last = %d\n", *last);
	printf("pos = %d\n", *position);
*/
	server[*position] = 1;
	
	/*	UPDATING SHARED PARAMS	*/
	if (*position == *last){
		*last = *last + 1;	
		*position = *position + 1;
	}
	else
		update_position(position, server, *last);

	update_system_state(my_mex, my_new_mex, mess_list, client_usr, *last, server);	
	
	/*	GIVE CONTROL TO OTHERS	*/
	sops.sem_op = 1;
	if (semop(sem_write, &sops, 1) == -1){
		perror("error incrementing sem");
		exit(EXIT_FAILURE);
	}

	return 0;
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
		if (scanf("%d", &code) == -1 && errno != EINTR){
			perror("error getting code");
			exit(EXIT_FAILURE);
		}
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
		if (scanf("%d", &again) == -1 && errno != EINTR){
			perror("error scanf at line 353");
			exit(EXIT_FAILURE);
		}
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



int gestore_eliminazioni(int acc_sock, char *usr, message **mex_list, int *my_mex, int *my_new_mex, int *server, int sem_write, int *position, int *last){
	int code, is_mine, compl, again, mode, ret = 0;
	struct sembuf sops;
	message *mex;

	printf("\nstarting gestore_eliminazioni.\n");	
	/*READ MODE*/	
	read_int(acc_sock, &mode, 369);
	if (mode >= 0)
		code = mode;
	else
		another_code:
		read_int(acc_sock, &code, 374);
	
//	printf("code = %d\n", code);
	if (code < 0){
		printf("opearazione annullata client side\n\n");
		return 1;
	}

	is_mine = my_mex[code];
	/*SEND IF THE CODE IS ACCEPTED*/
	write_int(acc_sock, is_mine, 383);

	if (is_mine == 1){
		printf("codice %d accettato\n", code);

		/*TRY GETTING CONTROL*/
		sops.sem_flg = 0;
		sops.sem_num = 0;
		sops.sem_op = -1;
		if (semop(sem_write, &sops, 1) == -1){
			printf("errore semop alla riga %d\n", 393);
			perror("error");
			exit(EXIT_FAILURE);
		}

		/*START EMPTYNG MESSAGE*/
		mex = mex_list[code];
		bzero(mex -> usr_destination, strlen(mex -> usr_destination));
		bzero(mex -> usr_sender, strlen(mex -> usr_destination));
		bzero(mex -> text, strlen(mex -> usr_destination));
	
		/*START UPDATING BITMASKS*/
		server[code] = 0;
		update_system_state(my_mex, my_new_mex, mex_list, usr, *last, server);

		/*START UPDATING SHARED PARAMS*/
		if (code == *last - 1)
			//*last = code;  /*se ho server = [0 0 0 0 0 1], e viene cancellato il mess in posizione 5, devo stampare [], e non [0 0 0 0 0]*/
			update_last(server, last);
		if (code < *position)
			*position = code;
		else
			update_position(position, server, *last);

		/*UPDATING SEM VALUE*/
		sops.sem_op = 1;
		if (semop(sem_write, &sops, 1) == -1){
			printf("errore semop alla riga %d\n", 408);
			perror("error");
			exit(EXIT_FAILURE);
		}

		/*SENDING ELIMINATION COMPLETED*/
		compl = 1;
		write_int(acc_sock, compl, 415);
	}
	else{//not is mine. 
		if (mode >= 0) //trying deleting again the same mex after read it.
			printf("messaggio già eliminato\n");
		else
			goto another_code;
	}

	if (mode < 0){
		read_int(acc_sock, &again, 444);
		if (again)
			goto another_code;
	}	
	return 1;	
}		
		
void stampa_messaggio(message *mess){
	char c;
	if (*(mess -> is_new))
		c = 'y';
	else
		c = 'n';
	
	printf(".....................................................................................\n\n");
	printf("\t\tcodice = %d\n", *(mess -> position));
	printf("is new = %c\n", c);
	printf("from:\n\t%s\n", mess -> usr_sender);	
	printf("to:\n\t%s\n", 	mess -> usr_destination);
	printf("object:\n\t%s\n", mess -> object);
	printf("text:\n\t%s\n\n", mess -> text);
	printf(".....................................................................................\n");

}


void invia_messaggio(int acc_sock, char *sender){
	char *destination, *obj, *mes;
	int len_dest, len_send, len_obj, len_mess, ret;

restart:
	//GETTING DATA AND THEIR LEN
	printf("inserisci l'username del destinatario (max %d caratteri):\n", MAX_USR_LEN);
	if (scanf("%ms", &destination) == -1 && errno != EINTR){
		perror("errore inizializzare destinazione");
		exit(-1);
	}
	fflush(stdin);	

	len_dest = strlen(destination);
	if (len_dest > MAX_USR_LEN){
		printf("usrname destinatario inserito troppo lungo. per favore riprova.\n\n");
		goto restart;
	}

	//its len was checked at login/registration act
	len_send = strlen(sender);

	printf("inserisci l'oggetto del messaggio (max %d caratteri):\n", MAX_OBJ_LEN);
	if (scanf("%m[^\n]", &obj) == -1 && errno != EINTR){
		perror("errore inizializzare oggetto");
		exit(-1);
	}
	fflush(stdin);

	len_obj = strlen(obj);
	if (len_obj > MAX_OBJ_LEN){
		printf("oggetto inserito troppo lungo. per favore riprova.\n\n");
		goto restart;
	}

	printf("inserisci il testo del messaggio: (max %d caratteri):\n", MAX_MESS_LEN);
	if (scanf("%m[^\n]", &mes) == -1 && errno != EINTR){
		perror("errore inizializzare messaggio");
		exit(-1);
	}
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



int gestore_letture(int acc_sock, message **mess_list, int *last, char *usr, int flag, int *my_mex, int *my_new_mex, int *server, int sem_write, int *position){
	//server side
	int i = 0, found = 0, isnew, len_send, len_obj, len_text, pos, again, ret = 0, temp_last = *last, wb;
	message *mex;
	char *sender, *obj, *text;

	for (i = 0; i < temp_last; i++){
		mex = mess_list[i];
			
		stampa_messaggio(mex);
		if (my_mex[i] == 1){		
			found = 1;

			//parsing dei campi del messaggio:
			isnew = my_new_mex[i];
			if (isnew == 0 && flag){//messaggio letto, e voglio solo i nuovi
				found = 0;
				continue;
			}

			/*WRITING FOUND*/
			write_int(acc_sock, found, 710);
			
			/*WRITING IS_NEW*/
			write_int(acc_sock, *(mex -> is_new), 714);

			/*WRITING SENDER*/
			write_string(acc_sock, mex -> usr_sender, 718);
			
			/*WRITING OBJECT*/		
			write_string(acc_sock, mex -> object, 723);
	
			/*WRITING TEXT*/
			write_string(acc_sock, mex -> text, 727);
	
			/*WRITING POSITION*/
			write_int(acc_sock, i, 731); //*(mex -> position), 731);

			*(mex -> is_new) = 0;		
			found = 0;
			/*READING IF WRITE BACK*/
			read_int(acc_sock, &wb, 613);
			if (wb == 1)
				ricevi_messaggio(acc_sock, mess_list, position, last, server, sem_write, my_mex, my_new_mex, usr, 1);	

read_usr_will:
			/*READING USR'S WILL*/
			read_int(acc_sock, &again, 647);	
			
			if (again < 0){
				ret = gestore_eliminazioni(acc_sock, usr, mess_list, my_mex, my_new_mex, server, sem_write, position, last);
				goto read_usr_will;
			}
			else if (again == 0)
				break;			
		}
	}
	if (i == temp_last)
		write_int(acc_sock, found, 615);
	
	return ret;
}



int managing_usr_menu(int acc_sock, message **message_list, int *position, int *last, char *usr, int *my_mex, int *my_new_mex, int *server, int sem_write, char *client_usrname){
	
	int operation = 0, ret, flag = 0;
	//checking for update
	ret = update_system_state(my_mex, my_new_mex, message_list, usr, *last, server);
	//sending ret
	write_int(acc_sock, ret, 714);

	while (operation != 6){
		
		printf(".....................................................................................\n");
		printf("\ngestore di: %s\n\n", client_usrname);		
		printf("stampo my_mex:\n");
		stampa_bitmask(my_mex, *last);

		printf("stampo my_new_mex:\n");
		stampa_bitmask(my_new_mex, *last);
	
		printf("stampo server:\n");
		stampa_bitmask(server, *last);

		printf("\nwaiting for an operation:\n");
		read_int(acc_sock, &operation, 693);	
		
		printf("inserita l'operazione %d\n", operation); 
		switch (operation){
			case 0:
				log_out(usr);
				return 0;
			case 1:
				flag = 0;	
is_read_op:
	                        ret = gestore_letture(acc_sock, message_list, last, usr, flag, my_mex, my_new_mex, server, sem_write, position);				
				if (ret >= 0)
					printf("messaggi consegnati con successo\n");
				else
					printf("errore consegna\n");
			
				ret = update_system_state(my_mex, my_new_mex, message_list, usr, *last, server);
				//sending if updated:
				write_int(acc_sock, ret, sizeof(ret));
				break;
			case 2:
				flag = 1;
				goto is_read_op;

			case 3:

				ret = ricevi_messaggio(acc_sock, message_list, position, last, server, sem_write, my_mex, my_new_mex, usr, 0);
				if (ret >= 0)
					printf("messaggio ricevuto con successo\n");
				else
					printf("errore ricezione\n");
				
				write_int(acc_sock, ret, sizeof(ret));
	
/*				printf("il messaggio inserito è:\n\n");
				stampa_messaggio(message_list[*position - 1]);//stampo accedendo alla lista*/
				break;

                	case 4:       
				ret = gestore_eliminazioni(acc_sock, usr, message_list, my_mex, my_new_mex, server, sem_write, position, last);
				break;
		        
			case 5:
				ret = update_system_state(my_mex, my_new_mex, message_list, usr, *last, server);
				write_int(acc_sock, ret, sizeof(ret));
				printf("stato aggiornato.\n");
				break;
		 	case 6:
				log_out(usr);
				close_server(acc_sock, usr);
			case 7:
				delete_user(acc_sock, usr, message_list, server, my_mex, last, position, sem_write);
				close_server(acc_sock, usr);
			case 8:
				server_test(acc_sock, message_list, position, sem_write);
				break;
			default:
               		        break;
       		}
	}
}

void close_server(int acc_sock, char *usr){
	if (close(acc_sock) == -1){
		perror("error closing accepted socket");
		exit(EXIT_FAILURE);
	}
	printf("starting log out\n");	
//	log_out(usr);	
	printf("collegamento chiuso con successo.\n");
        exit(EXIT_SUCCESS);	
}


void usr_registration_login(int sock_ds, char **usr){
//	printf("not implemented yet :)\n");

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
	
	if (scanf("%d", &operation) == -1 && errno != EINTR){
		perror("error accepting operation");
		exit(EXIT_FAILURE);
	}
	//while(getchar() != '\n') {};
	fflush(stdin);

	if (operation < 0 || operation > 2){
		printf("operazione non valida. riprova:\n\n");
		goto portal;
	        exit(EXIT_FAILURE);
	}

	
	//sending the selected operation:
	if (write(sock_ds, &operation, sizeof(operation)) == -1){
		perror("error sending usr's len");
		         exit(EXIT_FAILURE);

	}
	
	if (operation == 0){
		free(*usr);
		close_client(sock_ds);
	}

	//same structure for registration and login
get_usr:
	printf("inserisci username (max %d caratteri):\n", MAX_USR_LEN);
	if (scanf("%s", *usr) == -1 && errno != EINTR){
		perror("error reading username");
	        exit(EXIT_FAILURE);
	}
	fflush(stdin);

	len = strlen(*usr);
	if (len > MAX_USR_LEN){
		printf("usrname too long. try again:\n");
		goto get_usr;
	}
	if (write(sock_ds, &len, sizeof(len)) == -1){
		perror("error sending usr's len");
		exit(EXIT_FAILURE);
	}

	if (write(sock_ds, *usr, len) == -1){
		perror("error sending usr");
		exit(EXIT_FAILURE);
	}
pw_get:		
	printf("inserisci password (max %d caratteri):\n", MAX_PW_LEN);
	if (scanf("%ms", &pw) == -1 && errno != EINTR){
		perror("error reading pass");
	        exit(EXIT_FAILURE);

	}
	len = strlen(pw);
	if (len > MAX_PW_LEN){
		printf("password too long. try again:\n");
		goto pw_get;
	}

	//while(getchar() != '\n') {};
	fflush(stdin);

	if (write(sock_ds, &len, sizeof(len)) == -1){
		perror("error sending pw's len");
 		exit(EXIT_FAILURE);
	}
	
	if (write(sock_ds, pw, len) == -1){
		perror("error sending usr");
		exit(EXIT_FAILURE);
	}

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

void managing_usr_registration_login(int acc_sock, char **usr){
	int operation, len_usr, len_pw, i, ret, can_i_exit = 0;
	char *pw, *file_name, stored_pw[MAX_PW_LEN + 1], is_log, curr;
	int fileid;
	
check_operation:
	ret = 0;
	printf("waiting for an operation:\n");
	read_int(acc_sock, &operation, 931);	

	printf("operation %d accepted\n", operation);
	
	if (operation == 0){
		printf("selected exit option.\n");		

		exit(EXIT_SUCCESS);
	}
	else{
	      
		/*	READING USR	*/
		read_string(acc_sock, usr, 944);
		len_usr = strlen(*usr);
	
		if (len_usr > MAX_USR_LEN){
			printf("usrname too long\n");
			ret = 4;
			goto send_to_client;	
		}
	
		/*	READING PASSWORD	*/	
		read_string(acc_sock, &pw, 958);
		len_pw = strlen(pw);	

		/*	MAKING THE STRING: ".db/<name_user>.txt"	*/
		if ((file_name = malloc(sizeof(char) * (len_usr + 8) )) == NULL){
			perror("error creating file (1)");
			exit(EXIT_FAILURE);
		}
		bzero(file_name, (len_usr + 8));
		sprintf(file_name, ".db/%s.txt", *usr);

		if (operation == 1){
			printf("selected registration option.\nwating for data:");		
			//i have to sign in the username
			if ((fileid = open(file_name, O_CREAT | O_APPEND | O_RDWR | O_EXCL, 0666)) == -1){
				if (errno == EEXIST){ //file gia esistente
					ret = 1; 
					goto send_to_client;
				}
				else{
					perror("error creating file");
					exit(EXIT_FAILURE);
				}
			}
			//ive created file. i have to write pw and a bit: default 0.
			if (write(fileid, pw, len_pw) == -1 || write(fileid, "\n0", 2) == -1){
				perror("error writing file");
				exit(EXIT_FAILURE);
			}	
			printf("registrazione avvenuta.\n");
	
			goto send_to_client;
		}
	
		else{//it was a log in operation
			//opening file:
			printf("selected login option.\n");		
			if ((fileid = open(file_name, O_RDWR, 0666)) == -1){
				if (errno == ENOENT){ //file doesn exist
					ret = 2;
					goto send_to_client;
				}
				else{
					perror("error opening file");
					exit(EXIT_FAILURE);
				}
			}
			
			/*	STARTING LOGIN PROCEDURE	*/
			lseek(fileid, 0, SEEK_SET); //to start
			for (i = 0; i < MAX_PW_LEN + 1; i++){
				if (read(fileid, &curr, 1) == -1){
					perror("error checking pw(1)");
					exit(EXIT_FAILURE);
				}
				if (curr == '\n')
					break;
				else
					stored_pw[i] = curr;
			}
		
			/*	CHECKING IF PW IS CORRECT	*/
			if (strcmp(stored_pw, pw) != 0){
				printf("no matching pw\n");
				ret = 2; 
				goto send_to_client;
			}
		
			/*	CHECK IF YET LOGED	*/
			if (read(fileid, &is_log, 1) == -1){
				perror("error checking if logged");
				exit(EXIT_FAILURE);

			}
			if((atoi(&is_log)) == 1){
				printf("usr already logged\n");
				ret = 3; 
				goto send_to_client;
			}

			/*	SWITCHING THE LOG BIT	*/
			lseek(fileid, i + 1, SEEK_SET); 
			if (write(fileid, "1", 1) == -1){
				perror("error logging usr");
				exit(EXIT_FAILURE);

			}
			ret = 0;
			can_i_exit = 1;
		}
	}
send_to_client:		
	close(fileid);
	free(file_name);
	/*	SENDING SERVER ANSWER TO CLIENT	*/
	write_int(acc_sock, ret, 997);
/*	if (write(acc_sock, &ret, sizeof(ret)) == -1){       
 		perror("error sanding return value");
		exit(EXIT_FAILURE);
	}*/

	if (!can_i_exit)//registration option completed
		goto check_operation;
	else
		return;
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
	printf("|____ ________ ________ ________ ________ ________ ________ ________ ________ _____ _|\n\n");

	printf("quale operazione vuoi svolgere?\n");

	if (scanf("%d", &operation) == -1 && errno != EINTR){
		perror("error scanf");
		exit(-1);
	}
	//while(getchar() != '\n') {};	
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
//			fflush(stdin);
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
//			check_upd = 1;
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
			client_test(sock_ds);
			break;
		default:
			break;	
	}
	printf("premi INVIO per continuare: ");
	fflush(stdin);
	printf("\e[1;1H\e[2J");

	goto select_operation;

}

void close_client(int sock_ds){
	
	close(sock_ds);
	printf("l'applicazione si chiuderà in 3 secondi. arrivederci :)\n");
	if (!sleep(3)){
		printf("\e[1;1H\e[2J");
		exit(EXIT_SUCCESS);
	}
}


void log_out(char *usr){
	char *file_name, curr;
	int len, fileid, i;

	len = strlen(usr);
	if ((file_name = malloc(sizeof(char) * (len + 8))) == NULL){
		perror("error malloc. i cant solve the log out request");
		exit(EXIT_FAILURE);
	}
	sprintf(file_name, ".db/%s.txt", usr);

	len = strlen(file_name);
	if ((fileid = open(file_name, O_RDWR, 0666)) == -1){
		perror("error opening file to logout");
		exit(EXIT_FAILURE);
	}	
	
	for (i = 0; i < MAX_PW_LEN + 1; i++){
		if (read(fileid, &curr, 1) == -1){
			perror("error checking pw(1)");
			exit(EXIT_FAILURE);
		}
		if (curr == '\n')
			break;
	}
	
	if (write(fileid, "0", 1) == -1){
		perror("error logging usr");
		exit(EXIT_FAILURE);
	}

	close(fileid);
	free(file_name);
}


void update_last(int *server, int *last){
	int i, old = *last;

	for (i = old - 1; i > -1; i--)
		if (server[i] == 0)
			*last = i;
		
		else
			break;
}

int update_system_state(int *my_mex, int *my_new_mex, message **mex_list, char *usr, int last, int *server){
	int i, cmp, found_new = 0;
//	printf("\n\nupdate system:\n");

	for (i = 0; i < last; i++){
		if (!server[i]){
			my_mex[i] = -1;
			my_new_mex[i] = -1;
			continue;
		}
		cmp = strcmp(mex_list[i] -> usr_destination, usr);
		if (!cmp){
			my_mex[i] = 1;
			if (*(mex_list[i] -> is_new) == 1){
				my_new_mex[i] = 1;
				found_new = 1;
			}
			else{
				my_new_mex[i] = 0;
			}
		}
		else{//not message for me: or its free or its for others
			cmp = strcmp(mex_list[i] -> usr_destination, "");
			if (!cmp){ //free slot
				my_mex[i] = -1;
				my_new_mex[i] = -1;
//				server[i] = 0; 
			}
			else{ //for others
				my_mex[i] = 0;
				my_new_mex[i] = 0;
			}
		}
	}
	return found_new;
}

void update_position(int *position, int *server, int last){
	int i;
	for(i = 0; i < last + 1; i++)		
		if (!server[i]){
			*position = i;
			break;	
		}

}

void stampa_bitmask(int *bitmask, int last){
	int i;

	for(i = 0; i < last; i++)
		printf("%d ", bitmask[i]);
	printf("\n");
}



void write_string(int sock, char *string, int line){
	int len = strlen(string);
//	printf("string %s\nlen %d\n\n", string, len);
	if (write(sock, &len, sizeof(len)) == -1){
		printf("error write string at line %d (1)\n", line);
		perror("error");
		exit(EXIT_FAILURE);
	}

	if (write(sock, string, len) == -1){
		printf("error write string at line %d (2)\n", line);
		perror("error");
		exit(EXIT_FAILURE);
	}
}

void read_string(int sock, char **string, int line){
	int len;

	if (read(sock, &len, sizeof(len)) == -1){
		printf("error write at line %d (1)", line);
		perror("error");
		exit(EXIT_FAILURE);
	}
	
	if ((*string = malloc(sizeof(char) * len)) == NULL){
		printf("error malloc at line %d.\n", line);
		perror("error");
		exit(EXIT_FAILURE);
	}
	bzero(*string, len);

	if (read(sock, *string, len) == -1){
		printf("error write at %d (2).\n", line);
		perror("error");
		exit(EXIT_FAILURE);
	}
}

void write_int(int sock, int num, int line){;

	if (write(sock, &num, sizeof(num)) == -1){
		printf("error write at line %d.\n", line);
		perror("error");
		exit(EXIT_FAILURE);
	}
}

void read_int(int sock, int *num, int line){

	if (read(sock, num, sizeof(*num)) == -1){
		printf("error read at line %d.\n", line);
		perror("error");
		exit(EXIT_FAILURE);
	}
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

	/*CREATING THE STRING: 	RE: <object>	*/
	if ((re_obj = malloc(sizeof(char) * (len + 4))) == NULL){
		perror("error malloc at 1302");
		exit(EXIT_FAILURE);
	} bzero(re_obj, len + 4);

	sprintf(re_obj, "RE: %s", object);

	/*GETTING THE TEXT FROM USR*/
	printf("inserisci il messaggio:\n");
	if (scanf("%m[^\n]", &text) == -1 && errno != EINTR){
		perror("errore inizializzare oggetto");
		exit(-1);
	} 
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

//	write_string(sock_ds, usr, 1343);

	read_int(sock_ds, &ok, 1347);
	if (ok == 1)
		printf("account eliminato con successo. ");
	else
		printf("impossibile eliminare account.\n");

	return ok;
	
}

int delete_user(int acc_sock, char *usr, message **mex_list, int *server, int *my_mex, int *last, int *position, int sem_write){
	int len, bol, i, old_last = *last;
	char *dest;
	struct sembuf sops;
	message *mex;

	len = strlen(usr);
	dest = malloc(sizeof(char) * (len + 8));

	/*MAKE THE STRING .db/<usr>.txt */
	sprintf(dest, ".db/%s.txt", usr);
	
	if (unlink(dest) == -1){
		printf("impossibile eliminare file.\n");
		bol = 0;
	}
	else{
		printf("file eliminato\n");
		bol = 1;
	}
	
	/*must set 0 on server, and freeing my messages*/

	/*TRY GETTING CONTROL*/
	sops.sem_flg = 0;
	sops.sem_num = 0;
	sops.sem_op = -1;
	if (semop(sem_write, &sops, 1) == -1){
		printf("errore semop alla riga %d\n", 393);
		perror("error");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < *last; i++){
		if (my_mex[i] == 1){ //i is the index of a mex i have to free
	
			/*START EMPTYNG MESSAGE*/
			mex = mex_list[i];
			bzero(mex -> usr_destination, strlen(mex -> usr_destination));
			bzero(mex -> usr_sender, strlen(mex -> usr_destination));
			bzero(mex -> text, strlen(mex -> usr_destination));
	
			/*START UPDATING BITMASKS*/
			server[i] = 0;
//			update_system_state(my_mex, my_new_mex, mex_list, usr, *last, server);

			if (i < *position)
				*position = i; //eseguito almeno una volta 100%
		}
	}

	update_last(server, last);

	/*UPDATING SEM VALUE*/
	sops.sem_op = 1;
	if (semop(sem_write, &sops, 1) == -1){
		printf("errore semop alla riga %d\n", 408);
		perror("error");
		exit(EXIT_FAILURE);
	}

	write_int(acc_sock, bol, 1376);
	free(dest);
	return bol;
}


void write_mex(int sock, message *mex){
	int max_len = MAX_USR_LEN + MAX_USR_LEN + MAX_OBJ_LEN + MAX_MESS_LEN + 4;
	char one_string[max_len];

	bzero(one_string, max_len);

	/*	sender, destination, object, mex	*/
	sprintf(one_string, "%s\n%s\n%s\n%s\n", mex -> usr_sender, mex -> usr_destination, mex -> object, mex -> text);
	
	write_string(sock, one_string, 1433);
}




void read_mex(int sock, message **mex_list, int *position, int semid){
	int max_len = MAX_USR_LEN + MAX_USR_LEN + MAX_OBJ_LEN + MAX_MESS_LEN + 4, i;
	char *one_string, *token;
	struct sembuf sops;

	one_string = malloc(sizeof(char) * max_len);
	bzero(one_string, max_len);
	sops.sem_num = 0;
	sops.sem_flg = 0;
	sops.sem_op = -1;

	read_string(sock, &one_string, 1445);
	/*	TRY TO GET CONTROL	*/
	if (semop(semid, &sops, 1) == -1){
		perror("error at line 1453");
		exit(EXIT_FAILURE);
	}
	
	message *mex = mex_list[*position];

	/*	PARSING THE STRING	*/	
/*	sender, destination, object, mex	*/

	token = strtok(one_string, "\n");
	strcpy(mex -> usr_sender, token);

	for (i = 0; i < 3; i++){
		token = strtok(NULL, "\n");			
		switch (i){
			case 0:
				strcpy(mex -> usr_destination, token);
				break;
			case 1:
				strcpy(mex -> object, token);
				break;
			case 2:
				strcpy(mex -> text, token);
				break;
		}
	}


	/*	GIVE CONTROL TO OTHERS	*/

	sops.sem_op = 1;
	if (semop(semid, &sops, 1) == -1){
		perror("error at line 1453");
		exit(EXIT_FAILURE);
	}
	free(one_string);
}

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
	
//	printf("mex %p\n", mex);

//	stampa_messaggio(mex);
	write_mex(sock_ds, mex);
	free(mex);
}

void server_test(int acc_sock, message **mex_list, int *position, int semid){
	
	read_mex(acc_sock, mex_list, position, semid);

	stampa_messaggio(mex_list[*position]);
}
