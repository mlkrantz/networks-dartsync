#include "network.h"
#include "tracker_peer_table.h"
#include "peer2peer.h"
#include <pthread.h>

int num_finished = 0;
pthread_mutex_t *flow_control_mutex;



/****************************************************
 *** Test functions for single peer file transfer ***
 *** No longer useful in multi-peer file transfer ***
 ****************************************************/
void download_file(file_node* f_node) {
	if (f_node->num_peers == 0) {
		printf("download_file: No peer alive\n");
		return;
	}
	/* Connect to the send peer */
	unsigned long send_peer_ip = f_node->peers[0];
	printf("send_peer_ip: %ld\n", send_peer_ip);
	int peer_socket = create_client_socket_byIp(send_peer_ip, PEER_PORT);

	/* Create new connection with the peer, for one to one service */
	int download_port = PEER_PORT + 1;
	int server_socket, download_socket;
	while ((server_socket = create_server_socket(download_port)) < 0) {
		download_port++;
	}

	/* Initialize the message */
	peer_msg *message = (peer_msg*)malloc(sizeof(peer_msg));
	bzero(message, sizeof(peer_msg));
	message->recv_peer_ip = get_My_IP();
	message->recv_peer_port = download_port;
	sprintf(message->filename, "%s", f_node->name);
	if (send(peer_socket, message, sizeof(peer_msg), 0) < 0) {
		printf("Fail sending peer message\n");
	}
	free(message);	

	/* Wating for connecting */
	if ((download_socket = accept(server_socket, NULL, NULL)) < 0) {
		printf("Fail to accept the download_socket\n");
		close(peer_socket);
		close(server_socket);
		return;
	}

	/* Receive length of file */
	int fileLen;
	recv(download_socket, &fileLen, sizeof(int), 0);
	/* Create new local file */
	FILE *fp = fopen(f_node->name, "w");  
	if (fp == NULL)  {  
		printf("recvFile(): File:\t%s Can Not Open To Write!\n", f_node->name);  
		close(peer_socket);
		close(server_socket);
		close(download_socket);
		return;
	}  

	/* Receive data of file */
	char buffer[BUFFER_SIZE]; 
	bzero(buffer, sizeof(buffer));
	int download_length = 0;
	while (download_length < fileLen) {
		int buflen = 0, len;
		while (download_length < fileLen && buflen < BUFFER_SIZE) {
			if ((len = recv(download_socket, buffer + buflen, BUFFER_SIZE - buflen, 0)) < 0) {
				printf("Error receive file data\n");
				return;
			} else {
				buflen = buflen + len;
				download_length = download_length + len;
			}
		}
		int write_length = fwrite(buffer, sizeof(char), buflen, fp);  
		if (write_length < buflen) {  
			printf("recvFile(): File:\t%s Write Failed!\n", f_node->name);  
			return;  
		}
		bzero(buffer, BUFFER_SIZE);  
	}
	fclose(fp); 
	printf("Update file %s with timestamp %ld\n", f_node->name, f_node->timestamp);
	set_mtime(f_node->name, f_node->timestamp);
	close(peer_socket);
	close(download_socket);
}

