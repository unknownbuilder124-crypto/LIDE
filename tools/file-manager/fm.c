#include "fm.h"
#include "window_resize.h"
#include "recycle_bin.h"
#include <gio/gio.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
/* Forward declaration of browser function */
void browser_open_file(FileManager *fm, const gchar *path);

/* Clipboard for cut/copy */
static gchar *clipboard_path = NULL;
static gboolean clipboard_is_cut = FALSE;

/* History management */
typedef struct {
    GFile *location;
    GtkTreePath *scroll_position;
} HistoryEntry;

static GList *history = NULL;
static GList *current_history_pos = NULL;

/* Function prototypes for new features */
static void fm_show_context_menu(FileManager *fm, GdkEventButton *event, const gchar *selected_path, gboolean is_directory);
static void fm_cut_file(FileManager *fm, const gchar *path);
static void fm_copy_file(const gchar *path);
static void fm_paste_file(FileManager *fm, const gchar *dest_dir);
static void fm_move_to_trash(const gchar *path);
static void fm_delete_permanently(const gchar *path);
static void fm_open_in_terminal(const gchar *path);
static void fm_show_properties(const gchar *path);
static void fm_new_folder(FileManager *fm);
static void fm_new_file(FileManager *fm);
static void fm_load_directory_contents(FileManager *fm);
static void fm_update_navigation_buttons(FileManager *fm);
static void fm_add_to_history(FileManager *fm, GFile *file);
static void fm_empty_trash(FileManager *fm);

/* Helper functions for formatting */

/**
 * Formats file size to human-readable string.
 *
 * @param size Size in bytes.
 * @return Newly allocated string with formatted size.
 *         Caller must free with g_free().
 */
static gchar *format_size(guint64 size)

{
    if (size < 1024) return g_strdup_printf("%lu B", size);
    if (size < 1024 * 1024) return g_strdup_printf("%.1f KB", size / 1024.0);
    if (size < 1024 * 1024 * 1024) return g_strdup_printf("%.1f MB", size / (1024.0 * 1024.0));
    return g_strdup_printf("%.1f GB", size / (1024.0 * 1024.0 * 1024.0));
}

/**
 * Displays an input dialog to get user input.
 *
 * @param parent Parent window for the dialog.
 * @param title  Dialog title.
 * @param prompt Prompt text to display.
 * @return Newly allocated string with user input, or NULL if canceled.
 */
static gchar* show_input_dialog(GtkWindow *parent, const gchar *title, const gchar *prompt)

{
    GtkWidget *dialog = gtk_dialog_new_with_buttons(title,
                                                     parent,
                                                     GTK_DIALOG_MODAL,
                                                     "_Cancel", GTK_RESPONSE_CANCEL,
                                                     "_OK", GTK_RESPONSE_ACCEPT,
                                                     NULL);
    
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(box), 10);
    gtk_box_pack_start(GTK_BOX(content), box, TRUE, TRUE, 0);
    
    GtkWidget *label = gtk_label_new(prompt);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
    
    GtkWidget *entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);
    
    gtk_widget_show_all(dialog);
    
    gchar *result = NULL;
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        result = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
    }
    
    gtk_widget_destroy(dialog);
    return result;
}

/* Dragging functions */

/**
 * Callback for mouse button press on file manager window.
 * Initiates window dragging or resizing.
 *
 * @param widget The window widget.
 * @param event  Button event details.
 * @param data   FileManager instance.
 * @return       TRUE to stop event propagation.
 */
static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data)

{
    FileManager *fm = (FileManager *)data;

    if (event->button == 1)
    {
        fm->resize_edge = detect_resize_edge_absolute(GTK_WINDOW(fm->window), event->x_root, event->y_root);

        if (fm->resize_edge != RESIZE_NONE) {
            fm->is_resizing = 1;
        } else {
            fm->is_dragging = 1;
        }

        fm->drag_start_x = event->x_root;
        fm->drag_start_y = event->y_root;
        gtk_window_present(GTK_WINDOW(fm->window));
        return TRUE;
    }
    return FALSE;
}

/**
 * Callback for mouse button release on file manager window.
 * Terminates window dragging or resizing.
 *
 * @param widget The window widget.
 * @param event  Button event details.
 * @param data   FileManager instance.
 * @return       TRUE to stop event propagation.
 */
static gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer data)

{
    FileManager *fm = (FileManager *)data;

    if (event->button == 1) {
        fm->is_dragging = 0;
        fm->is_resizing = 0;
        fm->resize_edge = RESIZE_NONE;
        return TRUE;
    }
    return FALSE;
}

/**
 * Callback for mouse motion on file manager window.
 * Handles window dragging, resizing, and cursor updates.
 *
 * @param widget The window widget.
 * @param event  Motion event details.
 * @param data   FileManager instance.
 * @return       TRUE if event was handled.
 */
static gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer data)

