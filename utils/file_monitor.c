#include "file_monitor.h"
#include "network.h"
#include "tracker_peer_table.h"
#include <unistd.h>
#include "peer2peer.h"

char root_directory[128];
char *file_type_string[] = {"Folder", "File"};
file_node* file_table;
time_t last_table_update_time = 0;
int update_enable;

void block_update() {
    update_enable = 0;
}

void unblock_update() {
    update_enable = 1;
}

void watchDirectory(char* directory) {
    sprintf(root_directory, "%s", directory);
}

void file_table_initial() {
    file_table = (file_node*)malloc(sizeof(file_node));
    bzero(file_table, sizeof(file_node));
    /*
     * change long to unsigned int for use in raspberry pi
     */
    file_table->peers[0] = (unsigned int)get_My_IP();
}
file_node* get_my_file_table() {
    return file_table;
}
void file_table_free(file_node* file_node_head) {
    file_node* curr = file_node_head;
    while (curr != NULL) {
        file_node* temp = curr;
        curr = curr->next;
        free(temp);
    }
}
int file_table_update() {
    if (update_enable) {
        file_table_free(file_table);
        file_table_initial();
        file_node* runner = file_table;
        
        //modified
        /*
         * change long to unsigned int for use in raspberry pi
         */
        time_t tmpt_time_long;
        time(&tmpt_time_long);
        
        unsigned int tmpt_time;
        tmpt_time = (unsigned int)tmpt_time_long;
        
        
        int is_updated = file_table_update_helper(root_directory, &runner);
        //time(&last_table_update_time);
        last_table_update_time = tmpt_time;
        return is_updated;
    } else {
        return 0;
    }
}
void file_table_print() {
    printf("==================Local File Table======================\n");
    file_node* runner = file_table;
    while (runner->next != NULL) {
        printf("%s\t", runner->next->name);
        int len = strlen(runner->next->name);
        if (len < 8)
            printf("\t");
        printf("%s\t", file_type_string[(runner->next->type)/4-1]);
        printf("%u\t", runner->next->timestamp);
        int i;
        for (i = 0; i < runner->next->num_peers; i++) {
            printf("%s\t", inet_ntoa(*(struct in_addr*)&runner->next->peers[i]));
        }
        printf("\n");
        runner = runner->next;
    }
    printf("========================================================\n");
}
int file_table_update_helper(char* directory, file_node** last) {
    int is_updated = 0;
    struct stat attrib;
    stat(directory, &attrib);
    if (last_table_update_time <= attrib.st_mtime) {
        is_updated = 1;
    }
    DIR * dir_ptr;
    struct dirent * direntp;
    if((dir_ptr = opendir(directory))==NULL) {
        printf("Cannot open %s\n",directory);
    } else {
        while ((direntp = readdir(dir_ptr))!=NULL) {
            if (direntp->d_name[0] != '.' && direntp->d_name[strlen(direntp->d_name)-1] != '~') {//Hidden file not display
                file_node* new_node = (file_node*)malloc(sizeof(file_node));
                bzero(new_node, sizeof(file_node)); 
                (*last)->next = new_node;
                (*last) = new_node;
                sprintf(new_node->name, "%s/%s", directory, direntp->d_name);
                stat(new_node->name, &attrib);
                new_node->timestamp = attrib.st_mtime;
                new_node->num_peers = 1;
                new_node->peers[0] = file_table->peers[0];
                new_node->next = NULL;
                if (direntp->d_type == FILE_TYPE) {
                    new_node->size = get_file_size(new_node->name); 
                    new_node->type = FILE_TYPE;                   
                    if (last_table_update_time <= attrib.st_mtime) {
                        is_updated = 1;
                    }
                } else if (direntp->d_type == FOLDER_TYPE) {
                    new_node->size = 0;
                    new_node->type = FOLDER_TYPE;
                    char sub_directory[256];
                    bzero(sub_directory, 256);
                    sprintf(sub_directory, "%s/%s", directory, direntp->d_name);
                    is_updated = file_table_update_helper(sub_directory, last) || is_updated;
                } else {
                    printf("file_table_initial_helper : Unknow file type\n");
                }
            }
        }
        closedir(dir_ptr);
    }
    return is_updated;
}

