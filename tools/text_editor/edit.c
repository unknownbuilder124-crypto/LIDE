#include "edit.h"
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include <ctype.h>

// Global edit history
static EditHistory *history = NULL;
static GtkTextBuffer *global_buffer = NULL;
static FindReplaceData *find_replace_data = NULL;

// Print operation data structure
typedef struct {
    GtkTextView *text_view;
    GtkTextBuffer *buffer;
} PrintData;

// Initialize edit history
static EditHistory* edit_history_new(gint max_size) 
{
    EditHistory *h = g_new(EditHistory, 1);
    h->current = NULL;
    h->head = NULL;
    h->tail = NULL;
    h->max_stack_size = max_size;
    h->current_size = 0;
    return h;
}

// Add state to history
static void edit_history_push(GtkTextBuffer *buffer) 
{
    if (!history) {
        history = edit_history_new(50); // Max 50 undo steps
    }
    
    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    
    EditNode *node = g_new(EditNode, 1);
    node->text = text;
    node->start_mark = gtk_text_buffer_create_mark(buffer, NULL, &start, TRUE);
    node->end_mark = gtk_text_buffer_create_mark(buffer, NULL, &end, TRUE);
    node->next = NULL;
    node->prev = history->current;
    
    if (history->current) {
        // Remove any redo states
        EditNode *temp = history->current->next;
        while (temp) {
            EditNode *next = temp->next;
            g_free(temp->text);
            gtk_text_buffer_delete_mark(buffer, temp->start_mark);
            gtk_text_buffer_delete_mark(buffer, temp->end_mark);
            g_free(temp);
            temp = next;
            history->current_size--;
        }
        history->current->next = node;
    } else {
        history->head = node;
    }
    
    history->current = node;
    history->tail = node;
    history->current_size++;
    
    // Trim if too large
    while (history->current_size > history->max_stack_size && history->head != history->current) {
        EditNode *old_head = history->head;
        history->head = old_head->next;
        history->head->prev = NULL;
        g_free(old_head->text);
        gtk_text_buffer_delete_mark(buffer, old_head->start_mark);
        gtk_text_buffer_delete_mark(buffer, old_head->end_mark);
        g_free(old_head);
        history->current_size--;
    }
}

// Undo operation
void edit_undo(GtkWidget *widget, gpointer data) 
{
    (void)widget;
    GtkTextView *text_view = GTK_TEXT_VIEW(data);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
    
    if (history && history->current && history->current->prev) {
        history->current = history->current->prev;
        
        GtkTextIter start, end;
        gtk_text_buffer_get_iter_at_mark(buffer, &start, history->current->start_mark);
        gtk_text_buffer_get_iter_at_mark(buffer, &end, history->current->end_mark);
        
        gtk_text_buffer_set_text(buffer, history->current->text, -1);
    }
}

// Redo operation
void edit_redo(GtkWidget *widget, gpointer data)
{
    (void)widget;
    GtkTextView *text_view = GTK_TEXT_VIEW(data);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
    
    if (history && history->current && history->current->next) {
        history->current = history->current->next;
        
        GtkTextIter start, end;
        gtk_text_buffer_get_iter_at_mark(buffer, &start, history->current->start_mark);
        gtk_text_buffer_get_iter_at_mark(buffer, &end, history->current->end_mark);
        
        gtk_text_buffer_set_text(buffer, history->current->text, -1);
    }
}

// Cut operation
void edit_cut(GtkWidget *widget, gpointer data) 
{
    (void)widget;
    GtkTextView *text_view = GTK_TEXT_VIEW(data);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
    
    gtk_text_buffer_cut_clipboard(buffer, 
        gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), TRUE);
    edit_history_push(buffer);
}

