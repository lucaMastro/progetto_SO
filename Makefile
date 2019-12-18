SERVER		= server
CLIENT		= client
CLIENT-CRYPT	= client-crypt
SERVER-CRYPT	= server-crypt

SSRC	= src/server-side.c
CSRC	= src/client-side.c

H 	= src/helper
HS 	= src/helper-server
HC	= src/helper-client
HCC	= src/helper-client-crypt
HSC 	= src/helper-server-crypt

HSRC	= src/helper.c
HSSRC	= src/helper-server.c
HCSRC	= src/helper-client.c

crypt: $(CLIENT-CRYPT) $(SERVER-CRYPT) 
not-crypt: $(CLIENT) $(SERVER)
crypt-clean: clean $(CLIENT-CRYPT) $(SERVER-CRYPT) 
not-crypt-clean: clean $(CLIENT) $(SERVER)



$(SERVER): $(H).o $(HS).o $(SERVER).o 
	gcc -o $(SERVER) $(H).o $(HS).o -pthread src/$(SERVER).o 

$(SERVER-CRYPT): $(H).o $(HSC).o $(SERVER).o 
	gcc -o $(SERVER) $(H).o $(HSC).o -pthread src/$(SERVER).o -lcrypt

$(SERVER).o: 
	gcc -o src/$(SERVER).o $(SSRC) -c



$(CLIENT): $(H).o $(HC).o $(CLIENT).o 
	gcc -o $(CLIENT) $(H).o $(HC).o src/$(CLIENT).o 

$(CLIENT-CRYPT): $(H).o $(HCC).o $(CLIENT).o 
	gcc -o $(CLIENT) $(H).o $(HCC).o src/$(CLIENT).o -lcrypt 

$(CLIENT).o:  
	gcc -o src/$(CLIENT).o $(CSRC) -c



helper.o:
	gcc -o $(H).o $(H).c -c

helper-server.o:
	gcc -o $(HS).o $(HSSRC) -c

helper-client.o:
	gcc -o $(HC).o $(HCSRC) -c 	

$(HCC).o:
	gcc -o $(HCC).o $(HCSRC) -c -DCRYPT	

$(HSC).o:
	gcc -o $(HSC).o $(HSSRC) -c -DCRYPT
clean:
	rm -f src/*.o
	rm -f $(CLIENT)
	rm -f $(SERVER)
	rm -rf .db
