#ifndef LIDE_FM_H
#define LIDE_FM_H

#include <gtk/gtk.h>
#include <gio/gio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/statvfs.h>

typedef struct

{
    GtkWidget *window;
    GtkWidget *location_entry;
    GtkWidget *back_button;
    GtkWidget *forward_button;
    GtkWidget *up_button;
    GtkWidget *home_button;
    GtkWidget *refresh_button;
    GtkWidget *sidebar_tree;
    GtkWidget *main_tree;
    GtkWidget *status_label;
    GtkListStore *sidebar_store;
    GtkTreeStore *main_store;
    GFile *current_dir;
    GList *history;
    GList *history_pos;

    // For dragging and resizing
    int is_dragging;
    int is_resizing;
    int resize_edge;
    int drag_start_x;
    int drag_start_y;
} FileManager;

// Browser functions
void fm_open_location(FileManager *fm, const gchar *path);
void fm_go_up(FileManager *fm);
void fm_go_home(FileManager *fm);
void fm_go_back(FileManager *fm);
void fm_go_forward(FileManager *fm);
void fm_refresh(FileManager *fm);
void fm_on_row_activated(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *col, FileManager *fm);
void fm_on_location_activate(FileManager *fm);
void fm_update_status(FileManager *fm);

// Sidebar
void fm_populate_sidebar(FileManager *fm);
void fm_on_sidebar_row_activated(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *col, FileManager *fm);

#endif