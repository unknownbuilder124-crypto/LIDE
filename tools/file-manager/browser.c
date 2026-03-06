#include "fm.h"
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

static gchar *format_size(goffset size) 

{
    if (size < 1024) return g_strdup_printf("%ld B", (long)size);
    if (size < 1024 * 1024) return g_strdup_printf("%.1f KB", size / 1024.0);
    if (size < 1024 * 1024 * 1024) return g_strdup_printf("%.1f MB", size / (1024.0 * 1024.0));
    return g_strdup_printf("%.1f GB", size / (1024.0 * 1024.0 * 1024.0));
}

static gchar *format_time(GDateTime *dt) 

{
    return g_date_time_format(dt, "%Y-%m-%d %H:%M");
}

// Check if file is a text file based on extension
static gboolean is_text_file(const gchar *filename) 
{
    const gchar *ext = strrchr(filename, '.');
    if (!ext) return FALSE;
    
    // Common text file extensions
    const gchar *text_extensions[] = {
        ".txt", ".c", ".h", ".cpp", ".hpp", ".cc", ".cxx",
        ".py", ".pl", ".rb", ".sh", ".bash", ".zsh",
        ".js", ".html", ".htm", ".css", ".xml", ".json",
        ".md", ".markdown", ".ini", ".conf", ".cfg",
        ".log", ".csv", ".tsv", ".sql", ".java", ".kt",
        ".go", ".rs", ".swift", ".m", ".mm", ".php",
        ".asp", ".aspx", ".jsp", ".tcl", ".lua", ".r",
        ".yaml", ".yml", ".toml", ".makefile", ".mk",
        ".cmake", ".diff", ".patch", ".tex", ".bib",
        ".rst", ".asciidoc", ".pod", ".1", ".2", ".3",
        ".man", ".nfo", ".diz", ".gitignore", ".gitattributes",
        ".editorconfig", ".vim", ".el", ".scm", ".ss",
        ".clj", ".cljs", ".coffee", ".litcoffee", ".hs",
        ".lhs", ".erl", ".hrl", ".ex", ".exs", ".eex",
        ".leex", ".slim", ".haml", ".pug", ".jade",
        ".scss", ".sass", ".less", ".styl", ".vue",
        ".jsx", ".tsx", ".dart", ".groovy", ".gradle",
        ".properties", ".plist", ".xib", ".storyboard",
        ".strings", ".po", ".mo", ".json5", ".jsonc",
        ".proto", ".thrift", ".capnp", ".fbs", NULL
    };
    
    for (int i = 0; text_extensions[i] != NULL; i++) {
        if (g_str_has_suffix(filename, text_extensions[i])) {
            return TRUE;
        }
    }
    
    return FALSE;
}

// Open file with appropriate application - FIXED to use BlackLine editor
static void open_file_with_app(GtkWindow *parent, const gchar *path) 
{
    (void)parent; // Unused parameter
    
    // Check if it's a text file
    if (is_text_file(path)) {
        // Use BlackLine editor for text files - launch in background
        gchar *command = g_strdup_printf("blackline-editor \"%s\" &", path);
        system(command);
        g_free(command);
    } else {
        // For other files, use xdg-open
        gchar *command = g_strdup_printf("xdg-open \"%s\" &", path);
        system(command);
        g_free(command);
    }
}

// Helper function to load directory without history
static void load_directory(FileManager *fm, GFile *file)

{
    // Clear main store
    gtk_tree_store_clear(fm->main_store);

    // Enumerate directory
    GFileEnumerator *enumerator = g_file_enumerate_children(file,
        G_FILE_ATTRIBUTE_STANDARD_NAME ","
        G_FILE_ATTRIBUTE_STANDARD_TYPE ","
        G_FILE_ATTRIBUTE_STANDARD_SIZE ","
        G_FILE_ATTRIBUTE_TIME_MODIFIED,
        G_FILE_QUERY_INFO_NONE, NULL, NULL);
    
    if (!enumerator) {
        return;
    }

    GFileInfo *info;
    
    while ((info = g_file_enumerator_next_file(enumerator, NULL, NULL)) != NULL) 
    {
        const gchar *name = g_file_info_get_name(info);
        guint64 size = g_file_info_get_size(info);
        GFileType type = g_file_info_get_file_type(info);
        GDateTime *modified = g_file_info_get_modification_date_time(info);

        gchar *size_str = (type == G_FILE_TYPE_DIRECTORY) ? g_strdup("<DIR>") : format_size(size);
        const gchar *type_str;
        switch (type) {
            case G_FILE_TYPE_DIRECTORY: type_str = "Folder"; break;
            case G_FILE_TYPE_REGULAR: type_str = "File"; break;
            case G_FILE_TYPE_SYMBOLIC_LINK: type_str = "Link"; break;
            default: type_str = "Other";
        }

        gchar *time_str = modified ? format_time(modified) : g_strdup("");

        GtkTreeIter iter;
        gtk_tree_store_append(fm->main_store, &iter, NULL);
        gtk_tree_store_set(fm->main_store, &iter,
            0, name,
            1, size_str,
            2, type_str,
            3, time_str,
            -1);

        g_free(size_str);
        g_free(time_str);
        g_object_unref(info);
    }

    g_object_unref(enumerator);
    fm_update_status(fm);
}

