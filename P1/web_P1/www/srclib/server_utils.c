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
#include <syslog.h>
#define MAX 4096
#define MAX_ARG_SIZE 25

/*Funciones privadas*/
void _error_400(int clientsocket, char *server_root,char *server_signature, HttpRequest * request);
void _error_404(int clientsocket, char *server_root,char *server_signature, HttpRequest * request);
int _process_GET(int clientsocket, char *server_root, char *path, char *server_signature, HttpRequest *request);
int _process_GET_ARGS(int clientsocket, char *server_root, char *path, char *server_signature, HttpRequest *request);
int _process_POST(int clientsocket, char *server_root,char *path,char *server_signature,HttpRequest *httprequest, char *args_post);
int _process_options(int clientsocket, char *server_root, char *path, char *server_signature, HttpRequest *request);
int _get_type_extension(char *path, char *type_extension);
void get_current_date(char *date);

/********
* FUNCIÓN: int server_init()
* ARGS_IN: None
* DESCRIPCIÓN: Inicializa servidor: lee fichero de configuración y bind del socket.
* ARGS_OUT: -1 en caso de error, de lo contrario 0.
********/
int server_init(){

    FILE *confile;
    int serverSocket,max_clients,port,ret;
    char buff[1000];

    struct sockaddr_in serverAddr;

    /*Lee fichero de configuración --> port y max_clients*/
    confile = fopen("server.conf", "r");
    if (confile == NULL ) {
        syslog(LOG_ERR,"Error Abriendo fichero de configuración");
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
        syslog(LOG_ERR, "Error abriendo socket");
        return -1;
    } 

    /*registra direcciones*/
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); /*acepta petición de cualquier lado*/

    ret = bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
    if (ret == -1){
        syslog(LOG_ERR,"Error registrando dirección en Socket");
        return -1;
    }

    /*Escucha a conecciones*/
    ret = listen(serverSocket,max_clients);
    if (ret == -1){
        syslog(LOG_ERR,"Error escuchando conecciones");
        return -1;
    }

    return serverSocket;
}

/********
* FUNCIÓN: int server_accept_connection(int serverSocket)
* ARGS_IN: socket del servidor
* DESCRIPCIÓN: acepta conexión de un cliente.
* ARGS_OUT: -1 en caso de error, el socket del nuevo cliente en caso contrario.
********/
int server_accept_connection(int serverSocket){
    int clientSocket;
    struct sockaddr_in clientadress;

    socklen_t  lenght = sizeof(clientadress);
    clientSocket = accept(serverSocket,(struct sockaddr * )&clientadress,&lenght);
    
    if (clientSocket == -1) {
        syslog(LOG_ERR,"Error aceptando la conexión");
        return -1;
    }else{
        char *s = inet_ntoa(clientadress.sin_addr);
        syslog(LOG_INFO,"Conexión aceptada de %s\n", s);
        return clientSocket;
    }
}

/********
* FUNCIÓN: int server_process_request(int clientsocket, HttpRequest * httprequest)
* ARGS_IN: socket del cliente y estructura de petición http.
* DESCRIPCIÓN: se llama al recibir una petición http --> parsea la petición y llama a la función de manejo
* correspondiente dependiendo del verbo.
* ARGS_OUT: -1 en caso de error, 0 en caso contrario.
********/
int server_process_request(int clientsocket, HttpRequest * httprequest){
    FILE *confile;
    char buf[MAX],buff[1000], sever_signature[100], server_root[100], method[100],path[100], args_post[100];
    int pret;
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
            return -1; 
        }

        /* request is incomplete, continue the loop */
        assert(pret == -2);

        /*request is too long*/
        if (buflen == sizeof(buf)){
            return -1;
        }
            
    }

    sprintf(method, "%.*s", (int)method_len, httprequest->verbo);
    sprintf(path,"%.*s", (int)path_len, httprequest->path);

    /*Server signature and ROOT----------------------------------------------*/
    confile = fopen("server.conf", "r");
    if (confile == NULL ) {
        syslog(LOG_ERR,"Error Abriendo fichero de configuración");
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
        _error_400(clientsocket,server_root,"",httprequest);
    }

    /*Verifica verbo----------------------------------------------------------*/
    if(strcmp(method,"GET")==0){
        /*GET con argumentos*/
        if(strstr(path,"?") != NULL){
            _process_GET_ARGS(clientsocket,server_root,path,sever_signature,httprequest);
        }else{
        /*GET sin argumentos*/
            _process_GET(clientsocket,server_root,path,sever_signature,httprequest);
        }
    } else if(strcmp(method,"POST")==0){
        sprintf(args_post,"%s",buf + pret);
        /*POST sin argumentos en URL*/
        if(strstr(path,"?") == NULL){
            _process_POST(clientsocket,server_root,path,sever_signature,httprequest, args_post);
        }

    } else if(strcmp(method,"OPTIONS")==0){
        _process_options(clientsocket,server_root,path,sever_signature,httprequest);
    }else{
        _error_400(clientsocket,server_root,"",httprequest);
    }

    return 0;
}

