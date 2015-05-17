#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>

// #define SERVER_HOST_NAME "hw87244557-ThinkPad-Edge-E545"
#define SERVER_HOST_NAME "tahoe.cs.dartmouth.edu"
#define LENGTH_OF_LISTEN_QUEUE     20 
#define SERVER_PORT 7371
/* State */
#define SIGNAL_HEARTBEAT 1
#define SIGNAL_FILE_UPDATE 2
/* Interval */
#define HEARTBEAT_INTERVAL_SEC 10
#define FILE_UPDATE_INTERVAL_SEC 9
unsigned long get_My_IP();