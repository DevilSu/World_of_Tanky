#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/select.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define SERV_PORT 5000
#define MAXLINE 100
#define max(x,y) (x<y?y:x)
void str_cli(FILE *fp, int sockfd);
void sig_proc(int signo);

int main(int argc, char **argv)
{
	int sockfd, status;
	struct sockaddr_in servaddr;
	
	// if(argc!=2)
	// {
	// 	printf("usage: ./tcpechoclient <IPaddress>\n");
	// 	return 0;
	// }
	
	sockfd=socket(AF_INET, SOCK_STREAM, 0);
	bzero(&servaddr, sizeof(servaddr));

	servaddr.sin_family=AF_INET;
	// inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
	inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);
	servaddr.sin_port=htons(SERV_PORT);

	status=connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	// signal(SIGPIPE, sig_proc);
	if(status==-1){ printf("connect error\n"); return 0;}

	str_cli(stdin, sockfd);

	return 0;
}

void str_cli(FILE *fp, int sockfd)
{
	int maxfdp1, stdineof;
	fd_set rset;
	char sendline[MAXLINE], recvline[MAXLINE];

	stdineof=0; // use for test readable
	FD_ZERO(&rset);	// initial select
	for(;;)
	{
		if(stdineof==0) //??????
		FD_SET(fileno(fp), &rset);
		FD_SET(sockfd, &rset);
		maxfdp1=max(fileno(fp), sockfd)+1;
		select(maxfdp1, &rset, NULL, NULL, NULL);

		if(FD_ISSET(sockfd, &rset))	// socket is readable
		{
			
			if(read(sockfd, recvline, MAXLINE)==0)
			{
				if(stdineof==1) return; // normal termination
				else
				{
					printf("str_cli: server terminated permaturely\n");	
					exit(0);
				}
			}
			fputs("recv from server: ", stdout);
			fputs(recvline, stdout);
		}
		if(FD_ISSET(fileno(fp), &rset)) // input is readable
		{
			if(fgets(sendline, MAXLINE, fp)==NULL) //EOF, crtl+D in terminal input
			{
				fputs("sending FIN to server, closing connection\n", stdout);
				stdineof=1;
				shutdown(sockfd, SHUT_RD); // send FIN		
				FD_CLR(fileno(fp), &rset);
				continue;
			}
			write(sockfd, sendline, strlen(sendline));
		}
		memset(sendline, 0, MAXLINE);
		memset(recvline, 0, MAXLINE);
	}
}

void sig_proc(int signo)
{
	printf("server terminated\n");
	exit(0);
}