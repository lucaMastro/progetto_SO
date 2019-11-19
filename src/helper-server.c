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
	char buffer[MAX_USR_LEN + 1], *ret;
	int found;

	fd = fopen(".db/list.txt", "r");
	if (fd == NULL)
		error(378);
	
	while (1){
		found = 1;
		bzero(buffer, MAX_USR_LEN + 1); 
		ret = fgets(buffer, MAX_USR_LEN + 1, fd);
		if (ret == NULL)
			break;
		write_int(acc_sock, found, 383);
		write_string(acc_sock, buffer, 385);
	}

	found = 0;
	write_int(acc_sock, found, 383);
	fclose(fd);
}



void inizializza_server(message **mex_list){ //sequenza di messaggi
	int i, fileid;

	for (i = 0; i < MAX_NUM_MEX; i++){
//		mex = mex_list[i];	
		//sharing mex
		if ((mex_list[i] = (message*) mmap(NULL, (sizeof(message)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) == (message*) -1)
			error(42);
		
		//sharing usr_destination
		if ((mex_list[i] -> usr_destination = (char*) mmap(NULL, (sizeof(char) * MAX_USR_LEN), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) == (char*) -1){
			error(46);
		}	bzero(mex_list[i] -> usr_destination, MAX_USR_LEN);

		//sharing usr_sender
		if ((mex_list[i] -> usr_sender = (char*) mmap(NULL, (sizeof(char) * MAX_USR_LEN), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) == (char*) -1){
			error(51);
		}	bzero(mex_list[i] -> usr_sender, MAX_USR_LEN);

		//sharing object
		if ((mex_list[i] -> object = (char*) mmap(NULL, (sizeof(char) * MAX_OBJ_LEN), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) == (char*) -1){
			error(56);
		}	bzero(mex_list[i] -> object, MAX_OBJ_LEN);

		//sharing text
		if ((mex_list[i]-> text = (char*) mmap(NULL, (sizeof(char) * MAX_MESS_LEN), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) == (char*) -1){
			error(61);
		}	bzero(mex_list[i] -> text, MAX_MESS_LEN);
	
		//sharing is_new 
		if ((mex_list[i]-> is_new = (int*) mmap(NULL, (sizeof(int)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) == (int*) -1)
			error(66);
	
		//sharing position 
		if ((mex_list[i]-> position = (int*) mmap(NULL, (sizeof(int*)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) == (int*) -1)
			error(70);
		
		*(mex_list[i]-> position) = i;
	}

	//make a new hidden folder, if not exist
        if (mkdir(".db", S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO) == -1 && errno != EEXIST){
                perror("error creating < db >");
                exit(EXIT_FAILURE);
        }

        //creating the list of users
        fileid = open(".db/list.txt", O_CREAT|O_TRUNC, 0666);
        if (fileid == -1)
                error(142);
        else
                close(fileid);

	return;
}




int ricevi_messaggio(int acc_sock, message **mess_list, int *position, int *last, int *server, int sem_write, int *my_mex, int *my_new_mex, char *client_usr, int flag){ //flag == 1 means its a write back
	char *usr_destination, *usr_sender, *object, *text, *destination_file;
	int i, len, fileid, exist, get_out;
	struct sembuf sops;

	sops.sem_flg = 0;
	sops.sem_num = 0;
	sops.sem_op = -1;

start:

	send_file_db(acc_sock);

	/*	READING USR_DESTINATION	*/
	if(read_string(acc_sock, &usr_destination, 199))
		log_out(client_usr);
		
	len = strlen(usr_destination);

//	printf("usr dest = %s, len = %d\n", usr_destination, len);

	/*	CHECKING IF DESTINATION EXISTS AND SENDING RESPONSE	*/
	exist = check_destination(&usr_destination, NULL);	
	/*	SENDING IF EXISTS	 */
	write_int(acc_sock, exist, 269);
	if (!exist){
		if (!flag)
			goto start;
		else
			return 1;
	}

	/*	READING USR_SENDER	*/
	if (read_string(acc_sock, &usr_sender, 231))
		log_out(client_usr);

	/*	READING OBJECT	*/
	if (read_string(acc_sock, &object, 235))
		log_out(client_usr);

	/*	READING TEXT	*/
	if (read_string(acc_sock, &text, 238))
		log_out(client_usr);

	/*	TRY TO GET CONTROL	*/
	if (semop(sem_write, &sops, 1) == -1)
		error(255);

	message *mex = mess_list[*position];

	/*sprintf((mex -> usr_destination), "%s", usr_destination);
	sprintf((mex ->usr_sender), "%s", usr_sender);
	sprintf((mex -> object), "%s", object);
	sprintf((mex -> text), "%s", text);*/
	strcpy((mex -> usr_destination), usr_destination);
	strcpy((mex ->usr_sender), usr_sender);
	strcpy((mex -> object), object);
	strcpy((mex -> text), text);
	*(mex -> is_new) = 1;
	
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
//	stampa_messaggio(mex);
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
		
			/*	SENDING MEX	*/
		//	send_mex(acc_sock, mex);

                        *(mex -> is_new) = 0;
                        found = 0;

			/*READING USR_WILL*/
read_usr_will:
                        if (read_int(acc_sock, &op, 613))
				log_out(usr);
                        
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
        if (i == temp_last && leave == 0){
	//	printf("i = %d\ntl = %d\n", i, temp_last);
                write_int(acc_sock, found, 615);
	//	printf("wrote\n");
	}

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
                if (read_int(acc_sock, &operation, 693))
			log_out(client_usrname);

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
                                write_int(acc_sock, ret, 249);
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
				
				//sending if it was ok
                                write_int(acc_sock, ret, sizeof(ret));

/*                              printf("il messaggio inserito è:\n\n");
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
                                mng_cambio_pass(acc_sock, usr);
                                break;
                        default:
                                break;
                }
        }
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


void managing_usr_registration_login(int acc_sock, char **usr){
        int operation, len_usr, len_pw, i, ret, can_i_exit = 0, retry;
        char *pw, *file_name, stored_pw[MAX_PW_LEN + 1], is_log, curr;
        int fileid, fileid2;

check_operation:
	bzero(stored_pw, MAX_PW_LEN + 1);
        ret = 0;
        printf("waiting for an operation:\n");
        if (read_int(acc_sock, &operation, 931))
		log_out(*usr);

        printf("operation %d accepted\n", operation);

        if (operation == 0){
                printf("selected exit option.\n");

                exit(EXIT_SUCCESS);
        }
        else{
		/*	READING IF RETRY GETTING USRNAME	*/
		check_usr_restart:
		if (read_int(acc_sock, &retry, 344))
			log_out(*usr);

		switch (retry){
			case 1:
				goto check_usr_restart;
			case 2:
				goto check_operation;
			default:
				break;
		}
                /*      READING USR     */
                if (read_string(acc_sock, usr, 944))
			log_out(*usr);
		
		/*	READING IF RETRY GETTING PASSWORD	*/
		check_pw_restart:
		if (read_int(acc_sock, &retry, 344))
			log_out(*usr);

		switch (retry){
			case 1: 
				goto check_pw_restart;
			case 2:
				goto check_operation;
			default:
				break;
		}

                /*      READING PASSWORD        */
                if (read_string(acc_sock, &pw, 958))
			log_out(*usr);

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
//			printf("user = %s\n, len = %d\nbol = %d\n\n", *usr, len_usr, strcmp(*usr, "a\0"));
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

		//	printf("pw = %s, stored = %s\n", pw, stored_pw);
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
                return;
}






void close_server(int acc_sock, char *usr){
        if (close(acc_sock) == -1)
                error(693);

        printf("starting log out\n");
//      log_out(usr);   
        printf("collegamento chiuso con successo.\n");
        exit(EXIT_SUCCESS);
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
		log_out(usr);

        if (mode >= 0)
	 	code = mode;
	
        else{
                another_code:
                if (read_int(acc_sock, &code, 374))
			log_out(usr);
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
                if (semop(sem_write, &sops, 1) == -1)
                        error(422);

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
                if (read_int(acc_sock, &again, 444))
			log_out(usr);

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
                if (my_mex[i] == 1){ //i is the index of a mex i have to free

                        /*START EMPTYNG MESSAGE*/
                        mex = mex_list[i];
                        bzero(mex -> usr_destination, strlen(mex -> usr_destination));
                        bzero(mex -> usr_sender, strlen(mex -> usr_destination));
                        bzero(mex -> text, strlen(mex -> usr_destination));

                        /*START UPDATING BITMASKS*/
                        server[i] = 0;
//                      update_system_state(my_mex, my_new_mex, mex_list, usr, *last, server);

                        if (i < *position)
                                *position = i; //eseguito almeno una volta 100%
                }
        }

        update_last(server, last);

	/*	UPDATING LIST OF USERS	*/
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



void mng_cambio_pass(int acc_sock, char *my_usr){
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
		log_out(my_usr);

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





void store_mex(int sock, message **mex_list, int *position, int semid){ 
	/*store the mex in the correct position of the server's struct	*/

	struct sembuf sops;

	sops.sem_num = 0;
	sops.sem_flg = 0;
	sops.sem_op = -1;

	//	TRY TO GET CONTROL	
	if (semop(semid, &sops, 1) == -1)
		error(1356);
	
	message *mex = mex_list[*position];

	/*	STORING MEX IN THE CORRECT POSITION OF MEX_LIST	*/
	get_mex(sock, mex);

	//	GIVE CONTROL TO OTHERS	

	sops.sem_op = 1;
	if (semop(semid, &sops, 1) == -1)
		error(1386);
}

