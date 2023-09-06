#PRACTICA I REDES DE COMUNICACIONES II:

Primero: asegurarse que la carpeta recursos está dentro de la carpeta de la práctica y que está asignada a la variable server_root en el fichero server.conf, pues esta contiene todos los ficheros disponibles para la petición de recursos. 

Segundo: ejecutar el siguiente comando en la consola:
make all
./server

Para probar ejecución de scripts con GET:
http://localhost:8080/scripts/test.py?variableGET=hola&holaa=variable

Para probar GET y POST--> Página por defecto del servidor es index.html
http://localhost:8080

Para probar OPTIONS en otra  consola ejecutar el siguiente comando:
curl -i -X OPTIONS http://localhost:8080

Para ver LOGS del servidor escribir los siguientes comandos en la consola:
cd /var/log/
vi syslog