{
    FileManager *fm = (FileManager *)data;

    /* Update cursor for resize hints when not actively dragging/resizing */
    if (!fm->is_dragging && !fm->is_resizing) {
        int resize_edge = detect_resize_edge_absolute(GTK_WINDOW(fm->window), event->x_root, event->y_root);
        update_resize_cursor(widget, resize_edge);
    }

    if (fm->is_resizing) {
        int delta_x = event->x_root - fm->drag_start_x;
        int delta_y = event->y_root - fm->drag_start_y;

        int window_width, window_height;
        gtk_window_get_size(GTK_WINDOW(fm->window), &window_width, &window_height);

        apply_window_resize(GTK_WINDOW(fm->window), fm->resize_edge,
                           delta_x, delta_y, window_width, window_height);

        fm->drag_start_x = event->x_root;
        fm->drag_start_y = event->y_root;
        return TRUE;
    } else if (fm->is_dragging) {
        int dx = event->x_root - fm->drag_start_x;
        int dy = event->y_root - fm->drag_start_y;

        int x, y;
        gtk_window_get_position(GTK_WINDOW(fm->window), &x, &y);
        gtk_window_move(GTK_WINDOW(fm->window), x + dx, y + dy);

        fm->drag_start_x = event->x_root;
        fm->drag_start_y = event->y_root;
        return TRUE;
    }
    return FALSE;
}

/* Window control callbacks */

/**
 * Callback for minimize button click.
 *
 * @param button The button that was clicked.
 * @param window The window to minimize.
 */
static void on_minimize_clicked(GtkButton *button, gpointer window)

{
    (void)button;
    gtk_window_iconify(GTK_WINDOW(window));
}

/**
 * Callback for maximize/restore button click.
 * Toggles between maximized and restored states.
 *
 * @param button The button that was clicked.
 * @param window The window to maximize or restore.
 */
static void on_maximize_clicked(GtkButton *button, gpointer window)

{
    (void)button;
    GtkWindow *win = GTK_WINDOW(window);
    
    if (gtk_window_is_maximized(win)) {
        gtk_window_unmaximize(win);
    } else {
        gtk_window_maximize(win);
    }
}

/**
 * Callback for window state changes.
 * Updates the maximize button label when window is maximized or restored.
 *
 * @param window The window whose state changed.
 * @param event  Window state event details.
 * @param data   Maximize button widget.
 * @return       FALSE to allow further processing.
 */
static gboolean on_window_state_changed(GtkWidget *window, GdkEventWindowState *event, gpointer data)

{
    GtkButton *max_btn = GTK_BUTTON(data);
    
    if (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) {
        gtk_button_set_label(max_btn, "❐");
        gtk_widget_set_tooltip_text(GTK_WIDGET(max_btn), "Restore");
    } else {
        gtk_button_set_label(max_btn, "□");
        gtk_widget_set_tooltip_text(GTK_WIDGET(max_btn), "Maximize");
    }
    
    return FALSE;
}

/**
 * Callback for close button click.
 *
 * @param button The button that was clicked.
 * @param window The window to close.
 */
static void on_close_clicked(GtkButton *button, gpointer window)

{
    (void)button;
    gtk_window_close(GTK_WINDOW(window));
}

/**
 * Updates the state of navigation buttons (back/forward) based on history position.
 *
 * @param fm FileManager instance.
 */
static void fm_update_navigation_buttons(FileManager *fm)

{
    gtk_widget_set_sensitive(fm->back_button, (current_history_pos != NULL && current_history_pos->prev != NULL));
    gtk_widget_set_sensitive(fm->forward_button, (current_history_pos != NULL && current_history_pos->next != NULL));
}

/**
 * Adds the current location to navigation history.
 *
 * @param fm   FileManager instance.
 * @param file Current directory GFile.
 */
static void fm_add_to_history(FileManager *fm, GFile *file)

{
    /* If we're not at the end of history, truncate forward history */
    if (current_history_pos && current_history_pos->next) {
        GList *next = current_history_pos->next;
        GList *tmp;
        while (next) {
            tmp = next->next;
            HistoryEntry *entry = (HistoryEntry *)next->data;
            if (entry->location) g_object_unref(entry->location);
            if (entry->scroll_position) gtk_tree_path_free(entry->scroll_position);
            g_free(entry);
            g_list_free_1(next);
            next = tmp;
        }
        current_history_pos->next = NULL;
    }
    
    /* Create new history entry */
    HistoryEntry *entry = g_new(HistoryEntry, 1);
    entry->location = g_object_ref(file);
    
    /* Save current scroll position */
    GtkAdjustment *vadjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(
        gtk_widget_get_parent(GTK_WIDGET(fm->main_tree))));
    if (vadjust) {
        gdouble value = gtk_adjustment_get_value(vadjust);
        GtkTreePath *path = NULL;
        gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(fm->main_tree), 10, value, &path, NULL, NULL, NULL);
        entry->scroll_position = path ? gtk_tree_path_copy(path) : NULL;
        if (path) gtk_tree_path_free(path);
    } else {
        entry->scroll_position = NULL;
    }
    
    /* Add to history */
    history = g_list_append(history, entry);
    current_history_pos = g_list_last(history);
    
    fm_update_navigation_buttons(fm);
}

/**
 * Populates the sidebar with places including Home, Root, Recent, Starred, and Trash.
 *
 * @param fm FileManager instance.
 */
void fm_populate_sidebar(FileManager *fm)

