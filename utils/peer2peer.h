#include "file_monitor.h"

/*****************************************************
 ***************** struct definition *****************
 *****************************************************/

/*
 * for message transferred between peers
 */
typedef struct pr_msg{
	unsigned long recv_peer_ip;
	int recv_peer_port;
	int idx_of_this_peer;
	int num_peers;
	char filename[256];
}peer_msg;

/*
 * struct for dowloadhandler() and upload_handler()
 */
typedef struct _peer_info_t {
    char file_name[256];                // Current downloading file name
    //unsigned long file_time_stamp;    // Timestamp of current downloading file
    int sockfd;                         // TCP connection to this remote peer
    int idx_of_this_peer;               // where the trunk begins in the file
    int num_peers;                      // trunk length of the file
}peer_info_t;


/****************************************************
 *** Test functions for single peer file transfer ***
 *** No longer useful in multi-peer file transfer ***
 ****************************************************/
void download_file(file_node* f_node);
void* peer_handler(void* arg);


/****************************************************
 ******** currently used function definition *******
 ****************************************************/

/*
 * set the latest modified time of a given file
 * input parameters:
 *      filepath:           the name of file to change time
 *      time_last_modifed:  the latest modified time
 */
void set_mtime(char* filepath, time_t time_last_modified);

/*
 * When a file needs to be downloaded, this function will be called.
 * The input parameter is a struct file_node, containing that which peers have the file.
 * In this function, serveral threads will be created, each requesting a piece of the file from a peer.
 */
void download_file_multi_thread(file_node* f_node);

/*
 * In download_file_multi_thread(), serveral threads will be created. And this function is passed into each
 * thread, to explicitly receiving a trunk of the file from a perr.
 * The input parameter is of struct _peer_info_t, which can tell us:
 *      1. from which peer
 *      2. download which part
 *      3. of which file
 */
void* download_handler(void* arg);

/*
 * This function is passed into a thread created when the program begins.
 * It always listens on PEER_PORT port. When another peer connects on this port, it means that peer
 * will request data. So a new thread will be created, and upload_handler() will be passed into that thread
 * to explicitly handle the data transfer.
 * Input paramter: the sockfd of TCP connection the thread is listening on.
 */
void* peer_handler_multi_thread(void* arg);

/*
 * This function handles data uploading with a peer for a part of a file.
 * Input parameter:
 *      sockfd of the TCP connection between this peer and the peers asking for data.
 */
void* upload_handler(void* arg);