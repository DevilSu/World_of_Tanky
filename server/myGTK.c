#include "myGTK.h"

void *gtk_thread(void *arg)
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
        sprintf( label_name, "lbl_player_score_%d0", i );
        g_lbl_score[i] = GTK_WIDGET( gtk_builder_get_object(builder, label_name));
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
    g_timeout_add(100, gtk_state_update, g_lbl_state);
    g_timeout_add(100, gtk_timer_update, g_lbl_timer);
    g_timeout_add(100, gtk_player_info_update, g_lbl_player_info);
    g_timeout_add(100, gtk_target_info_update, g_lbl_target_info);
    g_timeout_add(100, gtk_player_update, g_lbl_player);
    g_timeout_add(100, gtk_target_update, g_lbl_target);
    g_timeout_add(100, gtk_score_update, g_lbl_target);

    gtk_main();
    return(NULL);
}

void on_btn_start_clicked(){
    gbl_button_pressed = 1;
    return;
}

// gboolean update(gpointer data)
// {
//     GtkLabel *label = (GtkLabel*)data;
//     char buf[256];
//     memset(buf, 0, 256);
//     snprintf(buf, 255, "Time elapsed: %d secs", ++sec_expired);
//     gtk_label_set_label(label, buf);
//     return continue_timer;
// }

gboolean gtk_state_update(gpointer data)
{
    GtkLabel *label = (GtkLabel*)data;
    char buf[256];
    memset(buf, 0, 256);
    snprintf(buf, 255, "%s", gbl_state);
    gtk_label_set_label(label, buf);
    return continue_timer;
}

gboolean gtk_timer_update(gpointer data)
{
    GtkLabel *label = (GtkLabel*)data;
    char buf[256];
    memset(buf, 0, 256);
    snprintf(buf, 255, "%-2d", gbl_state_time);
    gtk_label_set_label(label, buf);
    return continue_timer;
}

gboolean gtk_player_update(gpointer data)
{
    GtkLabel ***label = (GtkLabel***)data;
    int i, j;
    char buf[256];
    memset(buf, 0, 256);

    // switch(gbl_player_info){
    //     case UI_PLAYER_REGISTER:
    for( i=0; i<2; i++ ){
        for( j=0; j<GTK_NUM_PLAYER_EACH_TEM; j++ ){
            if(ui_info_player[i][j].valid){
                strcpy( buf, ui_info_player[i][j].dev->name );
                gtk_label_set_label(GTK_LABEL(g_lbl_player[i][j]), buf);
            }
            else{
                gtk_label_set_label(GTK_LABEL(g_lbl_player[i][j]), "x");
            }
        }
    }
            // break;
    // }
    return continue_timer;
}

gboolean gtk_player_info_update(gpointer data)
{
    GtkLabel *label = (GtkLabel*)data;
    int i,j;
    char buf[256];
    memset(buf, 0, 256);

    for( i=0; i<2; i++ ){
        for( j=0; j<GTK_NUM_PLAYER_EACH_TEM; j++ ){

            if(ui_info_player[i][j].valid){
                strcpy( buf, ui_info_player[i][j].dev->stat );
                gtk_label_set_label(GTK_LABEL(g_lbl_player_info[i][j]), buf);
            }
            else{
                gtk_label_set_label(GTK_LABEL(g_lbl_player_info[i][j]), "x");
            }
        }
    }
    return continue_timer;
}

gboolean gtk_target_info_update(gpointer data)
{
    GtkLabel *label = (GtkLabel*)data;
    int i;
    char buf[256];
    memset(buf, 0, 256);

    // snprintf(buf, 255, "%s", gbl_target_status);
    for( i=0; i<7; i++ ){
        if(ui_info_target[i].valid){
            strcpy( buf, ui_info_target[i].dev->stat );
            gtk_label_set_label(GTK_LABEL(g_lbl_target_info[i]), buf);
        }
        else{
            gtk_label_set_label(GTK_LABEL(g_lbl_target_info[i]), "x");
        }
    }
    return continue_timer;
}

gboolean gtk_target_update(gpointer data)
{
    GtkLabel *label = (GtkLabel*)data;
    int i;
    char buf[256];
    memset(buf, 0, 256);

    for( i=0; i<7; i++ ){
        if(ui_info_target[i].valid){
            strcpy( buf, ui_info_target[i].dev->name );
            gtk_label_set_label(GTK_LABEL(g_lbl_target[i]), buf);
        }
        else{
            gtk_label_set_label(GTK_LABEL(g_lbl_target[i]), "x");
        }
    }
    return continue_timer;
}

gboolean gtk_score_update(gpointer data)
{
    GtkLabel *label = (GtkLabel*)data;
    int i;
    char buf[256];
    memset(buf, 0, 256);

    for( i=0; i<2; i++ ){
        sprintf( buf, "%d", gbl_score[i] );
        gtk_label_set_label(GTK_LABEL(g_lbl_score[i]), buf);
    }
    return continue_timer;
}

void on_btn_hello_clicked()
{
    unsigned int count = 0;
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
