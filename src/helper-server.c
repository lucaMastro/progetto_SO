#define _XOPEN_SOURCE
#define _GNU_SOURCE
#include "../lib/helper.h"
#include "../lib/helper-server.h"
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
#include <crypt.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define fflush(stdin) while(getchar() != '\n')
#define audit printf("ok\n")


void send_file_db(int acc_sock){
	int fileid, i, found = 1, sentinel;
	char usr[MAX_USR_LEN + 1], curr;

	bzero(usr, MAX_USR_LEN + 1);

	fileid = open(".db/list.txt", O_RDONLY);
	if (fileid == -1)
		error(26);
	while(1){
		bzero(usr, MAX_USR_LEN + 1);
		for (i = 0; i < MAX_USR_LEN + 1; i++){ //voglio leggere sempre anche lo \n
			if ((sentinel = read(fileid, &curr, 1)) == -1)
				error(32);
			else if (sentinel == 0){ //fine file
				found = 0;
				break;
			}
				
			if (curr == '\n') //ultimo carattere di una riga. non lo copio in "usr".
				break;
			else
				usr[i] = curr;
		}
		write_int(acc_sock, found, 46);
		if (!found)
			break;
		//ho letto una riga. la scrivo:	
		write_string(acc_sock, usr, 45);
	}
	close(fileid);
}



void update_db_file(char *deleting_string){
	char curr_usr[MAX_USR_LEN + 2], del_string[MAX_USR_LEN + 2], c;
	int fileid, fileid2, i;

	/* curr_usr e del_string di dimensione "max_usr_len + 2" perchè devono 
	 * contenere sia il "\n" del file che il terminatore di stringa.
	 * del_string serve a fare il confronto con ogni riga letta: la riga letta
	 * dal file, infatti, conterrà il "\n" alla fine. in questo modo è possibile
	 * fare il confronto senza dover modificare la stringa passata come 
	 * parametro. */
	bzero(del_string, MAX_USR_LEN + 2);
	sprintf(del_string, "%s\n", deleting_string);
	
	fileid = open(".db/list.txt", O_CREAT | O_RDWR, 0666);
	if (fileid == -1)
		error(7);

	fileid2 = open(".db/list2.txt", O_CREAT | O_TRUNC | O_RDWR | O_APPEND, 0666);
	if (fileid2 == -1)
		error(22);

	lseek(fileid, 0, SEEK_SET);
	while (1){
		bzero(curr_usr, MAX_USR_LEN + 2);
		i = 0;
		c = '\0';

		while (c != '\n'){//while for reading a line
			if (read(fileid, &c, 1) == -1){
				perror("errore read");
				exit(-1);
			}
			curr_usr[i] = c;
			if (c == '\0')//alla fine del file
				break;
			i++;
		}

		if (strcmp(curr_usr, del_string) != 0)
			write(fileid2, curr_usr, strlen(curr_usr));
		if (c == '\0'){
			break;
		}
	}
	system("rm .db/list.txt\nmv .db/list2.txt .db/list.txt");
	close(fileid);
	close(fileid2);
}


message** inizializza_server(){ //sequenza di messaggi
	int i;
	message **mex_list;

	mex_list = (message**) malloc(sizeof(message) * MAX_NUM_MEX);
	if (mex_list == NULL){
		perror("error initializing mex list");
		exit(EXIT_FAILURE);
       	}

	for (i = 0; i < MAX_NUM_MEX; i++){
	       	
		/*  alloco solo l'indirizzo del messaggio, isnew, e position:
		 *  sono gli unici campi che lato client non mi vengono inviati, 
		 *  per cui nella funzione get_mex non verranno parsati (lato 
		 *  server). tutti gli altri campi, invece, vengono parsati 
		 *  da quella e funzione e, in quel momento, allocati.	*/

		mex_list[i] = (message*) malloc(sizeof(message));
		if (mex_list[i] == NULL)
			error(27);

		mex_list[i] -> is_new = 1;

		mex_list[i] -> position = i;

		mex_list[i] -> is_sender_deleted = 0;
	}
	
	return mex_list;
}




