CFLAGS = -Wextra -g 

objs = packethandler.o rawsocket.o

# regra default (primeira regra)
all: client server

# regras de ligacao
client: client.o $(objs)
server: server.o $(objs)

# regras de compilação
client.o: client.c
	cc -c client.c $(CFLAGS)
server.o: server.c 
	cc -c server.c $(CFLAGS)
packethandler.o: packethandler.c packethandler.h
rawsocket.o: rawsocket.c rawsocket.h

# remove arquivos temporários
clean:
	-rm -f *.o
 
# remove tudo o que não for o código-fonte
purge: clean
	-rm -f client server

runc: client
	sudo ./client

runs: server
	sudo ./server