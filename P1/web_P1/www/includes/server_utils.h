#include "picohttpparser.h"

/*Estructura de la petición*/
typedef struct {
    const char *verbo;
    const char *path;
    int version;    /*versión http*/
    struct phr_header cabeceras[50]; /*Cabeceras de la petición --> máximo 50*/
    char *cuerpo;   /*cuerpo de la petición*/
} HttpRequest;

int server_init();

int server_accept_connection(int serverSocket);

int server_process_request(int clientsocket,HttpRequest * httprequest);
