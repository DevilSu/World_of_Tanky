#include <gtk/gtk.h>

static gboolean continue_timer = TRUE;
int sec_expired=0;

GtkWidget *g_lbl_hello;
GtkWidget *g_lbl_count;
GtkWidget *g_lbl_player;
GtkWidget *g_lbl_target;

static gboolean update(gpointer data);
static gboolean gtk_state_update(gpointer data);

extern char gbl_state[];
extern int gbl_player_num, gbl_player_info;
extern int gbl_target_num, gbl_target_info;
static void *gtk_thread(void *arg)
{
    GtkBuilder      *builder; 
    GtkWidget       *window;
 
    // gtk_init(&argc, &argv);
 
    builder = gtk_builder_new();
    gtk_builder_add_from_file(builder, "window_main.glade", NULL);
 
    window          = GTK_WIDGET( gtk_builder_get_object(builder, "window_main"));
    g_lbl_hello     = GTK_WIDGET( gtk_builder_get_object(builder, "lbl_hello"));
    g_lbl_count     = GTK_WIDGET( gtk_builder_get_object(builder, "lbl_count"));
    g_lbl_player    = GTK_WIDGET( gtk_builder_get_object(builder, "lbl_player_00"));
    g_lbl_target    = GTK_WIDGET( gtk_builder_get_object(builder, "lbl_target_0"));

    gtk_label_set_text(GTK_LABEL(g_lbl_player), "No player");
    gtk_label_set_text(GTK_LABEL(g_lbl_target), "No target");
    gtk_builder_connect_signals(builder, NULL);
 
    g_object_unref(builder);
 
    gtk_widget_show(window);
    // g_timeout_add_seconds(1, update, g_lbl_player);
    g_timeout_add_seconds(1, gtk_state_update, g_lbl_hello);
    gtk_main();
}

static gboolean update(gpointer data)
{
    GtkLabel *label = (GtkLabel*)data;
    char buf[256];
    memset(buf, 0, 256);
    snprintf(buf, 255, "Time elapsed: %d secs", ++sec_expired);
    gtk_label_set_label(label, buf);
    return continue_timer;
}

static gboolean gtk_state_update(gpointer data)
{
    GtkLabel *label = (GtkLabel*)data;
    char buf[256];
    memset(buf, 0, 256);
    snprintf(buf, 255, "%s", gbl_state);
    gtk_label_set_label(label, buf);
    return continue_timer;
}

static gboolean gtk_player_update(gpointer data)
{
    GtkLabel *label = (GtkLabel*)data;
    char buf[256];
    memset(buf, 0, 256);

    switch(gbl_player_info){
        case UI_PLAYER_REGISTER:
            snprintf(buf, 255, "player%d", gbl_player_num);
            gtk_label_set_label(label, buf);
            break;
    }
    return continue_timer;
}

static gboolean gtk_target_update(gpointer data)
{
    GtkLabel *label = (GtkLabel*)data;
    char buf[256];
    memset(buf, 0, 256);

    switch(gbl_target_info){
        case UI_TARGET_REGISTER:
            snprintf(buf, 255, "target%d", gbl_target_num);
            gtk_label_set_label(label, buf);
            break;
    }
    return continue_timer;
}

void on_btn_hello_clicked()
{
    static unsigned int count = 0;
    char str_count[30] = {0};
    
    gtk_label_set_text(GTK_LABEL(g_lbl_hello), "Hello, world!");
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
