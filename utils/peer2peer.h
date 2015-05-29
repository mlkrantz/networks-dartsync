#ifndef PEER2PEER_H
#define PEER2PEER_H

#include "file_monitor.h"

/*****************************************************
 ***************** struct definition *****************
 *****************************************************/

/*
 * for message transferred between peers
 */
typedef struct pr_msg{
	/*
     * unused fields in the struct
     */
    //unsigned long recv_peer_ip;
	//int recv_peer_port;
	/*int idx_of_this_peer;
	int num_peers;*/
	char filename[256];
    int piece_start_idx;                // piece start index
    int piece_len;                      // piece length
}peer_msg;

/*
 * struct for dowload_handler() and upload_handler()
 */
typedef struct _peer_info_t {
    char file_name[256];                // Current downloading file name
    /*
     * change long to unsigned int for use in raspberry pi
     */
    //unsigned long file_time_stamp;    // Timestamp of current downloading file
    unsigned int file_time_stamp;
    int sockfd;                         // TCP connection to this remote peer
    int idx_of_this_peer;               // where the trunk begins in the file
    /* int num_peers;                      // trunk length of the file*/
    int piece_start_idx;                // piece start index
    int piece_len;                      // piece length
    
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

/*
 * This function parse the temporary file names, and return a peer_info_t struct
 * The valid fields in the struct is file_name, piece_start_idx, piece_len, and file_time_stamp
 */
peer_info_t *parse_tmpt_file_name(char *tmpt_file_name);

/*
 * This function gets the available peer index
 */
int get_available_peer_idx(int peer_num);

/*
 * This function checks if all the entries in peer_flag[] is zero
 */
int is_all_zero(int peer_num);

/*
 * This function prints the peer_flag table
 */
void print_peer_flag(int peer_num);

#endif
