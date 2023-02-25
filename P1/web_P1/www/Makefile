SRCLIB = ./srclib/
SRC = ./src/
OBJ = ./obj/

all: server_utils.o picohttpparser.o server client 

server:  $(SRC)server.c 
	gcc -g -pthread -o server $(SRC)server.c $(OBJ)server_utils.o $(OBJ)picohttpparser.o

client: $(SRC)client.c
	gcc -g -pthread -o client $(SRC)client.c

server_utils.o: $(SRCLIB)server_utils.c
	gcc -g -pthread -c $(SRCLIB)server_utils.c -o $(OBJ)server_utils.o

picohttpparser.o: $(SRCLIB)server_utils.c
	gcc -g -pthread -c $(SRCLIB)picohttpparser.c -o $(OBJ)picohttpparser.o

clean:
	rm server client -f $(OBJ)*.o