SERVER	= server
CLIENT	= client

SSRC	= src/server-side.c
CSRC	= src/client-side.c

H 	= src/helper
HS 	= src/helper-server
HC	= src/helper-client

HSRC	= src/helper.c
HSSRC	= src/helper-server.c
HCSRC	= src/helper-client.c

all: clean $(CLIENT) $(SERVER)

$(SERVER): $(H).o $(HS).o $(SERVER).o 
	gcc -o $(SERVER) $(H).o $(HS).o src/$(SERVER).o 

$(SERVER).o: 
	gcc -o src/$(SERVER).o $(SSRC) -c

$(CLIENT): $(H).o $(HC).o $(CLIENT).o 
	gcc -o $(CLIENT) $(H).o $(HC).o src/$(CLIENT).o 

$(CLIENT).o:  
	gcc -o src/$(CLIENT).o $(CSRC) -c

helper.o:
	gcc -o $(H).o $(H).c -c

helper-server.o:
	gcc -o $(HS).o $(HSSRC) -c

helper-client.o:
	gcc -o $(HC).o $(HCSRC) -c 	
clean:
	rm -f src/*.o
	rm -f $(CLIENT)
	rm -f $(SERVER)
