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
#include <sys/stat.h>
#include <fcntl.h>
#include "../includes/server_utils.h"
#include "../includes/picohttpparser.h"
#include <assert.h>
#include <time.h>
#define MAX 4096

/*Funciones privadas*/
void _error_400();
void _error_404();
int _process_GET(int clientsocket, char *server_root, char *path, char *server_signature, HttpRequest *request);
int _get_type_extension(char *path, char *type_extension);
int _script_execute();
void _current_date(char *date);

/*Inicializa servidor*/
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
        printf("Conexión aceptada de %s\n", s);
        return clientSocket;
    }
}

int server_process_request(int clientsocket, HttpRequest * httprequest){
    FILE *confile;
    char buf[MAX],buff[1000], sever_signature[100], server_root[100], method[100],path[100];
    int pret,i;
    size_t buflen = 0, prevbuflen = 0, method_len, path_len, num_headers;
    ssize_t rret;

    /*Parsea petición---------------------------------------------------------*/
    while (1) {
        /* read the request */
        while ((rret = read(clientsocket, buf + buflen, sizeof(buf) - buflen)) == -1 && errno == EINTR);

        if (rret <= 0)
            return -1;
        
        prevbuflen = buflen;
        buflen += rret;

        /* parse the request */
        num_headers = sizeof(httprequest->cabeceras) / sizeof(httprequest->cabeceras[0]);
        pret = phr_parse_request(buf, buflen, &(httprequest->verbo), &method_len, &(httprequest->path), &path_len,
                                &(httprequest->version), httprequest->cabeceras, &num_headers, prevbuflen);
        
        if (pret > 0){
            break; /* successfully parsed the request */
        }else if (pret == -1){
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

    sprintf(method, "%.*s", (int)method_len, httprequest->verbo);
    sprintf(path,"%.*s", (int)path_len, httprequest->path);

    /*Server signature and ROOT----------------------------------------------*/
    confile = fopen("server.conf", "r");
    if (confile == NULL ) {
        printf("Error Abriendo fichero de configuración");
        return -1;
    }
    while(fgets(buff,1000,confile)){
        if(strncmp("sever_signature",buff,strlen("sever_signature"))==0){
            strtok(buff," \n");
            strtok(NULL," \n");
            sprintf(sever_signature, "%s", strtok(NULL," \n"));
        }

        if(strncmp("server_root",buff,strlen("server_root"))==0){
            strtok(buff," \n");
            strtok(NULL," \n");
            sprintf(server_root, "%s", strtok(NULL," \n"));
        }
    }

    /*verifica versión------------------------------------------------------*/
    if((httprequest->version != 1)&&(httprequest->version != 0)){
        _error_400();
    }

    /*Verifica verbo----------------------------------------------------------*/
    if(strcmp(method,"GET")==0){
        _process_GET(clientsocket,server_root,path,sever_signature,httprequest);
    } else if(strcmp(method,"POST")==0){
        printf("Verbo es POST\n");
    } else if(strcmp(method,"OPTIONS")==0){
        printf("Verbo es OPTIONS\n");
    }else{
        _error_400();
    }


    /*DEBUGGING PURPOSES--------------------------------------------------------*/
    printf("request is %d bytes long\n", pret);
    printf("method is %.*s\n", (int)method_len, httprequest->verbo);
    printf("path is %.*s\n", (int)path_len, httprequest->path);
    printf("HTTP version is 1.%d\n", httprequest->version);
    printf("headers:\n");

    for (i = 0; i != num_headers; ++i) {
        printf("%.*s: %.*s\n", (int)httprequest->cabeceras[i].name_len, httprequest->cabeceras[i].name,
            (int)httprequest->cabeceras[i].value_len, httprequest->cabeceras[i].value);
    }

    return 0;
}

/*PROCESS GET*/
int _process_GET(int clientsocket, char *server_root, char *path, char *server_signature, HttpRequest *request){
    char rpta[1024], ruta_recurso[1000],type_extension[100],fecha_actual[128],last_modified[128];
    FILE *recurso;
    int ret,recurso_sock;
    struct stat s;

    /*Crea ruta a recurso*/
    sprintf(ruta_recurso, "%s%s",server_root,path);
    
    /*Abre y lee recurso*/
    recurso = fopen(ruta_recurso,"r");
    if (recurso == NULL){
        printf("Error buscando recurso %s\n", ruta_recurso);
        _error_404();
        return -1;
    }

    /*Obtiene tipo de fichero*/
    ret = _get_type_extension(ruta_recurso,type_extension);

    if(ret == -1){
        _error_400();
        fclose(recurso);
        return -1;
    }else if(ret == 1){
        _script_execute();
        fclose(recurso);
        return 0;
    }

    /*Obtiene last modified y Date*/
    _current_date(fecha_actual);
    stat(ruta_recurso, &s);
    strftime(last_modified, 128, "%a, %d %b %Y %H:%M:%S %Z", gmtime(&s.st_mtime));

    /*Construye cabecera http y envía*/
    sprintf(rpta,"HTTP/1.%d 200 OK\r\nDate: %s\r\nServer: %s\r\nContent-Length: %ld\r\nLast-Modified: %s\r\nContent-Type: %s\r\n\r\n",
    request->version,fecha_actual,server_signature, s.st_size, last_modified,type_extension);

    send(clientsocket, rpta, strlen(rpta), 0);

    /*Envia recurso*/
    recurso_sock = open(ruta_recurso, O_RDONLY);
    while ((ret = read(recurso_sock, rpta, 8192)) > 0) {
        send(clientsocket, rpta, ret, 0);
    }

    fclose(recurso);
    return 0;

}

/*Errores-->TODO*/
void _error_400(){

    printf("Error 400:bad request");
}

void _error_404(){
    printf("Error404\n");
}

/*funciones de apoyo*/

int _get_type_extension(char *path, char *type_extension){

    char *type;

    type = strrchr(path, '.');

    if(strcmp(type, ".gif")==0){
        strcpy(type_extension,"image/gif");
        return 0;
    }else if(strcmp(type,".txt")==0){
        strcpy(type_extension,"text/plain");
        return 0;
    }else if(strcmp(type,".html")==0){
        strcpy(type_extension,"text/html");
        return 0;
    }else if ((strcmp(type,".jpeg")==0)||(strcmp(type,".jpg")==0)){
        strcpy(type_extension,"image/jpeg");
        return 0;
    }else if ((strcmp(type,".mpeg")==0)||(strcmp(type,".mpg")==0)){
        strcpy(type_extension,"video/mpeg");
        return 0;
    }else if ((strcmp(type,".doc")==0)||(strcmp(type,".docx")==0)){
        strcpy(type_extension,"application/msword");
        return 0;
    }else if (strcmp(type,".pdf")==0){
        strcpy(type_extension,"application/pdf");
        return 0;
    }else if((strcmp(type,".php")==0)||(strcmp(type,".py")==0)){
        return 1; /*SCRIPT*/
    }else{
        return -1;
    }


}

int _script_execute(){
    return 0;
}

void _current_date(char *date) {
    time_t tiempo = time(0);
    struct tm *t = gmtime(&tiempo);
    strftime(date, 128, "%a, %d %b %Y %H:%M:%S %Z", t);
}