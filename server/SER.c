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

int main(int argc, char **argv)
{
	// set up GTK
	GtkBuilder      *builder; 
    GtkWidget       *window;
 
    gtk_init(&argc, &argv);
 
    builder = gtk_builder_new();
    gtk_builder_add_from_file(builder, "window_main.glade", NULL);
 
    window = GTK_WIDGET(gtk_builder_get_object(builder, "window_main"));
	g_lbl_hello = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_hello"));
	g_lbl_count = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_count"));
	g_lbl_r_score = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_r_score"));
	gtk_label_set_text(GTK_LABEL(g_lbl_r_score), "HHAHA");
    gtk_builder_connect_signals(builder, NULL);
 
    g_object_unref(builder);
 
    gtk_widget_show(window);
    g_timeout_add_seconds(1, update, g_lbl_r_score);
    gtk_main();

	// set up TCP server
	int state;
	char state_str;
	int lisfd, confd, sockfd;
	int flag=1, stat;
	int len=sizeof(int);
	socklen_t clilen;
	struct sockaddr_in servaddr, cliaddr;
	struct timeval select_tout;
	char buf[MAXLINE], msg[MAXLINE];
	time_t round_starting_time, cur_time;

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
	struct entry *dev_ptr;
    SLIST_INIT(&tnk_head);
    SLIST_INIT(&trg_head);
	for(i=0; i<NUM_OF_DEV; i++){
		dev[i].fd=-1;
		dev[i].new_comer = 0;
	}

	round_starting_time = time(NULL);
	for( state = STATE_NOTHIN;;)
	{
		// State updater
		cur_time = time(NULL);
		switch(state){
			case STATE_NOTHIN:
				if( cur_time - round_starting_time > 5 ){
					state = STATE_MOVING;
					round_starting_time = time(NULL);
					printf("\nstate change to STATE_MOVING\n");
				}
				break;
			case STATE_MOVING:
				if( cur_time - round_starting_time > 26 ){
					state = STATE_ENDING;
					round_starting_time = time(NULL);
					printf("\nstate change to STATE_ENDING\n");
				}
				break;
			case STATE_ENDING:
				if( cur_time - round_starting_time > 5 ){
					state = STATE_TRGTON;
					round_starting_time = time(NULL);
					printf("\nstate change to STATE_TRGTON\n");
				}
				break;
			case STATE_TRGTON:
				if( cur_time - round_starting_time > 8 ){
					state = STATE_SCNLSR;
					round_starting_time = time(NULL);
					printf("\nstate change to STATE_SCNLSR\n");
				}
				break;
			case STATE_SCNLSR:
				if( cur_time - round_starting_time > 8 ){
					state = STATE_TRGTOF;
					round_starting_time = time(NULL);
					printf("\nstate change to STATE_TRGTOF\n");
				}
				break;
			case STATE_TRGTOF:
				if( cur_time - round_starting_time > 8 ){
					state = STATE_NOTHIN;
					round_starting_time = time(NULL);
					printf("\nstate change to STATE_NOTHIN\n");
				}
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
					break;
				case TRGT:
					SLIST_INSERT_HEAD(&trg_head, dev_ptr, entries);
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