// Copy operation
void edit_copy(GtkWidget *widget, gpointer data)
{
    (void)widget;
    GtkTextView *text_view = GTK_TEXT_VIEW(data);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
    
    gtk_text_buffer_copy_clipboard(buffer, 
        gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
}

// Paste operation
void edit_paste(GtkWidget *widget, gpointer data) 
{
    (void)widget;
    GtkTextView *text_view = GTK_TEXT_VIEW(data);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
    
    gtk_text_buffer_paste_clipboard(buffer, 
        gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), NULL, TRUE);
    edit_history_push(buffer);
}

// Delete operation
void edit_delete(GtkWidget *widget, gpointer data) 
{
    (void)widget;
    GtkTextView *text_view = GTK_TEXT_VIEW(data);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
    
    GtkTextIter start, end;
    if (gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) {
        gtk_text_buffer_delete(buffer, &start, &end);
        edit_history_push(buffer);
    }
}

// Select all
void edit_select_all(GtkWidget *widget, gpointer data)
{
    (void)widget;
    GtkTextView *text_view = GTK_TEXT_VIEW(data);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
    
    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_select_range(buffer, &start, &end);
}

// Find dialog response
static void on_find_response(GtkDialog *dialog, gint response_id, gpointer user_data) 
{
    FindReplaceData *fr_data = (FindReplaceData *)user_data;
    
    if (response_id == GTK_RESPONSE_OK) {
        const gchar *search_text = gtk_entry_get_text(GTK_ENTRY(fr_data->find_entry));
        gboolean match_case = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(fr_data->match_case_check));
        gboolean whole_word = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(fr_data->whole_word_check));
        
        if (search_text && *search_text) {
            GtkTextIter start, match_start, match_end;
            GtkTextBuffer *buffer = fr_data->buffer;
            
            // Start from cursor or beginning
            gtk_text_buffer_get_iter_at_mark(buffer, &start, 
                gtk_text_buffer_get_insert(buffer));
            
            GtkTextIter search_end;
            gtk_text_buffer_get_end_iter(buffer, &search_end);
            
            GtkTextSearchFlags flags = GTK_TEXT_SEARCH_VISIBLE_ONLY;
            if (!match_case) flags |= GTK_TEXT_SEARCH_CASE_INSENSITIVE;
            
            if (gtk_text_iter_forward_search(&start, search_text, flags, 
                &match_start, &match_end, &search_end)) {
                
                // Check for whole word
                if (whole_word) {
                    GtkTextIter prev = match_start;
                    GtkTextIter next = match_end;
                    if (!gtk_text_iter_starts_word(&prev)) {
                        gtk_text_iter_backward_char(&prev);
                    }
                    if (!gtk_text_iter_ends_word(&next)) {
                        gtk_text_iter_forward_char(&next);
                    }
                    
                    if (!gtk_text_iter_starts_word(&prev) || !gtk_text_iter_ends_word(&next)) {
                        // Not a whole word, search again
                        gtk_text_iter_forward_char(&match_end);
                        // Recursive search would go here
                    }
                }
                
                gtk_text_buffer_select_range(buffer, &match_start, &match_end);
                gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(fr_data->window), &match_start, 0.0, TRUE, 0.0, 0.0);
            } else {
                // Wrap around
                gtk_text_buffer_get_start_iter(buffer, &start);
                if (gtk_text_iter_forward_search(&start, search_text, flags, 
                    &match_start, &match_end, &search_end)) {
                    gtk_text_buffer_select_range(buffer, &match_start, &match_end);
                    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(fr_data->window), &match_start, 0.0, TRUE, 0.0, 0.0);
                }
            }
        }
    }
    
    if (response_id == GTK_RESPONSE_DELETE_EVENT || response_id == GTK_RESPONSE_CLOSE) {
        gtk_widget_destroy(GTK_WIDGET(dialog));
        g_free(fr_data);
        find_replace_data = NULL;
    }
}

