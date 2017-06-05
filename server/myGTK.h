#include <string.h>
#include <gtk/gtk.h>
#include "INFO.h"

#define UI_PLAYER_UNDEFINED -1
#define UI_PLAYER_REGISTER  0
#define UI_TARGET_UNDEFINED -1
#define UI_TARGET_REGISTER  0

#define GTK_NUM_PLAYER_EACH_TEM 1
static gboolean continue_timer = TRUE;

GtkWidget *g_lbl_state;
GtkWidget *g_lbl_timer;
GtkWidget *g_lbl_count;
GtkWidget *g_lbl_player[2][GTK_NUM_PLAYER_EACH_TEM];
GtkWidget *g_lbl_player_info[2][GTK_NUM_PLAYER_EACH_TEM];
GtkWidget *g_lbl_target[7];
GtkWidget *g_lbl_target_info[7];
GtkWidget *g_lbl_score[2];

// static gboolean update(gpointer data);
void *gtk_thread(void *arg);
gboolean gtk_state_update(gpointer data);
gboolean gtk_timer_update(gpointer data);
gboolean gtk_player_info_update(gpointer data);
gboolean gtk_target_info_update(gpointer data);
gboolean gtk_player_update(gpointer data);
gboolean gtk_target_update(gpointer data);
gboolean gtk_score_update(gpointer data);

extern int sec_expired;
extern char gbl_game_start, gbl_button_pressed;
extern char gbl_state[];
extern int gbl_score[];
extern int gbl_state_time;
extern int gbl_player_num, gbl_player_info;
extern int gbl_target_num, gbl_target_info;
extern struct ui_info_node ui_info_player[][1], ui_info_target[];
