#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <memory.h>
#include <pthread.h>
#include "../utils/peer2peer.h"
#include "../utils/network.h"
/* Run on gile */
int main(int argc, char const *argv[])
{
	/* code */
	file_node *f_node = (file_node*)malloc(sizeof(file_node));
	bzero(f_node, sizeof(file_node));
	sprintf(f_node->name, "asd/pic.jpg");
	f_node->num_peers = 3;
	f_node->peers[0] = 550873729;// ip of bear.cs.dartmouth.edu
	f_node->peers[1] = 3587615361;//ip of spruce.cs.dartmouth.edu
    f_node->peers[2] = 2698422913; // ip of wildcat.cs.dartmouth.edu
    f_node->size = 36934;
	download_file_multi_thread(f_node);

	// sprintf(f_node->name, "asd/ds");
	// download_file(f_node);
	return 0;
}