int ricevi_messaggio(int acc_sock, message **mess_list, int *position, int *last, int *server, int sem_write, int *my_mex, int *my_new_mex, char *client_usr, int flag){ //flag == 1 means its a write back
	char *usr_destination; 
	int exist, retry, can_i_get = 1;
	struct sembuf sops;

	sops.sem_flg = 0;
	sops.sem_num = 0;
	sops.sem_op = -1;

	if (!flag){
		send_file_db(acc_sock);
	read_dest:
		//CHECKING ERROR ABOUT USR_LEN:
		if(read_int(acc_sock, &retry, 146))
			return -1;

		switch(retry){
			case 0:
				goto read_dest;
			case 1:
				return 1;
			default:
				break;
		}
	}

	/*	READING USR_DESTINATION	*/
	if(read_string(acc_sock, &usr_destination, 199))
		return -1;

	/*	CHECKING IF DESTINATION EXISTS AND SENDING RESPONSE	*/
	exist = check_destination(&usr_destination, NULL);	
	free(usr_destination);
	
	/*	SENDING IF EXISTS	 */
	write_int(acc_sock, exist, 269);

	if (!flag){
		//CHECKING USR_WILL. se exist == 0, retry non può essere -1 (retry == -1 quando va tutto bene)

		if (read_int(acc_sock, &retry, 146)){
			return -1;
		}

		switch(retry){
			case 0:
				goto read_dest;
			case 1:
				return 1;
			default:
				break;
		}

		read_obj:
		//CHECKING ERROR ABOUT OBJ_LEN:
		if (read_int(acc_sock, &retry, 146))
			return -1;
			
		switch(retry){
			case 0:
				goto read_dest;
			case 1:
				goto read_obj;
			case 2:
				return 1;
			default:
				break;
		}
	}

	else{//write back version
		if (!exist)
			return 1;
	}	

read_mess:
	//CHECKING ERROR ABOUT MESS_LEN:
	if (read_int(acc_sock, &retry, 146)){
		return -1;
	}

	if (!flag){
		switch(retry){
			case 0:
				goto read_dest;
			case 1:
				goto read_obj;
			case 2:
				goto read_mess;
			case 3:
				return 1;
			default:
				break;
		}
	}
	else{//CHECKING ERROR ABOUT MESS_LEN IN A WB CALL	
		switch(retry){
			case 0:
				goto read_mess;
			case 1:
				return 1;
			default:
				break;
		}
	}
	
//	printf("try to get control\n");
	
	/*	TRY TO GET CONTROL	*/
	if (semop(sem_write, &sops, 1) == -1)
		error(255);

//	printf("controllo preso\n");	
//	sleep(10); //wait 10 seconds to test concorrence
	
	if (*position == MAX_NUM_MEX){
		can_i_get = -1;
		printf("impossibile ricevere messaggio: spazio non disponibile\n");
		//WRITING THE BAD NEW
		write_int(acc_sock, can_i_get, 174);
		/*incremento del semaforo:*/		
        	sops.sem_op = 1;
	        if (semop(sem_write, &sops, 1) == -1)
        	        error(178);
		return 1; //back to main menu
	}

	/*WRITING IT WAS OK*/
	write_int(acc_sock, can_i_get, 182);

	message *mex = mess_list[*position];
	if (!get_mex(acc_sock, mex, 0)){
		/* ho catturato un ctrl+c client-side: devo incrementare il 
		 * semaforo prima di chiudere il collegamento */
        	sops.sem_op = 1;
	        if (semop(sem_write, &sops, 1) == -1)
        	        error(158);
		return -1;
	}

        /*      UPDATING SHARED PARAMS  */
	server[*position] = 1;

        if (*position == *last){
                *last = *last + 1;
                *position = *position + 1;
        }
        else
                update_position(position, server, *last);

        update_system_state(my_mex, my_new_mex, mess_list, client_usr, *last, server);

        /*      GIVE CONTROL TO OTHERS  */
        sops.sem_op = 1;
        if (semop(sem_write, &sops, 1) == -1)
                error(290);
	return 0;
}




