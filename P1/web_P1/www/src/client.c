#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> // bzero()
#include <sys/socket.h>
#include <unistd.h> // read(), write(), close()
#define PORT 8080

int main(){

    int clientsocket;
    struct sockaddr_in serveraddress;

    /*crea socket de cliente*/
    clientsocket = socket(AF_INET, SOCK_STREAM,0);

    if (clientsocket == -1){
        printf("Error creando socket de cliente");
        return -1;
    }

    bzero(&serveraddress,sizeof(serveraddress));

    /*establece direcciones de servidor*/
    serveraddress.sin_family = AF_INET;
    serveraddress.sin_addr.s_addr = inet_addr("127.0.0.1"); /*Local Host*/
    serveraddress.sin_port = htons(PORT);

    /*Establece conexión con servidor*/
    if (connect(clientsocket, (struct sockaddr *) &serveraddress,sizeof(serveraddress)) == -1){
        printf("Error conectandose con servidor");
        return -1;
    }

    printf("Conectado con servidor");



    return 0;

}