{
    gtk_list_store_clear(fm->sidebar_store);

    GtkTreeIter iter;

    /* Home */
    const gchar *home = g_get_home_dir();
    gtk_list_store_append(fm->sidebar_store, &iter);
    gtk_list_store_set(fm->sidebar_store, &iter, 0, "Home", -1);
    g_object_set_data_full(G_OBJECT(fm->sidebar_tree), "path_Home", g_strdup(home), g_free);

    /* Root */
    gtk_list_store_append(fm->sidebar_store, &iter);
    gtk_list_store_set(fm->sidebar_store, &iter, 0, "Root", -1);
    g_object_set_data_full(G_OBJECT(fm->sidebar_tree), "path_Root", g_strdup("/"), g_free);

    /* Recent */
    gtk_list_store_append(fm->sidebar_store, &iter);
    gtk_list_store_set(fm->sidebar_store, &iter, 0, "Recent", -1);

    /* Starred */
    gtk_list_store_append(fm->sidebar_store, &iter);
    gtk_list_store_set(fm->sidebar_store, &iter, 0, "Starred", -1);

    /* Trash */
    gchar *trash_path = g_build_filename(g_get_home_dir(), ".local/share/Trash/files", NULL);
    gtk_list_store_append(fm->sidebar_store, &iter);
    gtk_list_store_set(fm->sidebar_store, &iter, 0, "Trash", -1);
    g_object_set_data_full(G_OBJECT(fm->sidebar_tree), "path_Trash", trash_path, g_free);
}

/**
 * Handles sidebar row activation (click).
 *
 * @param tree The sidebar tree view.
 * @param path Activated tree path.
 * @param col  Tree view column.
 * @param fm   FileManager instance.
 */
void fm_on_sidebar_row_activated(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *col, FileManager *fm)

{
    GtkTreeIter iter;
    if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(fm->sidebar_store), &iter, path))
        return;

    gchar *text;
    gtk_tree_model_get(GTK_TREE_MODEL(fm->sidebar_store), &iter, 0, &text, -1);

    if (g_strcmp0(text, "Home") == 0) {
        fm_go_home(fm);
    } else if (g_strcmp0(text, "Root") == 0) {
        fm_open_location(fm, "/");
    } else if (g_strcmp0(text, "Recent") == 0) {
        gtk_label_set_text(GTK_LABEL(fm->status_label), "Recent files");
    } else if (g_strcmp0(text, "Starred") == 0) {
        gtk_label_set_text(GTK_LABEL(fm->status_label), "Starred items");
    } else if (g_strcmp0(text, "Trash") == 0) {
        gchar *trash_path = g_object_get_data(G_OBJECT(tree), "path_Trash");
        if (trash_path)
            fm_open_location(fm, trash_path);
    }

    g_free(text);
}

/**
 * Loads directory contents into the main tree view.
 * Handles special case for home directory showing XDG user directories.
 *
 * @param fm FileManager instance.
 */
static void fm_load_directory_contents(FileManager *fm)

{
    gtk_tree_store_clear(fm->main_store);

    const gchar *home = g_get_home_dir();
    gchar *current_dir_path = g_file_get_path(fm->current_dir);
    gboolean is_home = (g_strcmp0(current_dir_path, home) == 0);
    g_free(current_dir_path);

    if (is_home) {
        /* Show standard XDG user directories */
        GtkTreeIter iter;

        /* Define XDG user directories */
        const gchar *dir_names[] = {
            "Desktop", "Documents", "Downloads", "Music",
            "Pictures", "Public", "Videos", NULL
        };

        for (int i = 0; dir_names[i] != NULL; i++) {
            gchar *path = g_build_filename(home, dir_names[i], NULL);
            if (g_file_test(path, G_FILE_TEST_IS_DIR)) {
                GFile *file = g_file_new_for_path(path);
                GFileInfo *info = g_file_query_info(file,
                                                    "time::modified",
                                                    G_FILE_QUERY_INFO_NONE,
                                                    NULL, NULL);
                gchar *modified = NULL;
                if (info) {
                    GDateTime *dt = g_file_info_get_modification_date_time(info);
                    if (dt)
                        modified = g_date_time_format(dt, "%Y-%m-%d %H:%M");
                    g_object_unref(info);
                }
                if (!modified)
                    modified = g_strdup("");

                gtk_tree_store_append(fm->main_store, &iter, NULL);
                gtk_tree_store_set(fm->main_store, &iter,
                                   0, dir_names[i],
                                   1, "",
                                   2, "Folder",
                                   3, modified,
                                   4, path,
                                   -1);
                g_free(modified);
                g_object_unref(file);
            }
            g_free(path);
        }
    } else {
        /* Normal directory listing */
        GFileEnumerator *enumerator = g_file_enumerate_children(fm->current_dir,
                                                                "standard::name,standard::size,standard::content-type,time::modified",
                                                                G_FILE_QUERY_INFO_NONE,
                                                                NULL, NULL);
        if (!enumerator) {
            return;
        }

        GFileInfo *info;
        while ((info = g_file_enumerator_next_file(enumerator, NULL, NULL)) != NULL) {
            const gchar *name = g_file_info_get_name(info);
            guint64 size = g_file_info_get_size(info);
            const gchar *content_type = g_file_info_get_content_type(info);
            GDateTime *modified_dt = g_file_info_get_modification_date_time(info);
            GFileType file_type = g_file_info_get_file_type(info);

            gchar *size_str = (file_type == G_FILE_TYPE_DIRECTORY) ? g_strdup("") : format_size(size);
            gchar *modified = modified_dt ? g_date_time_format(modified_dt, "%Y-%m-%d %H:%M") : g_strdup("");
            gchar *type_str = (file_type == G_FILE_TYPE_DIRECTORY) ? g_strdup("Folder") : 
                              (file_type == G_FILE_TYPE_SYMBOLIC_LINK) ? g_strdup("Link") :
                              g_content_type_get_description(content_type);

            current_dir_path = g_file_get_path(fm->current_dir);
            gchar *full_path = g_build_filename(current_dir_path, name, NULL);
            g_free(current_dir_path);

            GtkTreeIter iter;
            gtk_tree_store_append(fm->main_store, &iter, NULL);
            gtk_tree_store_set(fm->main_store, &iter,
                               0, name,
                               1, size_str,
                               2, type_str,
                               3, modified,
                               4, full_path,
                               -1);

            g_free(size_str);
            g_free(modified);
            g_free(type_str);
            g_free(full_path);
            g_object_unref(info);
        }

        g_file_enumerator_close(enumerator, NULL, NULL);
        g_object_unref(enumerator);
    }

    current_dir_path = g_file_get_path(fm->current_dir);
    gtk_label_set_text(GTK_LABEL(fm->status_label), current_dir_path);
    g_free(current_dir_path);
}

