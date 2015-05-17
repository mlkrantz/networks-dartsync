#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
typedef struct _tracker_side_peer_t {
	//Remote peer IP address, 16 bytes.
	unsigned long ip;
	//Last alive timestamp of this peer.
	time_t last_time_stamp;
	//TCP connection to this remote peer.
	int sockfd;
	//Pointer to the next peer, linked list.
	struct _tracker_side_peer_t *next;
} tracker_peer_t;

void peer_table_initial();
void peer_table_add(unsigned long ip, int socket);
void peer_table_update_timestamp(unsigned long ip);
tracker_peer_t* get_peer_table();