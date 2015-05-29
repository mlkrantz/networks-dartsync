#include "network.h"
#include "tracker_peer_table.h"
#include "peer2peer.h"
#include <pthread.h>
#include <unistd.h>

/*int num_finished = 0;
pthread_mutex_t *flow_control_mutex;*/
int peer_flag[200];


/****************************************************
 *** Test functions for single peer file transfer ***
 *** No longer useful in multi-peer file transfer ***
 ****************************************************/
//void download_file(file_node* f_node) {
//    if (f_node->num_peers == 0) {
//        printf("download_file: No peer alive\n");
//        return;
//    }
//    /* Connect to the send peer */
//    unsigned long send_peer_ip = f_node->peers[0];
//    printf("send_peer_ip: %ld\n", send_peer_ip);
//    int peer_socket = create_client_socket_byIp(send_peer_ip, PEER_PORT);
//    
//    /* Create new connection with the peer, for one to one service */
//    int download_port = PEER_PORT + 1;
//    int server_socket, download_socket;
//    while ((server_socket = create_server_socket(download_port)) < 0) {
//        download_port++;
//    }
//    
//    /* Initialize the message */
//    peer_msg *message = (peer_msg*)malloc(sizeof(peer_msg));
//    bzero(message, sizeof(peer_msg));
//    message->recv_peer_ip = get_My_IP();
//    message->recv_peer_port = download_port;
//    sprintf(message->filename, "%s", f_node->name);
//    if (send(peer_socket, message, sizeof(peer_msg), 0) < 0) {
//        printf("Fail sending peer message\n");
//    }
//    free(message);
//    
//    /* Wating for connecting */
//    if ((download_socket = accept(server_socket, NULL, NULL)) < 0) {
//        printf("Fail to accept the download_socket\n");
//        close(peer_socket);
//        close(server_socket);
//        return;
//    }
//    
//    /* Receive length of file */
//    int fileLen;
//    recv(download_socket, &fileLen, sizeof(int), 0);
//    /* Create new local file */
//    FILE *fp = fopen(f_node->name, "w");
//    if (fp == NULL)  {
//        printf("recvFile(): File:\t%s Can Not Open To Write!\n", f_node->name);
//        close(peer_socket);
//        close(server_socket);
//        close(download_socket);
//        return;
//    }
//    
//    /* Receive data of file */
//    char buffer[BUFFER_SIZE];
//    bzero(buffer, sizeof(buffer));
//    int download_length = 0;
//    while (download_length < fileLen) {
//        int buflen = 0, len;
//        while (download_length < fileLen && buflen < BUFFER_SIZE) {
//            if ((len = recv(download_socket, buffer + buflen, BUFFER_SIZE - buflen, 0)) < 0) {
//                printf("Error receive file data\n");
//                return;
//            } else {
//                buflen = buflen + len;
//                download_length = download_length + len;
//            }
//        }
//        int write_length = fwrite(buffer, sizeof(char), buflen, fp);
//        if (write_length < buflen) {
//            printf("recvFile(): File:\t%s Write Failed!\n", f_node->name);
//            return;
//        }
//        bzero(buffer, BUFFER_SIZE);
//    }
//    fclose(fp);
//    printf("Update file %s with timestamp %ld\n", f_node->name, f_node->timestamp);
//    set_mtime(f_node->name, f_node->timestamp);
//    close(peer_socket);
//    close(download_socket);
//}
//
//void* peer_handler(void* arg) {
//    /* Create the main socket */
//    int peer_main_socket = *(int*)arg;
//    /* Waiting for incoming request */
//    while (1) {
//        int peer_socket;
//        if ((peer_socket = accept(peer_main_socket, NULL, NULL)) > 0) {
//            /* Receive the message from peer */
//            peer_msg *message = (peer_msg*)malloc(sizeof(peer_msg));
//            bzero(message, sizeof(peer_msg));
//            char buffer[BUFFER_SIZE];
//            bzero(buffer, sizeof(buffer));
//            int buflen = 0, len;
//            while (buflen < sizeof(peer_msg)) {
//                if ((len = recv(peer_socket, buffer + buflen, sizeof(peer_msg) - buflen, 0)) < 0) {
//                    printf("Error recv peer message\n");
//                    break;
//                } else {
//                    buflen = buflen + len;
//                }
//            }
//            memcpy(message, buffer, sizeof(peer_msg));
//            
//            /* Connect to the private file transport socket */
//            int upload_socket = create_client_socket_byIp(message->recv_peer_ip, message->recv_peer_port);
//            /* Try to open the file */
//            FILE *fp = fopen(message->filename, "r");
//            
//            if (fp == NULL)  {
//                printf("File:\t%s Not Found!\n", message->filename);
//                free(message);
//                break;
//            } else {
//                /* Send the file length */
//                fseek(fp,0,SEEK_END);
//                int fileLen = ftell(fp);
//                fseek(fp,0,SEEK_SET);
//                send(upload_socket, &fileLen, sizeof(int), 0);
//                /* Send the file data */
//                char buffer[BUFFER_SIZE];
//                bzero(buffer, BUFFER_SIZE);
//                int file_block_length = 0;
//                while( (file_block_length = fread(buffer, sizeof(char), BUFFER_SIZE, fp)) > 0) {
//                    // printf("file_block_length = %d\n", file_block_length);
//                    if (send(upload_socket, buffer, file_block_length, 0) < 0) {
//                        printf("Send File:\t%s Failed!\n", message->filename);
//                        break;
//                    }
//                    bzero(buffer, sizeof(buffer));
//                }
//                fclose(fp);
//            }
//            free(message);
//        } else {
//            printf("Fail accept in peer_handler\n");
//            break;
//        }
//    }
//    printf("peer_handler_thread exit\n");
//    pthread_exit(NULL);
//}

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
    /* variables */
    peer_info_t *peer_info = NULL;
    int fileLen = f_node->size, idx_last_slash = -1, len_file_name, i, start_offset, next_offset, part_length, num_tmpt_file, peer_available_idx = -1, write_length, mark;
    char curr_dir[256], relative_dir[256], folder_path[256], command[256], tempfilename[256], buffer[BUFFER_SIZE];
    FILE *fp_tmpt = NULL, *fp = NULL;
    
    
    if (f_node->num_peers == 0) {
        printf("download_file: No peer alive\n");
        return;
    }
    
    bzero(peer_flag, sizeof(int) * 200);
    
    /* Check if the fold exist, and get the path prefix of the file, and file name */
    bzero(curr_dir, 256);
    bzero(relative_dir, 256);
    bzero(folder_path, sizeof(folder_path));
    len_file_name = (int)strlen(f_node->name);
    for ( i = 0; i < len_file_name; i++) {
        folder_path[i] = f_node->name[i];
        if (folder_path[i] == '/') {
            if (opendir(folder_path) == NULL) {
                bzero(command, 256);
                sprintf(command, "mkdir %s", folder_path);
                system(command);
            }
            idx_last_slash = i;
        }
    }
    memcpy(curr_dir, f_node->name, idx_last_slash);
    memcpy(relative_dir, f_node->name + idx_last_slash + 1, strlen(f_node->name)-idx_last_slash-1);
    
    /* save the result of finding corresponding temporary files */
    bzero(command, 256);
    sprintf(command, "find ./%s -type f -name \"%s__*__%u~\" > tmpt_file~", curr_dir, relative_dir, f_node->timestamp);
    system(command);
    //printf("command is: %s\n", command);
    
    /* if there are no temporary files locally, we start downloading directly */
    if (get_file_size("tmpt_file~") <= 0) {
        
        printf("no temporary files, start new donwloading %s...\n", f_node->name);
        
        /* Create download thread */
        num_tmpt_file = f_node->num_peers;
        for (i = 0; i < f_node->num_peers; i++) {
            // create a struct peer_info_t to pass into the download_handler()
            peer_info = malloc(sizeof(peer_info_t));
            bzero(peer_info, sizeof(peer_info_t));
            peer_info->sockfd = create_client_socket_byIp(f_node->peers[i], PEER_PORT);
            //peer_info->sockfd = 1;
            
            // calculate the piece start indx and calculate the piece length
            start_offset = (int) ((long) fileLen * (long)i / (long) f_node->num_peers);
            next_offset = (int) ((long) fileLen * (long)(i + 1) / (long) f_node->num_peers);
            part_length = next_offset - start_offset;
            peer_info->piece_start_idx  = start_offset;
            peer_info->piece_len        = part_length;
            peer_info->file_time_stamp  = f_node->timestamp;
            peer_info->idx_of_this_peer = i;
            sprintf(peer_info->file_name, "%s", f_node->name);
            peer_flag[i] = 1;
            pthread_t thread;
            pthread_create(&thread, NULL, download_handler, (void *)peer_info);
        }
    }
    /* if there are temporary files, which means we resume from partial downloading */
    else {
        printf("temporary files exist, resume downloading %s...\n", f_node->name);
        num_tmpt_file = get_file_line_num("tmpt_file~");
        bzero(command, 256);
        sprintf(command, "rm -rf tmpt_file~");
        system(command);
    }
    
    while (1) {
        //print_peer_flag(f_node->num_peers);
        if (is_all_zero(f_node->num_peers)) {
            //printf("inside if-clause of is_all_zero()...\n");
            //printf("num_tmpt_file is %d\n", num_tmpt_file);
            mark = 0;
            for (i = 0; i < num_tmpt_file; i++) {
                bzero(tempfilename, 256);
                //sprintf(tempfilename, "%s_%dtemp", f_node->name, i);
                start_offset = (int) ((long) fileLen * (long)i / (long) num_tmpt_file);
                next_offset = (int) ((long) fileLen * (long)(i + 1) / (long) num_tmpt_file);
                part_length = next_offset - start_offset;
                sprintf(tempfilename, "%s__%d__%d__%u~", f_node->name, start_offset, part_length, f_node->timestamp);
                if (get_file_size(tempfilename) < part_length) {
                    //printf("looping for available peer...\n");
                    mark = 1;
                    while ((peer_available_idx = get_available_peer_idx(f_node->num_peers)) == -1) {
                        sleep(1);
                    }
                    peer_flag[peer_available_idx] = 1;
                    printf("resuming download %s\n", tempfilename);
                    peer_info = malloc(sizeof(peer_info_t));
                    peer_info = parse_tmpt_file_name(tempfilename);
                    peer_info->sockfd = create_client_socket_byIp(f_node->peers[peer_available_idx], PEER_PORT);
                    peer_info->idx_of_this_peer = peer_available_idx;
                    pthread_t thread;
                    pthread_create(&thread, NULL, download_handler, (void *)peer_info);
                }
            }
            if (mark == 0) {
                break;
            }
        }
        sleep(1);
    }
    
    
    /* Merge the file */
    fp = fopen(f_node->name, "w");
    if (fp == NULL)  {
        printf("download_file_multi_thread: File:\t%s Can Not Open To Write!\n", f_node->name);
        return;
    }
    for (i = 0; i < num_tmpt_file; i++) {
        bzero(tempfilename, 256);
        //sprintf(tempfilename, "%s_%dtemp", f_node->name, i);
        start_offset = (int) ((long) fileLen * (long)i / (long) num_tmpt_file);
        next_offset = (int) ((long) fileLen * (long)(i + 1) / (long) num_tmpt_file);
        part_length = next_offset - start_offset;
        sprintf(tempfilename, "%s__%d__%d__%u~", f_node->name, start_offset, part_length, f_node->timestamp);
        //printf("i %d, tempfilename %s\n", i, tempfilename);
        /* Open temp file */
        fp_tmpt = fopen(tempfilename, "r");
        if (fp_tmpt == NULL) {
            if (part_length != 0) {
                printf("%s Not found !\n", tempfilename);
            }
            // only when the file is empty, will the following line be executed
            continue;
        }
        /* Copy the data */
        bzero(buffer, sizeof(buffer));
        int buflen;
        while ((buflen = (int)fread(buffer, sizeof(char), BUFFER_SIZE, fp_tmpt)) > 0) {
            write_length = (int)fwrite(buffer, sizeof(char), buflen, fp);
        /*
         * should add bzero() after reading temporary files each time
         */
        
        
        
        }
        fclose(fp_tmpt);
        /* Remove the temp file */
        bzero(command, 256);
        sprintf(command, "rm %s", tempfilename);
        system(command);
    }
    fclose(fp);
    printf("Update file %s with timestamp %u\n", f_node->name, f_node->timestamp);
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
    // variable
    peer_info_t *peer_info = (peer_info_t *)arg;
    int peer_socket = peer_info->sockfd, fileLen;
    peer_msg *msg = NULL;
    FILE *fp = NULL;
    char tempfilename[256];
    
    /* check if the socket is valid */
    if (peer_socket < 0) {
        pthread_detach(pthread_self());
        //printf("Reach 2\n");
        peer_flag[peer_info->idx_of_this_peer] = 2;
        pthread_exit(NULL);
    }
    
    
    /* Create new local file */
    memset(tempfilename, 0, 256);
    sprintf(tempfilename, "%s__%d__%d__%u~", peer_info->file_name, peer_info->piece_start_idx, peer_info->piece_len, peer_info->file_time_stamp);
    
    /* Initialize the peer-to-peer message */
    msg = (peer_msg *)malloc(sizeof(peer_msg));
    bzero(msg, sizeof(peer_msg));
    sprintf(msg->filename, "%s", peer_info->file_name);
    msg->piece_start_idx = peer_info->piece_start_idx + get_file_size(tempfilename);
    msg->piece_len       = peer_info->piece_len - get_file_size(tempfilename);
    fileLen = msg->piece_len;
    
    
    /* print for debug */
    char *ip_addr = NULL;
    ip_addr = get_address_from_ip(peer_socket);
    printf("download %s begin at %d, length %d, from %s\n", peer_info->file_name, msg->piece_start_idx, msg->piece_len, ip_addr);
    free(ip_addr);
    
    
    send(peer_socket, (void *)msg, sizeof(peer_msg), 0);
    
    

    fp = fopen(tempfilename, "a");
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
                if ((len = (int)recv(peer_socket, buffer + buflen, BUFFER_SIZE - buflen, 0)) <= 0) {
                    printf("Error receive file data\n");
                    download_length = fileLen;
                    peer_flag[peer_info->idx_of_this_peer] = 2;
                    return NULL;
                } else {
                    buflen = buflen + len;
                    download_length = download_length + len;
                }
            }
            int write_length = (int)fwrite(buffer, sizeof(char), buflen, fp);
            fflush(fp);
            if (write_length < buflen) {
                printf("recvFile(): File:\t%s Write Failed!\n", peer_info->file_name);
                download_length = fileLen;
                break;
            }
            bzero(buffer, BUFFER_SIZE);
        }
        fclose(fp);
    }
    
    close(peer_socket);
    free(msg);
    //printf("Reach 1\n");
    pthread_detach(pthread_self());
    //printf("Reach 2\n");
    peer_flag[peer_info->idx_of_this_peer] = 0;
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
        int *peer_socket = malloc(sizeof(int));
        if ((*peer_socket = accept(peer_main_socket, NULL, NULL)) > 0) {
            pthread_t upload_thread;
            pthread_create(&upload_thread, NULL, upload_handler, peer_socket);
        }  else {
            printf("Failed to accept in peer_handler_multi_thread\n");
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
    free((int *)arg);
    /* Receive the message from peer */
    peer_msg *message = (peer_msg*)malloc(sizeof(peer_msg));
    bzero(message, sizeof(peer_msg));
    recv(peer_socket, message, sizeof(peer_msg), 0);
    
    //printf("%s: filename: %s, num_peers: %d\n", __func__, message->filename, message->num_peers);
    // get address from socket
    char *ip_addr = NULL;
    ip_addr = get_address_from_ip(peer_socket);
    printf("upload %s begin at %d, length %d, to %s\n", message->filename, message->piece_start_idx, message->piece_len, ip_addr);
    free(ip_addr);
    
    char buffer[BUFFER_SIZE];
    
    /* Try to open the file */
    FILE *fp = fopen(message->filename, "r");
    if (fp == NULL)  {  
        printf("File:\t%s Not Found!\n", message->filename);
    } else {
        int start_offset = message->piece_start_idx;
        int part_length = message->piece_len;
        fseek(fp, start_offset, SEEK_SET);
        int file_block_length;
        int length_sent = 0;
        while (length_sent < part_length) {
            file_block_length = (part_length - length_sent) >= BUFFER_SIZE ? BUFFER_SIZE : part_length - length_sent;
            bzero(buffer, BUFFER_SIZE);  
            fread(buffer, sizeof(char), file_block_length, fp);
            if (send(peer_socket, buffer, file_block_length, 0) < 0) {
                printf("Send File:\t%s Failed!\n", message->filename);  
                break;  
            }  else {
                length_sent = length_sent + file_block_length;
            }
            //printf("sleep 1s before sending another block...\n");
            //sleep(1);
            //printf("sleep is over!\n");
            
        }
        fclose(fp);
    }
    free(message);
    pthread_detach(pthread_self());
    pthread_exit(NULL);
}