/**
 * Opens a location and loads its contents.
 *
 * @param fm   FileManager instance.
 * @param path Path to open.
 */
void fm_open_location(FileManager *fm, const gchar *path)

{
    if (!path) return;

    GFile *new_dir = g_file_new_for_path(path);
    
    if (!g_file_query_exists(new_dir, NULL)) {
        g_object_unref(new_dir);
        return;
    }

    /* Add current location to history before changing */
    if (fm->current_dir) {
        fm_add_to_history(fm, fm->current_dir);
        g_object_unref(fm->current_dir);
    }
    
    fm->current_dir = g_object_ref(new_dir);
    fm_load_directory_contents(fm);

    /* Update location entry */
    gtk_entry_set_text(GTK_ENTRY(fm->location_entry), path);
    
    g_object_unref(new_dir);
}

/**
 * Handles row activation (double-click or Enter key).
 *
 * @param tree_view The tree view.
 * @param path      Activated tree path.
 * @param column    Tree view column.
 * @param fm        FileManager instance.
 */
void fm_on_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, FileManager *fm)

{
    GtkTreeIter iter;
    if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(fm->main_store), &iter, path))
        return;

    gchar *full_path;
    gtk_tree_model_get(GTK_TREE_MODEL(fm->main_store), &iter, 4, &full_path, -1);

    if (full_path) {
        if (g_file_test(full_path, G_FILE_TEST_IS_DIR)) {
            fm_open_location(fm, full_path);
        } else {
            /* Call external open function from browser.c */
            browser_open_file(fm, full_path);
        }
        g_free(full_path);
    }
}

/**
 * Creates a new folder in the current directory.
 *
 * @param fm FileManager instance.
 */
static void fm_new_folder(FileManager *fm)

{
    gchar *name = show_input_dialog(GTK_WINDOW(fm->window), "New Folder", "Enter folder name:");
    if (name && strlen(name) > 0) {
        gchar *current_path = g_file_get_path(fm->current_dir);
        gchar *new_path = g_build_filename(current_path, name, NULL);
        
        if (mkdir(new_path, 0755) == 0) {
            fm_refresh(fm);
        }
        
        g_free(new_path);
        g_free(current_path);
        g_free(name);
    }
}

/**
 * Creates a new empty file in the current directory.
 *
 * @param fm FileManager instance.
 */
static void fm_new_file(FileManager *fm)

{
    gchar *name = show_input_dialog(GTK_WINDOW(fm->window), "New File", "Enter file name:");
    if (name && strlen(name) > 0) {
        gchar *current_path = g_file_get_path(fm->current_dir);
        gchar *new_path = g_build_filename(current_path, name, NULL);
        
        FILE *f = fopen(new_path, "w");
        if (f) {
            fclose(f);
            fm_refresh(fm);
        }
        
        g_free(new_path);
        g_free(current_path);
        g_free(name);
    }
}

/**
 * Displays the context menu for the main view.
 *
 * @param fm           FileManager instance.
 * @param event        Button event that triggered the menu.
 * @param selected_path Path of selected item, or NULL if clicking empty space.
 * @param is_directory  TRUE if selected item is a directory.
 */
static void fm_show_context_menu(FileManager *fm, GdkEventButton *event, const gchar *selected_path, gboolean is_directory)

