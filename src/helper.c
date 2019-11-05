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
	char c = 'y';
/*	printf("mess = %p\n", mess);
	printf("isnew = %p, %d\n", mess->is_new, *(mess->is_new));
	printf("pos = %p\n", mess->position);
	printf("send = %p\n", mess->usr_sender);
	printf("dest = %p\n", mess->usr_destination);
	printf("obj = %p\n", mess->object);
	printf("text = %p\n", mess->text);
	
	if (*(mess -> is_new))
		c = 'y';
	else
		c = 'n';*/
	
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
	/*if (write(sock, &len, sizeof(len)) == -1)
		error(1172);*/
	write_int(sock, len, 50);

	if (write(sock, string, len) == -1)
		error(1175);
}

void read_string(int sock, char **string, int line){
	int len;

	read_int(sock, &len, 58);
	/*if (read(sock, &len, sizeof(len)) == -1)
		error(1182);*/
	
	if ((*string = malloc(sizeof(char) * len)) == NULL)
		error(1185);
	bzero(*string, len);

	if ( (len = read(sock, *string, len)) == -1)
		error(1189);
	printf("ho letto %d caratteri\n", len);
}

void write_int(int sock, int num, int line){;

	if (write(sock, &num, sizeof(num)) == -1)
		error(1195);
}


void read_int(int sock, int *num, int line){

	if (read(sock, num, sizeof(*num)) == -1)
		error(1202);
}


int get_mex(int sock, message *mex){ //store the sent mex in mex
	int max_len = MAX_USR_LEN + MAX_USR_LEN + MAX_OBJ_LEN + MAX_MESS_LEN + 6, i = 0, isnew;
	char *one_string, *token;

	read_string(sock, &one_string, 88);

	/*PARSING*/
		//isnew
	token = strtok(one_string, "\037");
	isnew = atoi(token);
	*(mex -> is_new) = isnew;
	printf("00. %p\n%p\n\n", mex -> is_new, token);
	while ((token = strtok(NULL, "\037")) != NULL){
		printf("%d. %p\n", i, mex -> is_new);
		switch(i){
			case 0:
				strcpy(mex -> usr_sender, token);
				break;
			case 1:
				strcpy(mex -> usr_destination, token);
				break;
			case 2:
				strcpy(mex -> object, token);
				break;
			case 3:
				strcpy(mex -> text, token);
				break;
			case 4:
				*(mex -> position) = atoi(token);
				break;
			default:
				break;
		}
		i++;
	}
	return isnew;
}

void send_mex(int sock, message *mex){
	int max_len = MAX_USR_LEN + MAX_USR_LEN + MAX_OBJ_LEN + MAX_MESS_LEN + 6;
	char one_string[max_len];

	bzero(one_string, max_len);
//	stampa_messaggio(mex);
	/*	isnew, sender, destination, object, text, position	*/
	sprintf(one_string, "%d\037%s\037%s\037%s\037%s\037%d", *(mex -> is_new), mex -> usr_sender, mex -> usr_destination, mex -> object, mex -> text, *(mex -> position));
	
	//printf("%s\n", one_string);	
	//printf("lem = %d\n", strlen(one_string));
	write_string(sock, one_string, 1433);
}
