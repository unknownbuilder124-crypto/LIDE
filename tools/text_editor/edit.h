#ifndef BLACKLINE_EDIT_H
#define BLACKLINE_EDIT_H

#include <gtk/gtk.h>

// Edit operation types
typedef enum {
    EDIT_UNDO,
    EDIT_REDO,
    EDIT_CUT,
    EDIT_COPY,
    EDIT_PASTE,
    EDIT_DELETE,
    EDIT_SELECT_ALL,
    EDIT_FIND,
    EDIT_REPLACE,
    EDIT_GOTO_LINE,
    EDIT_COMMENT_TOGGLE,
    EDIT_INDENT,
    EDIT_UNINDENT,
    EDIT_UPPERCASE,
    EDIT_LOWERCASE,
    EDIT_CAPITALIZE,
    EDIT_DUPLICATE_LINE,
    EDIT_DELETE_LINE,
    EDIT_JOIN_LINES,
    EDIT_SORT_LINES
} EditOperation;

// Find/Replace dialog data
typedef struct {
    GtkWidget *dialog;
    GtkWidget *find_entry;
    GtkWidget *replace_entry;
    GtkWidget *match_case_check;
    GtkWidget *whole_word_check;
    GtkTextBuffer *buffer;
    GtkWidget *window;
} FindReplaceData;

// Undo/Redo stack
typedef struct EditNode {
    gchar *text;
    GtkTextMark *start_mark;
    GtkTextMark *end_mark;
    struct EditNode *next;
    struct EditNode *prev;
} EditNode;

typedef struct {
    EditNode *current;
    EditNode *head;
    EditNode *tail;
    gint max_stack_size;
    gint current_size;
} EditHistory;

// Function prototypes - EDITING OPERATIONS
void edit_init(GtkTextBuffer *buffer);
void edit_undo(GtkWidget *widget, gpointer data);
void edit_redo(GtkWidget *widget, gpointer data);
void edit_cut(GtkWidget *widget, gpointer data);
void edit_copy(GtkWidget *widget, gpointer data);
void edit_paste(GtkWidget *widget, gpointer data);
void edit_delete(GtkWidget *widget, gpointer data);
void edit_select_all(GtkWidget *widget, gpointer data);

// Search operations
void edit_find(GtkWidget *widget, gpointer data);
void edit_replace(GtkWidget *widget, gpointer data);
void edit_goto_line(GtkWidget *widget, gpointer data);

// Text transformation
void edit_to_uppercase(GtkWidget *widget, gpointer data);
void edit_to_lowercase(GtkWidget *widget, gpointer data);
void edit_capitalize(GtkWidget *widget, gpointer data);

// Line operations
void edit_toggle_comment(GtkWidget *widget, gpointer data);
void edit_indent(GtkWidget *widget, gpointer data);
void edit_unindent(GtkWidget *widget, gpointer data);
void edit_duplicate_line(GtkWidget *widget, gpointer data);
void edit_delete_line(GtkWidget *widget, gpointer data);
void edit_join_lines(GtkWidget *widget, gpointer data);
void edit_sort_lines(GtkWidget *widget, gpointer data);

// Print
void edit_print(GtkWidget *widget, gpointer data);

#endif