// Find operation
void edit_find(GtkWidget *widget, gpointer data) 
{
    (void)widget;
    GtkTextView *text_view = GTK_TEXT_VIEW(data);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
    
    if (find_replace_data) {
        gtk_window_present(GTK_WINDOW(find_replace_data->dialog));
        return;
    }
    
    find_replace_data = g_new(FindReplaceData, 1);
    find_replace_data->buffer = buffer;
    find_replace_data->window = GTK_WIDGET(text_view);
    
    // Create dialog
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Find",
        GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(text_view))),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Find", GTK_RESPONSE_OK,
        "_Close", GTK_RESPONSE_CLOSE,
        NULL);
    
    find_replace_data->dialog = dialog;
    
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 10);
    gtk_box_pack_start(GTK_BOX(content), grid, TRUE, TRUE, 0);
    
    // Find entry
    GtkWidget *find_label = gtk_label_new("Find:");
    gtk_widget_set_halign(find_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), find_label, 0, 0, 1, 1);
    
    find_replace_data->find_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(find_replace_data->find_entry), "Search text...");
    gtk_grid_attach(GTK_GRID(grid), find_replace_data->find_entry, 1, 0, 2, 1);
    
    // Options
    find_replace_data->match_case_check = gtk_check_button_new_with_label("Match case");
    gtk_grid_attach(GTK_GRID(grid), find_replace_data->match_case_check, 1, 1, 1, 1);
    
    find_replace_data->whole_word_check = gtk_check_button_new_with_label("Whole word");
    gtk_grid_attach(GTK_GRID(grid), find_replace_data->whole_word_check, 2, 1, 1, 1);
    
    g_signal_connect(dialog, "response", G_CALLBACK(on_find_response), find_replace_data);
    
    gtk_widget_show_all(dialog);
}

// Replace dialog response
static void on_replace_response(GtkDialog *dialog, gint response_id, gpointer user_data) 
{
    FindReplaceData *fr_data = (FindReplaceData *)user_data;
    
    if (response_id == GTK_RESPONSE_OK) {
        const gchar *search_text = gtk_entry_get_text(GTK_ENTRY(fr_data->find_entry));
        const gchar *replace_text = gtk_entry_get_text(GTK_ENTRY(fr_data->replace_entry));
        gboolean match_case = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(fr_data->match_case_check));
        
        if (search_text && *search_text) {
            GtkTextIter start, match_start, match_end;
            GtkTextBuffer *buffer = fr_data->buffer;
            
            if (gtk_text_buffer_get_selection_bounds(buffer, &start, &match_end)) {
                // Replace selected text
                gchar *selected = gtk_text_buffer_get_text(buffer, &start, &match_end, FALSE);
                if (g_strcmp0(selected, search_text) == 0) {
                    gtk_text_buffer_delete(buffer, &start, &match_end);
                    gtk_text_buffer_insert(buffer, &start, replace_text, -1);
                }
                g_free(selected);
            } else {
                // Find and replace next
                gtk_text_buffer_get_iter_at_mark(buffer, &start, 
                    gtk_text_buffer_get_insert(buffer));
                
                GtkTextIter search_end;
                gtk_text_buffer_get_end_iter(buffer, &search_end);
                
                GtkTextSearchFlags flags = GTK_TEXT_SEARCH_VISIBLE_ONLY;
                if (!match_case) flags |= GTK_TEXT_SEARCH_CASE_INSENSITIVE;
                
                if (gtk_text_iter_forward_search(&start, search_text, flags, 
                    &match_start, &match_end, &search_end)) {
                    
                    gtk_text_buffer_delete(buffer, &match_start, &match_end);
                    gtk_text_buffer_insert(buffer, &match_start, replace_text, -1);
                    gtk_text_buffer_select_range(buffer, &match_start, &match_end);
                }
            }
        }
    }
    
    if (response_id == GTK_RESPONSE_DELETE_EVENT || response_id == GTK_RESPONSE_CLOSE) {
        gtk_widget_destroy(GTK_WIDGET(dialog));
        g_free(fr_data);
        find_replace_data = NULL;
    }
}

