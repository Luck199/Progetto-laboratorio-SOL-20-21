# Makefile
CC	=  gcc 
CFLAGS	= -lm -Wall -pedantic -g 

.PHONY: clean all

objects = server.o
objectsClient = client.o utility.o apiServer.o coda.c

server: server.o utility.o apiServer.o worker.o coda.c gestioneFile.o
	$(CC) -pthread -o $@ $^

client: $(objectsClient)
	$(CC) -o $@ $^

server.o: server.c utility.h apiServer.h coda.h gestioneFile.h
	$(CC) $(CFLAGS) -c $< 
	
apiServer.o: apiServer.c apiServer.h gestioneFile.h
	$(CC) $(CFLAGS) -c $<	

worker.o: worker.c worker.h  utility.h coda.h gestioneFile.h
	$(CC) $(CFLAGS) -c $<
	
utility.o: utility.c utility.h coda.o coda.h gestioneFile.h
	$(CC) $(CFLAGS) -c $<	
	
client.o: client.c client.h utility.h coda.h
	$(CC) $(CFLAGS) -c $<

coda.o: coda.c coda.h
	$(CC) $(CFLAGS) -c $<
	
gestioneFile.o: gestioneFile.c gestioneFile.h utility.h
	$(CC) $(CFLAGS) -c $<
	
all:
	make server
	make client

clean:
	-rm *.o
	-rm log.txt
	-rm sockfile
	echo tutto pulito
	 
test1:	
	make all
	./test1.sh
	
testClient:
	make client
	./testClient.sh
	