/********
* FUNCIÓN: int _process_POST(int clientsocket, char *server_root,char *path,char *server_signature,HttpRequest *httprequest, char *args_post)
* ARGS_IN: socket del cliente, raíz de servidor, path de recurso, nombre del servidor, estructura de petición, argumentos del cuerpo de POST
* DESCRIPCIÓN: Procesa petición http con verbo POST
* ARGS_OUT: -1 en caso de error, 0 en caso contrario.
********/
int _process_POST(int clientsocket, char *server_root,char *path,char *server_signature,HttpRequest *httprequest, char *args_post){
    
    char  ruta_recurso[500], type_extension[100], command[1200], env[40], resultado[600], fecha_actual[128], last_modified[128], rpta[1024];
    char *a1, **arguments, arg_static1[MAX_ARG_SIZE], arg_static2[MAX_ARG_SIZE], *arg_pointer1, *arg_pointer2;
    char args_concat_static[500], arg_concat1[MAX_ARG_SIZE];
    FILE *recurso, *salida;
    int rett, i, numargs, content_length, recurso_sock, ret;
    struct stat s;

    /*Ruta del servidor por defecto --> Index.html*/
    if(strlen(path)==1){
        sprintf(ruta_recurso, "%s/index.html",server_root);
    }else{
        /*Crea ruta a recurso*/
        sprintf(ruta_recurso, "%s%s",server_root,path);
    }

    /*Abre y lee recurso*/
    recurso = fopen(ruta_recurso,"r");
    if (recurso == NULL){
        syslog(LOG_ERR, "Error buscando recurso %s\n", ruta_recurso);
        _error_404(clientsocket,server_root, server_signature,httprequest);
        return -1;
    }
    fclose(recurso);

    rett = _get_type_extension(ruta_recurso,type_extension);

    /*argumentos en el cuerpo*/
    if(strlen(args_post)!=0){

        /*Tiene que ser .php o .py*/
        if(rett != 1){
            syslog(LOG_ERR,"Error con POST: Script tiene que ser .php o .py \n");
            _error_400(clientsocket,server_root,server_signature,httprequest);
            return -1;
        }

        /*Parse argumentos*/
        
        /*Número de argumentos*/
        for (i=0, numargs=0; args_post[i]; i++) numargs += (args_post[i] == '&');
        numargs ++;


        /*comando*/
        if(strcmp(type_extension, ".py")==0){
            strcpy(env, "python3");
        }else{
            strcpy(env, "php");
        }

        /*imprime argumentos en comando*/
        if(numargs == 1){
            a1 = strchr(args_post, '=') +1;
            sprintf(command, "%s %s %s",env, ruta_recurso, a1);
        }else{
        /*Más de un argumento*/
            arguments = (char **) malloc(numargs * sizeof(char *));

            arg_pointer1 = strtok(args_post, "&");
            i = 0;

            while(arg_pointer1 != NULL){

                strcpy(arg_static1, arg_pointer1);
                arg_pointer2 = strchr(arg_static1, '=') + 1;
                strcpy(arg_static2, arg_pointer2);
                
                arguments[i] = (char*)malloc( (strlen(arg_static2)* sizeof(char)) + 1);
                strcpy(arguments[i],arg_static2);

                arg_pointer1 = strtok(NULL, "&");
                i++;
            }

            for(i=0;i<numargs;i++){

                strcpy(arg_concat1,arguments[i]);
                strcat(args_concat_static,arg_concat1);

                if(i!=numargs-1) strcat(args_concat_static, " ");
            }

            for (i = 0; i<numargs; i++) free(arguments[i]);
            free(arguments);

            sprintf(command, "%s %s %s",env, ruta_recurso, args_concat_static);
        }

        syslog(LOG_INFO, "Ejecutando comando: %s\n",command);

        /*Ejecuta script*/
        salida= popen(command, "r");

        if(salida==NULL){
            syslog(LOG_ERR,"Error ejecutando script");
            return -1;
        }

        content_length = fread(resultado, sizeof(char), 600, salida);

        /*Obtiene last modified y Date*/
        get_current_date(fecha_actual);
        stat(ruta_recurso, &s);
        strftime(last_modified, 128, "%a, %d %b %Y %H:%M:%S %Z", gmtime(&s.st_mtime));

        /*Construye cabecera http y envía*/
        sprintf(rpta,"HTTP/1.%d 200 OK\r\nDate: %s\r\nServer: %s\r\nLast-Modified: %s\r\nContent-Length: %d\r\nContent-Type: text/html\r\n%s\r\n\r\n",
        httprequest->version,fecha_actual,server_signature, last_modified, content_length,resultado);

        send(clientsocket, rpta, strlen(rpta), 0);

        pclose(salida);

    }else{
        /*POST SIN ARGUMENTOS EN CUERPO --> Consigue recurso*/

        if((rett == -1)||(rett==1)){
        _error_400(clientsocket,server_root,server_signature,httprequest);
        fclose(recurso);
        return -1;
        }

        /*Obtiene last modified y Date*/
        get_current_date(fecha_actual);
        stat(ruta_recurso, &s);
        strftime(last_modified, 128, "%a, %d %b %Y %H:%M:%S %Z", gmtime(&s.st_mtime));

        /*Construye cabecera http y envía*/
        sprintf(rpta,"HTTP/1.%d 200 OK\r\nDate: %s\r\nServer: %s\r\nLast-Modified: %s\r\nContent-Length: %ld\r\nContent-Type: %s\r\n\r\n",
        httprequest->version,fecha_actual,server_signature, last_modified, s.st_size,type_extension);

        send(clientsocket, rpta, strlen(rpta), 0);

        /*Envia recurso*/
        recurso_sock = open(ruta_recurso, O_RDONLY);
        while ((ret = read(recurso_sock, rpta, 1024)) > 0) {
            send(clientsocket, rpta, ret, 0);
        }

    }
    
    return 0;
}


