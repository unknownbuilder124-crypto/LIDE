#include "auto.h"
#include <glib.h>
#include <glib/gstdio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

/* Global window reference for closing from child processes */
static GtkWidget *tools_window = NULL;

/**
 * Sets the global tools window reference.
 */
void set_tools_window(GtkWidget *window)
{
    tools_window = window;
}

/**
 * Extracts a value from .desktop file content.
 *
 * Searches for a key in the format "key=value" within the content.
 * Handles both start-of-file and newline-prefixed keys.
 *
 * @param content File content as string.
 * @param key Key to search for.
 * @return Newly allocated value string, or NULL if not found.
 */
static char* get_desktop_value(const char *content, const char *key)
{
    char *search = g_strdup_printf("\n%s=", key);
    char *found = strstr(content, search);
    g_free(search);
    
    if (!found) {
        search = g_strdup_printf("^%s=", key);
        if (content[0] == key[0] && strncmp(content, key, strlen(key)) == 0) {
            found = (char*)content;
        }
        g_free(search);
        if (!found) return NULL;
    } else {
        found++;
    }
    
    char *start = strchr(found, '=');
    if (!start) return NULL;
    start++;
    
    char *end = strchr(start, '\n');
    if (!end) end = start + strlen(start);
    
    int len = end - start;
    char *value = g_malloc(len + 1);
    strncpy(value, start, len);
    value[len] = '\0';
    
    char *trimmed = g_strstrip(value);
    if (trimmed != value) {
        char *new_val = g_strdup(trimmed);
        g_free(value);
        value = new_val;
    }
    
    return value;
}

/**
 * Extracts icon name from .desktop file content.
 *
 * @param content File content.
 * @return Icon string (absolute path or name), default icon if not found.
 */
static char* get_icon_from_desktop(const char *content)
{
    char *icon = get_desktop_value(content, "Icon");
    if (!icon) return g_strdup("application-x-executable");
    
    if (g_path_is_absolute(icon)) {
        return icon;
    }
    
    char *full_icon = g_strdup_printf("%s", icon);
    g_free(icon);
    return full_icon;
}

/**
 * Extracts executable command from .desktop file content.
 *
 * Removes % parameters (e.g., %U, %F) from the command line.
 *
 * @param content File content.
 * @return Executable command string, or NULL if not found.
 */
static char* get_exec_from_desktop(const char *content)
{
    char *exec = get_desktop_value(content, "Exec");
    if (!exec) return NULL;
    
    char *percent = strstr(exec, "%");
    if (percent) {
        *percent = '\0';
    }
    
    char *trimmed = g_strstrip(exec);
    if (trimmed != exec) {
        char *new_exec = g_strdup(trimmed);
        g_free(exec);
        return new_exec;
    }
    
    return exec;
}

/**
 * Determines if an application should be excluded from the tools container.
 *
 * Filters out system utilities, DE-specific apps, and BlackLine's own tools.
 *
 * @param name Application name.
 * @param exec Executable command.
 * @return TRUE if app should be excluded, FALSE otherwise.
 */
static gboolean should_exclude_app(const char *name, const char *exec)
{
    const char *exclude_names[] = {
        "gsettings", "gapplication", "gmain", "gdbus",
        "gvfs", "gnome", "kde", "xfce", "lxde",
        "pulseaudio", "blueman", "network", "system",
        "org.gnome", "org.kde", "blackline", "voidfox",
        "firefox-wrapper", "settings", "file-manager",
        "system-monitor", "calculator", "editor", "terminal",
        "image-viewer", NULL
    };
    
    for (int i = 0; exclude_names[i] != NULL; i++) {
        if (strstr(name, exclude_names[i]) ||
            (exec && strstr(exec, exclude_names[i]))) {
            return TRUE;
        }
    }
    
    return FALSE;
}

/**
 * Scans /usr/share/applications and /usr/local/share/applications for .desktop files.
 *
 * @param count Output parameter for number of detected apps.
 * @return Array of DetectedApp structures, or NULL if none found.
 */
