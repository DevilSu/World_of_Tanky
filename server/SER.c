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

void *gtk_thread(void *arg);
void gtk_tnk_bcast( char *str );
void gtk_trg_bcast( char *str, int team );
void gtk_tnk_init();
void gtk_trg_init();
void gtk_tnk_update( DEVICE *dev, char *str );
void gtk_trg_update( DEVICE *dev, char *str );
void gtk_tnk_register( DEVICE *dev );
void gtk_trg_register( DEVICE *dev );
void gtk_sco_increment( int team, int value );
void gtk_str_state_update( char *str );
void state_handler( int state, time_t round_starting_time, struct sStatus status );
void state_change( int target, struct sStatus status );
void packt_handler( int state, DEVICE *dev, char *buf, struct sStatus status );

char gbl_game_start = 0, gbl_button_pressed = 0;
char gbl_state[30] = "Game start";
int gbl_score[2];
int gbl_state_time;
int gbl_player_num, gbl_player_info;
int gbl_target_num, gbl_target_info;
struct ui_info_node ui_info_player[2][1], ui_info_target[7];

int main(int argc, char **argv)
{
	// set up GTK
	pthread_t tid;
	gtk_init(&argc, &argv);
	pthread_create(&tid, NULL, gtk_thread, NULL);

	// set up TCP server
	int cur_team = 0;
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

	// Config status
	struct sStatus status;
	status.state = &state;
	status.round_starting_time = &round_starting_time;
	status.cur_team = &cur_team;

	// set up device
	DEVICE dev[NUM_OF_DEV];
	for(i=0; i<NUM_OF_DEV; i++){
		dev[i].fd=-1;
	}

	gtk_tnk_init();
	gtk_trg_init();
	round_starting_time = time(NULL);
	for( state = STATE_NOTHIN;;)
	{
		// State updater
		cur_time = time(NULL);
		if(!gbl_game_start && gbl_button_pressed){
			gbl_game_start = 1;
			gbl_button_pressed = 0;

		}
		if(!gbl_game_start){
			round_starting_time = cur_time;
		}
		state_handler( state, round_starting_time, status );

		// Client state updating
		state_str = state + '0';
		for(i=0; i<=maxi; i++){

			// Update the other client
			if( dev[i].state != state ){
				dev[i].state = state;
				switch(dev[i].id){
					case TANK:
						if( state==STATE_MOVING || state==STATE_SCNLSR || state==STATE_NOTHIN ){
							write(dev[i].fd, &state_str, 1);
							printf("Send changing state %d(%c) to dev[%d]\n", state_str-'0', state_str, i);
						}
						break;
					case TRGT:
						if( state==STATE_TRGTON || state==STATE_TRGTOF ){
							if( dev[i].health>0 ){
								write(dev[i].fd, &state_str, 1);
								printf("Send changing state %d to dev[%d]\n", state_str-'0', i);
							}
							else{
								write(dev[i].fd, &state_str, 1);
								printf("Send changing state %d to dev[%d]\n", state_str-'0', i);
							}
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
					break;
				}
			}
			if(i==NUM_OF_DEV)printf("To many clients!\n"); // can't accept client

			// setup device info
			memset(buf, 0, MAXLINE);
			read(dev[i].fd, buf, 2); // ex. "2\n"
			dev[i].flag[0] = 0;
			dev[i].id=atoi(buf);
			dev[i].ip=cliaddr.sin_addr;
			dev[i].port=cliaddr.sin_port;

			sprintf(buf, "num%d", dev[i].fd);
			dev[i].name = (char*)malloc(30*sizeof(char));
			strcpy(dev[i].name,buf);

			sprintf(buf, "Register");
			dev[i].stat = (char*)malloc(30*sizeof(char));
			strcpy(dev[i].stat,buf);

			switch(dev[i].id){
				case TANK:
					gbl_player_num = 1;
					gbl_player_info = UI_PLAYER_REGISTER;
					gtk_tnk_register( &dev[i] );
					break;
				case TRGT:
					gbl_target_num = 1;
					gbl_target_info = UI_TARGET_REGISTER;
					for( j=0; j<2; j++ ) dev[i].health[j] = 1;
					gtk_trg_register( &dev[i] );
					break;
			}

			printf("id=%d, ip=%s, port=%d\n", dev[i].id, inet_ntoa(dev[i].ip), ntohs(dev[i].port));

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
					packt_handler( state, &dev[i], buf, status );
				}
				if(--nready<=0) break; // no more readable descriptors
			}
		}
	}
	return 0;
}