/********
* FUNCIÓN: int _process_POST(int clientsocket, char *server_root,char *path,char *server_signature,HttpRequest *httprequest)
* ARGS_IN: socket del cliente, raíz de servidor, path de recurso, nombre del servidor, estructura de petición
* DESCRIPCIÓN: Procesa petición http con verbo GET
* ARGS_OUT: -1 en caso de error, 0 en caso contrario.
********/
int _process_GET(int clientsocket, char *server_root, char *path, char *server_signature, HttpRequest *request){
    char rpta[1024], ruta_recurso[1000],type_extension[100],fecha_actual[128],last_modified[128];
    FILE *recurso;
    int ret,recurso_sock;
    struct stat s;

    /*Ruta del servidor por defecto --> /*/
    if(strlen(path)==1){
        sprintf(ruta_recurso, "%s/index.html",server_root);
    }else{
        /*Crea ruta a recurso*/
        sprintf(ruta_recurso, "%s%s",server_root,path);
    }
    
    /*Abre y lee recurso*/
    recurso = fopen(ruta_recurso,"r");
    if (recurso == NULL){
        syslog(LOG_ERR, "Error buscando recurso %s\n", ruta_recurso);
        _error_404(clientsocket,server_root, server_signature,request);
        return -1;
    }

    /*Obtiene tipo de fichero*/
    ret = _get_type_extension(ruta_recurso,type_extension);

    if((ret == -1)||(ret==1)){
        _error_400(clientsocket,server_root,server_signature,request);
        fclose(recurso);
        return -1;
    }

    /*Obtiene last modified y Date*/
    get_current_date(fecha_actual);
    stat(ruta_recurso, &s);
    strftime(last_modified, 128, "%a, %d %b %Y %H:%M:%S %Z", gmtime(&s.st_mtime));

    /*Construye cabecera http y envía*/
    sprintf(rpta,"HTTP/1.%d 200 OK\r\nDate: %s\r\nServer: %s\r\nLast-Modified: %s\r\nContent-Length: %ld\r\nContent-Type: %s\r\n\r\n",
    request->version,fecha_actual,server_signature, last_modified, s.st_size,type_extension);

    send(clientsocket, rpta, strlen(rpta), 0);

    /*Envia recurso*/
    recurso_sock = open(ruta_recurso, O_RDONLY);
    while ((ret = read(recurso_sock, rpta, 1024)) > 0) {
        send(clientsocket, rpta, ret, 0);
    }

    return 0;

}