void* peer_handler(void* arg) {
	/* Create the main socket */
	int peer_main_socket = *(int*)arg;
	/* Waiting for incoming request */
	while (1) {
		int peer_socket;
		if ((peer_socket = accept(peer_main_socket, NULL, NULL)) > 0) {			
			/* Receive the message from peer */
			peer_msg *message = (peer_msg*)malloc(sizeof(peer_msg));
			bzero(message, sizeof(peer_msg));
			char buffer[BUFFER_SIZE];
			bzero(buffer, sizeof(buffer));
			int buflen = 0, len;
			while (buflen < sizeof(peer_msg)) {
				if ((len = recv(peer_socket, buffer + buflen, sizeof(peer_msg) - buflen, 0)) < 0) {
					printf("Error recv peer message\n");
					break;
				} else {
					buflen = buflen + len;
				}
			}
			memcpy(message, buffer, sizeof(peer_msg));

			/* Connect to the private file transport socket */
			int upload_socket = create_client_socket_byIp(message->recv_peer_ip, message->recv_peer_port);
			/* Try to open the file */
			FILE *fp = fopen(message->filename, "r"); 			
			
			if (fp == NULL)  {  
				printf("File:\t%s Not Found!\n", message->filename);
				free(message);
				break;  
			} else {
				/* Send the file length */
				fseek(fp,0,SEEK_END);
				int fileLen = ftell(fp);
				fseek(fp,0,SEEK_SET);
				send(upload_socket, &fileLen, sizeof(int), 0);
				/* Send the file data */
				char buffer[BUFFER_SIZE];
				bzero(buffer, BUFFER_SIZE);  
				int file_block_length = 0;  
				while( (file_block_length = fread(buffer, sizeof(char), BUFFER_SIZE, fp)) > 0) {  
					// printf("file_block_length = %d\n", file_block_length);  
					if (send(upload_socket, buffer, file_block_length, 0) < 0) {  
						printf("Send File:\t%s Failed!\n", message->filename);  
						break;  
					}  
					bzero(buffer, sizeof(buffer));  
				}  
				fclose(fp);
			}
			free(message);
		} else {
			printf("Fail accept in peer_handler\n");
			break;
		}	
	}
	printf("peer_handler_thread exit\n");
	pthread_exit(NULL);
}

/****************************************************
 ********** currently used function  ****************
 ****************************************************/

/*
 * set the latest modified time of a given file
 * input parameters:
 *      filepath:           the name of file to change time
 *      time_last_modifed:  the latest modified time
 */
void set_mtime(char* filepath, time_t time_last_modified) {
	char date[16];
	// printf("the time in the set_mtime function is  %ld\n", time_last_modified);
	time_last_modified = time_last_modified - 14400;
	strftime(date,16,"%Y%m%d%H%M.%S",gmtime(&time_last_modified));
	char command[150];
	sprintf(command,"touch -t %s %s", date, filepath);

	// struct stat attrib;
	// stat(filepath, &attrib);
	// printf("mtime before modification is %ld\n", attrib.st_mtime);
	system(command);
	// stat(filepath, &attrib);
	// printf("mtime after modification is %ld\n", attrib.st_mtime);

}

/*
 * When a file needs to be downloaded, this function will be called.
 * The input parameter is a struct file_node, containing that which peers have the file.
 * In this function, serveral threads will be created, each requesting a piece of the file from a peer.
 */
void download_file_multi_thread(file_node* f_node) {
	if (f_node->num_peers == 0) {
		printf("download_file: No peer alive\n");
		return;
	}
    
    // added by Sha
    pthread_t thread[f_node->num_peers];
    peer_info_t peer_info;
    
	/* Check if the fold exist */
	char folder_path[256];
	bzero(folder_path, sizeof(folder_path));
	int len_file_name = strlen(f_node->name);
	int i;
	for ( i = 0; i < len_file_name; i++) {
		folder_path[i] = f_node->name[i];
		if (folder_path[i] == '/') {
			if (opendir(folder_path) == NULL) {
				char command[256];
				sprintf(command, "mkdir %s", folder_path);
				system(command);
			}
		} 
	}
    
	/* Create download thread */
	flow_control_mutex = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(flow_control_mutex, NULL);
	num_finished = 0;	
	for (i = 0; i < f_node->num_peers; i++) {
		pthread_mutex_lock(flow_control_mutex);
		f_node->peers[f_node->num_peers] = f_node->peers[i];
		pthread_t download_thread;
		pthread_create(&download_thread, NULL, download_handler, f_node);
	}
    
	/* Check if all the download thread has exit */
	while (num_finished < f_node->num_peers) {
		sleep(1);
	}
	free(flow_control_mutex);
	/* Merge the file */
	FILE *fp = fopen(f_node->name, "w");
	if (fp == NULL)  {  
		printf("download_file_multi_thread: File:\t%s Can Not Open To Write!\n", f_node->name);  
		return;
	} 
	for (i = 0; i < f_node->num_peers; i++) {
		char tempfilename[256];
		sprintf(tempfilename, "%s_%dtemp", f_node->name, i);
		/* Open temp file */
		FILE *temp_fp = fopen(tempfilename, "r");
		if (temp_fp == NULL) {
			printf("%s Not found !\n", tempfilename);
		}
		/* Copy the data */
		char buffer[BUFFER_SIZE]; 
		bzero(buffer, sizeof(buffer));
		int buflen;
		while ((buflen = fread(buffer, sizeof(char), BUFFER_SIZE, temp_fp)) > 0) {
			int write_length = fwrite(buffer, sizeof(char), buflen, fp); 
		}
		fclose(temp_fp);
		/* Remove the temp file */
		char command[256];
		sprintf(command, "rm %s", tempfilename);
		system(command);		 
	}
	fclose(fp);
	printf("Update file %s with timestamp %ld\n", f_node->name, f_node->timestamp);
	set_mtime(f_node->name, f_node->timestamp);
}

