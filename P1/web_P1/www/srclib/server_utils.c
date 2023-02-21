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
#include "../includes/server_utils.h"
#include "../includes/picohttpparser.h"
#include <assert.h>
#define MAX 4096

/*Funciones privadas*/
void _error_400();

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
    struct sockaddr_in clientadress;

    int lenght = sizeof(clientadress);
    clientSocket = accept(serverSocket,(struct sockaddr * )&clientadress,&lenght);
    if (clientSocket == -1) {
        printf("Error aceptando la conexión");
        return -1;
    }else{
        char *s = inet_ntoa(clientadress.sin_addr);
        printf("conexión aceptada de %s\n", s);
        return clientSocket;
    }
}

int server_process_request(int clientsocket){
    
    char buf[MAX], *method, *path;
    int pret, minor_version,i;
    struct phr_header headers[100];
    size_t buflen = 0, prevbuflen = 0, method_len, path_len, num_headers;
    ssize_t rret;

    
    while (1) {
        /* read the request */
        while ((rret = read(clientsocket, buf + buflen, sizeof(buf) - buflen)) == -1 && errno == EINTR)
            ;
        if (rret <= 0)
            return -1;
        
        prevbuflen = buflen;
        buflen += rret;

        /* parse the request */
        num_headers = sizeof(headers) / sizeof(headers[0]);
        pret = phr_parse_request(buf, buflen, &method, &method_len, &path, &path_len,
                                &minor_version, headers, &num_headers, prevbuflen);
        if (pret > 0)
            break; /* successfully parsed the request */
        else if (pret == -1){
            /*not parsed --> error 400*/
            _error_400();
            return -1; 
        }

        /* request is incomplete, continue the loop */
        assert(pret == -2);

        /*request is too long*/
        if (buflen == sizeof(buf)){
            _error_400();
            return -1;
        }
            
    }

    printf("request is %d bytes long\n", pret);
    printf("method is %.*s\n", (int)method_len, method);
    printf("path is %.*s\n", (int)path_len, path);
    printf("HTTP version is 1.%d\n", minor_version);
    printf("headers:\n");


    for (i = 0; i != num_headers; ++i) {
        printf("%.*s: %.*s\n", (int)headers[i].name_len, headers[i].name,
            (int)headers[i].value_len, headers[i].value);
    }

    return 0;
}

void _error_400(){
    printf("Error 400:bad request");
}