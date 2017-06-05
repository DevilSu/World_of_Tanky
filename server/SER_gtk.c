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

extern DEVICE *tnk_list, *trg_list;

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

extern char gbl_game_start;
extern char gbl_state[];
extern int gbl_score[];
extern int gbl_state_time;
extern int gbl_player_num, gbl_player_info;
extern int gbl_target_num, gbl_target_info;
extern struct ui_info_node ui_info_player[][1], ui_info_target[];

void gtk_trg_bcast( char *str, int team ){
	int j;
	for( j=0; j<7; j++ ){
		if( ui_info_target[j].valid && ui_info_target[j].dev->health[team]>0 ){
			strcpy(ui_info_target[j].dev->stat,str);
		}
	}	
}

void gtk_tnk_bcast( char *str ){
	int i, j;
	for( i=0; i<2; i++ ){
		for( j=0; j<1; j++ ){
			if(ui_info_player[i][j].valid){
				strcpy(ui_info_player[i][j].dev->stat,str);
			}
		}
	}
}

void gtk_tnk_update( DEVICE *dev, char *str ){
	int i, j;
	for( i=0; i<2; i++ ){
		for( j=0; j<1; j++ ){
			if(ui_info_player[i][j].valid && ui_info_player[i][j].dev==dev){
				strcpy(ui_info_player[i][j].dev->stat,str);
				return;
			}
		}
	}	
}

void gtk_trg_update( DEVICE *dev, char *str ){
	int i;
	for( i=0; i<7; i++ ){
		if(ui_info_target[i].valid && ui_info_target[i].dev==dev){
			strcpy(ui_info_target[i].dev->stat,str);
			return;
		}
	}
}

void gtk_trg_init(){
	int i;
	for( i=0; i<7; i++ ){
		ui_info_target[i].valid = 0;
	}	
}

void gtk_tnk_init(){
	int i, j;
	for( i=0; i<2; i++){
		for( j=0; j<1; j++ ){
			ui_info_player[i][j].valid = 0;
		}
	}	
}

void gtk_tnk_register( DEVICE *dev ){
	int i, j;
	for( i=0; i<1; i++ ){
		for( j=0; j<2; j++ ){
			if(ui_info_player[i][j].valid==0){
				ui_info_player[i][j].valid = 1;
				ui_info_player[i][j].dev = dev;
				break;
			}
		}
	}
}

void gtk_trg_register( DEVICE *dev ){
	int i;
	for( i=0; i<7; i++ ){
		if(ui_info_target[i].valid==0){
			ui_info_target[i].valid = 1;
			ui_info_target[i].dev = dev;
			break;
		}
	}
}

void gtk_sco_increment( int team, int value ){
	gbl_score[team] += value;
	return;
}

void gtk_str_state_update( char *str ){
	strcpy(gbl_state, str);
	return;
}