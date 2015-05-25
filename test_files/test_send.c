#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <memory.h>
#include <pthread.h>
#include <signal.h>
#include "../utils/peer2peer.h"
#include "../utils/network.h"
/* Run on bear */
int main(int argc, char const *argv[])
{
    signal(SIGPIPE, SIG_IGN);
	/* code */
	int peer_main_socket = create_server_socket(PEER_PORT);
	pthread_t thread;
	pthread_create(&thread, NULL, peer_handler_multi_thread,&peer_main_socket);

	pthread_join(thread,NULL);
	return 0;
}