void send_file_table(int socket) {
    int num_nodes = 1;
    file_node* runner = file_table;
    while (runner->next != NULL) {
        runner = runner->next;
        num_nodes++;
    }
    if (send(socket, &num_nodes, sizeof(int), 0) < 0) {
        printf("Error in send_file_table\n");
    }
    runner = file_table;
    while (runner != NULL) {
        printf("in %s, sizeof(file_node) is %lu\n", __func__, sizeof(file_node));
        if (send(socket, runner, sizeof(file_node), 0) < 0) {
            printf("Error in send_file_table\n");
        }
        runner = runner->next;
    }
}

void recv_file_table(int socket, file_node** new_table) {
    int num_nodes;
    if (recv(socket, &num_nodes, sizeof(int), 0) <= 0) {
        printf("Error in recv_file_table\n");
        *new_table = NULL;
        return;
    }
    // printf("Got %d file nodes\n", num_nodes);
    file_node* runner = *new_table;
    
    char *buffer = (char*)malloc(sizeof(file_node));
    int len, buflen;
    printf("===============Receive remote table===================\n");
    while (num_nodes > 0) {
        buflen = 0;
        bzero(buffer, sizeof(file_node));
        if (runner == NULL) {
            printf("in %s, sizeof(file_node) is %lu\n", __func__, sizeof(file_node));
            runner = (file_node*)malloc(sizeof(file_node));
            while (buflen < sizeof(file_node)) {
                if ((len = recv(socket, buffer + buflen, sizeof(file_node) - buflen, 0)) < 0) {
                    printf("Error in recv_file_table\n");
                    *new_table = NULL;
                    return;
                } else {
                    buflen = buflen + len;
                    // printf("receive len %d, total length %d\n", len, buflen);
                }
            }
            memcpy(runner, buffer, buflen);
            runner->next = NULL;
            /*
             * change long to unsigned int for use in raspberry pi
             */
            runner->peers[0] = (unsigned int)get_peer_IP(socket);
            *new_table = runner;
            // printf("From %s\n", inet_ntoa(*(struct in_addr*)&(*new_table)->peers[0]));
        } else {
            file_node* new_node = (file_node*)malloc(sizeof(file_node));
            while (buflen < sizeof(file_node)) {
                if ((len = recv(socket, buffer + buflen, sizeof(file_node) - buflen, 0)) < 0) {
                    printf("Error in recv_file_table\n");
                    *new_table = NULL;
                    return;
                } else {
                    buflen = buflen + len;
                    // printf("receive len %d, total length %d\n", len, buflen);
                }
            }
            memcpy(new_node, buffer, buflen);
            new_node->next = NULL;
            runner->next = new_node;
            runner = new_node;
            /* Print the file node entry */
            printf("%s\t", runner->name);
            int len = strlen(runner->name);
            if (len < 8)
                printf("\t");
            printf("%s\t", file_type_string[(runner->type)/4-1]);
            printf("%u\t", runner->timestamp);
            int i;
            for (i = 0; i < runner->num_peers; i++) {
                printf("%s\t", inet_ntoa(*(struct in_addr*)&runner->peers[i]));
            }
            printf("\n");
            /* ---------------------------- */
        }
        num_nodes--;
    }
    if (*new_table != NULL) {
        printf("===============From %s====================\n", inet_ntoa(*(struct in_addr*)&(*new_table)->peers[0]));
    }
    free(buffer);
}

