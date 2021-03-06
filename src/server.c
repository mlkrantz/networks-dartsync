#include "../utils/file_monitor.h"
#include "../utils/network.h"
#include "../utils/tracker_peer_table.h"
#include "../common/constants.h"
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

void* handshake_handler(void* arg);
int get_authorization(int client_handshake_socket);
void tracker_stop();
void* peer_live_handler(void* arg);

int server_socket;
int main_thread_alive = 1;
char *password = "";
char directory[MAX_DIR_LEN];

int main(int argc, char const *argv[]) {
    char *default_dir = "dartsync";
    printf("Enter the root directory that DartSync will monitor (default is " BLUE "%s" RESET "): ", default_dir);

    /* Default if nothing entered */
    if (fscanf(stdin, "%[^\n]s", directory) < 1) {
        strncpy(directory, default_dir, strlen(default_dir) + 1);
    }
    // printf("Monitoring root directory " BLUE "%s" RESET "\n", directory);

    char *prompt = "Enter a password peer nodes will use to connect: ";
    password = getpass(prompt);

	if ((server_socket = create_server_socket(SERVER_PORT)) < 0) {
		printf("Error creating server socket\n");
		exit(1);
	}	
	peer_table_initial();
	file_table_initial();
	signal(SIGINT, tracker_stop);
	pthread_t peer_live_thread;
	if (pthread_create(&peer_live_thread, NULL, peer_live_handler, NULL) < 0) {
		printf("Error in pthread_create peer_live_thread\n");
	}
	while (1) {
		int client_socket;
		if ((client_socket = accept(server_socket, NULL, NULL)) > 0) {			
			pthread_t handshake_thread;
			if (pthread_create(&handshake_thread, NULL, handshake_handler, (void *)&client_socket) < 0) {
				printf("Error in pthread_create()\n");
			}
		}		
	}	
	return 0;
}

void tracker_stop() {
    /* Avoid repeated calls in quick succession */
    if (main_thread_alive) {
        main_thread_alive = 0;
        shutdown(server_socket, SHUT_RDWR);
        close(server_socket);
        if (get_peer_table()) {
            peer_table_free(get_peer_table());
        }
        if (get_my_file_table()) {
            file_table_free(get_my_file_table());
        }
        sleep(1);
        exit(EXIT_SUCCESS);
    }
}

void* handshake_handler(void* arg) {
	int client_socket = *(int*)arg;
	/* Create new connection with the peer, for one to one service */
	int server_handshake_port = SERVER_PORT + 1;
	int server_handshake_socket;
	while ((server_handshake_socket = create_server_socket(server_handshake_port)) < 0) {
		server_handshake_port++;
	}
	send(client_socket, &server_handshake_port, sizeof(int), 0);
	int client_handshake_socket = accept(server_handshake_socket, NULL, NULL);

	/* Start the authorization process */
	int try = 0, pass = 0;
	while (try < MAX_TRY) {
		if (get_authorization(client_handshake_socket)) {
			pass = 1;
			send(client_handshake_socket, &pass, sizeof(int), 0);
			break;
		}
		send(client_handshake_socket, &pass, sizeof(int), 0);
		try++;
	}
	if (pass == 0) {
		shutdown(client_handshake_socket, SHUT_RDWR);
		close(client_handshake_socket);
		shutdown(server_handshake_socket, SHUT_RDWR);
		close(server_handshake_socket);
		fprintf(stderr, "Authentication error, handshake_handler exiting...\n");
		pthread_detach(pthread_self());
		pthread_exit(NULL);
	}
	printf("Connected to client on port %d\n", server_handshake_port);

    /* Send directory information to client */
    send(client_handshake_socket, directory, MAX_DIR_LEN, 0);

	/* After the authorization, update the peer table */
	// unsigned long client_ip;
	// recv(client_handshake_socket, &client_ip, sizeof(unsigned long), 0);
	unsigned long client_ip = get_peer_IP(client_handshake_socket);

	peer_table_add(client_ip, client_handshake_socket);
	peer_table_print();

	/* Start while loop to handle all the incoming messages from that peer */
	int state;
	while (recv(client_handshake_socket, &state, sizeof(int), 0) > 0) {
		// printf("Recv sth\n");
		file_node *client_table, *runner;
		switch (state) {
			/* Case 1 : It's a heartbeat signal from peer */
			case SIGNAL_HEARTBEAT:
				peer_table_update_timestamp(client_ip);
				printf("Get heartbeat from %s\n", inet_ntoa(*(struct in_addr*)&client_ip));
				// peer_table_print();
				break;
			/* Case 2 : It's a updated file table from peer */
			case SIGNAL_FILE_UPDATE:
				client_table = NULL;
				recv_file_table(client_handshake_socket, &client_table);
				sync_from_client(client_table);
				file_table_print();
				/* broadcast the updated file table to all peers */
				broadcast_file_table();
				file_table_free(client_table);
				break;
			default:
				printf("handshake_handler of %s: Unknown signal type %d\n", inet_ntoa(*(struct in_addr*)&client_ip), state);
		}
	}

	/* Update the peer table */
	peer_table_delete(client_ip);
	peer_table_print();

	/* Update the file table */
	delete_disconn_peer(client_ip);
	file_table_print();

	/* Shut down all the stuff of this thread */
	shutdown(client_handshake_socket, SHUT_RDWR);
	close(client_handshake_socket);
	shutdown(server_handshake_socket, SHUT_RDWR);
	close(server_handshake_socket);
	printf("handshake_handler for %s exit...\n", inet_ntoa(*(struct in_addr*)&client_ip));
	pthread_detach(pthread_self());
	pthread_exit(NULL);
}

int get_authorization(int client_handshake_socket) {
	int length;
	recv(client_handshake_socket, &length, sizeof(int), 0);
	char password_received[length+1];
	recv(client_handshake_socket, password_received, sizeof(char)*(length+1), 0);
	if (strcmp(password_received, password) == 0) {		
		return 1;
	} else {
		return 0;
	}
}

void* peer_live_handler(void* arg) {
	while (main_thread_alive) {
		peer_live_check();
		sleep(1);		
	}
	printf("\npeer_live_thread exit...\n");
	pthread_detach(pthread_self());
	pthread_exit(NULL);
}