// Replace operation
void edit_replace(GtkWidget *widget, gpointer data) 
{
    (void)widget;
    GtkTextView *text_view = GTK_TEXT_VIEW(data);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
    
    if (find_replace_data) {
        gtk_window_present(GTK_WINDOW(find_replace_data->dialog));
        return;
    }
    
    find_replace_data = g_new(FindReplaceData, 1);
    find_replace_data->buffer = buffer;
    find_replace_data->window = GTK_WIDGET(text_view);
    
    // Create dialog
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Replace",
        GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(text_view))),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Replace", GTK_RESPONSE_OK,
        "_Close", GTK_RESPONSE_CLOSE,
        NULL);
    
    find_replace_data->dialog = dialog;
    
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 10);
    gtk_box_pack_start(GTK_BOX(content), grid, TRUE, TRUE, 0);
    
    // Find entry
    GtkWidget *find_label = gtk_label_new("Find:");
    gtk_widget_set_halign(find_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), find_label, 0, 0, 1, 1);
    
    find_replace_data->find_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(find_replace_data->find_entry), "Search text...");
    gtk_grid_attach(GTK_GRID(grid), find_replace_data->find_entry, 1, 0, 2, 1);
    
    // Replace entry
    GtkWidget *replace_label = gtk_label_new("Replace:");
    gtk_widget_set_halign(replace_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), replace_label, 0, 1, 1, 1);
    
    find_replace_data->replace_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(find_replace_data->replace_entry), "Replace with...");
    gtk_grid_attach(GTK_GRID(grid), find_replace_data->replace_entry, 1, 1, 2, 1);
    
    // Options
    find_replace_data->match_case_check = gtk_check_button_new_with_label("Match case");
    gtk_grid_attach(GTK_GRID(grid), find_replace_data->match_case_check, 1, 2, 2, 1);
    
    g_signal_connect(dialog, "response", G_CALLBACK(on_replace_response), find_replace_data);
    
    gtk_widget_show_all(dialog);
}

// Callback for Enter key in Go to Line dialog
static void on_goto_entry_activate(GtkEntry *entry, gpointer dialog) 
{
    (void)entry;
    gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
}

// Goto line dialog response - FIXED VERSION
static void on_goto_response(GtkDialog *dialog, gint response_id, gpointer user_data) 
{
    GtkTextView *text_view = GTK_TEXT_VIEW(user_data);
    
    if (response_id == GTK_RESPONSE_OK) {
        GtkWidget *entry = g_object_get_data(G_OBJECT(dialog), "line-entry");
        const gchar *line_text = gtk_entry_get_text(GTK_ENTRY(entry));
        gint line_number = atoi(line_text);
        
        if (line_number > 0) {
            GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
            
            // Get the total number of lines
            gint total_lines = gtk_text_buffer_get_line_count(buffer);
            
            // Clamp line number to valid range
            if (line_number > total_lines) {
                line_number = total_lines;
            }
            
            if (line_number >= 1) {
                GtkTextIter iter;
                gtk_text_buffer_get_iter_at_line(buffer, &iter, line_number - 1);
                gtk_text_buffer_place_cursor(buffer, &iter);
                gtk_text_view_scroll_to_iter(text_view, &iter, 0.0, TRUE, 0.0, 0.0);
            }
        }
    }
    
    // Always destroy dialog for any response (OK, Cancel, or close)
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

// Goto line - FIXED VERSION (no lambda)
void edit_goto_line(GtkWidget *widget, gpointer data) 
{
    (void)widget;
    GtkTextView *text_view = GTK_TEXT_VIEW(data);
    
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Go to Line",
        GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(text_view))),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Go", GTK_RESPONSE_OK,
        "_Cancel", GTK_RESPONSE_CANCEL,
        NULL);
    
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(box), 10);
    gtk_box_pack_start(GTK_BOX(content), box, TRUE, TRUE, 0);
    
    GtkWidget *label = gtk_label_new("Enter line number:");
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
    
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "e.g., 42");
    gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);
    
    // Connect Enter key in entry to activate Go button (using regular function, not lambda)
    g_signal_connect(entry, "activate", G_CALLBACK(on_goto_entry_activate), dialog);
    
    g_object_set_data(G_OBJECT(dialog), "line-entry", entry);
    g_object_set_data(G_OBJECT(dialog), "text-view", text_view);
    
    // Connect dialog response
    g_signal_connect(dialog, "response", G_CALLBACK(on_goto_response), text_view);
    
    gtk_widget_show_all(dialog);
}