void sync_with_server(file_node* server_table) {
    update_enable = 0;
    file_node* runner_client = file_table;
    char command[256], file_name[256];
    int is_updated = 0;
    FILE *f;
    
    while (runner_client->next != NULL) {
        file_node* runner_server = server_table;
        int is_exist = 0;
        while (runner_server->next != NULL) {
            if (strcmp(runner_client->next->name, runner_server->next->name) == 0) {
                is_exist = 1;
                break;
            }
            runner_server = runner_server->next;
        }
        if (is_exist == 0) {
            printf("%s should be deleted\n", runner_client->next->name);
            is_updated = 1;
            /* Delete the local file or local folder*/
            safe_delete(runner_client->next->name);
        }
        runner_client = runner_client->next;
    }
    
    file_node* runner_server = server_table;
    while (runner_server->next != NULL) {
        runner_client = file_table;
        int is_exist = 0;
        while (runner_client->next != NULL) {
            if (strcmp(runner_client->next->name, runner_server->next->name) == 0) {
                is_exist = 1;
                break;
            }
            runner_client = runner_client->next;
        }
        if (is_exist == 0) {
            printf("%s should be added\n", runner_server->next->name);
            is_updated = 1;
            if (runner_server->next->type == FILE_TYPE) {
                /* Download the file from peer */
                download_file_multi_thread(runner_server->next);
            } else {
                /* Add the folder if needed */
                safe_add_folder(runner_server->next->name);
            }
        } else {
            if (runner_client->next->timestamp < runner_server->next->timestamp) {                
                /* If both the file_node are folder, we won't update it */
                if (runner_client->next->type != FOLDER_TYPE || runner_server->next->type != FOLDER_TYPE) {
                    printf("%s should be updated\n", runner_server->next->name);
                    is_updated = 1;
                    safe_delete(runner_client->next->name);
                    if (runner_server->next->type == FOLDER_TYPE) {
                        safe_add_folder(runner_server->next->name);
                    } else {
                        /* Download the file from peer */
                        download_file_multi_thread(runner_server->next);
                    }
                }                
            }
        }
        runner_server = runner_server->next;
    }
    
    /*
     *  Add code to delete useless temporary files
     */
    bzero(command, 256);
    sprintf(command, "find . -type f -name \"*~\" > file_to_be_deleted~");
    system(command);
    f = fopen("file_to_be_deleted~", "r");
    bzero(file_name, 256);
    while (fgets(file_name, 256, f)) {
        file_name[strlen(file_name)-1] = '\0';
        bzero(command, 256);
        sprintf(command, "rm -rf %s", file_name);
        system(command);
        bzero(file_name, 256);
    }
    bzero(command, 256);
    sprintf(command, "rm -rf file_to_be_deleted~");
    system(command);
    
    /* === If updated, refresh the local file table === */
    if (is_updated) {
        file_table_free(file_table);
        file_table_initial();
        file_node* runner = file_table;
        is_updated = file_table_update_helper(root_directory, &runner);
        last_table_update_time = 0;
    }
    /* === Make sure the file_table_update() function will detect the update === */
    update_enable = 1;
}

/* Get new file infomation from client file table, and update the local server table. Called by the tracker */
void sync_from_client(file_node* client_table) {
    // printf(v"file table at present::::::::::::::::\n");
    // file_table_print();
    unsigned long client_IP = client_table->peers[0];
    file_node* runner_server = file_table;
    /* Looking for file in server_table, but not in client_table */
    while (runner_server != NULL && runner_server->next != NULL) {
        file_node* runner_client = client_table;
        int is_exist = 0;
        while (runner_client->next != NULL) {
            if (strcmp(runner_server->next->name, runner_client->next->name) == 0) {
                is_exist = 1;
                break;
            }
            runner_client = runner_client->next;
        }
        if (is_exist == 0) {
            int i;
            int peer_exist = 0;
            for (i = 0; i < runner_server->next->num_peers; i++) {
                // printf("%s\t", inet_ntoa(*(struct in_addr*)&runner_server->next->peers[i]));
                // printf("%s\n", inet_ntoa(*(struct in_addr*)&client_IP));//For temp test
                if (runner_server->next->peers[i] == client_IP) {
                    peer_exist = 1;
                    break;
                }
            }
            if (peer_exist) {
                /* If peer's ip exists in the file node, we delete the file node from server table */
                printf("server should delete %s\n", runner_server->next->name);
                /* Delete the file node from server table */
                file_node *temp = runner_server->next;
                runner_server->next = temp->next;
                free(temp);
                continue;
            } else {
                /* If peer's ip doesn't exist in the file node, we ask client to add this file */
                printf("client should add %s\n", runner_server->next->name);
            }
        }
        runner_server = runner_server->next;
    }
    // printf("file table in the middle::::::::::::::::\n");
    // file_table_print();
    file_node* runner_client = client_table;
    while (runner_client->next != NULL) {
        // printf("reach .. %s\n", runner_client->next->name);
        // file_table_print();
        runner_server = file_table;
        int is_exist = 0;
        while (runner_server->next != NULL) {
            if (strcmp(runner_client->next->name, runner_server->next->name) == 0) {
                is_exist = 1;
                break;
            }
            runner_server = runner_server->next;
        }
        if (is_exist == 0) {
            /* If file exists in client table, but not in server table */
            printf("server should add %s\n", runner_client->next->name);
            // /* Add the file node to server table */
            file_node *temp = (file_node*)malloc(sizeof(file_node));
            bzero(temp, sizeof(file_node));
            temp->size = runner_client->next->size;
            temp->type = runner_client->next->type;
            sprintf(temp->name, "%s", runner_client->next->name);
            temp->timestamp = runner_client->next->timestamp;
            temp->num_peers = runner_client->next->num_peers;
            temp->peers[0] = client_IP;
            /* Add to the server table list */
            temp->next = file_table->next;
            file_table->next = temp;
        } else {			
            if ((runner_client->next->timestamp < runner_server->next->timestamp) && 
                (runner_client->next->type != FOLDER_TYPE || runner_server->next->type != FOLDER_TYPE)) {
                printf("client should update %s\n", runner_client->next->name);
            } else if ((runner_client->next->timestamp > runner_server->next->timestamp) && 
                          (runner_client->next->type != FOLDER_TYPE || runner_server->next->type != FOLDER_TYPE)) {
                    printf("server should update %s\n", runner_client->next->name);
                    runner_server->next->size = runner_client->next->size;
                    runner_server->next->type = runner_client->next->type;
                    runner_server->next->timestamp = runner_client->next->timestamp;
                    runner_server->next->num_peers = runner_client->next->num_peers;
                    runner_server->next->peers[0] = client_IP;                
            } else { 
                /* Update the peers in the file node */
                int i;
                int peer_exist = 0;
                for (i = 0; i < runner_server->next->num_peers; i++) {
                    if (runner_server->next->peers[i] == client_IP) {
                        peer_exist = 1;
                        break;
                    }
                }
                if (peer_exist == 0) {
                    runner_server->next->peers[runner_server->next->num_peers] = client_IP;
                    runner_server->next->num_peers++;
                }
            }
        }
        runner_client = runner_client->next;
    }
}