static void add_to_history(FileManager *fm, GFile *file, int add_to_history_flag)

{
    gchar *path = g_file_get_path(file);
    
    if (add_to_history_flag && fm->current_dir) {
        // If we're not at the end of history, truncate forward history
        if (fm->history_pos && fm->history_pos->next) {
            GList *next = fm->history_pos->next;
            GList *tmp;
            while (next) {
                tmp = next->next;
                g_object_unref(next->data);
                g_list_free_1(next);
                next = tmp;
            }
            fm->history_pos->next = NULL;
        }
        
        // Add current dir to history
        fm->history = g_list_append(fm->history, g_object_ref(fm->current_dir));
        fm->history_pos = NULL; // Reset position to end
    }
    
    // Set new current directory
    if (fm->current_dir) g_object_unref(fm->current_dir);
    fm->current_dir = g_object_ref(file);
    
    // Update location entry
    gtk_entry_set_text(GTK_ENTRY(fm->location_entry), path);
    g_free(path);
    
    // Update button states
    gtk_widget_set_sensitive(fm->back_button, (fm->history != NULL));
    gtk_widget_set_sensitive(fm->forward_button, (fm->history_pos && fm->history_pos->next));
}

void fm_open_location(FileManager *fm, const gchar *path) 

{
    GFile *file = g_file_new_for_path(path);
    
    if (!g_file_query_exists(file, NULL)) 
    {
        g_object_unref(file);
        return;
    }

    // Add to history and load directory
    add_to_history(fm, file, 1);
    load_directory(fm, file);
    g_object_unref(file);
}

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

void fm_go_home(FileManager *fm) 

{
    const gchar *home = g_get_home_dir();
    fm_open_location(fm, home);
}

void fm_go_back(FileManager *fm) 

{
   // If at a position, move back
    if (fm->history_pos) {
        if (fm->history_pos->prev) {
            fm->history_pos = fm->history_pos->prev;
        }
    } else if (!fm->history) return;
    
     {
        // Not in history, go to last item
        fm->history_pos = g_list_last(fm->history);
    }
    
    if (fm->history_pos) {
        GFile *file = G_FILE(fm->history_pos->data);
        gchar *path = g_file_get_path(file);
        
        // Update current dir WITHOUT adding to history
        if (fm->current_dir) g_object_unref(fm->current_dir);
        fm->current_dir = g_object_ref(file);
        gtk_entry_set_text(GTK_ENTRY(fm->location_entry), path);
        load_directory(fm, file); // Just load, don't add to history
        g_free(path);
    }
    
    // Update button states
    gtk_widget_set_sensitive(fm->back_button, (fm->history_pos && fm->history_pos->prev));
    gtk_widget_set_sensitive(fm->forward_button, (fm->history_pos && fm->history_pos->next));
}

void fm_go_forward(FileManager *fm) 

{
    if (!fm->history_pos || !fm->history_pos->next) return;
    
    fm->history_pos = fm->history_pos->next;
    GFile *file = G_FILE(fm->history_pos->data);
    gchar *path = g_file_get_path(file);
    
    // Update current dir WITHOUT adding to history
    if (fm->current_dir) g_object_unref(fm->current_dir);
    fm->current_dir = g_object_ref(file);
    gtk_entry_set_text(GTK_ENTRY(fm->location_entry), path);
    load_directory(fm, file); // Just load, don't add to history
    g_free(path);
    
    // Update button states
    gtk_widget_set_sensitive(fm->back_button, (fm->history_pos && fm->history_pos->prev));
    gtk_widget_set_sensitive(fm->forward_button, (fm->history_pos && fm->history_pos->next));
}

void fm_refresh(FileManager *fm) 

{
    if (!fm->current_dir) return;
    gchar *path = g_file_get_path(fm->current_dir);
    fm_open_location(fm, path);
    g_free(path);
}

