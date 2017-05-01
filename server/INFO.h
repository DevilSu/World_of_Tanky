#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <wait.h>
#include <sys/select.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

// maximum number of devices
#define NUM_OF_DEV 20

// ID
#define TANK 2
#define TRGT 3

// int devnum=-1;

// server action
#define SER_ST0P 2 // tell tank ot target to stop
#define SER_MOVE_Y 0 // tell tank to move
#define SER_MOVE_N 1 // tell tank to move
#define SER_SHOT_Y 2 // tell tank to turn on lazer
#define SER_SHOT_N 3 // tell tank to turn on lazer

#define SER_SCAN_Y 4 // tell target to scan
#define SER_SCAN_N 5 // tell target to scan

// tank action
#define TNK_MOVE_C 2 // tell server move complete
#define TNK_PING 3


typedef struct device
{
	// basic info
    int fd;
    int id;
    struct in_addr ip;
    unsigned short int port;

    // device spec info
    // ?????
    int health;

}DEVICE;

// DEVICE dev[NUM_OF_DEV];
