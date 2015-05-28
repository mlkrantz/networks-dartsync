#ifndef FILE_MONITOR_H
#define FILE_MONITOR_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <memory.h>

#define FILE_TYPE 8
#define FOLDER_TYPE 4
typedef struct node{
	//the size of the file
	int size;
	//the name of the file
	char name[256];
	//the type of the file FILE_TYPE or FOLDER_TYPE
	int type;
	//the timestamp when the file is modified or created
	time_t timestamp;
	//pointer to build the linked list
	struct node *next;
	//for the file table on peers, it is the ip address of the peer
	//for the file table on tracker, it records the ip of all peers which has the newest edition of the file
	int num_peers;
	unsigned long peers[200];
}file_node;


void watchDirectory(char* directory);
void block_update();
void unblock_update();
void file_table_initial();
file_node* get_my_file_table();
void file_table_free(file_node* file_node_head);
void file_table_print();
//return 1 if updated, 0 if not updated
int file_table_update();
int file_table_update_helper(char* directory, file_node** last);
void send_file_table(int socket);
void recv_file_table(int socket, file_node** new_table);
void delete_disconn_peer(unsigned long client_IP);
void sync_with_server(file_node* server_table);
void sync_from_client(file_node* client_table);
void broadcast_file_table();

// added by Sha
int get_file_size(char *file_name);
int get_file_line_num(char *file_name);

// added by Wei
/*
 * Delete file or folder only when exists 
 */
void safe_delete(char* file_name);
/*
 * Create a folder when not exist. If the parent directory does not exist, create the parent directory first
 */
void safe_add_folder(char* folder_name);

#endif
