#include "network.h"
#include <unistd.h>
#include <sys/socket.h>

unsigned long get_My_IP() {
	char hostname[1024];
	hostname[1023] = '\0';
	gethostname(hostname, 1023);
	struct hostent *host;
	if ((host =  gethostbyname(hostname)) == NULL) { // get the host info
		printf("get_My_IP: gethostbyname failed\n");
		return 0;
	}
	char* ip = inet_ntoa (*(struct in_addr *)*host->h_addr_list);
	return inet_addr(ip);
}
/* Get the unsigned long type ip of peer from a socket */
unsigned long get_peer_IP(int peer_socket) {
	char *ip_addr = NULL;
    	ip_addr = get_address_from_ip(peer_socket);
    	return inet_addr(ip_addr);
}
int create_server_socket(int ServerPort){
	// set server socket's address information    
	struct sockaddr_in server_addr;  
	bzero(&server_addr, sizeof(server_addr));  
	server_addr.sin_family = AF_INET;  
	server_addr.sin_addr.s_addr = htons(INADDR_ANY);  
	server_addr.sin_port = htons(ServerPort);  

	// create a stream socket    
	int server_socket = socket(PF_INET, SOCK_STREAM, 0);  
	if (server_socket < 0)  {  
		printf("create_server_socket(): Create socket failed!\n");  
		return -1;  
	}  

    // allow for socket reuse
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        printf("create_server_socket(): Could not set socket option\n");
        return -1;
    }

	// bind the socket with the server's address
	if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)))  {  
		printf("create_server_socket(): Server bind port %d failed!\n", ServerPort);
		return -1;  
	}  

	// listen on the socket
	if (listen(server_socket, LENGTH_OF_LISTEN_QUEUE))  {  
		printf("create_server_socket(): Server listen failed!\n");  
		return -1;  
	}  
	return server_socket;
}

int create_client_socket_byIp(unsigned long ServerIp, int ServerPort) {
	struct sockaddr_in client_addr;  
	bzero(&client_addr, sizeof(client_addr));  
	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = htons(INADDR_ANY); 
	client_addr.sin_port = htons(0); 

	int client_socket = socket(AF_INET, SOCK_STREAM, 0);  
	if (client_socket < 0)  {  
		printf("create_client_socket(): Create socket failed!\n");  
        		return -1;
	}  

	if (bind(client_socket, (struct sockaddr*)&client_addr, sizeof(client_addr)))  {  
		printf("create_client_socket(): Client bind port failed!\n");  
        		return -1;
	}  

	struct sockaddr_in server_addr; 
	bzero(&server_addr, sizeof(server_addr));  
	server_addr.sin_family = AF_INET;  


	struct in_addr in;
	in.s_addr = (unsigned int)ServerIp;

	char* ip = inet_ntoa(in);
	if (inet_aton(ip, &server_addr.sin_addr) == 0)  {  
		printf("create_client_socket(): Server IP address error!\n");  
        		return -1;
	}  

	server_addr.sin_port = htons(ServerPort);  
	socklen_t server_addr_length = sizeof(server_addr);  

	if (connect(client_socket, (struct sockaddr*)&server_addr, server_addr_length) < 0) {  
		printf("create_client_socket(): Can not connect to %s!\n", ip);  
        		return -1;
	}  
	return client_socket;
}

int create_client_socket(char* HostName, int ServerPort) {
	struct sockaddr_in client_addr;  
	bzero(&client_addr, sizeof(client_addr));  
	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = htons(INADDR_ANY); 
	client_addr.sin_port = htons(0); 

	int client_socket = socket(AF_INET, SOCK_STREAM, 0);  
	if (client_socket < 0)  {  
		printf("create_client_socket(): Create socket failed!\n");  
        		return -1;
	}  

	if (bind(client_socket, (struct sockaddr*)&client_addr, sizeof(client_addr)))  {  
		printf("create_client_socket(): Client bind port failed!\n");  
        return -1;
	}  

	struct sockaddr_in  server_addr;  
	bzero(&server_addr, sizeof(server_addr));  
	server_addr.sin_family = AF_INET;  

	struct hostent *host;
	if ((host = gethostbyname(HostName)) == NULL) { // get the host info
		printf("create_client_socket(): get server host by name failed\n");
        return -1;
	}
	char* ip = inet_ntoa (*(struct in_addr *)*host->h_addr_list);
	if (inet_aton(ip, &server_addr.sin_addr) == 0)  {  
		printf("create_client_socket(): Server IP address error!\n");  
        return -1;
	}  

	server_addr.sin_port = htons(ServerPort);  
	socklen_t server_addr_length = sizeof(server_addr);  

	if (connect(client_socket, (struct sockaddr*)&server_addr, server_addr_length) < 0) {  
		printf("create_client_socket(): Can not connect to %s!\n", ip);  
        return -1;
	}  
	return client_socket;
}


/*
 * This function returns the ip address given socket
 */
char *get_address_from_ip(int socket) {
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    int res = getpeername(socket, (struct sockaddr *)&addr, &addr_size);
    char *clientip = malloc(20);
    bzero(clientip, 20);
    strcpy(clientip, inet_ntoa(addr.sin_addr));
    return clientip;
}