{
    GtkWidget *menu = gtk_menu_new();
    GtkWidget *item;

    if (selected_path) {
        /* File/Directory specific options */
        item = gtk_menu_item_new_with_label("Open");
        g_signal_connect_swapped(item, "activate", G_CALLBACK(browser_open_file), fm);
        g_object_set_data_full(G_OBJECT(item), "path", g_strdup(selected_path), g_free);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

        item = gtk_menu_item_new_with_label("Cut");
        g_signal_connect_swapped(item, "activate", G_CALLBACK(fm_cut_file), fm);
        g_object_set_data_full(G_OBJECT(item), "path", g_strdup(selected_path), g_free);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

        item = gtk_menu_item_new_with_label("Copy");
        g_signal_connect_swapped(item, "activate", G_CALLBACK(fm_copy_file), (gpointer)selected_path);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

        item = gtk_menu_item_new_with_label("Rename");
        g_signal_connect_swapped(item, "activate", G_CALLBACK(fm_new_folder), fm); /* Placeholder */
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

        /* Special handling for trash directory: show Empty Trash instead of Delete */
        if (is_trash_directory(selected_path)) {
            item = gtk_menu_item_new_with_label("Empty Trash");
            g_signal_connect_swapped(item, "activate", G_CALLBACK(fm_empty_trash), fm);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        } else {
            item = gtk_menu_item_new_with_label("Delete");
            g_signal_connect_swapped(item, "activate", G_CALLBACK(fm_move_to_trash), (gpointer)selected_path);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        }

        gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

        item = gtk_menu_item_new_with_label("Properties");
        g_signal_connect_swapped(item, "activate", G_CALLBACK(fm_show_properties), (gpointer)selected_path);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    }

    /* Always show these options (for empty space or as additional options) */
    if (!selected_path) {
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
    }

    item = gtk_menu_item_new_with_label("New Folder");
    g_signal_connect_swapped(item, "activate", G_CALLBACK(fm_new_folder), fm);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_menu_item_new_with_label("New File");
    g_signal_connect_swapped(item, "activate", G_CALLBACK(fm_new_file), fm);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    if (clipboard_path) {
        if (!selected_path) {
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
        }
        
        item = gtk_menu_item_new_with_label("Paste");
        g_signal_connect_swapped(item, "activate", G_CALLBACK(fm_paste_file), fm);
        gchar *current_dir_path = g_file_get_path(fm->current_dir);
        g_object_set_data_full(G_OBJECT(item), "dest", current_dir_path, g_free);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    }

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    item = gtk_menu_item_new_with_label("Open in Terminal");
    if (selected_path) {
        g_signal_connect_swapped(item, "activate", G_CALLBACK(fm_open_in_terminal), (gpointer)selected_path);
    } else {
        gchar *current_path = g_file_get_path(fm->current_dir);
        g_signal_connect_swapped(item, "activate", G_CALLBACK(fm_open_in_terminal), (gpointer)current_path);
        g_free(current_path);
    }
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    gtk_widget_show_all(menu);
    gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent*)event);
}

/**
 * Callback for right-click on main tree view.
 * Shows context menu for the selected item or empty space.
 *
 * @param widget The tree view.
 * @param event  Button event.
 * @param fm     FileManager instance.
 * @return       TRUE to stop event propagation.
 */
static gboolean on_main_tree_button_press(GtkWidget *widget, GdkEventButton *event, FileManager *fm)

{
    if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
        GtkTreePath *path;
        if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget), event->x, event->y, &path, NULL, NULL, NULL)) {
            gtk_tree_view_set_cursor(GTK_TREE_VIEW(widget), path, NULL, FALSE);
            GtkTreeIter iter;
            if (gtk_tree_model_get_iter(GTK_TREE_MODEL(fm->main_store), &iter, path)) {
                gchar *full_path;
                gchar *type_str;
                gtk_tree_model_get(GTK_TREE_MODEL(fm->main_store), &iter, 2, &type_str, 4, &full_path, -1);
                gboolean is_dir = (g_strcmp0(type_str, "Folder") == 0);
                fm_show_context_menu(fm, event, full_path, is_dir);
                g_free(full_path);
                g_free(type_str);
            }
            gtk_tree_path_free(path);
        } else {
            /* Click on empty space */
            fm_show_context_menu(fm, event, NULL, FALSE);
        }
        return TRUE;
    }
    return FALSE;
}

/* Action functions */

/**
 * Cuts a file or directory (marks for move operation).
 *
 * @param fm   FileManager instance.
 * @param path Path to cut.
 */
static void fm_cut_file(FileManager *fm, const gchar *path)

{
    if (clipboard_path)
        g_free(clipboard_path);
    clipboard_path = g_strdup(path);
    clipboard_is_cut = TRUE;
    gtk_label_set_text(GTK_LABEL(fm->status_label), "Cut: ready to paste");
}

/**
 * Copies a file or directory.
 *
 * @param path Path to copy.
 */
static void fm_copy_file(const gchar *path)

{
    if (clipboard_path)
        g_free(clipboard_path);
    clipboard_path = g_strdup(path);
    clipboard_is_cut = FALSE;
}

/**
 * Pastes the cut or copied file to the destination directory.
 *
 * @param fm       FileManager instance.
 * @param dest_dir Destination directory path.
 */
static void fm_paste_file(FileManager *fm, const gchar *dest_dir)

{
    if (!clipboard_path) return;

    GFile *src = g_file_new_for_path(clipboard_path);
    GFile *dest = g_file_new_for_path(dest_dir);
    GFile *dest_file = g_file_get_child(dest, g_path_get_basename(clipboard_path));

    GError *error = NULL;
    if (clipboard_is_cut) {
        if (!g_file_move(src, dest_file, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, &error)) {
            g_printerr("Move failed: %s\n", error->message);
            g_error_free(error);
        } else {
            gtk_label_set_text(GTK_LABEL(fm->status_label), "Moved successfully");
        }
    } else {
        if (!g_file_copy(src, dest_file, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, &error)) {
            g_printerr("Copy failed: %s\n", error->message);
            g_error_free(error);
        } else {
            gtk_label_set_text(GTK_LABEL(fm->status_label), "Copied successfully");
        }
    }

    g_object_unref(src);
    g_object_unref(dest);
    g_object_unref(dest_file);

    g_free(clipboard_path);
    clipboard_path = NULL;
    clipboard_is_cut = FALSE;

    fm_refresh(fm);
}

