#include "network.h"

unsigned long get_My_IP() {
	char hostname[1024];
	hostname[1023] = '\0';
	gethostname(hostname, 1023);
	struct hostent *host;
	if ((host =  gethostbyname(hostname)) == NULL) { // get the host info
		printf("get_My_IP : gethostbyname failed.\n");
		return 0;
	}
	char* ip = inet_ntoa (*(struct in_addr *)*host->h_addr_list);
	return inet_addr(ip);
}

int create_server_socket(int ServerPort){
	// set server socket's address information    
	struct sockaddr_in   server_addr;  
	bzero(&server_addr, sizeof(server_addr));  
	server_addr.sin_family = AF_INET;  
	server_addr.sin_addr.s_addr = htons(INADDR_ANY);  
	server_addr.sin_port = htons(ServerPort);  

	// create a stream socket    
	int server_socket = socket(PF_INET, SOCK_STREAM, 0);  
	if (server_socket < 0)  {  
		printf("create_server_socket(): Create Socket Failed!\n");  
		return -1;  
	}  

	// bind the socket with the server's address
	if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)))  {  
		// printf("create_server_socket(): Server Bind Port: %d Failed!\n", ServerPort);  
		return -1;  
	}  

	// listen the socket
	if (listen(server_socket, LENGTH_OF_LISTEN_QUEUE))  {  
		printf("create_server_socket(): Server Listen Failed!\n");  
		return -1;  
	}  
	return server_socket;
}

int create_client_socket(int ServerPort){
	struct sockaddr_in client_addr;  
	bzero(&client_addr, sizeof(client_addr));  
	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = htons(INADDR_ANY); 
	client_addr.sin_port = htons(0); 

	int client_socket = socket(AF_INET, SOCK_STREAM, 0);  
	if (client_socket < 0)  {  
		printf("create_client_socket(): Create Socket Failed!\n");  
		exit(1);  
	}  

	if (bind(client_socket, (struct sockaddr*)&client_addr, sizeof(client_addr)))  {  
		printf("create_client_socket(): Client Bind Port Failed!\n");  
		exit(1);  
	}  

	struct sockaddr_in  server_addr;  
	bzero(&server_addr, sizeof(server_addr));  
	server_addr.sin_family = AF_INET;  

	struct hostent *host;
	if ((host =  gethostbyname(SERVER_HOST_NAME)) == NULL) { // get the host info
		printf("create_client_socket() : get server host by name failed.\n");
		exit(1);
	}
	char* ip = inet_ntoa (*(struct in_addr *)*host->h_addr_list);
	if (inet_aton(ip, &server_addr.sin_addr) == 0)  {  
		printf("create_client_socket(): Server IP Address Error!\n");  
		exit(1);  
	}  

	server_addr.sin_port = htons(ServerPort);  
	socklen_t server_addr_length = sizeof(server_addr);  

	if (connect(client_socket, (struct sockaddr*)&server_addr, server_addr_length) < 0) {  
		printf("create_client_socket(): Can Not Connect To %s!\n", ip);  
		exit(1);  
	}  
	return client_socket;
}