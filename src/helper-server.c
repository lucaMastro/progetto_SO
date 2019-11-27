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
#include <sys/ipc.h>
#include <sys/sem.h>

#define fflush(stdin) while(getchar() != '\n')
#define audit printf("ok\n")


void send_file_db(int acc_sock){
	FILE *fd;
	char *buffer, *ret;
	int found, i = 0;

	buffer = malloc(sizeof(char) * (MAX_USR_LEN + 2));
	if (buffer == NULL)
		error(27);

	fd = fopen(".db/list.txt", "r");
	if (fd == NULL)
		error(378);
	while (1){
		found = 1;
		bzero(buffer, MAX_USR_LEN + 2);
		ret = fgets(buffer, MAX_USR_LEN + 2, fd);
		if (ret == NULL || strcmp(ret, "\n") == 0)
			break;
		write_int(acc_sock, found, 383);
		write_string(acc_sock, buffer, 385);
	//	printf("%d. %s.%s.\n\n", i, ret,buffer);
		i++;
	}
	
	found = 0;
	write_int(acc_sock, found, 383);
	fclose(fd);
	free(buffer);
}


void update_db_file(char *deleting_string){
	char string[MAX_USR_LEN + 1], del_string[MAX_USR_LEN + 1], c;
	int fileid, fileid2, i;

	fileid = open(".db/list.txt", O_CREAT | O_RDWR, 0666);
	if (fileid == -1)
		error(7);

	fileid2 = open(".db/list2.txt", O_CREAT | O_TRUNC | O_RDWR | O_APPEND, 0666);
	if (fileid2 == -1)
		error(22);

	lseek(fileid, 0, SEEK_SET);
	while (1){
		bzero(string, MAX_USR_LEN);
		i = 0;
		c = '\0';
		while (c != '\n'){
			if (read(fileid, &c, 1) == -1){
				perror("errore read");
				exit(-1);
			}
			string[i] = c;
			if (c == '\0')
				break;
			i++;
		}
		sprintf(del_string, "%s\n", deleting_string);
		if (strcmp(string, del_string) != 0)
			write(fileid2, string, i);
		if (c == '\0'){
			break;
		}
	}
	system("rm .db/list.txt\nmv .db/list2.txt .db/list.txt");
	close(fileid);
	close(fileid2);
}


message** inizializza_server(){ //sequenza di messaggi
	int i, fileid;
	message **mex_list;
//	message *mex;

	mex_list = (message**) malloc(sizeof(message*) * MAX_NUM_MEX);
	if (mex_list == NULL){
		perror("error initializing mex list");
		exit(EXIT_FAILURE);
       	}

	for (i = 0; i < MAX_NUM_MEX; i++){
	//	mex = mex_list[i];
	       	
		/*alloco solo l'indirizzo del messaggio, isnew, e position:
		 *  sono gli unici campi che lato client non mi vengono inviati, 
		 *  per cui nella funzione get_mex non verranno parsati. tutti
		 *  gli altri campi, invece, vengono parsati da quella e funzione
		 *  e, in quel momento, allocati.	*/
		mex_list[i] = (message*) malloc(sizeof(message));
		if (mex_list[i] == NULL)
			error(27);

	/*	if ((mex_list[i] -> is_new = (int *) malloc(sizeof(int))) == NULL)
			error(53);*/
	
		mex_list[i] -> is_new = 1;

	/*	if ((mex_list[i] -> position = (int*) malloc(sizeof(int))) == NULL)
			error(56);*/

		mex_list[i] -> position = i;

		mex_list[i] -> is_sender_deleted = 0;
	}
	
	return mex_list;
}