// Toggle comment (assumes C-style comments)
void edit_toggle_comment(GtkWidget *widget, gpointer data)
{
    (void)widget;
    GtkTextView *text_view = GTK_TEXT_VIEW(data);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
    
    GtkTextIter start, end;
    if (!gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) {
        gtk_text_buffer_get_iter_at_mark(buffer, &start, 
            gtk_text_buffer_get_insert(buffer));
        end = start;
    }
    
    // Get line bounds
    GtkTextIter line_start = start;
    GtkTextIter line_end = end;
    gtk_text_iter_set_line_offset(&line_start, 0);
    gtk_text_iter_forward_to_line_end(&line_end);
    
    gchar *line_text = gtk_text_buffer_get_text(buffer, &line_start, &line_end, FALSE);
    
    if (g_str_has_prefix(line_text, "//")) {
        // Uncomment
        GtkTextIter delete_start = line_start;
        GtkTextIter delete_end = line_start;
        gtk_text_iter_forward_chars(&delete_end, 2);
        gtk_text_buffer_delete(buffer, &delete_start, &delete_end);
    } else {
        // Comment
        gtk_text_buffer_insert(buffer, &line_start, "//", -1);
    }
    
    g_free(line_text);
    edit_history_push(buffer);
}

// Indent
void edit_indent(GtkWidget *widget, gpointer data) 
{
    (void)widget;
    GtkTextView *text_view = GTK_TEXT_VIEW(data);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
    
    GtkTextIter start, end;
    if (!gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) {
        gtk_text_buffer_get_iter_at_mark(buffer, &start, 
            gtk_text_buffer_get_insert(buffer));
        gtk_text_buffer_get_iter_at_mark(buffer, &end, 
            gtk_text_buffer_get_insert(buffer));
    }
    
    GtkTextIter line_start = start;
    gtk_text_iter_set_line_offset(&line_start, 0);
    gtk_text_buffer_insert(buffer, &line_start, "    ", -1);
    
    edit_history_push(buffer);
}

// Unindent
void edit_unindent(GtkWidget *widget, gpointer data) 
{
    (void)widget;
    GtkTextView *text_view = GTK_TEXT_VIEW(data);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
    
    GtkTextIter start, end;
    if (!gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) {
        gtk_text_buffer_get_iter_at_mark(buffer, &start, 
            gtk_text_buffer_get_insert(buffer));
        end = start;
    }
    
    GtkTextIter line_start = start;
    gtk_text_iter_set_line_offset(&line_start, 0);
    
    // Check for spaces or tabs
    GtkTextIter check = line_start;
    gint spaces = 0;
    while (spaces < 4 && !gtk_text_iter_ends_line(&check)) {
        gunichar c = gtk_text_iter_get_char(&check);
        if (c == ' ') {
            spaces++;
            gtk_text_iter_forward_char(&check);
        } else if (c == '\t') {
            spaces = 4;
            break;
        } else {
            break;
        }
    }
    
    if (spaces > 0) {
        GtkTextIter delete_end = line_start;
        gtk_text_iter_forward_chars(&delete_end, spaces);
        gtk_text_buffer_delete(buffer, &line_start, &delete_end);
    }
    
    edit_history_push(buffer);
}

