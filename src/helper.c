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


