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

#include <arpa/inet.h>

int serverSocket,client_socket;

void sig_int(int signo){
    printf("Terminación de proceso...");
    close(serverSocket);
    close(client_socket);
    exit(0);
}


int  main()
{
    int childpid;

    /*Configura función de manejo de interrupción de programa*/
    signal(SIGINT,sig_int);

    /*Inicializa socket del servidor*/
    serverSocket = server_init();
    if (serverSocket == -1) return -1;

    
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
            server_process_request(client_socket);

            exit(0);
        }

        close(client_socket); /* parent closes connected socket */
    }
}