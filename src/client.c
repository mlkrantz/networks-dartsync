#include "../utils/network.h"
#include "../utils/peer2peer.h"
#include "../common/constants.h"
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#define BUF_SIZE 100

void peer_stop();
int get_authorization(int client_handshake_socket);
int is_valid_IP(char* location, struct sockaddr_in *addr);
void* heartbeat(void* arg);
void* file_checker(void* arg);
void* tracker_handler(void* arg);
int main_thread_alive = 1;

int main(int argc, char const *argv[]) {
    /* Get tracker info */
    char tracker_location[BUF_SIZE];
    printf("Enter tracker hostname or IP address: ");
    fscanf(stdin, "%s", tracker_location);

    struct sockaddr_in sa;
    int client_main_socket = -1;
    int server_handshake_port = -1;
    int client_handshake_socket = -1;

    /* Connect using IP or hostname */
    if (is_valid_IP(tracker_location, &sa)) {
        /* Greeting with tracker on the public connection */
        unsigned long addr = sa.sin_addr.s_addr;
        client_main_socket = create_client_socket_byIp(addr, SERVER_PORT);

        /* Create private connection with tracker */
        recv(client_main_socket, &server_handshake_port, sizeof(int), 0);
        client_handshake_socket = create_client_socket_byIp(addr, server_handshake_port);
    } else {
	    /* Greeting with tracker on the public connection */
	    client_main_socket = create_client_socket(tracker_location, SERVER_PORT);

	    /* Create private connection with tracker */
	    recv(client_main_socket, &server_handshake_port, sizeof(int), 0);
	    client_handshake_socket = create_client_socket(tracker_location, server_handshake_port);
    }

	/* Start the authorization process */
	int try = 0, pass = 0;
	while (try < MAX_TRY) {
		if (get_authorization(client_handshake_socket)) {
			pass = 1;
			break;
		}
		fprintf(stderr, "Error: incorrect password...\n");
		try++;
	}
	if (pass) {
		printf("Connected to server!\n");
	} else {
		fprintf(stderr, "Failed to connect to server. Exiting...\n");
        shutdown(client_main_socket, SHUT_RDWR);
        close(client_main_socket);
        shutdown(client_handshake_socket, SHUT_RDWR);
        close(client_handshake_socket);
        exit(EXIT_FAILURE);
	}

	/* Register on the tracker's peer table */
	unsigned long my_ip = get_My_IP();
	send(client_handshake_socket, &my_ip, sizeof(unsigned long), 0);

    /* Stop threads if SIGINT */
    signal(SIGINT, peer_stop);

	/* Start heartbeat thread */
	pthread_t heartbeat_thread;
	if (pthread_create(&heartbeat_thread, NULL, heartbeat, (void*)&client_handshake_socket) < 0) {
		printf("Error creating heartbeat thread\n");
	}

	/* Start file checker thread */
	pthread_t filechecker_thread;
	if (pthread_create(&filechecker_thread, NULL, file_checker, (void*)&client_handshake_socket) < 0) {
		printf("Error creating file_checker thread\n");
	}

	/* Start tracker handler thread */
	pthread_t tracker_handler_thread;
	if (pthread_create(&tracker_handler_thread, NULL, tracker_handler, (void*)&client_handshake_socket) < 0) {
		printf("Error creating tracker_handler thread\n");
	}
	
	/* Start peer handler thread */
	int peer_main_socket = create_server_socket(PEER_PORT);
	pthread_t peer_handler_thread;
	if (pthread_create(&peer_handler_thread, NULL, peer_handler_multi_thread, (void*)&peer_main_socket) < 0) {
		printf("Error creating peer_handler thread\n");
	}

	pthread_join(heartbeat_thread, NULL);
	pthread_join(filechecker_thread, NULL);
	shutdown(client_main_socket, SHUT_RDWR);
	close(client_main_socket);
	shutdown(client_handshake_socket, SHUT_RDWR);
	close(client_handshake_socket);
	shutdown(peer_main_socket, SHUT_RDWR);
	close(peer_main_socket);
	pthread_join(tracker_handler_thread, NULL);
	pthread_join(peer_handler_thread, NULL);
	
    return 0;
}

void peer_stop() {
	main_thread_alive = 0;
	printf("\nmain_thread_alive has been set to 0\n");
	printf("Waiting for all subthreads to terminate. Please wait patiently ^_^\n");
}

int get_authorization(int client_handshake_socket) {
	char *prompt = "Please input password before logging into Dartsync: ";
	char *password = getpass(prompt);
	int length = (int)strlen(password);
	send(client_handshake_socket, &length, sizeof(int), 0);
	send(client_handshake_socket, password, sizeof(char)*(length+1), 0);
	int result;
	recv(client_handshake_socket, &result, sizeof(int), 0);
	return result;
}

void* heartbeat(void* arg) {
	int client_handshake_socket = *(int*)arg;
	/* Send heartbeat */
	int state = SIGNAL_HEARTBEAT;
	while (main_thread_alive) {
		send(client_handshake_socket, &state, sizeof(int), 0);
		printf("Send heartbeat\n");
		sleep(HEARTBEAT_INTERVAL_SEC);
	}
	printf("heartbeat_thread exit\n");
	pthread_exit(NULL);
}

void* file_checker(void* arg) {
	int client_handshake_socket = *(int*)arg;
	watchDirectory("asd");
	file_table_initial();
	unblock_update();
	/* Monitor file directory periodically */
	int state = SIGNAL_FILE_UPDATE;
	while (main_thread_alive) {
		if (file_table_update()) {
			send(client_handshake_socket, &state, sizeof(int), 0);
			printf("Send file update\n");
			file_table_print();
			send_file_table(client_handshake_socket);
		}
		sleep(FILE_UPDATE_INTERVAL_SEC);
	}
	file_table_free(get_my_file_table());
	printf("filechecker_thread exit\n");
	pthread_exit(NULL);
}

void* tracker_handler(void* arg) {
	int client_handshake_socket = *(int*)arg;
	file_node *server_table, *runner;
	while (main_thread_alive) {		
		server_table = NULL;
		recv_file_table(client_handshake_socket, &server_table);
		if (server_table == NULL) {
            if (main_thread_alive) {
                printf("File table receive failed, check tracker status. Exiting...\n");
                main_thread_alive = 0;
            }
			continue;
		}	
        printf("before sync_with_server()\n");
		sync_with_server(server_table);
        printf("after sync_with_server()\n");
		file_table_free(server_table);
	}
	printf("tracker_handler_thread exit\n");
	pthread_exit(NULL);
}

int is_valid_IP(char* location, struct sockaddr_in *addr) {
    /* Store address in sin_addr upon success */
    int result = inet_pton(AF_INET, location, &addr->sin_addr);
    return result > 0;
}