DetectedApp* detect_installed_apps(int *count)
{
    DetectedApp *apps = NULL;
    *count = 0;
    
    const char *dirs[] = {"/usr/share/applications", "/usr/local/share/applications", NULL};
    
    for (int d = 0; dirs[d] != NULL; d++) {
        GDir *dir = g_dir_open(dirs[d], 0, NULL);
        if (!dir) continue;
        
        const char *entry;
        while ((entry = g_dir_read_name(dir)) != NULL) {
            if (!g_str_has_suffix(entry, ".desktop")) continue;
            
            char *path = g_build_filename(dirs[d], entry, NULL);
            char *content = NULL;
            gsize length;
            
            if (!g_file_get_contents(path, &content, &length, NULL)) {
                g_free(path);
                continue;
            }
            
            char *name = get_desktop_value(content, "Name");
            char *exec = get_exec_from_desktop(content);
            char *categories = get_desktop_value(content, "Categories");
            char *no_display = get_desktop_value(content, "NoDisplay");
            
            if (name && exec && !should_exclude_app(name, exec)) {
                if (no_display && strcmp(no_display, "true") == 0) {
                    g_free(no_display);
                } else {
                    apps = g_realloc(apps, sizeof(DetectedApp) * (*count + 1));
                    apps[*count].name = name;
                    apps[*count].icon = get_icon_from_desktop(content);
                    apps[*count].exec = exec;
                    (*count)++;
                    name = NULL;
                    exec = NULL;
                }
            }
            
            g_free(name);
            g_free(exec);
            g_free(categories);
            g_free(no_display);
            g_free(content);
            g_free(path);
        }
        
        g_dir_close(dir);
    }
    
    return apps;
}

/**
 * Frees memory allocated for detected applications.
 */
void free_detected_apps(DetectedApp *apps, int count)
{
    if (!apps) return;
    
    for (int i = 0; i < count; i++) {
        g_free(apps[i].name);
        g_free(apps[i].icon);
        g_free(apps[i].exec);
    }
    g_free(apps);
}

/**
 * Launches a detected application and closes the tools window.
 *
 * @param button The button that was clicked.
 * @param window Parent window (unused).
 */
static void launch_detected_app(GtkButton *button, gpointer window)
{
    (void)window;
    
    char *exec = (char*)g_object_get_data(G_OBJECT(button), "exec");
    if (!exec) return;
    
    pid_t pid = fork();
    if (pid == 0) {
        char *argv[4];
        argv[0] = "/bin/sh";
        argv[1] = "-c";
        argv[2] = exec;
        argv[3] = NULL;
        execvp(argv[0], argv);
        exit(0);
    } else if (pid > 0) {
        if (tools_window) {
            gtk_window_close(GTK_WINDOW(tools_window));
        }
    }
}

/**
 * Event handler for launching detected applications.
 *
 * @param widget The widget that received the event.
 * @param event Button event details.
 * @param window Parent window (unused).
 * @return TRUE to indicate event was handled.
 */
static gboolean launch_detected_app_event(GtkWidget *widget, GdkEventButton *event, gpointer window)
{
    (void)widget;
    (void)event;
    (void)window;
    
    char *exec = (char*)g_object_get_data(G_OBJECT(widget), "exec");
    if (!exec) return TRUE;
    
    pid_t pid = fork();
    if (pid == 0) {
        char *argv[4];
        argv[0] = "/bin/sh";
        argv[1] = "-c";
        argv[2] = exec;
        argv[3] = NULL;
        execvp(argv[0], argv);
        exit(0);
    } else if (pid > 0) {
        if (tools_window) {
            gtk_window_close(GTK_WINDOW(tools_window));
        }
    }
    return TRUE;
}

/**
 * Creates a ToolItem from a detected application.
 *
 * @param app Detected application data.
 * @param window Parent window reference.
 * @return ToolItem structure ready for use.
 */
ToolItem create_tool_item_from_detected(DetectedApp *app, gpointer window)
{
    ToolItem item;
    item.name = g_strdup(app->name);
    item.icon = g_strdup(app->icon);
    item.button_callback = launch_detected_app;
    item.event_callback = launch_detected_app_event;
    item.user_data = window;
    return item;
}

/**
 * Merges static tools and detected applications into a single array.
 *
 * @param static_tools Array of static tools.
 * @param static_count Number of static tools.
 * @param detected_apps Array of detected applications.
 * @param detected_count Number of detected applications.
 * @param window Parent window reference.
 * @param merged_count Output parameter for merged array size.
 * @return Newly allocated merged ToolItem array.
 */
ToolItem* merge_tool_items(ToolItem *static_tools, int static_count,
                           DetectedApp *detected_apps, int detected_count,
                           gpointer window, int *merged_count)
{
    *merged_count = static_count + detected_count;
    ToolItem *merged = g_malloc(sizeof(ToolItem) * (*merged_count));
    
    /* Copy static tools */
    for (int i = 0; i < static_count; i++) {
        merged[i].name = g_strdup(static_tools[i].name);
        merged[i].icon = g_strdup(static_tools[i].icon);
        merged[i].button_callback = static_tools[i].button_callback;
        merged[i].event_callback = static_tools[i].event_callback;
        merged[i].user_data = static_tools[i].user_data;
    }
    
    /* Convert and append detected apps */
    for (int i = 0; i < detected_count; i++) {
        merged[static_count + i] = create_tool_item_from_detected(&detected_apps[i], window);
    }
    
    return merged;
}