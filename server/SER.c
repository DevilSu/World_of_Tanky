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

#include "INFO.h"

#define SERV_PORT 5000
#define LISTENQ 1024
#define MAXLINE 100

int main(int argc, char **argv)
{
	// set up TCP server
	int state;
	char state_str;
	int lisfd, confd, sockfd;
	int flag=1, stat;
	int len=sizeof(int);
	socklen_t clilen;
	struct sockaddr_in servaddr, cliaddr;
	char buf[MAXLINE], msg[MAXLINE];

	strcpy(msg,"Server say hello\n");
	printf("msg = %s\n", msg);

	lisfd=socket(AF_INET, SOCK_STREAM, 0);
	stat=setsockopt(lisfd, SOL_SOCKET, SO_REUSEADDR, &flag, len);

	bzero(&servaddr, sizeof(servaddr));

	servaddr.sin_family=AF_INET;
	servaddr.sin_port=htons(SERV_PORT);
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);

	bind(lisfd, (struct sockaddr*) &servaddr, sizeof(servaddr));
	listen(lisfd, LISTENQ);

	// set up select function
	int i, maxi, maxfd;
	int nready;
	ssize_t n;
	fd_set rset, allset;

	maxfd=lisfd;
	maxi=-1;
	FD_ZERO(&allset);
	FD_SET(lisfd, &allset);

	// set up device
	DEVICE dev[NUM_OF_DEV];
	for(i=0; i<NUM_OF_DEV; i++){
		dev[i].fd=-1;
		dev[i].new_comer = 0;
	}

	for( state = STATE_NOTHIN;;)
	{

		// State updating
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
		nready=select(maxfd+1, &rset, NULL, NULL, NULL);
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

					// Data from other clients
					switch(state){
						case STATE_NOTHIN:
							switch(dev[i].id){
								case 2: // Ignore tank's ping
								case 3: // Ignore target's ping
									break;
							}
							break;
						case STATE_MOVING:
							switch(dev[i].id){
								case 2:	// Identify tank to move ONCE
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
