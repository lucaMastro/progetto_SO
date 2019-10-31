SERVER	= server
CLIENT	= client

SSRC	= server-side.c
CSRC	= client-side.c

all: $(CLIENT) $(SERVER)

$(SERVER): helper.o helper-server.o $(SERVER).o 
	gcc -o $(SERVER) helper.o helper-server.o $(SERVER).o 

$(SERVER).o: helper.h helper-server.h
	gcc -o $(SERVER).o $(SSRC) -c

$(CLIENT): helper.o helper-client.o $(CLIENT).o 
	gcc -o $(CLIENT) helper.o helper-client.o $(CLIENT).o 

$(CLIENT).o: helper.h helper-client.h 
	gcc -o $(CLIENT).o $(CSRC) -c

helper.o:
	gcc -o helper.o helper.c -c
helper-server.o:
	gcc -o helper-server.o helper-server.c -c 
helper-client.o:
	gcc -o helper-client.o helper-client.c -c 

clean:
	rm -f *.o
	rm -f $(CLIENT)
	rm -f $(SERVER)
