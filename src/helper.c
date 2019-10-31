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

void write_string(int sock, char *string, int line){
	int len = strlen(string);
//	printf("string %s\nlen %d\n\n", string, len);
	if (write(sock, &len, sizeof(len)) == -1)
		error(1172);

	if (write(sock, string, len) == -1)
		error(1175);
}

void read_string(int sock, char **string, int line){
	int len;

	if (read(sock, &len, sizeof(len)) == -1)
		error(1182);
	
	if ((*string = malloc(sizeof(char) * len)) == NULL)
		error(1185);
	bzero(*string, len);

	if (read(sock, *string, len) == -1)
		error(1189);
}

void write_int(int sock, int num, int line){;

	if (write(sock, &num, sizeof(num)) == -1)
		error(1195);
}


void read_int(int sock, int *num, int line){

	if (read(sock, num, sizeof(*num)) == -1)
		error(1202);
}


void get_mex(int sock, message **mex){ //store the sent mex in *mex
	int max_len = MAX_USR_LEN + MAX_USR_LEN + MAX_OBJ_LEN + MAX_MESS_LEN + 4, i;
	char *one_string, *token;

	one_string = malloc(sizeof(char) * max_len);
	if (one_string == NULL)
		error(768);
	bzero(one_string, max_len);

	read_string(sock, &one_string, 1445);
	

	//	PARSING THE STRING		
	//sender, destination, object, mex	

	token = strtok(one_string, "\033");
	strcpy((*mex) -> usr_sender, token);

	for (i = 0; i < 3; i++){
		token = strtok(NULL, "\033");			
		switch (i){
			case 0:
				strcpy((*mex) -> usr_destination, token);
				break;
			case 1:
				strcpy((*mex) -> object, token);
				break;
			case 2:
				strcpy((*mex) -> text, token);
				break;
		}
	}
	free(one_string);
}

void send_mex(int sock, message *mex){
	int max_len = MAX_USR_LEN + MAX_USR_LEN + MAX_OBJ_LEN + MAX_MESS_LEN + 4;
	char one_string[max_len];

	bzero(one_string, max_len);

	//	sender, destination, object, mex	
	sprintf(one_string, "%s\033%s\033%s\033%s\033", mex -> usr_sender, mex -> usr_destination, mex -> object, mex -> text);
	
	write_string(sock, one_string, 1433);
}
