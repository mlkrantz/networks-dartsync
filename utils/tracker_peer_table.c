#include "tracker_peer_table.h"
#include "network.h"
tracker_peer_t* peer_table;

void peer_table_initial() {
	peer_table = (tracker_peer_t*)malloc(sizeof(tracker_peer_t));
	bzero(peer_table, sizeof(tracker_peer_t));
}

void peer_table_add(unsigned long ip, int socket) {
	tracker_peer_t * new_peer_entry = (tracker_peer_t*)malloc(sizeof(tracker_peer_t));
	bzero(new_peer_entry, sizeof(tracker_peer_t));
	new_peer_entry->ip = ip;
	time(&(new_peer_entry->last_time_stamp));
	new_peer_entry->sockfd = socket;
	new_peer_entry->next = peer_table->next;
	peer_table->next = new_peer_entry;
}

void peer_table_update_timestamp(unsigned long ip) {
	tracker_peer_t* runner = peer_table->next;
	while (runner != NULL && runner->ip != ip) {
		runner = runner->next;
	}
	if (runner != NULL) {
		time(&(runner->last_time_stamp));
	}
}

void peer_table_delete(unsigned long ip) {
	tracker_peer_t* runner = peer_table;
	while (runner->next != NULL && runner->next->ip != ip) {
		runner = runner->next;
	}
	if (runner->next != NULL) {
		tracker_peer_t* temp = runner->next;
		runner->next = temp->next;
		free(temp);
	}
}

void peer_table_print() {
	printf("********** Peer Table ***********\n");
	tracker_peer_t* runner = peer_table;
	while (runner->next != NULL) {
		struct in_addr ip_address;
		ip_address.s_addr = runner->next->ip;
		char* ip = inet_ntoa(ip_address);
		printf("%s timestamp: %ld \n", ip, runner->next->last_time_stamp);
		runner = runner->next;
	}
	printf("*********************************\n");
}

tracker_peer_t* get_peer_table() {
	return peer_table;
}

void peer_table_free(tracker_peer_t* peer_node) {
	tracker_peer_t *curr = peer_node;
	while (curr != NULL) {
		tracker_peer_t *temp = curr;
		curr = curr->next;
		free(temp);
	}
}

void peer_live_check() {
	tracker_peer_t* runner = peer_table;
	time_t current_time;
	time(&current_time);
	while (runner->next != NULL) {
		if (current_time - runner->next->last_time_stamp > HEARTBEAT_INTERVAL_SEC + 1) {
			printf("peer_live_check : Have not get heartbeat from %s\n", inet_ntoa(*(struct in_addr*)&runner->next->ip));
			tracker_peer_t* temp = runner->next;
			runner->next = temp->next;
			free(temp);
			continue;
		}
		runner = runner->next;
	}
}

