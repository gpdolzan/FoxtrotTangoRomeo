CFLAGS = -Wextra -g 

objs = packethandler.o rawsocket.o md5.o

# regra default (primeira regra)
all: client server

# regras de ligacao
client: client.o $(objs)
server: server.o $(objs)

# regras de compilação
client.o: client.c client.h
	cc -c client.c $(CFLAGS)
server.o: server.c 
	cc -c server.c $(CFLAGS)
packethandler.o: packethandler.c packethandler.h
rawsocket.o: rawsocket.c rawsocket.h
md5.o: md5.c md5.h

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