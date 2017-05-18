#include <gtk/gtk.h>

static gboolean continue_timer = TRUE;
int sec_expired=0;

GtkWidget *g_lbl_hello;
GtkWidget *g_lbl_count;
GtkWidget *g_lbl_r_score;

static gboolean update(gpointer data);

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
    g_lbl_r_score   = GTK_WIDGET( gtk_builder_get_object(builder, "lbl_r_score"));
    gtk_label_set_text(GTK_LABEL(g_lbl_r_score), "HHAHA");
    gtk_builder_connect_signals(builder, NULL);
 
    g_object_unref(builder);
 
    gtk_widget_show(window);
    g_timeout_add_seconds(1, update, g_lbl_r_score);
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