#include "utils/file_monitor.h"
#include "utils/network.h"
#include <pthread.h>
#include <signal.h>
#define MAX_TRY 3

void peer_stop();
int get_authorization(int client_handshake_socket);
void* heartbeat(void* arg);
void* file_checker(void* arg);
void* tracker_handler(void* arg);
int client_socket;

int main(int argc, char const *argv[]) {
	/* Greeting with tracker on the public connection */
	client_socket = create_client_socket(SERVER_PORT);
	signal(SIGINT, peer_stop);

	/* Create private connection with tracker */
	int server_handshake_port;
	recv(client_socket, &server_handshake_port, sizeof(int), 0);
	int client_handshake_socket = create_client_socket(server_handshake_port);

	/* Start the authorization process */
	int try = 0, pass = 0;
	while (try < MAX_TRY) {
		if (get_authorization(client_handshake_socket)) {
			pass =1;
			break;
		}
		printf("Incorrect password..\n");
		try++;
	}
	if (pass) {
		printf("connet to server\n");
	} else {
		printf("Fail connet to server\n");
	}

	/* Register on the tracker's peer table */
	unsigned long my_ip = get_My_IP();
	send(client_handshake_socket, &my_ip, sizeof(unsigned long), 0);

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
	
	pthread_join(heartbeat_thread, NULL);
	return 0;
}

void peer_stop() {
	shutdown(client_socket, SHUT_RDWR);
	close(client_socket);
	exit(1);
}
int get_authorization(int client_handshake_socket) {
	printf("Please input password before login the Dartsync\n");
	char password[100];
	scanf("%s", password);
	int length = strlen(password);
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
	while (1) {
		send(client_handshake_socket, &state, sizeof(int), 0);
		printf("Send heartbeat\n");
		sleep(HEARTBEAT_INTERVAL_SEC);
	}
	pthread_detach(pthread_self());
	pthread_exit(NULL);
}

void* file_checker(void* arg) {
	int client_handshake_socket = *(int*)arg;
	watchDirectory("asd");
	file_table_initial();
	unblock_update();
	/* Monitor file directory periodically */
	int state = SIGNAL_FILE_UPDATE;
	while (1) {
		if (file_table_update()) {
			send(client_handshake_socket, &state, sizeof(int), 0);
			printf("Send file update\n");
			file_table_print();
			send_file_table(client_handshake_socket);
		}
		sleep(FILE_UPDATE_INTERVAL_SEC);
	}
	pthread_detach(pthread_self());
	pthread_exit(NULL);
}

void* tracker_handler(void* arg) {
	int client_handshake_socket = *(int*)arg;
	file_node *server_table, *runner;
	while (1) {		
		server_table = NULL;
		recv_file_table(client_handshake_socket, &server_table);	
		printf("=========Server Table=======\n");	
		// runner = server_table;
		// while (runner->next != NULL) {
		// 	printf("%s\t", runner->next->name);
		// 	int i;
		// 	for (i = 0; i < runner->next->num_peers; i++) {
		// 		printf("%s\t", inet_ntoa(*(struct in_addr*)&runner->next->peers[i]));
		// 	}
		// 	printf("\n");
		// 	runner = runner->next;
		// }
		// printf("=================================\n");
		sync_with_server(server_table);
		file_table_free(server_table);
	}
}