// Convert to uppercase
void edit_to_uppercase(GtkWidget *widget, gpointer data)
{
    (void)widget;
    GtkTextView *text_view = GTK_TEXT_VIEW(data);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
    
    GtkTextIter start, end;
    if (gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) {
        gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
        for (gint i = 0; text[i]; i++) {
            text[i] = toupper(text[i]);
        }
        gtk_text_buffer_delete(buffer, &start, &end);
        gtk_text_buffer_insert(buffer, &start, text, -1);
        g_free(text);
        edit_history_push(buffer);
    }
}

// Convert to lowercase
void edit_to_lowercase(GtkWidget *widget, gpointer data) 
{
    (void)widget;
    GtkTextView *text_view = GTK_TEXT_VIEW(data);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
    
    GtkTextIter start, end;
    if (gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) {
        gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
        for (gint i = 0; text[i]; i++) {
            text[i] = tolower(text[i]);
        }
        gtk_text_buffer_delete(buffer, &start, &end);
        gtk_text_buffer_insert(buffer, &start, text, -1);
        g_free(text);
        edit_history_push(buffer);
    }
}

// Capitalize words
void edit_capitalize(GtkWidget *widget, gpointer data) 
{
    (void)widget;
    GtkTextView *text_view = GTK_TEXT_VIEW(data);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
    
    GtkTextIter start, end;
    if (gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) {
        gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
        gboolean new_word = TRUE;
        for (gint i = 0; text[i]; i++) {
            if (isalpha(text[i])) {
                if (new_word) {
                    text[i] = toupper(text[i]);
                    new_word = FALSE;
                } else {
                    text[i] = tolower(text[i]);
                }
            } else {
                new_word = TRUE;
            }
        }
        gtk_text_buffer_delete(buffer, &start, &end);
        gtk_text_buffer_insert(buffer, &start, text, -1);
        g_free(text);
        edit_history_push(buffer);
    }
}

// Duplicate line
void edit_duplicate_line(GtkWidget *widget, gpointer data) 
{
    (void)widget;
    GtkTextView *text_view = GTK_TEXT_VIEW(data);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
    
    GtkTextIter cursor;
    gtk_text_buffer_get_iter_at_mark(buffer, &cursor, 
        gtk_text_buffer_get_insert(buffer));
    
    GtkTextIter line_start = cursor;
    GtkTextIter line_end = cursor;
    gtk_text_iter_set_line_offset(&line_start, 0);
    gtk_text_iter_forward_to_line_end(&line_end);
    gtk_text_iter_forward_char(&line_end); // Include newline
    
    gchar *line = gtk_text_buffer_get_text(buffer, &line_start, &line_end, FALSE);
    gtk_text_buffer_insert(buffer, &line_end, line, -1);
    g_free(line);
    
    edit_history_push(buffer);
}

// Delete line
void edit_delete_line(GtkWidget *widget, gpointer data) 
{
    (void)widget;
    GtkTextView *text_view = GTK_TEXT_VIEW(data);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
    
    GtkTextIter cursor;
    gtk_text_buffer_get_iter_at_mark(buffer, &cursor, 
        gtk_text_buffer_get_insert(buffer));
    
    GtkTextIter line_start = cursor;
    GtkTextIter line_end = cursor;
    gtk_text_iter_set_line_offset(&line_start, 0);
    gtk_text_iter_forward_to_line_end(&line_end);
    gtk_text_iter_forward_char(&line_end); // Include newline
    
    gtk_text_buffer_delete(buffer, &line_start, &line_end);
    edit_history_push(buffer);
}