void packt_handler( int state, DEVICE *dev, char *buf, struct sStatus status ){
	int cur_team = *(status.cur_team);

	// Data from other clients
	printf("Receive a packet: %s from %s\n", buf, dev->name);
	switch(state){
		case STATE_NOTHIN:
			switch(dev->id){
				case TRGT: // Ignore target's ping
					break;
			}
			break;
		case STATE_MOVING:
			switch(dev->id){
				case TANK:
					// Ignore tank's ping
					printf("INFO: Tank %s\n", buf);
					switch(buf[0]){
						case STATE_MOVING+'0':
							gtk_tnk_update( dev, "Moving");
							break;
						case 'k':
							gtk_tnk_update( dev, "Finish");
							break;
						default:
							gtk_tnk_update( dev, "Error!");
							break;
					}
					break;
				case TRGT: // Ignore target's ping
					break;
			}
			break;
		case STATE_ENDING:
			switch(dev->id){
				case TANK:	// Identify tank to stop ONCE
				case TRGT: // Ignore target's ping
					break;
			}
			break;
		case STATE_TRGTON:
			switch(dev->id){
				case TRGT:
					if(buf[0]=='3') dev->flag[0] = 1;
					else{
						// TODO: Echo error
					}
					// Identify target to scan always
					break;
			}
			break;
		case STATE_SCNLSR:
			switch(dev->id){
				case TANK:	// Identify tank to open laser ONCE
				case TRGT: // Identify target to scan always 
					break;
			}
			break;
		case STATE_TRGTOF:
			switch(dev->id){
				case TRGT: // Ignore target's ping
					printf("INFO: %s(%d)\n", buf, atoi(buf));
					if(dev->health[cur_team]<=0){
						gtk_trg_update( dev, "Struggle" );
						dev->flag[1] = 1;
					}
					else if(atoi(buf)){
						if(dev->health[cur_team]>0)
							gtk_sco_increment(cur_team,atoi(buf));
						dev->health[cur_team] -= atoi(buf);
						if(dev->health[cur_team]>0)
							gtk_trg_update( dev, "Hit" );
						else
							gtk_trg_update( dev, "Broken" );
						dev->flag[1] = 1;
					}
					else if(buf[0]=='0'){
						gtk_trg_update( dev, "Save" );
						dev->flag[1] = 1;
					}
					else{
						gtk_trg_update( dev, "Error" );
					}
					break;
				case TANK:	// Identify tank ONCE
					break;
			}
			break;
	}

}
void state_handler( int state, time_t round_starting_time, struct sStatus status ){
	int i;
	time_t cur_time = time(NULL);
	switch(state){
		case STATE_NOTHIN:
			if( cur_time - round_starting_time > TIME_NOTHIN ){
				state_change(STATE_MOVING, status);
			}
			else if( gbl_button_pressed ){
				state_change(STATE_MOVING, status);
				gbl_button_pressed = 0;
			}
			gbl_state_time = round_starting_time - cur_time + TIME_NOTHIN;
			break;
		case STATE_MOVING:
			if( cur_time - round_starting_time > TIME_MOVING ){
				state_change(STATE_ENDING, status);
			}
			else if( gbl_button_pressed ){
				state_change(STATE_ENDING, status);
				gbl_button_pressed = 0;
			}
			gbl_state_time = round_starting_time - cur_time + TIME_MOVING;
			break;
		case STATE_ENDING:
			if( cur_time - round_starting_time > TIME_ENDING ){
				state_change(STATE_TRGTON, status);
			}
			else if( gbl_button_pressed ){
				state_change(STATE_TRGTON, status);
				gbl_button_pressed = 0;
			}
			gbl_state_time = round_starting_time - cur_time + TIME_ENDING;
			break;
		case STATE_TRGTON:
			if( cur_time - round_starting_time > TIME_TRGTON ){
				state_change(STATE_SCNLSR, status);
			}
			else if( gbl_button_pressed ){
				state_change(STATE_SCNLSR, status);
				gbl_button_pressed = 0;
			}
			else{
				for( i=0; i<7; i++ ){
					if(ui_info_target[i].valid && ui_info_target[i].dev->health[*status.cur_team]>0 && ui_info_target[i].dev->flag[0]==0){
						break;
					}
				}
				if(i==7){
					state_change(STATE_SCNLSR, status);
				}
			}
			gbl_state_time = round_starting_time - cur_time + TIME_TRGTON;
			break;
		case STATE_SCNLSR:
			if( cur_time - round_starting_time > TIME_SCNLSR ){
				state_change(STATE_TRGTOF, status);
			}
			else if( gbl_button_pressed ){
				state_change(STATE_TRGTOF, status);
				gbl_button_pressed = 0;
			}
			gbl_state_time = round_starting_time - cur_time + TIME_SCNLSR;
			break;
		case STATE_TRGTOF:
			if( cur_time - round_starting_time > TIME_TRGTOF ){
				state_change(STATE_NOTHIN, status);
			}
			else if( gbl_button_pressed ){
				state_change(STATE_NOTHIN, status);
				gbl_button_pressed = 0;
			}
			else{
				for( i=0; i<7; i++ ){
					if(ui_info_target[i].valid && ui_info_target[i].dev->health[*status.cur_team]>0 && ui_info_target[i].dev->flag[1]==0){
						break;
					}
				}
				if(i==7){
					state_change(STATE_NOTHIN, status);
				}
			}
			gbl_state_time = round_starting_time - cur_time + TIME_TRGTOF;
			break;
	}
}