/********
* FUNCIÓN: int _process_GET_ARGS(int clientsocket, char *server_root, char *path, char *server_signature, HttpRequest *request){
* ARGS_IN: socket del cliente, raíz de servidor, path de recurso, nombre del servidor, estructura de petición
* DESCRIPCIÓN: Procesa petición http con verbo GET con argumentos en la URL, ejecuta script correspondiente.
* ARGS_OUT: -1 en caso de error, 0 en caso contrario.
********/
int _process_GET_ARGS(int clientsocket, char *server_root, char *path, char *server_signature, HttpRequest *request){
    char *ret, *retargs, path_noargs[250], args[200], ruta_recurso[500], type_extension[100], command[1200], env[40], resultado[600], fecha_actual[128], last_modified[128], rpta[1024];
    char *a1, **arguments, arg_static1[MAX_ARG_SIZE], arg_static2[MAX_ARG_SIZE], *arg_pointer1, *arg_pointer2;
    int rett, i, content_length, numargs;
    FILE *recurso, *salida;
    struct stat s;

    char args_concat_static[500], arg_concat1[MAX_ARG_SIZE];

    /*Quitar argumentos de path*/
    ret = strtok(path, "?");
    strcpy(path_noargs,ret);
    retargs = strtok(NULL, "?");
    strcpy(args, retargs);

    /*Crea ruta a recurso*/
    sprintf(ruta_recurso, "%s%s",server_root,path_noargs);

    /*Abre y lee script*/
    recurso = fopen(ruta_recurso,"r");
    if (recurso == NULL){
        syslog(LOG_ERR,"Error buscando recurso %s\n", ruta_recurso);
        _error_404(clientsocket,server_root, server_signature,request);
        return -1;
    }
    fclose(recurso);

    rett = _get_type_extension(ruta_recurso,type_extension);

    /*Tiene que ser .php o .py*/
    if(rett != 1){
        syslog(LOG_ERR, "Error con GET: Script tiene que ser .php o .py \n");
        _error_400(clientsocket,server_root,server_signature,request);
        return -1;
    }

    /*Número de argumentos*/
    for (i=0, numargs=0; args[i]; i++) numargs += (args[i] == '&');
    numargs ++;

    /*comando*/
    if(strcmp(type_extension, ".py")==0){
        strcpy(env, "python3");
    }else{
        strcpy(env, "php");
    }

    /*imprime argumentos en comando*/
    if(numargs == 1){
        a1 = strchr(args, '=') +1;
        sprintf(command, "%s %s %s",env, ruta_recurso, a1);
    }else{
    /*Más de un argumento*/
        arguments = (char **) malloc(numargs * sizeof(char *));

        arg_pointer1 = strtok(args, "&");
        i = 0;

        while(arg_pointer1 != NULL){

            strcpy(arg_static1, arg_pointer1);
            arg_pointer2 = strchr(arg_static1, '=') + 1;
            strcpy(arg_static2, arg_pointer2);
            
            arguments[i] = (char*)malloc( (strlen(arg_static2)* sizeof(char)) + 1);
            strcpy(arguments[i],arg_static2);

            arg_pointer1 = strtok(NULL, "&");
            i++;
        }

        for(i=0;i<numargs;i++){

            strcpy(arg_concat1,arguments[i]);
            strcat(args_concat_static,arg_concat1);

            if(i!=numargs-1) strcat(args_concat_static, " ");
        }

        for (i = 0; i<numargs; i++) free(arguments[i]);
        free(arguments);

        sprintf(command, "%s %s %s",env, ruta_recurso, args_concat_static);
    }


    syslog(LOG_INFO, "Ejecutando comando: %s\n",command);

    /*Ejecuta script*/
    salida= popen(command, "r");

    if(salida==NULL){
        syslog(LOG_ERR,"Error ejecutando script");
        return -1;
    }

    content_length = fread(resultado, sizeof(char), 600, salida);

    /*Obtiene last modified y Date*/
    get_current_date(fecha_actual);
    stat(ruta_recurso, &s);
    strftime(last_modified, 128, "%a, %d %b %Y %H:%M:%S %Z", gmtime(&s.st_mtime));

    /*Construye cabecera http y envía*/
    sprintf(rpta,"HTTP/1.%d 200 OK\r\nDate: %s\r\nServer: %s\r\nLast-Modified: %s\r\nContent-Length: %d\r\nContent-Type: text/html\r\n%s\r\n\r\n",
    request->version,fecha_actual,server_signature, last_modified, content_length,resultado);

    send(clientsocket, rpta, strlen(rpta), 0);

    pclose(salida);

    return 0;
}