int ricevi_messaggio(int acc_sock, message **mess_list, int *position, int *last, int *server, int sem_write, int *my_mex, int *my_new_mex, char *client_usr, int flag){ //flag == 1 means its a write back
	char *usr_destination, *usr_sender, *object, *text, *destination_file;
	int i, len, fileid, exist, get_out, retry, can_i_get = 1;
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

	/*	SENDING IF EXISTS	 */
	write_int(acc_sock, exist, 269);

	if (!flag){
		//CHECKING USR_WILL. se exist == 0, retry non può essere -1
		if (read_int(acc_sock, &retry, 146))
			return -1;

		if (!flag){
			switch(retry){
				case 0:
					goto read_dest;
				case 1:
					return 1;
				default:
					break;
			}
		}

	read_obj:
		//CHECKING ERROR ABOUT OBJ_LEN:
		if (read_int(acc_sock, &retry, 146))
			return -1;
		if (!flag){
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
	}

	else{//write back version
		if (!exist)
			return 1;
	}	

read_mess:
	//CHECKING ERROR ABOUT MESS_LEN:
	if (read_int(acc_sock, &retry, 146))
		return -1;

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
	
/*	if (!exist){
		if (!flag){ //non è un write back: leggo se riprovare
			read_int(acc_sock, &retry, 146);
			if (retry) //user wants to go back to main menu
				return 1;
			else
				goto read_dest;
		}
			
		else
			return 1;
	}*/


	/*	TRY TO GET CONTROL	*/
	if (semop(sem_write, &sops, 1) == -1)
		error(255);
	
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
		/*i catched a ctrl+c signal in client-side. i have to increment 
		 * the semaphore*/
        	sops.sem_op = 1;
	        if (semop(sem_write, &sops, 1) == -1)
        	        error(158);
		return -1;
	}
	stampa_messaggio(mex);

	server[*position] = 1;

        /*      UPDATING SHARED PARAMS  */
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
        int i, found = 0, isnew, len_send, len_obj, len_text, pos, again, ret = 0, temp_last = *last, op, leave = 0;
        message *mex;
//      char *sender, *obj, *text;

        for (i = 0; i < temp_last; i++){
		if (leave)
			break;

                mex = mess_list[i];
    //          stampa_messaggio(mex);
                if (my_mex[i] == 1){

                        //parsing dei campi del messaggio:
                        isnew = my_new_mex[i];
                        if (isnew == 0 && flag)//messaggio letto, e voglio solo i nuovi
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
        	                        //goto read_usr_will;
					break;
				
				case 2:
					break;
				
				case 3:
					leave = 1;
					break;
			}
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
                                	//log_out(client_usrname);
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

                                ret = ricevi_messaggio(acc_sock, message_list, position, last, server, sem_write, my_mex, my_new_mex, client_usrname, 0);
                                if (ret < 0){
					//log_out(client_usrname);
                                	close_server(acc_sock, client_usrname, 1);
					return 1;
				}
/*                              printf("il messaggio inserito è:\n\n");
                                stampa_messaggio(message_list[*position - 1]);//stampo accedendo alla lista*/
                                break;

                        case 4:
                                ret = gestore_eliminazioni(acc_sock, client_usrname, message_list, my_mex, my_new_mex, server, sem_write, position, last);
				if (ret < 0){
                                	//log_out(client_usrname);
                                	close_server(acc_sock, client_usrname, 1);
					return 1;
				}
                                break;

                        case 5:
                                ret = update_system_state(my_mex, my_new_mex, message_list, client_usrname, *last, server);
                                write_int(acc_sock, ret, sizeof(ret));
                                printf("stato aggiornato.\n");
                                break;
                        case 6:
                                //log_out(client_usrname);
                                close_server(acc_sock, client_usrname, 1);
				return 1;
                        case 7:
                                delete_user(acc_sock, client_usrname, message_list, server, my_mex, last, position, sem_write);
                                close_server(acc_sock, client_usrname, 0);
				return 1;
                        case 8:
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



int managing_usr_registration_login(int acc_sock, char **usr){
        int operation, len_usr, len_pw, i, ret, can_i_exit = 0, retry;
        char *pw, *file_name, stored_pw[MAX_PW_LEN + 1], is_log, curr;
        int fileid, fileid2;

check_operation:
        bzero(stored_pw, MAX_PW_LEN + 1);
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
		//(*usr)[strlen(*usr)] = '\0';
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

                /*      MAKING THE STRING: ".db/<name_user>.txt"        */
                if ((file_name = malloc(sizeof(char) * (len_usr + 8) )) == NULL)
                        error(867);

                bzero(file_name, (len_usr + 8));
                sprintf(file_name, ".db/%s.txt", *usr);

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
                        if (write(fileid, pw, len_pw) == -1 || write(fileid, "\n0", 2) == -1)
                                error(885);

                        //updating the list
                        fileid2 = open(".db/list.txt", O_CREAT|O_RDWR|O_APPEND, 0666);
                        if (fileid2 == -1)
                                error(439);
//                      printf("user = %s\n, len = %d\nbol = %d\n\n", *usr, len_usr, strcmp(*usr, "a\0"));
                        if (write(fileid2, *usr, strlen(*usr))  == -1 || write(fileid2, "\n", 1) == -1)
                                error(441);
                        close(fileid2);

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
                                else
                                        error(900);
                        }

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

                //      printf("pw = %s, stored = %s\n", pw, stored_pw);
                        /*      CHECKING IF PW IS CORRECT       */
                        if (strcmp(stored_pw, pw) != 0){
                                printf("no matching pw\n");
                                ret = 2;
                                goto send_to_client;
                        }

                        /*      CHECK IF YET LOGED      */
                        if (read(fileid, &is_log, 1) == -1)
                                error(923);

                        if((atoi(&is_log)) == 1){
                                printf("usr already logged\n");
                                ret = 3;
                                goto send_to_client;
                        }

                        /*      SWITCHING THE LOG BIT   */
                        lseek(fileid, i + 1, SEEK_SET);
                        if (write(fileid, "1", 1) == -1)
                                error(934);

                        ret = 0;
                        can_i_exit = 1;
                }
        }