int gestore_letture(int acc_sock, message **mess_list, int *last, char *usr, int flag, int *my_mex, int *my_new_mex, int *server, int sem_write, int *position){
        //server side
        int i, found = 0, isnew, ret = 0, temp_last = *last, op, leave = 0, code, starting_index = 0;
        message *mex;
	
	if (flag == 2){
		//devo leggere un solo messaggio, inviatomi dal client
		if (read_int(acc_sock, &code, 331))
			return -1;
		if (code == -1)
			return 1; //exit conditions
		else{
			/* setto i valori in modo che nel for il primo (e 
			 * solo) messaggio ad essere letto, in caso di flag == 2, 
			 * sia quello il cui codice è specificato in "code".*/
			starting_index = code;
			temp_last = starting_index + 1;
		}
	}

        for (i = starting_index; i < temp_last; i++){
		if (leave)
			break;

                mex = mess_list[i];
                if (my_mex[i] == 1){

                        //parsing dei campi del messaggio:
                        isnew = my_new_mex[i];
                        if (isnew == 0 && flag == 1)//messaggio letto, e voglio solo i nuovi
                                continue;
                        
                        found = 1;
                        write_int(acc_sock, found, 219);
  					
			/*	SENDING MEX	*/
			send_mex(acc_sock, mex, 1);

                        mex -> is_new = 0;
                        found = 0;
			
			/*READING USR_WILL*/
read_usr_will:
        	
			if (read_int(acc_sock, &op, 613))	
				return -1;

			switch(op){
				case 0:
					ricevi_messaggio(acc_sock, mess_list, position, last, server, sem_write, my_mex, my_new_mex, usr, 1);
					break;
				case 1:
					ret = gestore_eliminazioni(acc_sock, usr, mess_list, my_mex, my_new_mex, server, sem_write, position, last);
					break;
				case 2:
					break;
				case 3:
					leave = 1;
					break;
			}
			if (flag == 2)
				leave = 1;				
		}
        }
        if (i == temp_last && leave == 0)
                write_int(acc_sock, found, 615);	
        return ret;
}






int managing_usr_menu(int acc_sock, message **message_list, int *position, int *last, char *client_usrname, int *my_mex, int *my_new_mex, int *server, int sem_write){

        int operation = 0, ret, flag = 0;
        //checking for update
        ret = update_system_state(my_mex, my_new_mex, message_list, client_usrname, *last, server);
        //sending ret
        write_int(acc_sock, ret, 714);

        while (operation != 7){

                printf(".....................................................................................\n");
                printf("......................................MAIN_MENU......................................\n");
                printf("\ngestore di: %s\n\n", client_usrname);
                printf("stampo my_mex:\n");
                stampa_bitmask(my_mex, *last);

                printf("stampo my_new_mex:\n");
                stampa_bitmask(my_new_mex, *last);

                printf("stampo server:\n");
                stampa_bitmask(server, *last);

                printf("\nwaiting for an operation:\n");
                if (read_int(acc_sock, &operation, 693)){
			close_server(acc_sock, client_usrname, 1); 
			return 1;
		}

                printf("inserita l'operazione %d\n", operation);
                switch (operation){
                        case 0:
                                log_out(client_usrname);
                                return 0;
                        case 1:
                                flag = 0;
is_read_op:
                                ret = gestore_letture(acc_sock, message_list, last, client_usrname, flag, my_mex, my_new_mex, server, sem_write, position);
                                if (ret >= 0)
                                        printf("messaggi consegnati con successo\n");
                                else{
                                	close_server(acc_sock, client_usrname, 1);
					return 1;
				}

                                ret = update_system_state(my_mex, my_new_mex, message_list, client_usrname, *last, server);
                                //sending if updated:
                                write_int(acc_sock, ret, 249);
                                break;
                        case 2:
                                flag = 1;
                                goto is_read_op;
			case 3:
				flag = 2;
				goto is_read_op;
                        case 4:

                                ret = ricevi_messaggio(acc_sock, message_list, position, last, server, sem_write, my_mex, my_new_mex, client_usrname, 0);
                                if (ret < 0){
                                	close_server(acc_sock, client_usrname, 1);
					return 1;
				}
                                break;

                        case 5:
                                ret = gestore_eliminazioni(acc_sock, client_usrname, message_list, my_mex, my_new_mex, server, sem_write, position, last);
				if (ret < 0){
                                	close_server(acc_sock, client_usrname, 1);
					return 1;
				}
                                ret = update_system_state(my_mex, my_new_mex, message_list, client_usrname, *last, server);
                                write_int(acc_sock, ret, 453);
                                break;

                        case 6:
                                ret = update_system_state(my_mex, my_new_mex, message_list, client_usrname, *last, server);
                                write_int(acc_sock, ret, 458);
                                printf("stato aggiornato.\n");
                                break;
                        case 7:
                                close_server(acc_sock, client_usrname, 1);
				return 1;
                        case 8:
                                delete_user(acc_sock, client_usrname, message_list, server, my_mex, last, position, sem_write);
                                close_server(acc_sock, client_usrname, 0);
				free(client_usrname);
				return 1;
                        case 9:
                                ret = mng_cambio_pass(acc_sock, client_usrname);
				if (ret < 0){
                                	close_server(acc_sock, client_usrname, 1);
					return 1;
				}
                                break;
                        default:
                                break;
                }
        }
}


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