/**
 * Moves a file to the trash.
 *
 * @param path Path to trash.
 */
static void fm_move_to_trash(const gchar *path)

{
    GFile *file = g_file_new_for_path(path);
    GError *error = NULL;
    if (!g_file_trash(file, NULL, &error)) {
        g_printerr("Trash failed: %s\n", error->message);
        g_error_free(error);
    }
    g_object_unref(file);
}

/**
 * Permanently deletes a file (bypasses trash).
 *
 * @param path Path to delete.
 */
static void fm_delete_permanently(const gchar *path)

{
    GFile *file = g_file_new_for_path(path);
    GError *error = NULL;
    if (!g_file_delete(file, NULL, &error)) {
        g_printerr("Delete failed: %s\n", error->message);
        g_error_free(error);
    }
    g_object_unref(file);
}

/**
 * Empties the trash permanently.
 *
 * @param fm FileManager instance.
 */
static void fm_empty_trash(FileManager *fm)
{
    if (empty_trash()) {
        gtk_label_set_text(GTK_LABEL(fm->status_label), "Trash emptied");
        fm_refresh(fm);
    } else {
        gtk_label_set_text(GTK_LABEL(fm->status_label), "Failed to empty trash");
    }
}

/**
 * Opens a terminal in the specified directory.
 *
 * @param path Path to open terminal in.
 */
static void fm_open_in_terminal(const gchar *path)

{
    gchar *dir = g_path_get_dirname(path);
    
    pid_t pid = fork();
    if (pid == 0) {
        execl("./blackline-terminal", "blackline-terminal", NULL);
        exit(0);
    }
    
    g_free(dir);
}

/**
 * Shows properties dialog for a file or directory.
 *
 * @param path Path to show properties for.
 */
static void fm_show_properties(const gchar *path)

{
    GFile *file = g_file_new_for_path(path);
    GFileInfo *info = g_file_query_info(file, "standard::*,time::*", G_FILE_QUERY_INFO_NONE, NULL, NULL);
    if (!info) {
        g_object_unref(file);
        return;
    }

    const gchar *name = g_file_info_get_display_name(info);
    guint64 size = g_file_info_get_size(info);
    const gchar *type = g_file_info_get_content_type(info);
    GDateTime *modified = g_file_info_get_modification_date_time(info);

    gchar *size_str = format_size(size);
    gchar *modified_str = modified ? g_date_time_format(modified, "%Y-%m-%d %H:%M:%S") : g_strdup("Unknown");

    GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                               "Properties of %s\n\nSize: %s\nType: %s\nModified: %s",
                                               name, size_str, type ? type : "unknown", modified_str);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    g_free(size_str);
    g_free(modified_str);
    g_object_unref(info);
    g_object_unref(file);
}

/* Navigation helpers */

/**
 * Navigates to the user's home directory.
 *
 * @param fm FileManager instance.
 */
void fm_go_home(FileManager *fm)

{
    const gchar *home = g_get_home_dir();
    fm_open_location(fm, home);
}

/**
 * Navigates to the parent directory.
 *
 * @param fm FileManager instance.
 */
void fm_go_up(FileManager *fm)

{
    if (!fm->current_dir) return;
    
    GFile *parent = g_file_get_parent(fm->current_dir);
    if (parent) {
        gchar *path = g_file_get_path(parent);
        fm_open_location(fm, path);
        g_free(path);
        g_object_unref(parent);
    }
}

/**
 * Navigates backward in history.
 *
 * @param fm FileManager instance.
 */
void fm_go_back(FileManager *fm)

{
    if (!current_history_pos || !current_history_pos->prev) return;
    
    current_history_pos = current_history_pos->prev;
    HistoryEntry *entry = (HistoryEntry *)current_history_pos->data;
    
    if (fm->current_dir) {
        g_object_unref(fm->current_dir);
    }
    fm->current_dir = g_object_ref(entry->location);
    
    gchar *path = g_file_get_path(fm->current_dir);
    gtk_entry_set_text(GTK_ENTRY(fm->location_entry), path);
    g_free(path);
    
    fm_load_directory_contents(fm);
    
    /* Restore scroll position */
    if (entry->scroll_position) {
        gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(fm->main_tree), entry->scroll_position, NULL, TRUE, 0.5, 0.0);
    }
    
    fm_update_navigation_buttons(fm);
}

/**
 * Navigates forward in history.
 *
 * @param fm FileManager instance.
 */
void fm_go_forward(FileManager *fm)

