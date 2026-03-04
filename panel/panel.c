#include <gtk/gtk.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include "tools/minimized_container.h"  // Add this include

static void launch_tools(GtkButton *button, gpointer data) 
{
    (void)button;
    (void)data;
    pid_t pid = fork();
    if (pid == 0) {
        execl("/home/israfil/Desktop/LIDE/blackline-tools", "blackline-tools", NULL);
        exit(0);
    }
}

static void do_nothing(GtkButton *button, gpointer data) 
{
    (void)button;
    (void)data;
}

static gboolean update_clock(gpointer label) 
{
    time_t rawtime;
    struct tm *timeinfo;
    char buffer[64];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    strftime(buffer, sizeof(buffer), "%a %d %b %H:%M:%S", timeinfo);
    gtk_label_set_text(GTK_LABEL(label), buffer);
    return G_SOURCE_CONTINUE;
}

static void activate(GtkApplication *app, gpointer user_data) 
{
    (void)app;
    (void)user_data;
    
    // Initialize the minimized container
    minimized_container_initialize();
    
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "BlackLine Panel");
    gtk_window_set_default_size(GTK_WINDOW(window), -1, 35);
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
    gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_DOCK);
    
    // Full width
    GdkDisplay *gdisplay = gdk_display_get_default();
    GdkMonitor *monitor = gdk_display_get_primary_monitor(gdisplay);
    GdkRectangle geom;
    gdk_monitor_get_geometry(monitor, &geom);
    gtk_window_set_default_size(GTK_WINDOW(window), geom.width, 35);
    gtk_window_move(GTK_WINDOW(window), 0, 0);
    
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_add(GTK_CONTAINER(window), box);
    
    // BlackLine button
    GtkWidget *btn1 = gtk_button_new_with_label("BlackLine");
    g_signal_connect(btn1, "clicked", G_CALLBACK(do_nothing), NULL);
    gtk_box_pack_start(GTK_BOX(box), btn1, FALSE, FALSE, 0);
    
    // Tools button
    GtkWidget *btn2 = gtk_button_new_with_label("Tools");
    g_signal_connect(btn2, "clicked", G_CALLBACK(launch_tools), NULL);
    gtk_box_pack_start(GTK_BOX(box), btn2, FALSE, FALSE, 0);
    
    // Minimized Apps button (📌)
    GtkWidget *min_btn = minimized_container_get_toggle_button();
    gtk_box_pack_start(GTK_BOX(box), min_btn, FALSE, FALSE, 0);
    
    GtkWidget *spacer = gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(box), spacer, TRUE, TRUE, 0);
    
    // Clock with date
    GtkWidget *clock = gtk_label_new(NULL);
    update_clock(clock);
    g_timeout_add_seconds(1, update_clock, clock);
    gtk_box_pack_end(GTK_BOX(box), clock, FALSE, FALSE, 10);
    
    gtk_widget_show_all(window);
}

int main(int argc, char **argv) 
{
    GtkApplication *app = gtk_application_new("org.blackline.panel", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}