int managing_usr_registration_login(int acc_sock, char **usr, int sem_log){
        int operation, len_usr, len_pw, i, ret, can_i_exit = 0, retry;
        char *pw, *file_name,  is_log, curr;
        int fileid, fileid2;
	struct sembuf sops;
#ifndef CRYPT
	char stored_pw[MAX_PW_LEN + 1];
#else
	char stored_pw[13 + 1]; //13 is the crypted password's len
	char salt[2 + 1], *crypted;
	struct crypt_data data;
	data.initialized = 0;
#endif

check_operation:

#ifndef CRYPT
        bzero(stored_pw, MAX_PW_LEN + 1);
#else
        bzero(stored_pw, 14);
	random_salt(salt);
#endif

        ret = 0;
        printf("waiting for an operation:\n");
        if (read_int(acc_sock, &operation, 931)){
		close_server(acc_sock, *usr, 0);
		return 0;
	}

        printf("operation %d accepted\n", operation);

        if (operation == 0){
                printf("selected exit option.\n");
		return 0;
        }
        else{
                /*      READING IF RETRY GETTING USRNAME        */
                check_usr_restart:
                if (read_int(acc_sock, &retry, 344)){
			close_server(acc_sock, *usr, 0);
			return 0;
		}

                switch (retry){
                        case 1:
                                goto check_usr_restart;
                        case 2:
                                goto check_operation;
                        default:
                                break;
                }
                /*      READING USR     */
                if (read_string(acc_sock, usr, 944)){
			close_server(acc_sock, *usr, 0);
			return 0;
		}

                /*      READING IF RETRY GETTING PASSWORD       */
                check_pw_restart:
                if (read_int(acc_sock, &retry, 344)){
			close_server(acc_sock, *usr, 0);
			return 0;
		}
                switch (retry){
                        case 1:
                                goto check_pw_restart;
                        case 2:
                                goto check_operation;
                        default:
                                break;
                }

                /*      READING PASSWORD        */
                if (read_string(acc_sock, &pw, 958)){
			close_server(acc_sock, *usr, 0);
			return 0;
		}

                len_pw = strlen(pw);
		len_usr = strlen(*usr);

                /*      MAKING THE STRING: ".db/<name_user>.txt"        */
                if ((file_name = (char*) malloc(sizeof(char) * (len_usr + 9) )) == NULL)
                        error(867);

                bzero(file_name, (len_usr + 9));
                sprintf(file_name, ".db/%s.txt", *usr);

#ifdef CRYPT
		/*crypting password*/
		len_pw = 13; // strlen(crypted) == 13

#endif

                if (operation == 1){
                        printf("selected registration option.\n");
                        //i have to sign in the username
                        if ((fileid = open(file_name, O_CREAT | O_APPEND | O_RDWR | O_EXCL, 0666)) == -1){
                                if (errno == EEXIST){ //file gia esistente
                                        ret = 1;
                                        goto send_to_client;
                                }
                                else
                                        error(881);
                        }

                        //ive created file. i have to write pw and a bit: default 0.
	#ifdef CRYPT
			crypted = crypt_r(pw, salt, &data);
                        if (write(fileid, crypted, len_pw) == -1 || write(fileid, "\n0", 2) == -1)
                                error(885);	
	#else
                        if (write(fileid, pw, len_pw) == -1 || write(fileid, "\n0", 2) == -1)
                                error(885);
	#endif

                        //updating the list
                        fileid2 = open(".db/list.txt", O_CREAT|O_RDWR|O_APPEND, 0666);
                        if (fileid2 == -1)
                                error(439);

                        if (write(fileid2, *usr, strlen(*usr))  == -1 || write(fileid2, "\n", 1) == -1)
                                error(441);
                        close(fileid2);
        		close(fileid);

                        printf("registrazione avvenuta.\n");

                        goto send_to_client;
                }

                else{//it was a log in operation
                        //opening file:
                        printf("selected login option.\n");
			sops.sem_flg = 0;
			sops.sem_num = 0;
			sops.sem_op = -1;

                        if ((fileid = open(file_name, O_RDWR, 0666)) == -1){
                                if (errno == ENOENT){ //file doesn exist
                                        ret = 2;
                                        goto send_to_client;
                                }
                                else
                                        error(900);
                        }
		#ifdef CRYPT
                        /*      STARTING LOGIN PROCEDURE        */
                        lseek(fileid, 0, SEEK_SET); //to start
			for (i = 0; i < 13 + 1; i++){
                                if (read(fileid, &curr, 1) == -1)
                                        error(907);
                                if (curr == '\n')
                                        break;
                                else
                                        stored_pw[i] = curr;

				//salvo i primi due caratteri in salt:
				if (i == 0 || i == 1)
					salt[i] = curr;
                        }
			crypted = crypt_r(pw, salt, &data);

                        /*      CHECKING IF PW IS CORRECT       */
                        if (strcmp(stored_pw, crypted) != 0){
                                printf("no matching pw\n");
                                ret = 2;
                                goto send_to_client;
                        }

		#else
                        /*      STARTING LOGIN PROCEDURE        */
                        lseek(fileid, 0, SEEK_SET); //to start
			for (i = 0; i < MAX_PW_LEN + 1; i++){
                                if (read(fileid, &curr, 1) == -1)
                                        error(907);
                                if (curr == '\n')
                                        break;
                                else
                                        stored_pw[i] = curr;
                        }

                        /*      CHECKING IF PW IS CORRECT       */
                        if (strcmp(stored_pw, pw) != 0){
                                printf("no matching pw\n");
                                ret = 2;
                                goto send_to_client;
                        }
		#endif
                        /*      CHECK IF YET LOGED      */
			/* deve essere fatto in modo atomico */
			if (semop(sem_log, &sops, 1) == -1)
				error(702);

                        if (read(fileid, &is_log, 1) == -1)
                                error(923);

			/* test sncronizzazione
			if (!strcmp(*usr, "a")){
				printf("sleeping\n");
				sleep(10);
			}*/

                        if((atoi(&is_log)) == 1){
                                printf("usr already logged\n");
				//incremento semaforo
				sops.sem_op = 1;
				if (semop(sem_log, &sops, 1) == -1)
					error(702);
                                ret = 3;
                                goto send_to_client;
                        }

                        /*      SWITCHING THE LOG BIT   */
                        lseek(fileid, i + 1, SEEK_SET);
                        if (write(fileid, "1", 1) == -1)
                                error(934);

			sops.sem_op = 1;
			if (semop(sem_log, &sops, 1) == -1)
				error(702);

        		close(fileid);
                        
			ret = 0;
                        can_i_exit = 1;
                }
        }
send_to_client:
        free(file_name);
	free(pw); //read_string doesnt free the string
	if (operation == 1)
		/* if it was a registration, i have to free the usrname, 'cause
		 * i will read another string, using malloc again for the new usr */
		free(*usr); 
	

        /*      SENDING SERVER ANSWER TO CLIENT */
        write_int(acc_sock, ret, 997);

        if (!can_i_exit)//registration option completed
                goto check_operation;
        else
                return 1;
}