/*
 * This function parse the temporary file names, and return a peer_info_t struct
 * The valid fields in the struct is file_name, piece_start_idx, piece_len, and file_time_stamp
 * The temporary file name is of such format: <filename>__<startidx>__<expectedFileLength>__<timestamp>__~
 */
peer_info_t *parse_tmpt_file_name(char *tmpt_file_name) {
    peer_info_t *return_t = malloc(sizeof(peer_info_t));
    bzero(return_t, sizeof(peer_info_t));
    char buf[20], *pch_last = tmpt_file_name, *pch;
    
    // get file_name
    pch = strstr(tmpt_file_name, "__");
    memcpy(return_t->file_name, pch_last, pch - pch_last);
    pch = pch+2;
    pch_last = pch;
    
    // get startidx
    bzero(buf, 20);
    pch = strstr(pch, "__");
    memcpy(buf, pch_last, pch - pch_last);
    return_t->piece_start_idx = atoi(buf);
    pch = pch + 2;
    pch_last = pch;
    
    // get expectedFileLength
    bzero(buf, 20);
    pch = strstr(pch_last, "__");
    memcpy(buf, pch_last, pch - pch_last);
    return_t->piece_len = atoi(buf);
    pch = pch + 2;
    pch_last = pch;
    
    // get timestamp
    bzero(buf, 20);
    pch = strstr(pch_last, "~");
    memcpy(buf, pch_last, pch - pch_last);
    /*
     * change long to unsigned int for use in raspberry pi
     */
    return_t->file_time_stamp = (unsigned int)atol(buf);
    pch = pch + 2;
    pch_last = pch;
    
    return return_t;
}


/*
 * This function gets the available peer index
 */
int get_available_peer_idx(int peer_num) {
    int i;
    for (i = 0; i < peer_num; i++) {
        if (peer_flag[i] == 0) {
            return i;
        }
    }
    return -1;
}


/*
 * This function checks if all the entries in peer_flag[] is zero
 */
int is_all_zero(int peer_num) {
    int i;
    for (i = 0; i < peer_num; i++) {
        if (peer_flag[i] == 1) {
            return 0;
        }
    }
    return 1;
}


/*
 * This function prints the peer_flag table
 */
void print_peer_flag(int peer_num) {
    printf("peer_flag: ");
    int i;
    for (i = 0; i < peer_num; i++) {
        printf("%d ", peer_flag[i]);
    }
    printf("\n");
    
}
