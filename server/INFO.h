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

struct tank_info{
    char name[30];
    int side, id;   // (side,id) is a bijection to each player
    int state;
    int connected;
    int score;
};

struct trgt_info{   // (5,1,2) = RXXBB
    int total, red, blue;
};

struct state_info{
    int state;
    int time_last;
};

struct popup_info{
    char *msg;
};

struct log_info{
    char *msg;
};

struct ui_info{
    struct tank_info tank[2];
    struct trgt_info target;
    struct state_info state;
    struct popup_info popup;
    struct log_info log;
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
    int health;

}DEVICE;


SLIST_HEAD(slisthead, entry)\
    trg_head = SLIST_HEAD_INITIALIZER(trg_head),\
    tnk_head = SLIST_HEAD_INITIALIZER(tnk_head);
struct slisthead *headp;
struct entry {
    SLIST_ENTRY(entry) entries;
    struct device *ptr;
}*np;