void close_server(int acc_sock, char *usr, int flag){ 
	/* flag == 1 means i have to logout. flag != 1 when im deleting usr */
  	if (close(acc_sock) == -1)
                error(541);

	if (flag)        
	   	log_out(usr);   
        printf("collegamento chiuso con successo.\n");
}


int update_system_state(int *my_mex, int *my_new_mex, message **mex_list, char *usr, int last, int *server){
        int i, cmp, found_new = 0;
	/* aggiorna le bitmask. *server deve già essere aggiornato. */

        for (i = 0; i < last; i++){
                if (!server[i]){ //free slot
                        my_mex[i] = -1;
                        my_new_mex[i] = -1;
                }
		else{
			//check if it was sent to me
	                cmp = strcmp(mex_list[i] -> usr_destination, usr);
        	        if (!cmp){ //its mine
                	        my_mex[i] = 1;
                        	if (mex_list[i] -> is_new == 1){
                                	my_new_mex[i] = 1;
	                                found_new = 1;
        	                }
                	        else{
                        	        my_new_mex[i] = 0;
                        	}
	                }
	                else{//not message for me
				my_mex[i] = 0;
				my_new_mex[i] = 0;
        	        }
		}
        }
        return found_new;
}

void update_position(int *position, int *server, int last){
        int i;
	/* position rappresenta la posizione in cui verrà memorizzato il prossimo
	 * messaggio che verrà inviato. voglio che venga inserito nel primo slot
	 * utile: se ho server = [1, 1], e il messaggio in posizione 0 viene 
	 * eliminato (server = [0, 1]) voglio che il prossimo messaggio venga
	 * memorizzato in posizione 0. */
        for(i = 0; i < last + 1; i++){
                if (!server[i]){
                        *position = i;
                        break;
                }
	}
}