void state_change( int target, struct sStatus status ){
	int i, cur_team;
	char buf[50];

	cur_team = *status.cur_team;
	switch(target){
		case STATE_MOVING:
			*status.state = STATE_MOVING;
			*status.round_starting_time = time(NULL);
			printf("\nstate change to STATE_MOVING\n");
			sprintf( buf, "STATE_MOVING%d", *status.cur_team);
			gtk_str_state_update(buf);
			// gtk_str_state_update("STATE_MOVING");
			break;
		case STATE_ENDING:
			*status.state = STATE_ENDING;
			*status.round_starting_time = time(NULL);
			printf("\nstate change to STATE_ENDING\n");
			gtk_str_state_update("STATE_ENDING");
			gtk_tnk_bcast("Idle");
			break;
		case STATE_TRGTON:
			*status.state = STATE_TRGTON;
			*status.round_starting_time = time(NULL);
			printf("\nstate change to STATE_TRGTON\n");
			gtk_str_state_update("STATE_TRGTON");
			for( i=0; i<7; i++ ){
				if(ui_info_target[i].valid && ui_info_target[i].dev->health[*status.cur_team]>0)
					gtk_trg_update( ui_info_target[i].dev, "Scanning" );
				else if(ui_info_target[i].valid)
					gtk_trg_update( ui_info_target[i].dev, "Dead" );

				// Reset required flag
				if(ui_info_target[i].valid){
					ui_info_target[i].dev->flag[0] = 0;
				}

			}
			break;
		case STATE_SCNLSR:
			*status.state = STATE_SCNLSR;
			*status.round_starting_time = time(NULL);
			printf("\nstate change to STATE_SCNLSR\n");
			gtk_str_state_update("STATE_SCNLSR");
			break;
		case STATE_TRGTOF:
			*status.state = STATE_TRGTOF;
			*status.round_starting_time = time(NULL);
			printf("\nstate change to STATE_TRGTOF\n");
			gtk_str_state_update("STATE_TRGTOF");

			for( i=0; i<7; i++ ){
				// Reset required flag
				if(ui_info_target[i].valid){
					ui_info_target[i].dev->flag[1] = 0;
				}
			}
			break;
		case STATE_NOTHIN:
			*status.state = STATE_NOTHIN;
			*(status.cur_team) = (cur_team+1)%2;
			*status.round_starting_time = time(NULL);
			printf("\nstate change to STATE_NOTHIN\n");
			gtk_str_state_update("STATE_NOTHIN");
			for( i=0; i<7; i++ ){
				if(ui_info_target[i].valid && ui_info_target[i].dev->health[*status.cur_team]>0)
					gtk_trg_update( ui_info_target[i].dev, "Idle" );
				else if(ui_info_target[i].valid)
					gtk_trg_update( ui_info_target[i].dev, "Dead" );
			}
			// gtk_trg_bcast("Idle", cur_team);
			break;
	}
}