/*
 * In download_file_multi_thread(), serveral threads will be created. And this function is passed into each
 * thread, to explicitly receiving a trunk of the file from a perr.
 * The input parameter is of struct _peer_info_t, which can tell us:
 *      1. from which peer
 *      2. download which part
 *      3. of which file
 */
void* download_handler(void* arg) {
	file_node* f_node = (file_node*)arg;
	
    /* Connect to the send peer, each download handler will have distinguished send_peer_ip */
	unsigned long send_peer_ip = f_node->peers[f_node->num_peers];
	pthread_mutex_unlock(flow_control_mutex);	
	int peer_socket = create_client_socket_byIp(send_peer_ip, PEER_PORT);
	
    /* Create new connection with the peer, for one to one service */
	int download_port = PEER_PORT + 1;
	int server_socket, download_socket;
	while ((server_socket = create_server_socket(download_port)) < 0) {
		download_port++;
	}
	
    /* Initialize the message */
	peer_msg *message = (peer_msg*)malloc(sizeof(peer_msg));
	bzero(message, sizeof(peer_msg));
	message->recv_peer_ip = get_My_IP();
	message->recv_peer_port = download_port;
	int i = 0;
	while (f_node->peers[i] != send_peer_ip) {
		i++;
	}
	if (i == f_node->num_peers) {
		printf("download_handler: Error getting the right idx_of_this_peer\n");
	} else {
		message->idx_of_this_peer = i;
	}
	message->num_peers = f_node->num_peers;
	sprintf(message->filename, "%s", f_node->name);
	if (send(peer_socket, message, sizeof(peer_msg), 0) < 0) {
		printf("Fail sending peer message\n");
	}
	free(message);	
	
    /* Wating for connecting */
	if ((download_socket = accept(server_socket, NULL, NULL)) < 0) {
		printf("Fail to accept the download_socket\n");
		close(peer_socket);
		close(server_socket);
		num_finished++;
		pthread_detach(pthread_self());
		pthread_exit(NULL);
	}
	
    /* Receive length of file */
	int fileLen;
	recv(download_socket, &fileLen, sizeof(int), 0);
	
    /* Create new local file */
	char tempfilename[256];
	sprintf(tempfilename, "%s_%dtemp", f_node->name, i);
	FILE *fp = fopen(tempfilename, "w");  
	if (fp == NULL)  {  
		printf("download_handler: File:\t%s Can Not Open To Write!\n", tempfilename);  
	} else {
		/* Receive data of file */
		char buffer[BUFFER_SIZE]; 
		bzero(buffer, sizeof(buffer));
		int download_length = 0;
		while (download_length < fileLen) {
			int buflen = 0, len;
			while (download_length < fileLen && buflen < BUFFER_SIZE) {
				if ((len = recv(download_socket, buffer + buflen, BUFFER_SIZE - buflen, 0)) < 0) {
					printf("Error receive file data\n");
					download_length = fileLen;
					break;
				} else {
					buflen = buflen + len;
					download_length = download_length + len;
				}
			}
			int write_length = fwrite(buffer, sizeof(char), buflen, fp);  
			if (write_length < buflen) {  
				printf("recvFile(): File:\t%s Write Failed!\n", f_node->name);  
				download_length = fileLen;
				break; 
			}
			bzero(buffer, BUFFER_SIZE);  
		}
		fclose(fp); 	
	}
	num_finished++;
	close(peer_socket);
	close(server_socket);
	close(download_socket);
	// printf("Reach 1\n");
	pthread_detach(pthread_self());
	// printf("Reach 2\n");
	pthread_exit(NULL);
}