void stampa_bitmask(int *bitmask, int last){
        int i;
	
	printf("[");
        for(i = 0; i < last; i++){
                printf("%d", bitmask[i]);
		if (i != last - 1)
			printf(", ");
	}
        printf("]\n");
}

int gestore_eliminazioni(int acc_sock, char *usr, message **mex_list, int *my_mex, int *my_new_mex, int *server, int sem_write, int *position, int *last){
	/*mode >= 0 means this func is called after reading a mex */
        int code, is_mine, conf, again, mode, ret = 0;
        struct sembuf sops;
        message *mex;

        /*READ MODE*/
        if (read_int(acc_sock, &mode, 369))
		return -1;

        if (mode >= 0)
	 	code = mode;	
        else{
                another_code:
                if (read_int(acc_sock, &code, 374))
			return -1;
	}

        if (code < 0){
                printf("operazione annullata client side\n\n");
                return 1;
        }

        is_mine = my_mex[code];
        /*SEND IF THE CODE IS ACCEPTED*/
        write_int(acc_sock, is_mine, 383);

        if (is_mine == 1){
		if (mode < 0){ 
	                mex = mex_list[code];
			send_mex(acc_sock, mex, 1);
			//il messaggio è stato letto in fase di eliminazione:
			mex -> is_new = 0;

			/*LEGGO CONFERMA ELIMINAZIONE*/
	                if (read_int(acc_sock, &conf, 881))
				return -1;
			if (!conf)
				return 1;
		}
		

//		printf("try to get control\n");
                
		/*TRY GETTING CONTROL*/
                sops.sem_flg = 0;
                sops.sem_num = 0;
                sops.sem_op = -1;
                if (semop(sem_write, &sops, 1) == -1)
                        error(398);

//		sleep(10); //wait 10 seconds to test concorrence
//		printf("ho preso il controllo\n");
//
                /*START EMPTYNG MESSAGE*/
                mex = mex_list[code];
                free(mex -> usr_destination);
                free(mex -> usr_sender);
                free(mex -> object);
                free(mex -> text);
		mex -> is_new = 1;

                /*START UPDATING BITMASKS*/
                server[code] = 0;
                update_system_state(my_mex, my_new_mex, mex_list, usr, *last, server);

                /*START UPDATING SHARED PARAMS*/
                if (code == *last - 1)
                        /* se ho server = [0 0 0 0 0 1], e viene cancellato il mess
			 * in posizione 5, devo stampare [], e non [0 0 0 0 0]*/
                        update_last(server, last);
                if (code < *position)
                        *position = code;
                else
                        update_position(position, server, *last);


                /*UPDATING SEM VALUE*/
                sops.sem_op = 1;
                if (semop(sem_write, &sops, 1) == -1)
                        error(422);

                /*SENDING ELIMINATION COMPLETED*/
                write_int(acc_sock, 1, 415);
        }

        if (mode < 0){ 
		/* se chiamata separatamente dalle letture, verifico se eliminare 
		 * un altro mess o no, indipendentemente dal fatto che il primo 
		 * codice fosse accettabile o meno */
                if (read_int(acc_sock, &again, 444))
			return -1;

                if (again)
                        goto another_code;
        }
        return 1;
}




