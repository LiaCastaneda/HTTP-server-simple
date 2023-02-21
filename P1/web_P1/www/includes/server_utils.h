int server_init();

int server_accept_connection(int serverSocket);

int server_process_request(int clientsocket);
