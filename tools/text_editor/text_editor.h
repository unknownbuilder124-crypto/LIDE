#ifndef TEXT_EDITOR_H
#define TEXT_EDITOR_H

#include <gtk/gtk.h>
#include <gio/gio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    GtkWidget *window;
    GtkWidget *text_view;
    GtkTextBuffer *buffer;
    GtkWidget *status_label;
    GtkWidget *find_dialog;
    GtkWidget *find_entry;
    GtkWidget *replace_entry;
    char *current_file;
    gboolean modified;

    // For dragging and resizing
    int is_dragging;
    int is_resizing;
    int resize_edge;
    int drag_start_x;
    int drag_start_y;
} Editor;

// Document functions
void editor_new(Editor *ed);
void editor_open(Editor *ed);
void editor_save(Editor *ed);
void editor_save_as(Editor *ed);
void editor_close(Editor *ed);

// Edit functions
void editor_cut(Editor *ed);
void editor_copy(Editor *ed);
void editor_paste(Editor *ed);
void editor_undo(Editor *ed);
void editor_redo(Editor *ed);

// Search functions
void editor_find(Editor *ed);
void editor_find_next(Editor *ed);
void editor_replace(Editor *ed);
void editor_replace_all(Editor *ed);

// UI helpers
void editor_update_title(Editor *ed);
void editor_update_status(Editor *ed);
void prepare_dialog(GtkWidget *dialog);

#endif