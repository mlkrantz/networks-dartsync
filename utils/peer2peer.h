#include "file_monitor.h"
typedef struct pr_msg{
	unsigned long recv_peer_ip;
	int recv_peer_port;
	int idx_of_this_peer;
	int num_peers;
	char filename[256];
}peer_msg;
void download_file(file_node* f_node);
void* peer_handler(void* arg);
void set_mtime(char* filepath, time_t time_last_modified);

void download_file_multi_thread(file_node* f_node);
void* download_handler(void* arg);
void* peer_handler_multi_thread(void* arg);
void* upload_handler(void* arg);