void fm_on_location_activate(FileManager *fm) 

{
    const gchar *text = gtk_entry_get_text(GTK_ENTRY(fm->location_entry));
    fm_open_location(fm, text);
}

void fm_on_row_activated(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *col, FileManager *fm) 

{
    GtkTreeModel *model = gtk_tree_view_get_model(tree);
    GtkTreeIter iter;
    if (!gtk_tree_model_get_iter(model, &iter, path)) return;

    gchar *name;
    gtk_tree_model_get(model, &iter, 0, &name, -1);
    if (!name) return;

    GFile *current = g_file_new_for_path(gtk_entry_get_text(GTK_ENTRY(fm->location_entry)));
    GFile *child = g_file_get_child(current, name);
    g_free(name);

    GFileType type = g_file_query_file_type(child, G_FILE_QUERY_INFO_NONE, NULL);
    if (type == G_FILE_TYPE_DIRECTORY) 
    {
        gchar *child_path = g_file_get_path(child);
        fm_open_location(fm, child_path);
        g_free(child_path);
    } else {
        // Open file with appropriate application (text files go to BlackLine editor)
        gchar *file_path = g_file_get_path(child);
        
        // Debug: print what we're trying to open
        g_print("Opening file: %s\n", file_path);
        
        open_file_with_app(GTK_WINDOW(fm->window), file_path);
        g_free(file_path);
    }
    g_object_unref(child);
    g_object_unref(current);
}

void fm_update_status(FileManager *fm) 

{
    if (!fm->current_dir) return;

    GFileInfo *info = g_file_query_filesystem_info(fm->current_dir,
        G_FILE_ATTRIBUTE_FILESYSTEM_FREE "," G_FILE_ATTRIBUTE_FILESYSTEM_SIZE, NULL, NULL);
    if (info) {
        guint64 free = g_file_info_get_attribute_uint64(info, G_FILE_ATTRIBUTE_FILESYSTEM_FREE);
        guint64 total = g_file_info_get_attribute_uint64(info, G_FILE_ATTRIBUTE_FILESYSTEM_SIZE);
        gchar *free_str = format_size(free);
        gchar *total_str = format_size(total);
        gchar *status = g_strdup_printf("Free: %s of %s", free_str, total_str);
        gtk_label_set_text(GTK_LABEL(fm->status_label), status);
        g_free(free_str);
        g_free(total_str);
        g_free(status);
        g_object_unref(info);
    }
}

void fm_populate_sidebar(FileManager *fm) 

{
    gtk_list_store_clear(fm->sidebar_store);
    GtkTreeIter iter;

    gtk_list_store_append(fm->sidebar_store, &iter);
    gtk_list_store_set(fm->sidebar_store, &iter, 0, "Home", -1);

    gtk_list_store_append(fm->sidebar_store, &iter);
    gtk_list_store_set(fm->sidebar_store, &iter, 0, "Root (/)", -1);

    gtk_list_store_append(fm->sidebar_store, &iter);
    gtk_list_store_set(fm->sidebar_store, &iter, 0, "Desktop", -1);

    gtk_list_store_append(fm->sidebar_store, &iter);
    gtk_list_store_set(fm->sidebar_store, &iter, 0, "Documents", -1);

    gtk_list_store_append(fm->sidebar_store, &iter);
    gtk_list_store_set(fm->sidebar_store, &iter, 0, "Downloads", -1);
}

void fm_on_sidebar_row_activated(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *col, FileManager *fm) 

{
    GtkTreeModel *model = gtk_tree_view_get_model(tree);
    GtkTreeIter iter;
    if (!gtk_tree_model_get_iter(model, &iter, path)) return;

    gchar *name;
    gtk_tree_model_get(model, &iter, 0, &name, -1);
    if (!name) return;

    const gchar *home = g_get_home_dir();
    
    if (g_strcmp0(name, "Home") == 0) {
        fm_go_home(fm);
    } else if (g_strcmp0(name, "Root (/)") == 0) {
        fm_open_location(fm, "/");
    } else if (g_strcmp0(name, "Desktop") == 0) {
        gchar *path = g_build_filename(home, "Desktop", NULL);
        fm_open_location(fm, path);
        g_free(path);
    } else if (g_strcmp0(name, "Documents") == 0) {
        gchar *path = g_build_filename(home, "Documents", NULL);
        fm_open_location(fm, path);
        g_free(path);
    } else if (g_strcmp0(name, "Downloads") == 0) {
        gchar *path = g_build_filename(home, "Downloads", NULL);
        fm_open_location(fm, path);
        g_free(path);
    }
    g_free(name);
}