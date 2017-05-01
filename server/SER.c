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
	char state[2] = {'0','\n'};
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
	for(i=0; i<NUM_OF_DEV; i++)dev[i].fd=-1;

	for(;;)
	{
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
			if(--nready<=0)continue; // no more readable descriptors
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
					if(dev[i].id==2){
						switch(atoi(buf)){
							case TNK_PING:
								printf("Ping from tank, return %c(%d)\n", state[0], state[0]);
								write(sockfd, state, 2);
								break;
						}
						// write(sockfd, &state, 1);
					}
					else if(dev[i].id==4){
						write(sockfd, msg, strlen(msg));
						switch(buf[0]){
							case '0':
								state[0] = SER_MOVE_Y + '0';
								printf("state = MOVE\n");
								break;
							case '2':
								state[0] = SER_SHOT_Y + '0';
								printf("state = SHOT\n");
								break;
							default:
								state[0] = SER_MOVE_N + '0';
								printf("state = STAY\n");
								break;
						}
						// state = *buf;
						// printf("state = %c\n", state);
					}
					else
						write(sockfd, buf, n);
					//---------------------
					// game(dev[i]);
					//---------------------
				}
				if(--nready<=0) break; // no more readable descriptors
			}
		}
	}
	return 0;
}