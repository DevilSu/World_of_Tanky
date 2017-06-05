#ifndef INFO_H
#define INFO_H
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

#include <sys/queue.h>

// maximum number of devices
#define NUM_OF_DEV 20

// ID
#define TANK 2
#define TRGT 3

// int devnum=-1;

// server action
#define SER_ST0P 2 // tell tank ot target to stop
#define SER_MOVE_BGN 0 // tell tank to move
#define SER_MOVE_END 1 // tell tank to move
#define SER_SHOT_BGN 2 // tell tank to turn on lazer
#define SER_SHOT_END 3 // tell tank to turn on lazer

#define SER_SCAN_BGN 4 // tell target to scan
#define SER_SCAN_END 5 // tell target to scan

// tank action
#define TNK_MOVE_CPL 2 // tell server move complete
#define TNK_PING 3

// State
#define STATE_NOTHIN 0
#define	STATE_MOVING 1
#define	STATE_ENDING 2
#define STATE_TRGTON 3
#define	STATE_SCNLSR 4
#define STATE_TRGTOF 5

#define TIME_NOTHIN 5
#define TIME_MOVING 26
#define TIME_ENDING 3
#define TIME_TRGTON 3
#define TIME_SCNLSR 5
#define TIME_TRGTOF 3

struct sStatus{
    int *state;
    int *cur_team;
    time_t *round_starting_time;
};

typedef struct device
{
	// basic info
    int fd;
    int id;
    struct in_addr ip;
    unsigned short int port;

    // device spec info
    // ?????
    int state;
    int new_comer;
    int health[2];
    char *name, *stat;
    int flag[2];

}DEVICE;

struct ui_info_node{
    DEVICE *dev;
    char valid;
};

#endif