/* Used by server to delete disconnected peer from file table */
void delete_disconn_peer(unsigned long client_IP) {
    file_node* runner_server = file_table;
    while (runner_server->next != NULL) {
        int i;
        int peer_exist = 0;
        for (i = 0; i < runner_server->next->num_peers; i++) {
            if (runner_server->next->peers[i] == client_IP) {
                peer_exist = 1;
                break;
            }
        }
        if (peer_exist) {
            runner_server->next->num_peers--;
            runner_server->next->peers[i] = runner_server->next->peers[runner_server->next->num_peers];
        }
        runner_server = runner_server->next;
    }
}

/* Used by server to broad the updated file table to all the peers */
void broadcast_file_table() { 
    tracker_peer_t *runner = get_peer_table();
    while (runner->next != NULL) {
        printf("Broadcast to %s\n", inet_ntoa(*(struct in_addr*)&runner->next->ip));
        send_file_table(runner->next->sockfd);
        runner = runner->next;
    }
}

/* get file length */
int get_file_size(char *file_name) {
    FILE *f = fopen(file_name, "r");
    int size = 0;
    if (f == NULL) {
        //printf("open file failed!\n");
        return 0;
    }
    
    fseek(f, 0, SEEK_END);
    size = (int)ftell(f);
    fclose(f);
    return size;
}

/* get the number of lines in the file */
int get_file_line_num(char *file_name) {
    FILE *f = fopen(file_name, "r");
    char tmpt_file_name[256];
    int num_line = 0;
    if (f == NULL) {
        printf("%s open file failed!\n", __func__);
        return -1;
    }
    while (fgets(tmpt_file_name, 256, f)) {
        num_line++;
    }
    fclose(f);
    return num_line;
}
/*
 * Delete file or folder only when exists 
 */
void safe_delete(char* file_name) {
    if ((access(file_name, F_OK)) != -1)  {
        // printf("File %s exist.\n", file_name);
        char command[256];
        bzero(command, 256);
        sprintf(command, "rm -rf %s", file_name);
        system(command);
    } else {  
        // printf("File %s not exist!\n", file_name);
    }
}
/*
 * Create a folder when not exist. If the parent directory does not exist, create the parent directory first
 */
void safe_add_folder(char* folder_name) {
    char folder_path[256], command[256];
    bzero(folder_path, sizeof(folder_path));
    int len_folder_name = (int)strlen(folder_name);
    int i;
    for ( i = 0; i < len_folder_name; i++) {
        folder_path[i] = folder_name[i];
        if (folder_path[i] == '/') {
            if (opendir(folder_path) == NULL) {
                bzero(command, 256);
                sprintf(command, "mkdir %s", folder_path);
                system(command);
            }
        }
    }
    if (opendir(folder_name) == NULL) {
        bzero(command, 256);
        sprintf(command, "mkdir %s", folder_name);
        system(command);
    }
}
