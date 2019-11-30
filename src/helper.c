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

/*	if (mess -> is_sender_deleted)
		c = 'y';
	else
		c = 'n';
	
	printf("is sender deleted:\n\t%c\n\n", c);*/
	printf(".....................................................................................\n");

}

void write_string(int sock, char *string, int line){
	int len = strlen(string);
//	printf("len = %d\n", len);

	/*if (write(sock, &len, sizeof(len)) == -1)
		error(1172);*/
//	printf("len. ");
	write_int(sock, len, line);
	//audit;
//	printf("***WRITE STRING: %s.\n\n", string);

	if (write(sock, string, len) == -1)
		error(line);
//	printf("scrivo: %s.\n", string);
}

int read_string(int sock, char **string, int line){
	int len;

	if (read_int(sock, &len, 58))
		return 1;
//	printf("len: %d\n", len);	
	if ((*string = (char*) malloc(sizeof(char) * (len + 1))) == NULL)
		error(62);
	bzero(*string, len+1);
	
	if ( (read(sock, *string, len)) == -1)
		error(line);
//	printf("letto: %s.\n", *string);

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
//	printf("\n***WRITE INT: string_v: %s. int_v: %d, len = %d\n", str_num, num, strlen(str_num));

	if (write(sock, str_num, MAX_CIFRE) == -1)
		error(line);
//	printf("scrivo: %s\n", str_num);
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
//	printf("letto: %s.\n", str_num);	
	*num = atoi(str_num);
	//printf("\n***READ INT: string_v: %s. int_v: %d\n", str_num, *num);
	free(str_num);
	if (*num == MAX_NUM_MEX + 1)
		return 1;
	return 0;
}


int get_mex(int sock, message *mex, int alloca_position){ //store the sent mex in mex

	int i = 0;
	char *one_string, *token;
	
	if (read_string(sock, &one_string, 88))
		return 0;

//	printf("onestring %s.\n", one_string);
	/*PARSING*/
	token = strtok(one_string, "\037");
//	printf("%ld\n", strlen(one_string));
	
	if ((mex -> usr_destination = (char*) calloc(MAX_USR_LEN + 1, sizeof(char))) == NULL)	
		error(30);
	//audit;
	strcpy(mex -> usr_destination, token);
	//audit;
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
	//	printf("%s\n", token);
	//	printf("usr_destination: %s\n\n", mex -> usr_destination);
		i++;
	}
	return 1;
}

void send_mex(int sock, message *mex, int invia_is_new_and_position){
	int max_len = MAX_USR_LEN + MAX_USR_LEN + MAX_OBJ_LEN + MAX_MESS_LEN + 6;
	char one_string[max_len];

	bzero(one_string, max_len);
//	stampa_messaggio(mex);
	/*	sender, object, text, destination, is_new, position*/
	if (invia_is_new_and_position)
		sprintf(one_string, "%s\037%s\037%s\037%s\037%d\037%d\037%d", mex -> usr_destination, mex -> usr_sender, mex -> object, mex -> text, mex -> is_new, mex -> position, mex -> is_sender_deleted);
	
	else
		sprintf(one_string, "%s\037%s\037%s\037%s", mex -> usr_destination, mex -> usr_sender, mex -> object, mex -> text);
	
	//printf("%s\n", one_string);	
	//printf("cmp = %d\n", strcmp(one_string, "a\037RE: a\037asd\037a"));
	write_string(sock, one_string, 1433);
}
