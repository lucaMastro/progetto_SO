#include "../lib/helper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define fflush(stdin) while(getchar() != '\n')
#define audit printf("ok\n")

void error(int line){
	printf("error at line %d:\n", line);
	perror("error");
	exit(EXIT_FAILURE);
}

void stampa_messaggio(message *mess){
	char c;

	if (mess -> is_new)
		c = 'y';
	else
		c = 'n';
	
	printf(".....................................................................................\n\n");
	printf("\t\tcodice = %d\n", mess -> position);
	printf("is new = %c\n", c);
	printf("from:\n\t%s\n", mess -> usr_sender);	
	printf("to:\n\t%s\n", mess -> usr_destination);
	printf("object:\n\t%s\n", mess -> object);
	printf("text:\n\t%s\n\n", mess -> text);
	printf(".....................................................................................\n");

}

void write_string(int sock, char *string, int line){
	int len = strlen(string);
	write_int(sock, len, line);

	if (write(sock, string, len) == -1)
		error(line);
}

int read_string(int sock, char **string, int line){
	int len;

	if (read_int(sock, &len, 58))
		return 1;

	if ((*string = (char*) malloc(sizeof(char) * (len + 1))) == NULL)
		error(62);
	bzero(*string, len+1);
	
	if ( (read(sock, *string, len)) == -1)
		error(line);

	return 0;
}

void write_int(int sock, int num, int line){;
	//IMPEDIRE L'INSERIMENTO DI NUMERI N TALI CHE:  |N| > 9999.
	char *str_num = (char*) malloc(sizeof(char) * (MAX_CIFRE + 1)); //-9999
	if (str_num == NULL){
		printf("error helper at line: 77.\n");
		error(line);
	}
	bzero(str_num, MAX_CIFRE + 1);

	sprintf(str_num, "%d", num);

	if (write(sock, str_num, MAX_CIFRE) == -1)
		error(line);
	free(str_num);
}


int read_int(int sock, int *num, int line){

	char *str_num = (char*) malloc(sizeof(char) * (MAX_CIFRE + 1));
	if (str_num == NULL){
		printf("error helper at line: 77.\n");
		error(line);
	}
	bzero(str_num, MAX_CIFRE + 1);

	if (read(sock, str_num, MAX_CIFRE) == -1)
		error(line);
	*num = atoi(str_num);
	free(str_num);
	if (*num == MAX_NUM_MEX + 1)
		return 1;
	return 0;
}


int get_mex(int sock, message *mex, int alloca_position){ //store the sent mex in mex

	int i = 0;
	char *one_string, *token;
	
	if (read_string(sock, &one_string, 88)){
		printf("inserito terminatore\n");
		return 0;
	}

	/*PARSING*/
	token = strtok(one_string, "\037");
	
	if ((mex -> usr_destination = (char*) calloc(MAX_USR_LEN + 1, sizeof(char))) == NULL)	
		error(30);
	strcpy(mex -> usr_destination, token);
	while ((token = strtok(NULL, "\037")) != NULL){
		switch(i){
			case 0:
				if ((mex -> usr_sender = (char*) calloc(MAX_USR_LEN + 1, sizeof(char))) == NULL)
					error(30);
				strcpy(mex -> usr_sender, token);
				break;
			case 1:
				if ((mex -> object = (char*) calloc(MAX_OBJ_LEN + 1, sizeof(char))) == NULL)
					error(33);
				strcpy(mex -> object, token);

				break;
			case 2:
				if ((mex -> text = (char*) calloc(MAX_MESS_LEN + 1, sizeof(char))) == NULL)
					error(36);
					strcpy(mex -> text, token);
				break;
        
			case 3:
				mex -> is_new = atoi(token);
				break;
			case 4:
				mex -> position = atoi(token);
				break;
			case 5:
				mex -> is_sender_deleted = atoi(token);
				break;
			default:
				break;
		}
		i++;
	}
	free(one_string);
	return 1;
}

void send_mex(int sock, message *mex, int invia_is_new_and_position){
	int another_esc = 0, len;

	len = strlen(mex -> usr_destination) + strlen(mex -> usr_sender) + strlen(mex -> object) + strlen(mex -> text); //lunghezza delle stringhe

	if (invia_is_new_and_position){
		/* devo aggiungere la lunghezza del numero "mex -> position"
		 * per ora ho MAX_CIFRE = 5, e non devo gestire i numeri negativi. contiene già il terminatore di stringa, perchè il max position valido è 1024. */
		char *str_num = (char*) malloc(sizeof(char*) * MAX_CIFRE);  
		if (str_num == NULL)
			error(184);
		bzero(str_num, MAX_CIFRE);
		sprintf(str_num, "%d", mex -> position);
		len += strlen(str_num); //aggiungo il numero di cifre di mex->position
		free(str_num);

		len += 2; //1 per is_new, 1 per is_sender_deleted
		len += 6; //caratteri di escape
	}
	else
		len += 3; //caratteri di escape
	

	if (len == MAX_NUM_MEX + 1){
		audit;
		len++; //inserisco un altro carattere di escape.
		another_esc = 1;
	}

	char one_string[len + 1];//terminatore di stringa
	bzero(one_string, len + 1);

	if (invia_is_new_and_position){ //server to client
		if (!another_esc)
			sprintf(one_string, "%s\037%s\037%s\037%s\037%d\037%d\037%d", mex -> usr_destination, mex -> usr_sender, mex -> object, mex -> text, mex -> is_new, mex -> position, mex -> is_sender_deleted);
		else{
			audit;
			sprintf(one_string, "%s\037\037%s\037%s\037%s\037%d\037%d\037%d", mex -> usr_destination, mex -> usr_sender, mex -> object, mex -> text, mex -> is_new, mex -> position, mex -> is_sender_deleted);
		}
	}
	
	else{ //client to server
		if (!another_esc)
			sprintf(one_string, "%s\037%s\037%s\037%s", mex -> usr_destination, mex -> usr_sender, mex -> object, mex -> text);
		else
			sprintf(one_string, "%s\037\037%s\037%s\037%s", mex -> usr_destination, mex -> usr_sender, mex -> object, mex -> text);
	}
	
	write_string(sock, one_string, 1433);
}