// Join lines
void edit_join_lines(GtkWidget *widget, gpointer data)
{
    (void)widget;
    GtkTextView *text_view = GTK_TEXT_VIEW(data);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
    
    GtkTextIter start, end;
    if (!gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) {
        gtk_text_buffer_get_iter_at_mark(buffer, &start, 
            gtk_text_buffer_get_insert(buffer));
        end = start;
    }
    
    GtkTextIter line_start = start;
    GtkTextIter line_end = end;
    gtk_text_iter_set_line_offset(&line_start, 0);
    
    // Find the end of the last line
    gtk_text_iter_forward_line(&line_end);
    
    // Replace newlines with spaces
    gchar *text = gtk_text_buffer_get_text(buffer, &line_start, &line_end, FALSE);
    for (gint i = 0; text[i]; i++) {
        if (text[i] == '\n') text[i] = ' ';
    }
    
    gtk_text_buffer_delete(buffer, &line_start, &line_end);
    gtk_text_buffer_insert(buffer, &line_start, text, -1);
    g_free(text);
    
    edit_history_push(buffer);
}

// Comparison function for sort
static gint compare_lines(gconstpointer a, gconstpointer b) 
{
    return g_strcmp0(*(const gchar**)a, *(const gchar**)b);
}

// Sort lines
void edit_sort_lines(GtkWidget *widget, gpointer data) 
{
    (void)widget;
    GtkTextView *text_view = GTK_TEXT_VIEW(data);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
    
    GtkTextIter start, end;
    if (!gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) {
        gtk_text_buffer_get_start_iter(buffer, &start);
        gtk_text_buffer_get_end_iter(buffer, &end);
    }
    
    GtkTextIter line_start = start;
    GtkTextIter line_end = end;
    gtk_text_iter_set_line_offset(&line_start, 0);
    
    // Get all lines
    GPtrArray *lines = g_ptr_array_new();
    GtkTextIter current = line_start;
    
    while (gtk_text_iter_compare(&current, &line_end) < 0) {
        GtkTextIter next_line = current;
        gtk_text_iter_forward_line(&next_line);
        
        gchar *line = gtk_text_buffer_get_text(buffer, &current, &next_line, FALSE);
        g_ptr_array_add(lines, line);
        
        current = next_line;
    }
    
    // Sort lines
    g_ptr_array_sort(lines, compare_lines);
    
    // Rebuild text
    gtk_text_buffer_delete(buffer, &line_start, &line_end);
    
    for (guint i = 0; i < lines->len; i++) {
        gchar *line = g_ptr_array_index(lines, i);
        gtk_text_buffer_insert(buffer, &line_start, line, -1);
        g_free(line);
    }
    
    g_ptr_array_free(lines, TRUE);
    edit_history_push(buffer);
}

// Draw page callback for printing
static void on_draw_page(GtkPrintOperation *operation, GtkPrintContext *context, gint page_nr, gpointer user_data) 
{
    (void)operation;
    (void)page_nr;
    PrintData *data = (PrintData *)user_data;
    
    cairo_t *cr = gtk_print_context_get_cairo_context(context);
    double width = gtk_print_context_get_width(context);
    double height = gtk_print_context_get_height(context);
    
    // Set up text rendering
    cairo_set_source_rgb(cr, 0, 0, 0); // Black text
    
    // Get text from buffer
    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(data->buffer, &start);
    gtk_text_buffer_get_end_iter(data->buffer, &end);
    gchar *text = gtk_text_buffer_get_text(data->buffer, &start, &end, FALSE);
    
    // Create Pango layout for text wrapping
    PangoLayout *layout = gtk_print_context_create_pango_layout(context);
    pango_layout_set_text(layout, text, -1);
    pango_layout_set_width(layout, (width - 100) * PANGO_SCALE); // Margins
    
    // Set font
    PangoFontDescription *font_desc = pango_font_description_from_string("Monospace 10");
    pango_layout_set_font_description(layout, font_desc);
    pango_font_description_free(font_desc);
    
    // Set line spacing
    pango_layout_set_spacing(layout, 2 * PANGO_SCALE);
    
    // Draw text
    cairo_move_to(cr, 50, 50); // Top margin
    pango_cairo_show_layout(cr, layout);
    
    // Add page numbers
    int n_pages = gtk_print_operation_get_n_pages_to_print(operation);
    if (n_pages > 1) {
        gchar *page_text = g_strdup_printf("Page %d of %d", page_nr + 1, n_pages);
        
        cairo_move_to(cr, width - 100, height - 30);
        
        // Create a new layout for page number
        PangoLayout *page_layout = gtk_print_context_create_pango_layout(context);
        pango_layout_set_text(page_layout, page_text, -1);
        
        // Set smaller font for page numbers
        PangoFontDescription *page_font = pango_font_description_from_string("Sans 8");
        pango_layout_set_font_description(page_layout, page_font);
        pango_font_description_free(page_font);
        
        pango_cairo_show_layout(cr, page_layout);
        
        g_object_unref(page_layout);
        g_free(page_text);
    }
    
    g_object_unref(layout);
    g_free(text);
}