{
    if (!current_history_pos || !current_history_pos->next) return;
    
    current_history_pos = current_history_pos->next;
    HistoryEntry *entry = (HistoryEntry *)current_history_pos->data;
    
    if (fm->current_dir) {
        g_object_unref(fm->current_dir);
    }
    fm->current_dir = g_object_ref(entry->location);
    
    gchar *path = g_file_get_path(fm->current_dir);
    gtk_entry_set_text(GTK_ENTRY(fm->location_entry), path);
    g_free(path);
    
    fm_load_directory_contents(fm);
    
    /* Restore scroll position */
    if (entry->scroll_position) {
        gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(fm->main_tree), entry->scroll_position, NULL, TRUE, 0.5, 0.0);
    }
    
    fm_update_navigation_buttons(fm);
}

/**
 * Refreshes the current directory view.
 *
 * @param fm FileManager instance.
 */
void fm_refresh(FileManager *fm)

{
    if (!fm->current_dir) return;
    gchar *path = g_file_get_path(fm->current_dir);
    fm_open_location(fm, path);
    g_free(path);
}

/**
 * Callback for location entry activation (Enter key).
 * Navigates to the entered path.
 *
 * @param fm FileManager instance.
 */
void fm_on_location_activate(FileManager *fm)

{
    const gchar *text = gtk_entry_get_text(GTK_ENTRY(fm->location_entry));
    if (text && *text) {
        fm_open_location(fm, text);
    }
}

/**
 * Updates the status label with current directory path.
 *
 * @param fm FileManager instance.
 */
void fm_update_status(FileManager *fm)

{
    if (fm->current_dir) {
        gchar *path = g_file_get_path(fm->current_dir);
        gtk_label_set_text(GTK_LABEL(fm->status_label), path);
        g_free(path);
    }
}

/* Main activation */

/**
 * Application activation callback.
 * Creates and displays the file manager window.
 *
 * @param app        The GtkApplication instance.
 * @param user_data  User data (unused).
 *
 * @sideeffect Creates file manager UI and initializes state.
 */
static void activate(GtkApplication *app, gpointer user_data)