void update_last(int *server, int *last){
        int i, old = *last;

	/* voglio che l'ultimo valore da stampare sia un 1. procedendo all'indietro,
	 * ogni volta che trovo uno 0 diminuisco il valore di *last. quando trovo un
	 * 1 interrompo la ricerca e l'aggiornamento.
	 *
	 * NOTA: questa funzione DEVE essere chiamata solo dopo aver preso un 
	 * 	 gettone dal semaforo per le scritture: *last è, infatti, un 
	 * 	 parametro condiviso a tutti i threads. */

        for (i = old - 1; i > -1; i--)
                if (server[i] == 0)
                        *last = i;
                else
                        break;
}



int delete_user(int acc_sock, char *usr, message **mex_list, int *server, int *my_mex, int *last, int *position, int sem_write){
        int len, bol, i, old_last = *last;
        char *dest;
        struct sembuf sops;
        message *mex;

        len = strlen(usr);
        dest = (char*) malloc(sizeof(char) * (len + 9));
	bzero(dest, len+9);
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

        /*TRY GETTING CONTROL*/
        sops.sem_flg = 0;
        sops.sem_num = 0;
        sops.sem_op = -1;
        if (semop(sem_write, &sops, 1) == -1)
                error(1294);

	//sleep(10); //wait 10 seconds to test concorrence	
	
        for (i = 0; i < *last; i++){
		mex = mex_list[i];
		if (!strcmp(mex -> usr_destination, usr)){
                	/* non viene usato:
			 * if (my_mex[i] == 1){
			 * perchè altrimenti ho dei problemi di concorrenza:
			 * a invia un messaggio a b
			 * 				b decide di eliminarsi
			 * messaggio inviato mentre l'eliminazione di b è in attesa.
			 * b riprende e completa l'eliminazione
			 *
			 * in questo scenario, il my_mex di b non viene aggiornato:
			 * dopo la consegna del messaggio, viene aggiornato il 
			 * my_mex di a, che ha consegnato il messaggio, non quello 
			 * di b. */

			/*START EMPTYNG MESSAGE*/

        	        free(mex -> usr_destination);
	                free(mex -> usr_sender);
                	free(mex -> object);
        	        free(mex -> text);
			mex -> is_sender_deleted = 0;

                        /*START UPDATING BITMASKS*/
                        server[i] = 0;

                        if (i < *position)
                                *position = i;
                }
		else{
			/* non è un messaggio per me: controllo se l'ho inviato io 
			 * e, in tal caso, setto a 1 il campo "is_sender_deleted":
			 * questo impedisce all' usr_destination di rispondere */
			if (!strcmp(mex -> usr_sender, usr))
			       mex -> is_sender_deleted = 1;		
		}
        }

        update_last(server, last);
	update_db_file(usr);

        /*UPDATING SEM VALUE*/
        sops.sem_op = 1;
        if (semop(sem_write, &sops, 1) == -1)
                error(1319);

        write_int(acc_sock, bol, 1376);
        free(dest);
        return bol;
}