// Begin print operation - calculate pages
static void on_begin_print_simple(GtkPrintOperation *operation, GtkPrintContext *context, gpointer user_data) 
{
    PrintData *data = (PrintData *)user_data;
    
    // Get text from buffer
    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(data->buffer, &start);
    gtk_text_buffer_get_end_iter(data->buffer, &end);
    gchar *text = gtk_text_buffer_get_text(data->buffer, &start, &end, FALSE);
    
    // Rough estimate: count characters and assume ~3000 chars per page
    int char_count = strlen(text);
    int n_pages = (char_count / 3000) + 1;
    if (n_pages < 1) n_pages = 1;
    
    gtk_print_operation_set_n_pages(operation, n_pages);
    
    g_free(text);
    (void)context;
}

// Print operation
void edit_print(GtkWidget *widget, gpointer data) 
{
    (void)widget;
    GtkTextView *text_view = GTK_TEXT_VIEW(data);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
    
    // Create print operation
    GtkPrintOperation *print = gtk_print_operation_new();
    
    // Create print data
    PrintData *print_data = g_new(PrintData, 1);
    print_data->text_view = text_view;
    print_data->buffer = buffer;
    
    // Set up print settings (simplified - no margin functions)
    GtkPrintSettings *settings = gtk_print_settings_new();
    gtk_print_settings_set_paper_size(settings, 
        gtk_paper_size_new(gtk_paper_size_get_default()));
    gtk_print_settings_set_orientation(settings, GTK_PAGE_ORIENTATION_PORTRAIT);
    
    gtk_print_operation_set_print_settings(print, settings);
    gtk_print_operation_set_allow_async(print, TRUE);
    gtk_print_operation_set_unit(print, GTK_UNIT_POINTS);
    gtk_print_operation_set_use_full_page(print, TRUE);
    gtk_print_operation_set_embed_page_setup(print, TRUE);
    
    // Connect signals
    g_signal_connect(print, "begin-print", G_CALLBACK(on_begin_print_simple), print_data);
    g_signal_connect(print, "draw-page", G_CALLBACK(on_draw_page), print_data);
    
    // Run print dialog
    GError *error = NULL;
    GtkPrintOperationResult res = gtk_print_operation_run(print, 
        GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
        GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(text_view))), 
        &error);
    
    if (res == GTK_PRINT_OPERATION_RESULT_ERROR) {
        GtkWidget *error_dialog = gtk_message_dialog_new(
            GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(text_view))),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            "Error printing: %s", error->message);
        g_signal_connect(error_dialog, "response", G_CALLBACK(gtk_widget_destroy), NULL);
        gtk_widget_show(error_dialog);
        g_error_free(error);
    }
    
    g_object_unref(settings);
    g_object_unref(print);
    g_free(print_data);
}

// Initialize edit features
void edit_init(GtkTextBuffer *buffer) 
{
    global_buffer = buffer;
    history = edit_history_new(50);
    
    // Connect to buffer changes for undo history
    g_signal_connect_swapped(buffer, "changed", G_CALLBACK(edit_history_push), buffer);
}