#include <string.h>
#include <gtk/gtk.h>

#define UI_PLAYER_UNDEFINED -1
#define UI_PLAYER_REGISTER  0
#define UI_TARGET_UNDEFINED -1
#define UI_TARGET_REGISTER  0

#define GTK_NUM_PLAYER_EACH_TEM 1
static gboolean continue_timer = TRUE;
int sec_expired=0;

GtkWidget *g_lbl_state;
GtkWidget *g_lbl_timer;
GtkWidget *g_lbl_count;
GtkWidget *g_lbl_player[2][GTK_NUM_PLAYER_EACH_TEM];
GtkWidget *g_lbl_player_info[2][GTK_NUM_PLAYER_EACH_TEM];
GtkWidget *g_lbl_target[7];
GtkWidget *g_lbl_target_info[7];

// static gboolean update(gpointer data);
static gboolean gtk_state_update(gpointer data);
static gboolean gtk_timer_update(gpointer data);
static gboolean gtk_player_info_update(gpointer data);
static gboolean gtk_target_info_update(gpointer data);
static gboolean gtk_player_update(gpointer data);
static gboolean gtk_target_update(gpointer data);

extern char gbl_game_start;
extern char gbl_state[];
extern char gbl_player_status[][1][30];
extern char gbl_player[][1][30];
extern char gbl_target_status[][30];
extern char gbl_target[][30];
extern int gbl_state_time;
extern int gbl_player_num, gbl_player_info;
extern int gbl_target_num, gbl_target_info;

static void *gtk_thread(void *arg)
{
    int i,j;
    char label_name[50];
    GtkBuilder      *builder; 
    GtkWidget       *window;
 
    // gtk_init(&argc, &argv);
 
    builder = gtk_builder_new();
    gtk_builder_add_from_file(builder, "window_main.glade", NULL);
 
    window          = GTK_WIDGET( gtk_builder_get_object(builder, "window_main"));
    g_lbl_state     = GTK_WIDGET( gtk_builder_get_object(builder, "lbl_state"));
    g_lbl_timer     = GTK_WIDGET( gtk_builder_get_object(builder, "lbl_timer"));
    g_lbl_count     = GTK_WIDGET( gtk_builder_get_object(builder, "lbl_count"));
    for( i=0; i<2; i++ ){
        for( j=0; j<GTK_NUM_PLAYER_EACH_TEM; j++ ){
            snprintf(label_name, 255, "lbl_player_%d%d", i, j);
            g_lbl_player[i][j] = GTK_WIDGET( gtk_builder_get_object(builder, label_name));

            snprintf(label_name, 255, "lbl_player_status_%d%d", i, j);
            g_lbl_player_info[i][j] = GTK_WIDGET( gtk_builder_get_object(builder, label_name));
        }
    }
    
    for( i=0; i<7; i++ ){
        snprintf(label_name, 255, "lbl_target_%d", i);
        g_lbl_target[i] = GTK_WIDGET( gtk_builder_get_object(builder, label_name));    
        gtk_label_set_text(GTK_LABEL(g_lbl_target[i]), "No target");

        snprintf(label_name, 255, "lbl_target_status_%d", i);
        g_lbl_target_info[i]    = GTK_WIDGET( gtk_builder_get_object(builder, label_name));
        gtk_label_set_text(GTK_LABEL(g_lbl_target_info[i]), "x");
    }
    
    gtk_label_set_text(GTK_LABEL(g_lbl_player[0][0]), "Default player 1");
    gtk_label_set_text(GTK_LABEL(g_lbl_player[1][0]), "Default player 2");

    gtk_builder_connect_signals(builder, NULL);
 
    g_object_unref(builder);
 
    gtk_widget_show(window);
    // g_timeout_add_seconds(1, update, g_lbl_player);
    g_timeout_add_seconds(1, gtk_state_update, g_lbl_state);
    g_timeout_add_seconds(1, gtk_timer_update, g_lbl_timer);
    g_timeout_add_seconds(1, gtk_player_info_update, g_lbl_player_info);
    g_timeout_add_seconds(1, gtk_target_info_update, g_lbl_target_info);
    g_timeout_add_seconds(1, gtk_player_update, g_lbl_player);
    g_timeout_add_seconds(1, gtk_target_update, g_lbl_target);

    gtk_main();
    return(NULL);
}