int check_destination(char **usr_destination, char **dest){ 
       	/* se dest è != NULL, si copierà il valore ".db/<usr_dest>.txt\0". 
	 * deve essere però già allocato */
        int len = strlen(*usr_destination) + 9, fileid;
        char *destination_file;
	
        if ((destination_file = (char*) malloc(sizeof(char) * len)) == NULL)
                error(191);
        bzero(destination_file, len);
        sprintf(destination_file, ".db/%s.txt", *usr_destination);

        if (dest != NULL)
                strcpy(*dest, destination_file);

        if ((fileid = open(destination_file, O_RDONLY)) == -1){
                if (errno == ENOENT) //file doesnt exist
                        return 0;
                else
                        error(205);
        }

        if (close(fileid) == -1)
                error(209);

        free(destination_file);
        return 1;
}




int mng_cambio_pass(int acc_sock, char *my_usr){
        int exist, len, fileid, pw_len, ret = 0;
        char *dest_file, *new_pw;

#ifdef CRYPT
	char salt[2 + 1];
	char *crypted;
	struct crypt_data data;
	data.initialized = 0;
#endif

	len = strlen(my_usr) + 9;
        dest_file = (char *) malloc(sizeof(char) * len);
        if (dest_file == NULL)
                error(1442);

        if (read_string(acc_sock, &new_pw, 1548)){
		free(dest_file);
		return -1;
	}

        pw_len = strlen(new_pw);
//	printf("pw_len = %d\n", pw_len);

        exist = check_destination(&my_usr, &dest_file);
	/* non è necessario un controllo su exist, perchè se sto chiamando la 
	 * funzione devo essere per forza loggato */

  	/*      APRO IL FILE, LO SVUOTO*/        
	fileid = open(dest_file, O_RDWR|O_TRUNC|O_APPEND, 0666);
	if (fileid == -1){
		free(dest_file);
 		perror("error at line 1544");
		goto exit_lab;
	}	
	free(dest_file);
#ifdef CRYPT
	/*crypto la pass*/
	random_salt(salt);
	crypted = crypt_r(new_pw, salt, &data);
	/*      SCRIVO LA NUOVA PW E IL LOG-BIT = 1     */
	if (write(fileid, crypted, strlen(crypted)) == -1 || write(fileid, "\n1", 2) == -1) {
		perror("error at 1559");
		goto exit_lab;
	}
#else
	/*      SCRIVO LA NUOVA PW E IL LOG-BIT = 1     */
	if (write(fileid, new_pw, pw_len) == -1 || write(fileid, "\n1", 2) == -1) {
		perror("error at 1559");
		goto exit_lab;
	}
#endif
	/*      CHIUDO FILE     */
	if (close(fileid) == -1){
		perror("error at 1566");
		goto exit_lab;
	}
	ret = 1;
       // }
exit_lab:
	free(new_pw);
        write_int(acc_sock, ret, 1572);
        if (!ret)
                exit(EXIT_FAILURE);
	return 0;
}



void log_out(char *usr){
        char *file_name, curr;
        int len, fileid, i, end_cycle_value;

#ifdef CRYPT
	end_cycle_value = 13;
#else
	end_cycle_value = MAX_PW_LEN;
#endif

        len = strlen(usr);
        if ((file_name = (char*) malloc(sizeof(char) * (len + 9))) == NULL)
                error(1076);
	bzero(file_name, len +9);
        sprintf(file_name, ".db/%s.txt", usr);
        len = strlen(file_name);
        if ((fileid = open(file_name, O_RDWR, 0666)) == -1)
                error(1082);

        for (i = 0; i < end_cycle_value + 1; i++){
                if (read(fileid, &curr, 1) == -1)
                        error(1086);

                if (curr == '\n')
                        break;
        }

        if (write(fileid, "0", 1) == -1)
                error(1093);

        close(fileid);
        free(file_name);
	free(usr);
}



