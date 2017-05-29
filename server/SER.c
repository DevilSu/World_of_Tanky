//gcc -o SER SER.c INFO.h myGTK.h -Wall `pkg-config --cflags --libs gtk+-3.0` -export-dynamic
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
#include <arpa/inet.h>

#include <gtk/gtk.h>

#include "INFO.h"
#include "myGTK.h"

#define SERV_PORT 5000
#define LISTENQ 1024
#define MAXLINE 100

DEVICE *tnk_list, *trg_list;

static void *gtk_thread(void *arg);

char gbl_game_start = 0;
char gbl_state[30] = "Game start";
char gbl_player[2][1][30];
char gbl_player_status[2][1][30];
char gbl_target[7][30];
char gbl_target_status[7][30];
int gbl_state_time;
int gbl_player_num, gbl_player_info;
int gbl_target_num, gbl_target_info;

int main(int argc, char **argv)
{
	// set up GTK
	pthread_t tid;
	gtk_init(&argc, &argv);
	pthread_create(&tid, NULL, gtk_thread, NULL);

	// set up TCP server
	int state;
	char state_str;
	int lisfd, confd, sockfd;
	int flag=1;
	int len=sizeof(int);
	socklen_t clilen;
	struct sockaddr_in servaddr, cliaddr;
	struct timeval select_tout;
	char buf[MAXLINE], msg[MAXLINE];
	time_t round_starting_time, cur_time;

	strcpy(msg,"Server say hello\n");
	printf("msg = %s\n", msg);

	lisfd=socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(lisfd, SOL_SOCKET, SO_REUSEADDR, &flag, len);

	bzero(&servaddr, sizeof(servaddr));

	servaddr.sin_family=AF_INET;
	servaddr.sin_port=htons(SERV_PORT);
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);

	bind(lisfd, (struct sockaddr*) &servaddr, sizeof(servaddr));
	listen(lisfd, LISTENQ);

	// set up select function
	int i, j, maxi, maxfd;
	int nready;
	ssize_t n;
	fd_set rset, allset;

	maxfd=lisfd;
	maxi=-1;
	FD_ZERO(&allset);
	FD_SET(lisfd, &allset);

	// set up device
	DEVICE dev[NUM_OF_DEV];
	struct entry *dev_ptr;
    SLIST_INIT(&tnk_head);
    SLIST_INIT(&trg_head);
	for(i=0; i<NUM_OF_DEV; i++){
		dev[i].fd=-1;
		dev[i].new_comer = 0;
	}

	for( i=0; i<2; i++){
		for( j=0; j<1; j++ ){
			strcpy(gbl_player[i][j], "a");
			strcpy(gbl_player_status[i][j], "a");
		}
	}
	for( i=0; i<7; i++ ){
		strcpy(gbl_target[i], "a");
		strcpy(gbl_target_status[i], "a");
	}
	round_starting_time = time(NULL);
	for( state = STATE_NOTHIN;;)
	{
		// State updater
		cur_time = time(NULL);
		if(!gbl_game_start){
			round_starting_time = cur_time;
		}
		switch(state){
			case STATE_NOTHIN:
				if( cur_time - round_starting_time > 5 ){
					state = STATE_MOVING;
					round_starting_time = time(NULL);
					printf("\nstate change to STATE_MOVING\n");
					strcpy(gbl_state,"STATE_MOVING");
				}
				gbl_state_time = round_starting_time - cur_time + 5;
				break;
			case STATE_MOVING:
				if( cur_time - round_starting_time > 26 ){
					state = STATE_ENDING;
					round_starting_time = time(NULL);
					printf("\nstate change to STATE_ENDING\n");
					strcpy(gbl_state,"STATE_ENDING");
					strcpy(gbl_player_status[1][0],"Idle");
				}
				gbl_state_time = round_starting_time - cur_time + 26;
				break;
			case STATE_ENDING:
				if( cur_time - round_starting_time > 5 ){
					state = STATE_TRGTON;
					round_starting_time = time(NULL);
					printf("\nstate change to STATE_TRGTON\n");
					strcpy(gbl_state,"STATE_TRGTON");
					strcpy(gbl_target_status[2],"Scanning");
				}
				gbl_state_time = round_starting_time - cur_time + 5;
				break;
			case STATE_TRGTON:
				if( cur_time - round_starting_time > 8 ){
					state = STATE_SCNLSR;
					round_starting_time = time(NULL);
					printf("\nstate change to STATE_SCNLSR\n");
					strcpy(gbl_state,"STATE_SCNLSR");
				}
				gbl_state_time = round_starting_time - cur_time + 8;
				break;
			case STATE_SCNLSR:
				if( cur_time - round_starting_time > 8 ){
					state = STATE_TRGTOF;
					round_starting_time = time(NULL);
					printf("\nstate change to STATE_TRGTOF\n");
					strcpy(gbl_state,"STATE_TRGTOF");
				}
				gbl_state_time = round_starting_time - cur_time + 8;
				break;
			case STATE_TRGTOF:
				if( cur_time - round_starting_time > 8 ){
					state = STATE_NOTHIN;
					round_starting_time = time(NULL);
					printf("\nstate change to STATE_NOTHIN\n");
					strcpy(gbl_state,"STATE_NOTHIN");
					strcpy(gbl_target_status[2],"Idle");
				}
				gbl_state_time = round_starting_time - cur_time + 8;
				break;
		}

		// Client state updating
		state_str = state + '0';
		for(i=0; i<=maxi; i++){

			// Update new-coming client
			if(dev[i].new_comer && dev[i].fd>=0){
				write(dev[i].fd, &state_str, 1);
				printf("Send initial  state %d to dev[%d]\n", state_str-'0', i);
				dev[i].state = state;
				dev[i].new_comer = 0;
			}

			// Update the other client
			else if( dev[i].new_comer==0 && dev[i].state != state ){
				dev[i].state = state;
				switch(dev[i].id){
					case 2:
						if( state==STATE_MOVING || state==STATE_SCNLSR || state==STATE_NOTHIN ){
							write(dev[i].fd, &state_str, 1);
							printf("Send changing state %d(%c) to dev[%d]\n", state_str-'0', state_str, i);
						}
						break;
					case 3:
						if( state==STATE_TRGTON || state==STATE_TRGTOF ){
							write(dev[i].fd, &state_str, 1);
							printf("Send changing state %d to dev[%d]\n", state_str-'0', i);
						}
						break;
				}
			}
		}

		// Read data from clients
		rset=allset;
		select_tout.tv_sec = 1;
		select_tout.tv_usec = 0;
		nready=select(maxfd+1, &rset, NULL, NULL, &select_tout);
		if(FD_ISSET(lisfd, &rset)) // new client connection
		{
			clilen=sizeof(cliaddr);
			confd=accept(lisfd, (struct sockaddr*)&cliaddr, &clilen);

			for(i=0; i<NUM_OF_DEV; i++)
			{
				if(dev[i].fd<0)
				{
					dev[i].fd=confd; // save descriptor
					dev[i].new_comer = 1;
					dev_ptr = (struct entry*) malloc( sizeof(struct entry));
					dev_ptr->ptr = &dev[i];
					break;
				}
			}
			if(i==NUM_OF_DEV)printf("To many clients!\n"); // can't accept client

			// setup device info
			memset(buf, 0, MAXLINE);
			read(dev[i].fd, buf, 2); // ex. "2\n"
			dev[i].id=atoi(buf);
			dev[i].ip=cliaddr.sin_addr;
			dev[i].port=cliaddr.sin_port;

			switch(dev[i].id){
				case TANK:
					SLIST_INSERT_HEAD(&tnk_head, dev_ptr, entries);
					gbl_player_num = 1;
					gbl_player_info = UI_PLAYER_REGISTER;
					break;
				case TRGT:
					SLIST_INSERT_HEAD(&trg_head, dev_ptr, entries);
					gbl_target_num = 1;
					gbl_target_info = UI_TARGET_REGISTER;
					break;
			}

			printf("id=%d, ip=%s, port=%d\n", dev[i].id, inet_ntoa(dev[i].ip), ntohs(dev[i].port));
			printf("Tank list\n");
			SLIST_FOREACH(np, &tnk_head, entries){
				printf("Client = %d\n", np->ptr->fd);
				printf("\t> id = %d\n", np->ptr->id);
				printf("\t> id = %s\n", inet_ntoa(np->ptr->ip));
			}
			printf("Target list\n");
			SLIST_FOREACH(np, &trg_head, entries){
				printf("Client = %d\n", np->ptr->fd);
				printf("\t> id = %d\n", np->ptr->id);
				printf("\t> id = %s\n", inet_ntoa(np->ptr->ip));
			}


			FD_SET(confd, &allset); //add new descriptor to set
			if(confd>maxfd)maxfd=confd; // for select()
			if(i>maxi)maxi=i; // max index in  client[] array
			if(--nready<=0) continue; // no more readable descriptors
		}

		for(i=0; i<=maxi; i++) // check all clients for data
		{
			memset(buf, 0, MAXLINE);
			if((sockfd=dev[i].fd)<0)continue; // skip empty client[]
			if(FD_ISSET(sockfd, &rset))
			{
				if((n=read(sockfd, buf, MAXLINE))==0) //connection closed by client
				{
					printf("connection closed by client %d\n", i);
					close(sockfd);
					FD_CLR(sockfd, &allset);
					dev[i].fd=-1;
					switch(dev[i].id){
						case TANK:
							SLIST_FOREACH(np, &tnk_head, entries){
								if(np->ptr==&dev[i]){
									SLIST_REMOVE(&tnk_head, np, entry, entries);
									break;
								}
							}
							break;
						case TRGT:
							SLIST_FOREACH(np, &trg_head, entries){
								if(np->ptr==&dev[i]){
									SLIST_REMOVE(&trg_head, np, entry, entries);
									break;
								}
							}
							break;
					}

				}
				else
				{
					printf("recv from client %d: ", i);
					fputs(buf, stdout);
					printf("\n");

					// State master
					if(	dev[i].id==4){
						if(n==1) state = (state+1)%6;
						else state = buf[0]-'0';
						printf("state = %d\n", state);
						continue;
					}

					// Data from other clients
					switch(state){
						case STATE_NOTHIN:
							switch(dev[i].id){
								case 3: // Ignore target's ping
									break;
							}
							break;
						case STATE_MOVING:
							switch(dev[i].id){
								case 2:
									// Ignore tank's ping
									printf("INFO: Tank %s\n", buf);
									switch(buf[0]){
										case STATE_MOVING+'0':
											strcpy(gbl_player_status[1][0],"Moving");
											break;
										case 'k':
											strcpy(gbl_player_status[1][0],"Finish");
											break;
										default:
											strcpy(gbl_player_status[1][0],"Error!");
											break;
									}
									break;
								case 3: // Ignore target's ping
									break;
							}
							break;
						case STATE_ENDING:
							switch(dev[i].id){
								case 2:	// Identify tank to stop ONCE
								case 3: // Ignore target's ping
									break;
							}
							break;
						case STATE_TRGTON:
							switch(dev[i].id){
								case 2:	// Ignore tank's ping
								case 3: // Identify target to scan always
									break;
							}
							break;
						case STATE_SCNLSR:
							switch(dev[i].id){
								case 2:	// Identify tank to open laser ONCE
								case 3: // Identify target to scan always 
									break;
							}
							break;
						case STATE_TRGTOF:
							switch(dev[i].id){
								case 3: // Ignore target's ping
									printf("INFO: %s\n", buf);
									switch(atoi(buf)){
										case 1:
											strcpy(gbl_target_status[2],"Hit");
											break;
										case 0:
											strcpy(gbl_target_status[2],"Save");
											break;
										default:
											strcpy(gbl_target_status[2],"Error!");
											break;
									}

									break;
								case 2:	// Identify tank ONCE
									break;
							}
							break;
					}
				}
				if(--nready<=0) break; // no more readable descriptors
			}
		}
	}
	return 0;
}
