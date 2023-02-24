#include <stdio.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include "../includes/server_utils.h"
#include <netinet/in.h>
#include <sys/wait.h>
#include <arpa/inet.h>

int serverSocket,client_socket;
HttpRequest *httprequest;

void sig_int(int signo){
    printf("\nTerminación de proceso...");
    close(client_socket);
    close(serverSocket);
    free(httprequest);
    exit(EXIT_SUCCESS);
}


int  main()
{
    int childpid;


    /*Configura función de manejo de interrupción de programa*/
    signal(SIGINT,sig_int);

    /*Inicializa socket del servidor*/
    serverSocket = server_init();
    if (serverSocket == -1) return -1;

    /*Reserva memoria para petición*/
    httprequest = (HttpRequest *) malloc(sizeof(HttpRequest));

    /*Recibe conexiones entrantes*/
    for ( ; ; ) {

        /*Crea nuevo socket por conexión*/
        client_socket = server_accept_connection(serverSocket);

        if (client_socket == -1) {
            continue;
        }

        /*Crea un nuevo hilo por conexión*/
        if ( (childpid = fork()) == 0) { /* child process */    
            /* process request */
            printf("Proceso creado");
            server_process_request(client_socket,httprequest);
            
            /*Proceso hijo termina*/
            close(client_socket);
            exit(EXIT_SUCCESS); 
            
        } else if (childpid <0){
            exit(EXIT_FAILURE);
        }

        close(client_socket); 

    }

    if(httprequest!=NULL) free(httprequest);

    /*cierra servidor*/
    close(serverSocket);
    exit(EXIT_SUCCESS);

    return 0;
}