/*
 * This function is passed into a thread created when the program begins.
 * It always listens on PEER_PORT port. When another peer connects on this port, it means that peer
 * will request data. So a new thread will be created, and upload_handler() will be passed into that thread
 * to explicitly handle the data transfer.
 * Input paramter: the sockfd of TCP connection the thread is listening on.
 */
void* peer_handler_multi_thread(void* arg) {
	/* Get the main socket */
	int peer_main_socket = *(int*)arg;
	/* Waiting for incoming request */
	while (1) {
		int peer_socket;
		if ((peer_socket = accept(peer_main_socket, NULL, NULL)) > 0) {
			printf("peer_handler_multi_thread : Receive download request\n");
			pthread_t upload_thread;
			pthread_create(&upload_thread, NULL, upload_handler, &peer_socket);
		}  else {
			printf("Fail accept in peer_handler_multi_thread\n");
			break;
		}
	}
	printf("peer_handler_thread exit\n");
	pthread_exit(NULL);
}

/*
 * This function handles data uploading with a peer for a part of a file.
 * Input parameter:
 *      sockfd of the TCP connection between this peer and the peers asking for data.
 */
void* upload_handler(void* arg) {
	/* Get peer socket */
	int peer_socket = *(int*)arg;
	/* Receive the message from peer */
	peer_msg *message = (peer_msg*)malloc(sizeof(peer_msg));
	bzero(message, sizeof(peer_msg));
	char buffer[BUFFER_SIZE];
	bzero(buffer, sizeof(buffer));
	int buflen = 0, len;
	while (buflen < sizeof(peer_msg)) {
		if ((len = recv(peer_socket, buffer + buflen, sizeof(peer_msg) - buflen, 0)) < 0) {
			printf("Error recv peer message\n");
			break;
		} else {
			buflen = buflen + len;
		}
	}
	memcpy(message, buffer, sizeof(peer_msg));
	/* Connect to the private file transport socket */
	int upload_socket = create_client_socket_byIp(message->recv_peer_ip, message->recv_peer_port);
	/* Try to open the file */
	FILE *fp = fopen(message->filename, "r");
	if (fp == NULL)  {  
		printf("File:\t%s Not Found!\n", message->filename);
	} else {
		/* Get the total length of the file */
		fseek(fp, 0, SEEK_END);
		int fileLen = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		/* Calculate the start offset and the length of the part of file to be sent */
		int start_offset = (int) ((long) fileLen * (long)(message->idx_of_this_peer) / (long) message->num_peers);
		int next_offset = (int) ((long) fileLen * (long)(message->idx_of_this_peer + 1) / (long) message->num_peers);
		int part_length = next_offset - start_offset;
		/* Send the file length */
		send(upload_socket, &part_length, sizeof(int), 0);
		/* Send the file data */
		fseek(fp, start_offset, SEEK_SET);
		int file_block_length;
		int length_sent = 0;
		while (length_sent < part_length) {
			file_block_length = (part_length - length_sent) >= BUFFER_SIZE ? BUFFER_SIZE : part_length - length_sent;
			bzero(buffer, BUFFER_SIZE);  
			fread(buffer, sizeof(char), file_block_length, fp);
			if (send(upload_socket, buffer, file_block_length, 0) < 0) {  
				printf("Send File:\t%s Failed!\n", message->filename);  
				break;  
			}  else {
				length_sent = length_sent + file_block_length;
			}
		}
		fclose(fp);
	}
	free(message); 
	pthread_detach(pthread_self());
	pthread_exit(NULL);
}