{
    FileManager *fm = g_new(FileManager, 1);
    fm->history = NULL;
    fm->history_pos = NULL;
    fm->current_dir = NULL;
    fm->is_dragging = 0;
    fm->is_resizing = 0;
    fm->resize_edge = RESIZE_NONE;

    /* Main window */
    fm->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(fm->window), "Blackline File Manager");
    gtk_window_set_default_size(GTK_WINDOW(fm->window), 900, 600);
    gtk_window_set_position(GTK_WINDOW(fm->window), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(fm->window), 0);
    gtk_window_set_decorated(GTK_WINDOW(fm->window), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(fm->window), TRUE);
    
    gtk_widget_add_events(fm->window, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK);
    g_signal_connect(fm->window, "button-press-event", G_CALLBACK(on_button_press), fm);
    g_signal_connect(fm->window, "button-release-event", G_CALLBACK(on_button_release), fm);
    g_signal_connect(fm->window, "motion-notify-event", G_CALLBACK(on_motion_notify), fm);

    /* CSS */
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "window { background-color: #1e1e1e; color: #e0e0e0; }\n"
        "treeview { background-color: #252525; color: #e0e0e0; }\n"
        "treeview:selected { background-color: #0d6efd; color: #ffffff; }\n"
        "treeview:selected:focus { background-color: #0b5ed7; }\n"
        "entry { background-color: #2d2d2d; color: #e0e0e0; border: 1px solid #404040; }\n"
        "entry:focus { border-color: #0d6efd; }\n"
        "button { background-color: #2d2d2d; color: #e0e0e0; border: 1px solid #404040; }\n"
        "button:hover { background-color: #3d3d3d; border-color: #0d6efd; }\n"
        "button:active { background-color: #1e1e1e; }\n"
        "statusbar { background-color: #1a1a1a; color: #e0e0e0; border-top: 1px solid #404040; }\n"
        "scrolledwindow { border: 1px solid #404040; }\n",
        -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);

    /* Main vertical box */
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(fm->window), vbox);

    /* Title bar */
    GtkWidget *title_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_name(title_bar, "title-bar");
    gtk_widget_set_size_request(title_bar, -1, 30);
    gtk_box_pack_start(GTK_BOX(vbox), title_bar, FALSE, FALSE, 0);

    GtkWidget *title_label = gtk_label_new("Blackline File Manager");
    gtk_label_set_xalign(GTK_LABEL(title_label), 0.0);
    gtk_box_pack_start(GTK_BOX(title_bar), title_label, TRUE, TRUE, 10);

    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_end(GTK_BOX(title_bar), button_box, FALSE, FALSE, 5);

    GtkWidget *min_btn = gtk_button_new_with_label("─");
    gtk_widget_set_size_request(min_btn, 30, 25);
    g_signal_connect(min_btn, "clicked", G_CALLBACK(on_minimize_clicked), fm->window);
    gtk_box_pack_start(GTK_BOX(button_box), min_btn, FALSE, FALSE, 0);

    GtkWidget *max_btn = gtk_button_new_with_label("□");
    gtk_widget_set_size_request(max_btn, 30, 25);
    g_signal_connect(max_btn, "clicked", G_CALLBACK(on_maximize_clicked), fm->window);
    g_signal_connect(fm->window, "window-state-event", G_CALLBACK(on_window_state_changed), max_btn);
    gtk_box_pack_start(GTK_BOX(button_box), max_btn, FALSE, FALSE, 0);

    GtkWidget *close_btn = gtk_button_new_with_label("✕");
    gtk_widget_set_size_request(close_btn, 30, 25);
    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_close_clicked), fm->window);
    gtk_box_pack_start(GTK_BOX(button_box), close_btn, FALSE, FALSE, 0);

    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 0);

    /* Toolbar */
    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 5);

    fm->back_button = gtk_button_new_with_label("←");
    gtk_box_pack_start(GTK_BOX(toolbar), fm->back_button, FALSE, FALSE, 0);
    g_signal_connect_swapped(fm->back_button, "clicked", G_CALLBACK(fm_go_back), fm);

    fm->forward_button = gtk_button_new_with_label("→");
    gtk_box_pack_start(GTK_BOX(toolbar), fm->forward_button, FALSE, FALSE, 0);
    g_signal_connect_swapped(fm->forward_button, "clicked", G_CALLBACK(fm_go_forward), fm);

    fm->up_button = gtk_button_new_with_label("↑");
    gtk_box_pack_start(GTK_BOX(toolbar), fm->up_button, FALSE, FALSE, 0);
    g_signal_connect_swapped(fm->up_button, "clicked", G_CALLBACK(fm_go_up), fm);

    fm->home_button = gtk_button_new_with_label("🏠");
    gtk_box_pack_start(GTK_BOX(toolbar), fm->home_button, FALSE, FALSE, 0);
    g_signal_connect_swapped(fm->home_button, "clicked", G_CALLBACK(fm_go_home), fm);

    fm->refresh_button = gtk_button_new_with_label("↻");
    gtk_box_pack_start(GTK_BOX(toolbar), fm->refresh_button, FALSE, FALSE, 0);
    g_signal_connect_swapped(fm->refresh_button, "clicked", G_CALLBACK(fm_refresh), fm);

    fm->location_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(toolbar), fm->location_entry, TRUE, TRUE, 0);
    g_signal_connect_swapped(fm->location_entry, "activate", G_CALLBACK(fm_on_location_activate), fm);

    /* Horizontal paned */
    GtkWidget *hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), hpaned, TRUE, TRUE, 5);

    /* Sidebar */
    GtkWidget *sidebar_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sidebar_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_paned_pack1(GTK_PANED(hpaned), sidebar_scroll, FALSE, FALSE);

    fm->sidebar_store = gtk_list_store_new(1, G_TYPE_STRING);
    fm->sidebar_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(fm->sidebar_store));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(fm->sidebar_tree), FALSE);
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(fm->sidebar_tree), -1, "Places", renderer, "text", 0, NULL);
    gtk_container_add(GTK_CONTAINER(sidebar_scroll), fm->sidebar_tree);
    g_signal_connect(fm->sidebar_tree, "row-activated", G_CALLBACK(fm_on_sidebar_row_activated), fm);
    fm_populate_sidebar(fm);

    /* Main view */
    GtkWidget *main_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(main_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_paned_pack2(GTK_PANED(hpaned), main_scroll, TRUE, TRUE);

    /* Tree store with 5 columns: name, size, type, modified, full_path (hidden) */
    fm->main_store = gtk_tree_store_new(5, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    fm->main_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(fm->main_store));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(fm->main_tree), TRUE);

    renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes("Name", renderer, "text", 0, NULL);
    gtk_tree_view_column_set_sort_column_id(col, 0);
    gtk_tree_view_append_column(GTK_TREE_VIEW(fm->main_tree), col);

    renderer = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Size", renderer, "text", 1, NULL);
    gtk_tree_view_column_set_sort_column_id(col, 1);
    gtk_tree_view_append_column(GTK_TREE_VIEW(fm->main_tree), col);

    renderer = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Type", renderer, "text", 2, NULL);
    gtk_tree_view_column_set_sort_column_id(col, 2);
    gtk_tree_view_append_column(GTK_TREE_VIEW(fm->main_tree), col);

    renderer = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Modified", renderer, "text", 3, NULL);
    gtk_tree_view_column_set_sort_column_id(col, 3);
    gtk_tree_view_append_column(GTK_TREE_VIEW(fm->main_tree), col);

    gtk_container_add(GTK_CONTAINER(main_scroll), fm->main_tree);
    g_signal_connect(fm->main_tree, "row-activated", G_CALLBACK(fm_on_row_activated), fm);
    g_signal_connect(fm->main_tree, "button-press-event", G_CALLBACK(on_main_tree_button_press), fm);

    /* Status bar */
    fm->status_label = gtk_label_new("Ready");
    gtk_label_set_xalign(GTK_LABEL(fm->status_label), 0);
    gtk_box_pack_start(GTK_BOX(vbox), fm->status_label, FALSE, FALSE, 2);

    /* Initialize navigation buttons state */
    fm_update_navigation_buttons(fm);

    /* Start at home */
    fm_go_home(fm);

    gtk_widget_show_all(fm->window);
    g_object_set_data_full(G_OBJECT(fm->window), "fm", fm, g_free);
}

/**
 * Application entry point.
 *
 * @param argc Argument count from command line.
 * @param argv Argument vector from command line.
 * @return     Exit status from g_application_run().
 */
int main(int argc, char **argv)

{
    GtkApplication *app = gtk_application_new("org.blackline.filemanager", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}