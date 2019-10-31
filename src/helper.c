#include "helper.h"
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


/*
void write_mex(int sock, message *mex){
	int max_len = MAX_USR_LEN + MAX_USR_LEN + MAX_OBJ_LEN + MAX_MESS_LEN + 4;
	char one_string[max_len];

	bzero(one_string, max_len);

	//	sender, destination, object, mex	
	sprintf(one_string, "%s\n%s\n%s\n%s\n", mex -> usr_sender, mex -> usr_destination, mex -> object, mex -> text);
	
	write_string(sock, one_string, 1433);
}*/

/*
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
	//	TRY TO GET CONTROL	
	if (semop(semid, &sops, 1) == -1)
		error(1356);
	
	message *mex = mex_list[*position];

	//	PARSING THE STRING		
	//sender, destination, object, mex	

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


	//	GIVE CONTROL TO OTHERS	

	sops.sem_op = 1;
	if (semop(semid, &sops, 1) == -1)
		error(1386);

	free(one_string);
}*/
