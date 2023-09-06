SRCLIB = ./srclib/
SRC = ./src/
OBJ = ./obj/

all: server_utils.o picohttpparser.o a_lib server 

server.o:  $(SRC)server.c
	gcc -g -Wall -pthread -c $(SRC)server.c -o $(OBJ)server.o
	
server: server.o
	gcc -g -Wall -pthread $(OBJ)server.o -L. -lserver_utils -o server

server_utils.o: $(SRCLIB)server_utils.c
	gcc -g -Wall -pthread -c $(SRCLIB)server_utils.c -o $(OBJ)server_utils.o

picohttpparser.o: $(SRCLIB)server_utils.c
	gcc -g -Wall -pthread -c $(SRCLIB)picohttpparser.c -o $(OBJ)picohttpparser.o

a_lib:
	ar -rcs libserver_utils.a $(OBJ)server_utils.o $(OBJ)picohttpparser.o

clean:
	rm server libserver_utils.a -f $(OBJ)*.o