void on_btn_start_clicked(){
    gbl_game_start = 1;
    return;
}

// static gboolean update(gpointer data)
// {
//     GtkLabel *label = (GtkLabel*)data;
//     char buf[256];
//     memset(buf, 0, 256);
//     snprintf(buf, 255, "Time elapsed: %d secs", ++sec_expired);
//     gtk_label_set_label(label, buf);
//     return continue_timer;
// }

static gboolean gtk_state_update(gpointer data)
{
    GtkLabel *label = (GtkLabel*)data;
    char buf[256];
    memset(buf, 0, 256);
    snprintf(buf, 255, "%s", gbl_state);
    gtk_label_set_label(label, buf);
    return continue_timer;
}

static gboolean gtk_timer_update(gpointer data)
{
    GtkLabel *label = (GtkLabel*)data;
    char buf[256];
    memset(buf, 0, 256);
    snprintf(buf, 255, "%-2d", gbl_state_time);
    gtk_label_set_label(label, buf);
    return continue_timer;
}

static gboolean gtk_player_update(gpointer data)
{
    GtkLabel ***label = (GtkLabel***)data;
    int i, j;
    char buf[256];
    memset(buf, 0, 256);

    // switch(gbl_player_info){
    //     case UI_PLAYER_REGISTER:
    for( i=0; i<2; i++ ){
        for( j=0; j<GTK_NUM_PLAYER_EACH_TEM; j++ ){
            // snprintf(buf, 255, "player%d(%d,%d)", gbl_player_num,i,j);
            snprintf(buf, 255, "%s", gbl_player[i][j]);
            gtk_label_set_label(GTK_LABEL(g_lbl_player[i][j]), buf);
        }
    }
            // break;
    // }
    return continue_timer;
}

static gboolean gtk_player_info_update(gpointer data)
{
    GtkLabel *label = (GtkLabel*)data;
    int i,j;
    char buf[256];
    memset(buf, 0, 256);

    for( i=0; i<2; i++ ){
        for( j=0; j<GTK_NUM_PLAYER_EACH_TEM; j++ ){
            // snprintf(buf, 255, "player%d(%d,%d)", gbl_player_num,i,j);
            snprintf(buf, 255, "%s", gbl_player_status[i][j]);
            gtk_label_set_label(GTK_LABEL(g_lbl_player_info[i][j]), buf);
        }
    }
    return continue_timer;
}

static gboolean gtk_target_info_update(gpointer data)
{
    GtkLabel *label = (GtkLabel*)data;
    int i;
    char buf[256];
    memset(buf, 0, 256);

    // snprintf(buf, 255, "%s", gbl_target_status);
    for( i=0; i<7; i++ ){
        strcpy( buf, gbl_target_status[i] );
        gtk_label_set_label(GTK_LABEL(g_lbl_target_info[i]), buf);
    }
    return continue_timer;
}

static gboolean gtk_target_update(gpointer data)
{
    GtkLabel *label = (GtkLabel*)data;
    int i;
    char buf[256];
    memset(buf, 0, 256);

    for( i=0; i<7; i++ ){
        strcpy( buf, gbl_target[i] );
        gtk_label_set_label(GTK_LABEL(g_lbl_target[i]), buf);
    }
    // switch(gbl_target_info){
    //     case UI_TARGET_REGISTER:
    //         snprintf(buf, 255, "target%d", gbl_target_num);
    //         gtk_label_set_label(GTK_LABEL(g_lbl_target[1]), buf);
    //         break;
    // }
    return continue_timer;
}

void on_btn_hello_clicked()
{
    static unsigned int count = 0;
    char str_count[30] = {0};
    
    gtk_label_set_text(GTK_LABEL(g_lbl_state), "Hello, world!");
    count++;
    sprintf(str_count, "%d", count);
    gtk_label_set_text(GTK_LABEL(g_lbl_count), str_count);
    return;
}
 
// called when window is closed
void on_window_main_destroy()
{
    gtk_main_quit();
    return;
}