/********
* FUNCIÓN: int _process_options(int clientsocket, char *server_root, char *path, char *server_signature, HttpRequest *request)
* ARGS_IN: socket del cliente, raíz de servidor, path de recurso, nombre del servidor, estructura de petición
* DESCRIPCIÓN: Procesa petición http con verbo OPTIONS, devuelve respuesta con verbos soportados por el servidor.
* ARGS_OUT: -1 en caso de error, 0 en caso contrario.
********/
int _process_options(int clientsocket, char *server_root, char *path, char *server_signature, HttpRequest *request){
    char  fecha_actual[128], cabecera[400];

    get_current_date(fecha_actual);

    sprintf(cabecera,"HTTP/1.%d 200 OK\r\nDate: %s\r\nServer: %s\r\nContent-Length: 0\r\nAllow: GET,POST,OPTIONS\r\n\r\n",
    request->version,fecha_actual,server_signature);

    send(clientsocket, cabecera, strlen(cabecera), 0);

    return 0;

}

/********
* FUNCIÓN: void _error_400(int clientsocket, char *server_root,char *server_signature, HttpRequest * request)
* ARGS_IN: socket del cliente, raíz de servidor, path de recurso, nombre del servidor, estructura de petición
* DESCRIPCIÓN: Envía respuesta de error 400 Bad request.
* ARGS_OUT: Ninguno
********/
void _error_400(int clientsocket, char *server_root,char *server_signature, HttpRequest * request){

    char rpta[1024], fecha_actual[128], type_extension[100], cabecera[400];
    int  lenght;

    sprintf(type_extension, "text/html");

    /*Obtiene Date*/
    get_current_date(fecha_actual);

    /*imprime respuesta*/
    lenght = sprintf(rpta, "<doctype html>\n"
          "<html>\n"
          "<head><meta charset=\"UTF-8\"></head>\n"
          "<body><h1>Error 400 Bad Request</h1></body>\n"
          "</html>");

    /*Envía cabecera*/
    sprintf(cabecera,"HTTP/1.%d 400 Bad Request\r\nDate: %s\r\nServer: %s\r\nContent-Length: %d\r\nContent-Type: %s\r\n\r\n",
    request->version,fecha_actual,server_signature, lenght,type_extension);

    send(clientsocket, cabecera, strlen(cabecera), 0);

    /*Envia error*/
    send(clientsocket, rpta, strlen(rpta), 0);
}

/********
* FUNCIÓN: void _error_404(int clientsocket, char *server_root,char *server_signature, HttpRequest * request)
* ARGS_IN: socket del cliente, raíz de servidor, path de recurso, nombre del servidor, estructura de petición
* DESCRIPCIÓN: Envía respuesta de error 404 Not Found.
* ARGS_OUT: Ninguno
********/
void _error_404(int clientsocket, char *server_root,char *server_signature, HttpRequest * request){
    char rpta[1024], fecha_actual[128], type_extension[100], cabecera[400];
    int  lenght;

    sprintf(type_extension, "text/html");

    /*Obtiene Date*/
    get_current_date(fecha_actual);

    /*imprime respuesta*/
    lenght = sprintf(rpta, "<doctype html>\n"
          "<html>\n"
          "<head><meta charset=\"UTF-8\"></head>\n"
          "<body><h1>Error 404 Not Found</h1></body>\n"
          "</html>");

    /*Envía cabecera*/
    sprintf(cabecera,"HTTP/1.%d 404 Not Found\r\nDate: %s\r\nServer: %s\r\nContent-Length: %d\r\nContent-Type: %s\r\n\r\n",
    request->version,fecha_actual,server_signature, lenght,type_extension);

    send(clientsocket, cabecera, strlen(cabecera), 0);

    /*Envia error*/
    send(clientsocket, rpta, strlen(rpta), 0);
}


/********
* FUNCIÓN: int _get_type_extension(char *path, char *type_extension)
* ARGS_IN: ruta y puntero con tipo de extensión por rellenar.
* DESCRIPCIÓN: consigue tipo de extensión de recurso solicitado
* ARGS_OUT: -1 en caso de error, 0 en caso de recurso, 1 en caso de script
********/
int _get_type_extension(char *path, char *type_extension){

    char *type;

    type = strrchr(path, '.');
    
    if(type == NULL){
        return -1;
    }

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
    }else if(strcmp(type,".php")==0){
        strcpy(type_extension,".php");
        return 1; /*SCRIPT*/
    }else if(strcmp(type,".py")==0){
        strcpy(type_extension,".py");
        return 1; /*SCRIPT*/
    }else{
        return -1;
    }


}

/********
* FUNCIÓN: void get_current_date(char *date) 
* ARGS_IN: puntero de char con fehca por rellenar
* DESCRIPCIÓN: consigue fecha actual
* ARGS_OUT: Ninguno
********/
void get_current_date(char *date) {
    time_t hora = time(0);
    struct tm *t = gmtime(&hora);
    strftime(date, 128, "%a, %d %b %Y %H:%M:%S %Z", t);
}