send_to_client:
        close(fileid);
        free(file_name);
        /*      SENDING SERVER ANSWER TO CLIENT */
        write_int(acc_sock, ret, 997);

        if (!can_i_exit)//registration option completed
                goto check_operation;
        else
                return 1;

}






void close_server(int acc_sock, char *usr, int flag){ //flag = 1 means i have to logout
      if (close(acc_sock) == -1)
                error(541);

//        printf("starting log out\n");
	if (flag)        
	   	log_out(usr);   
        printf("collegamento chiuso con successo.\n");
}


int update_system_state(int *my_mex, int *my_new_mex, message **mex_list, char *usr, int last, int *server){
        int i, cmp, found_new = 0;
//      printf("\n\nupdate system:\n");

        for (i = 0; i < last; i++){
                if (!server[i]){
                        my_mex[i] = -1;
                        my_new_mex[i] = -1;
                        continue;
                }
                cmp = strcmp(mex_list[i] -> usr_destination, usr);
                if (!cmp){
                        my_mex[i] = 1;
                        if (mex_list[i] -> is_new == 1){
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
//                              server[i] = 0;
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
	
	printf("[");
        for(i = 0; i < last; i++){
                printf("%d", bitmask[i]);
		if (i != last - 1)
			printf(", ");
	}
        printf("]\n");
}

int gestore_eliminazioni(int acc_sock, char *usr, message **mex_list, int *my_mex, int *my_new_mex, int *server, int sem_write, int *position, int *last){
        int code, is_mine, compl, again, mode, ret = 0;
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

      //printf("code = %d\n", code);
        if (code < 0){
                printf("opearazione annullata client side\n\n");
                return 1;
        }

        is_mine = my_mex[code];
//	printf("ismine = %d\n", is_mine);
        /*SEND IF THE CODE IS ACCEPTED*/
        write_int(acc_sock, is_mine, 383);

        if (is_mine == 1){
  //              printf("codice %d accettato\n", code);

                /*TRY GETTING CONTROL*/
                sops.sem_flg = 0;
                sops.sem_num = 0;
                sops.sem_op = -1;
                if (semop(sem_write, &sops, 1) == -1)
                        error(398);


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
                        //*last = code;  /*se ho server = [0 0 0 0 0 1], e viene cancellato il mess in posizione 5, devo stampare [], e non [0 0 0 0 0]*/
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
                compl = 1;
                write_int(acc_sock, compl, 415);
        }
        else{//not is mine.
    /*            if (mode >= 0) //trying deleting again the same mex after read it: non più possibile
                        printf("messaggio già eliminato\n");
                else*/
                        goto another_code;
        }

        if (mode < 0){
                if (read_int(acc_sock, &again, 444))
			return -1;

                if (again)
                        goto another_code;
        }
        return 1;
}




void update_last(int *server, int *last){
        int i, old = *last;

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
        if (semop(sem_write, &sops, 1) == -1)
                error(1294);

        for (i = 0; i < *last; i++){
		mex = mex_list[i];
                if (my_mex[i] == 1){ //i is the index of a mex i have to free

                        /*START EMPTYNG MESSAGE*/

        	        free(mex -> usr_destination);
	                free(mex -> usr_sender);
                	free(mex -> object);
        	        free(mex -> text);
			mex -> is_sender_deleted = 0;

                        /*START UPDATING BITMASKS*/
                        server[i] = 0;
//                      update_system_state(my_mex, my_new_mex, mex_list, usr, *last, server);

                        if (i < *position)
                                *position = i; //eseguito almeno una volta 100%
                }
		else{//non è un messaggio per me: controllo se l'ho inviato io e, in tal caso, setto a 1 il campo "is_sender_deleted"
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
        int len = strlen(*usr_destination) + 8, fileid, copy = 0;
        char *destination_file;
	
        if (dest != NULL)
                copy = 1;

        if ((destination_file = malloc(sizeof(char) * len)) == NULL)
                error(191);

        bzero(destination_file, len);
        sprintf(destination_file, ".db/%s.txt", *usr_destination);
        if (copy)
                strcpy(*dest, destination_file);

        if ((fileid = open(destination_file, O_RDONLY)) == -1){
                if (errno == ENOENT){
                        //file doesnt exist
                        return 0;
                }
                else
                        error(205);
        }

        if (close(fileid) == -1)
                error(209);

        free(destination_file);
        return 1;
}


/*void server_test(int acc_sock, message **mex_list, int *position, int semid){

        read_mex(acc_sock, mex_list, position, semid);

        stampa_messaggio(mex_list[*position]);
}*/



int mng_cambio_pass(int acc_sock, char *my_usr){
        int exist, len, fileid, pw_len, ret = 0;
        char *dest_file, *new_pw;

        len = strlen(my_usr) + 8;
        dest_file = malloc(sizeof(char) * len);

        if (dest_file == NULL)
                error(1442);

	/*new_pw = malloc(sizeof(char) * MAX_PW_LEN);
	if (new_pw == NULL)
		error(694);
	bzero(new_pw, MAX_PW_LEN);*/

        if (read_string(acc_sock, &new_pw, 1548))
		return -1;

        pw_len = strlen(new_pw);
	printf("pw_len = %d\n", pw_len);

        exist = check_destination(&my_usr, &dest_file);
        if (exist){
                /*      APRO IL FILE, LO SVUOTO*/
                fileid = open(dest_file, O_RDWR|O_TRUNC|O_APPEND, 0666);
                if (fileid == -1){
                        perror("error at line 1544");
                        goto exit_lab;
                }

                /*      SCRIVO LA NUOVA PW E IL LOG-BIT = 1     */
                if (write(fileid, new_pw, pw_len) == -1 || write(fileid, "\n1", 2) == -1) {
                        perror("error at 1559");
                        goto exit_lab;
                }
                /*      CHIUDO FILE     */
                if (close(fileid) == -1){
                        perror("error at 1566");
                        goto exit_lab;
                }
                ret = 1;
        }
exit_lab:
	//free(new_pw);
        write_int(acc_sock, ret, 1572);
        if (!ret)
                exit(EXIT_FAILURE);
	return 0;
}



void log_out(char *usr){
        char *file_name, curr;
        int len, fileid, i;

        len = strlen(usr);
        if ((file_name = malloc(sizeof(char) * (len + 8))) == NULL)
                error(1076);

        sprintf(file_name, ".db/%s.txt", usr);
        len = strlen(file_name);
        if ((fileid = open(file_name, O_RDWR, 0666)) == -1)
                error(1082);

        for (i = 0; i < MAX_PW_LEN + 1; i++){
                if (read(fileid, &curr, 1) == -1)
                        error(1086);

                if (curr == '\n')
                        break;
        }

        if (write(fileid, "0", 1) == -1)
                error(1093);

        close(fileid);
        free(file_name);
}




/*
void store_mex(int sock, message **mex_list, int *position, int semid){ 
	/*store the mex in the correct position of the server's struct	

	struct sembuf sops;

	sops.sem_num = 0;
	sops.sem_flg = 0;
	sops.sem_op = -1;

	//	TRY TO GET CONTROL	
	if (semop(semid, &sops, 1) == -1)
		error(1356);
	
	message *mex = mex_list[*position];

	/*	STORING MEX IN THE CORRECT POSITION OF MEX_LIST	
	get_mex(sock, mex);

	//	GIVE CONTROL TO OTHERS	

	sops.sem_op = 1;
	if (semop(semid, &sops, 1) == -1)
		error(1386);
}*/
