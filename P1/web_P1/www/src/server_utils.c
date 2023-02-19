#include <stdio.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include "server_utils.h"

int server_init(){

    FILE *confile;
    int serverSocket,max_clients,port,ret;
    char buff[1000];

    struct sockaddr_in serverAddr;

    /*Lee fichero de configuración --> port y max_clients*/
    confile = fopen("server.conf", "r");
    if (confile == NULL ) {
        printf("Error Abriendo fichero de configuración");
        return -1;
    }

    while(fgets(buff,1000,confile)){
        if(strncmp("listen_port",buff,strlen("listen_port"))==0){
            strtok(buff," \n");
            strtok(NULL," \n");
            port = atoi(strtok(NULL," \n"));
        }

        if(strncmp("max_clients",buff,strlen("max_clients"))==0){
            strtok(buff," \n");
            strtok(NULL," \n");
            max_clients = atoi(strtok(NULL," \n"));
        }

    }

    /*Abre Socket del servidor*/
    serverSocket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

    if (serverSocket == -1){
        printf("Error abriendo socket");
        return -1;
    } 

    /*registra direcciones*/
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); /*acepta petición de cualquier lado*/

    ret = bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
    if (ret == -1){
        printf("Error registrando dirección en Socket");
        return -1;
    }


    /*Escucha a conecciones*/
    ret = listen(serverSocket,max_clients);
    if (ret == -1){
        printf("Error escuchando conecciones");
        return -1;
    }

    return serverSocket;
}

int server_accept_connection(int serverSocket){
    int clientSocket;
    clientSocket = accept(serverSocket,NULL,NULL);
    if (clientSocket == -1) {
        printf("Error aceptando la conexión");
        return -1;
    }

    return clientSocket;
}