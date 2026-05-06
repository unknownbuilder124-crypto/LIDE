#ifndef LIDE_FM_H
#define LIDE_FM_H

#include <gtk/gtk.h>
#include <gio/gio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/statvfs.h>

/**
 * fm.h
 * 
 * File manager interface and type definitions.
 * Declares file browser UI callbacks, file operation handlers, and
 * directory navigation interface.
 *
 * This module is part of the LIDE desktop environment system.
 * See the main window manager (wm/) and session management (session/)
 * for system architecture overview.
 */

/**
 * File Manager main structure.
 * Encapsulates all UI components and state for the file manager.
 */
typedef struct

{
    GtkWidget *window;           /* Main application window */
    GtkWidget *location_entry;   /* Entry widget showing current path */
    GtkWidget *back_button;      /* Button to navigate backward in history */
    GtkWidget *forward_button;   /* Button to navigate forward in history */
    GtkWidget *up_button;        /* Button to navigate to parent directory */
    GtkWidget *home_button;      /* Button to navigate to user's home directory */
    GtkWidget *refresh_button;   /* Button to refresh current directory view */
    GtkWidget *sidebar_tree;     /* Tree view showing bookmarks and system locations */
    GtkWidget *main_tree;        /* Tree view showing files and directories */
    GtkWidget *status_label;     /* Label showing current status (free space, selected items) */
    GtkListStore *sidebar_store; /* List store for sidebar entries */
    GtkTreeStore *main_store;    /* Tree store for main file listing */
    GFile *current_dir;          /* Current directory being displayed */
    GList *history;              /* Navigation history list (GList of gchar*) */
    GList *history_pos;          /* Current position in navigation history */

    /* Window manipulation state */
    int is_dragging;             /* TRUE while user is dragging the window */
    int is_resizing;             /* TRUE while user is resizing the window */
    int resize_edge;             /* Which edge/corner is being resized (from window_resize.h) */
    int drag_start_x;            /* Initial mouse X position for drag operation */
    int drag_start_y;            /* Initial mouse Y position for drag operation */
} FileManager;

/* Navigation and directory operations */
void fm_open_location(FileManager *fm, const gchar *path);
void fm_go_up(FileManager *fm);
void fm_go_home(FileManager *fm);
void fm_go_back(FileManager *fm);
void fm_go_forward(FileManager *fm);
void fm_refresh(FileManager *fm);

/* Callbacks for UI events */
void fm_on_row_activated(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *col, FileManager *fm);
void fm_on_location_activate(FileManager *fm);
void fm_update_status(FileManager *fm);

/* Sidebar management */
void fm_populate_sidebar(FileManager *fm);
void fm_on_sidebar_row_activated(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *col, FileManager *fm);

#endif /* LIDE_FM_H */