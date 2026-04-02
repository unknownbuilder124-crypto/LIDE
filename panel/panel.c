#include <gtk/gtk.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <string.h>
#include "tools/minimized_container.h"
#include "network_stats.h"
#include "connection_details.h"
#include "internet_settings.h"
#include "wifi_list.h"
#include "wifi_connect.h"
#include "upload.h"
#include "download.h"

/* Debounce timer for brightness slider */
static guint brightness_timeout_id = 0;

/* Define HISTORY_SIZE before using it */
#define HISTORY_SIZE 60

/* Include monitor.h for CpuData and MemData definitions */
#include "../tools/system-monitor/monitor.h"

/* Define the global variables expected by the system monitor code */
double cpu_history[HISTORY_SIZE] = {0};
int cpu_history_index = 0;
double mem_history[HISTORY_SIZE] = {0};
int mem_history_index = 0;

/* Include system monitor files after defining the globals and HISTORY_SIZE */
#include "../tools/system-monitor/cpu.c"
#include "../tools/system-monitor/memory.c"

static pid_t tools_pid = 0;

/* Global stat variables - renamed to avoid conflicts with monitor.h */
static double panel_cpu_percent = 0.0;
static guint64 panel_mem_total = 0;
static guint64 panel_mem_available = 0;
static guint64 panel_mem_used = 0;
static int panel_mem_percent = 0;

/* PERSISTENT CPU DATA - must be static to track previous values across calls
 * This is critical for accurate CPU percentage calculation */
static CpuData panel_cpu_data = {0};

/* CPU history for smooth display */
static float panel_cpu_history[5] = {0};
static int panel_cpu_history_index = 0;

/* Battery and Network status */
static int battery_percent = -1;
static gboolean battery_charging = FALSE;
static gboolean is_wifi_connected = FALSE;
static gboolean is_internet_connected = FALSE;
static gchar *wifi_ssid = NULL;

/* Forward declarations for cleanup functions */
void wifi_list_cleanup(void);
void wifi_connect_cleanup(void);
void internet_settings_cleanup(void);

/* Forward declarations for network window */
static GtkWidget *network_window = NULL;
static void update_network_window_display(GtkWidget *vbox);

/* Forward declaration for refresh function */
void refresh_wifi_network_list(GtkWidget *window);

/* Forward declaration for WiFi connection callback */
static void on_wifi_network_clicked(GtkButton *btn, gpointer data);

/* Forward declaration for set_brightness function */
static void set_brightness(int level);

/* ====================
 * BATTERY FUNCTIONS
 * ==================== */

/**
 * Reads current battery percentage from sysfs.
 * Attempts BAT0 first, falls back to BAT1.
 *
 * @return Battery percentage (0-100), or -1 if not found.
 */
static int get_battery_percent(void)
{
    FILE *fp;
    int battery = -1;
    
    fp = fopen("/sys/class/power_supply/BAT0/capacity", "r");
    if (fp == NULL) {
        fp = fopen("/sys/class/power_supply/BAT1/capacity", "r");
    }
    
    if (fp != NULL) {
        if (fscanf(fp, "%d", &battery) != 1) {
            battery = -1;
        }
        fclose(fp);
    }
    
    if (battery < 0) battery = -1;
    if (battery > 100) battery = 100;
    
    return battery;
}

/**
 * Checks whether battery is currently charging.
 *
 * @return TRUE if charging or full, FALSE otherwise.
 */
static gboolean check_battery_charging(void)
{
    FILE *fp;
    char status[64] = {0};
    
    fp = fopen("/sys/class/power_supply/BAT0/status", "r");
    if (fp == NULL) {
        fp = fopen("/sys/class/power_supply/BAT1/status", "r");
    }
    
    if (fp != NULL) {
        if (fgets(status, sizeof(status), fp)) {
            fclose(fp);
            return (strstr(status, "Charging") != NULL || strstr(status, "Full") != NULL);
        }
        fclose(fp);
    }
    
    return FALSE;
}

/* ====================
 * NETWORK FUNCTIONS
 * ==================== */

/**
 * Checks internet connectivity by pinging Google DNS (8.8.8.8).
 * Updates global is_internet_connected flag.
 */
static void check_internet_connectivity(void)
{
    int ret = system("ping -c 1 -W 1 8.8.8.8 >/dev/null 2>&1");
    is_internet_connected = (ret == 0);
}

/**
 * Checks current WiFi connection status using nmcli.
 * Updates global is_wifi_connected and wifi_ssid.
 */
static void check_wifi_status(void)
{
    FILE *fp = popen("nmcli -t -f NAME,TYPE connection show --active 2>/dev/null | grep wifi | cut -d: -f1", "r");
    if (fp != NULL) {
        char ssid[256] = {0};
        if (fgets(ssid, sizeof(ssid), fp) != NULL) {
            size_t len = strlen(ssid);
            if (len > 0 && ssid[len-1] == '\n') {
                ssid[len-1] = '\0';
            }
            is_wifi_connected = (strlen(ssid) > 0);
            if (wifi_ssid != NULL) g_free(wifi_ssid);
            wifi_ssid = (strlen(ssid) > 0) ? g_strdup(ssid) : NULL;
        }
        pclose(fp);
    }
}

/**
 * Checks whether WiFi radio is enabled via nmcli.
 *
 * @return TRUE if WiFi is enabled, FALSE otherwise.
 */
static gboolean is_wifi_enabled(void)
{
    FILE *fp = popen("nmcli radio wifi 2>/dev/null | tr -d '\\n'", "r");
    if (!fp) return TRUE;
    
    char status[16] = {0};
    if (fgets(status, sizeof(status), fp)) {
        pclose(fp);
        return (strcmp(status, "enabled") == 0);
    }
    pclose(fp);
    return TRUE;
}

/**
 * Toggles WiFi on/off via nmcli.
 * Updates the toggle button text and refreshes network list if the window is open.
 *
 * @param widget The toggle button widget (unused).
 * @param data   User data (unused).
 *
 * @sideeffect Executes nmcli commands to control WiFi radio.
 */
static void toggle_wifi_simple(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;
    
    /* Get current WiFi state */
    gboolean currently_enabled = is_wifi_enabled();
    
    /* Toggle WiFi */
    if (currently_enabled) {
        system("nmcli radio wifi off 2>/dev/null");
    } else {
        system("nmcli radio wifi on 2>/dev/null");
        /* Give it time to enable and scan */
        sleep(1);
        system("nmcli device wifi rescan 2>/dev/null");
    }
    
    /* Update the button text if the network window is open */
    if (network_window != NULL) {
        GtkWidget *vbox = g_object_get_data(G_OBJECT(network_window), "vbox");
        if (vbox) {
            /* Find and update the toggle button */
            GList *children = gtk_container_get_children(GTK_CONTAINER(vbox));
            for (GList *l = children; l != NULL; l = l->next) {
                GtkWidget *child = GTK_WIDGET(l->data);
                const gchar *name = gtk_widget_get_name(child);
                if (name && g_strcmp0(name, "wifi-toggle-btn") == 0) {
                    /* Update button text based on new state */
                    if (currently_enabled) {
                        gtk_button_set_label(GTK_BUTTON(child), "🟢 Turn On WiFi");
                    } else {
                        gtk_button_set_label(GTK_BUTTON(child), "🔴 Turn Off WiFi");
                    }
                    break;
                }
            }
            g_list_free(children);
            
            /* Refresh the network list */
            refresh_wifi_network_list(network_window);
        }
    }
}

/**
 * Refreshes the WiFi network list in the main window.
 * Scans for available networks and creates connectable buttons.
 *
 * @param window The network window widget.
 *
 * @sideeffect Destroys and recreates the network list container.
 */
void refresh_wifi_network_list(GtkWidget *window)
{
    GtkWidget *vbox = g_object_get_data(G_OBJECT(window), "vbox");
    if (!vbox) return;
    
    GList *children = gtk_container_get_children(GTK_CONTAINER(vbox));
    for (GList *l = children; l != NULL; l = l->next) {
        GtkWidget *child = GTK_WIDGET(l->data);
        if (GTK_IS_FRAME(child)) {
            GtkWidget *frame_child = gtk_bin_get_child(GTK_BIN(child));
            if (frame_child && GTK_IS_SCROLLED_WINDOW(frame_child)) {
                GtkWidget *scroll = frame_child;
                GtkWidget *old_list = gtk_bin_get_child(GTK_BIN(scroll));
                if (old_list) {
                    gtk_widget_destroy(old_list);
                }
                
                /* Create new network list with connectable buttons */
                GtkWidget *new_list = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
                gtk_container_set_border_width(GTK_CONTAINER(new_list), 10);
                
                if (!is_wifi_enabled()) {
                    GtkWidget *wifi_off = gtk_label_new("WiFi is disabled");
                    gtk_box_pack_start(GTK_BOX(new_list), wifi_off, TRUE, TRUE, 0);
                } else {
                    /* Get WiFi networks via nmcli */
                    FILE *fp = popen("nmcli -t -f SSID,SIGNAL,SECURITY device wifi list --rescan yes 2>/dev/null", "r");
                    if (fp) {
                        char line[512];
                        while (fgets(line, sizeof(line), fp)) {
                            line[strcspn(line, "\n")] = 0;
                            if (strlen(line) < 2) continue;
                            
                            char *ssid = strtok(line, ":");
                            char *signal_str = strtok(NULL, ":");
                            char *security = strtok(NULL, ":");
                            
                            if (!ssid || strcmp(ssid, "SSID") == 0 || strlen(ssid) == 0) continue;
                            
                            int signal = signal_str ? atoi(signal_str) : 0;
                            
                            /* Determine signal icon */
                            const char *signal_icon = "🔴";
                            if (signal >= 80) signal_icon = "🟢";
                            else if (signal >= 60) signal_icon = "🟢";
                            else if (signal >= 40) signal_icon = "🟡";
                            else if (signal >= 20) signal_icon = "🟠";
                            
                            /* Security icon */
                            const char *lock_icon = "";
                            if (security && strlen(security) > 0 && strcmp(security, "--") != 0) {
                                lock_icon = "🔒 ";
                            }
                            
                            gchar *btn_label = g_strdup_printf("%s %s%s   %d%%", signal_icon, lock_icon, ssid, signal);
                            GtkWidget *net_btn = gtk_button_new_with_label(btn_label);
                            g_free(btn_label);
                            
                            /* Store network info */
                            g_object_set_data_full(G_OBJECT(net_btn), "ssid", g_strdup(ssid), g_free);
                            g_object_set_data_full(G_OBJECT(net_btn), "security", g_strdup(security ? security : ""), g_free);
                            
                            /* Connect to callback */
                            g_signal_connect(net_btn, "clicked", G_CALLBACK(on_wifi_network_clicked), window);
                            
                            gtk_widget_set_margin_top(net_btn, 2);
                            gtk_widget_set_margin_bottom(net_btn, 2);
                            gtk_box_pack_start(GTK_BOX(new_list), net_btn, FALSE, FALSE, 0);
                        }
                        pclose(fp);
                    }
                }
                
                gtk_container_add(GTK_CONTAINER(scroll), new_list);
                gtk_widget_show_all(window);
                break;
            }
        }
    }
    g_list_free(children);
}

/**
 * Callback for WiFi network button clicks.
 * Handles connection to selected network, showing password dialog for secured networks.
 *
 * @param btn  The clicked network button.
 * @param data Parent window for dialogs.
 *
 * @sideeffect Executes nmcli connection commands.
 * @sideeffect Shows password dialog for secured networks.
 */
static void on_wifi_network_clicked(GtkButton *btn, gpointer data)
{
    GtkWidget *parent = GTK_WIDGET(data);
    const char *ssid = g_object_get_data(G_OBJECT(btn), "ssid");
    const char *security = g_object_get_data(G_OBJECT(btn), "security");
    
    if (!ssid) return;
    
    /* Check if network is secured */
    gboolean secured = (security && strlen(security) > 0 && strcmp(security, "--") != 0);
    
    if (secured) {
        /* Show password dialog for secured networks */
        show_wifi_connect_dialog(parent, ssid);
    } else {
        /* Connect directly to open networks */
        gchar *cmd = g_strdup_printf("nmcli device wifi connect '%s' 2>/dev/null", ssid);
        int result = system(cmd);
        g_free(cmd);
        
        /* Show result dialog */
        GtkWidget *status_dialog = gtk_message_dialog_new(
            GTK_WINDOW(parent),
            GTK_DIALOG_MODAL,
            result == 0 ? GTK_MESSAGE_INFO : GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            result == 0 ? "Connection Successful" : "Connection Failed"
        );
        
        if (result == 0) {
            gtk_message_dialog_format_secondary_text(
                GTK_MESSAGE_DIALOG(status_dialog),
                "Successfully connected to %s", ssid
            );
        } else {
            gtk_message_dialog_format_secondary_text(
                GTK_MESSAGE_DIALOG(status_dialog),
                "Failed to connect to %s.\nPlease try again.", ssid
            );
        }
        
        gtk_dialog_run(GTK_DIALOG(status_dialog));
        gtk_widget_destroy(status_dialog);
    }
    
    /* Refresh the network list */
    refresh_wifi_network_list(parent);
}

/* ====================
 * BRIGHTNESS FUNCTIONS - Improved for system compatibility
 * ==================== */

/**
 * Gets current screen brightness using multiple fallback methods.
 * Tries xbacklight, sysfs backlight interface, brightnessctl, and xrandr.
 *
 * @return Brightness percentage (10-100), default 100 on failure.
 */
static int get_brightness(void)
{
    int brightness = 100; /* Default value */
    
    /* Method 1: Try xbacklight first (most common) */
    FILE *fp = popen("xbacklight -get 2>/dev/null", "r");
    if (fp) {
        char buf[16] = {0};
        if (fgets(buf, sizeof(buf), fp)) {
            float val = atof(buf);
            if (val > 0 && val <= 100) {
                brightness = (int)val;
                pclose(fp);
                return brightness;
            }
        }
        pclose(fp);
    }
    
    /* Method 2: Try sysfs backlight interface */
    const char *backlight_paths[] = {
        "/sys/class/backlight/intel_backlight/brightness",
        "/sys/class/backlight/radeon_bl0/brightness",
        "/sys/class/backlight/nvidia_backlight/brightness",
        "/sys/class/backlight/acpi_video0/brightness",
        "/sys/class/backlight/amdgpu_bl0/brightness",
        NULL
    };
    
    const char *max_paths[] = {
        "/sys/class/backlight/intel_backlight/max_brightness",
        "/sys/class/backlight/radeon_bl0/max_brightness",
        "/sys/class/backlight/nvidia_backlight/max_brightness",
        "/sys/class/backlight/acpi_video0/max_brightness",
        "/sys/class/backlight/amdgpu_bl0/max_brightness",
        NULL
    };
    
    for (int i = 0; backlight_paths[i] != NULL; i++) {
        FILE *f = fopen(backlight_paths[i], "r");
        if (f) {
            int current = 0;
            if (fscanf(f, "%d", &current) == 1) {
                fclose(f);
                
                /* Get max brightness */
                FILE *fmax = fopen(max_paths[i], "r");
                if (fmax) {
                    int max_val = 0;
                    if (fscanf(fmax, "%d", &max_val) == 1 && max_val > 0) {
                        fclose(fmax);
                        brightness = (current * 100) / max_val;
                        return brightness;
                    }
                    fclose(fmax);
                }
            }
            fclose(f);
        }
    }
    
    /* Method 3: Try brightnessctl */
    fp = popen("brightnessctl g 2>/dev/null", "r");
    if (fp) {
        int current = 0;
        if (fscanf(fp, "%d", &current) == 1) {
            pclose(fp);
            fp = popen("brightnessctl m 2>/dev/null", "r");
            if (fp) {
                int max = 0;
                if (fscanf(fp, "%d", &max) == 1 && max > 0) {
                    brightness = (current * 100) / max;
                }
                pclose(fp);
                return brightness;
            }
        } else {
            pclose(fp);
        }
    }
    
    /* Method 4: Try xrandr as fallback */
    fp = popen("xrandr --verbose | grep -i 'brightness' | head -1 | awk '{print $2}' | sed 's/\\.//'", "r");
    if (fp) {
        char buf[16] = {0};
        if (fgets(buf, sizeof(buf), fp)) {
            int val = atoi(buf);
            if (val >= 10 && val <= 100) {
                brightness = val;
            }
        }
        pclose(fp);
    }
    
    return brightness;
}

/**
 * Sets screen brightness using multiple fallback methods.
 *
 * @param level Desired brightness percentage (10-100).
 *
 * @sideeffect Executes system commands to adjust brightness.
 */
static void set_brightness(int level)
{
    if (level < 10) level = 10;
    if (level > 100) level = 100;
    
    /* Method 1: Try xbacklight first */
    gchar *cmd = g_strdup_printf("xbacklight -set %d 2>/dev/null", level);
    int result = system(cmd);
    g_free(cmd);
    
    if (result == 0) return; /* Success with xbacklight */
    
    /* Method 2: Try sysfs backlight interface */
    const char *backlight_paths[] = {
        "/sys/class/backlight/intel_backlight/brightness",
        "/sys/class/backlight/radeon_bl0/brightness",
        "/sys/class/backlight/nvidia_backlight/brightness",
        "/sys/class/backlight/acpi_video0/brightness",
        "/sys/class/backlight/amdgpu_bl0/brightness",
        NULL
    };
    
    const char *max_paths[] = {
        "/sys/class/backlight/intel_backlight/max_brightness",
        "/sys/class/backlight/radeon_bl0/max_brightness",
        "/sys/class/backlight/nvidia_backlight/max_brightness",
        "/sys/class/backlight/acpi_video0/max_brightness",
        "/sys/class/backlight/amdgpu_bl0/max_brightness",
        NULL
    };
    
    for (int i = 0; backlight_paths[i] != NULL; i++) {
        FILE *fmax = fopen(max_paths[i], "r");
        if (fmax) {
            int max_val = 0;
            if (fscanf(fmax, "%d", &max_val) == 1 && max_val > 0) {
                fclose(fmax);
                
                int new_brightness = (level * max_val) / 100;
                
                /* Write to brightness file */
                FILE *f = fopen(backlight_paths[i], "w");
                if (f) {
                    fprintf(f, "%d", new_brightness);
                    fclose(f);
                    return; /* Success */
                }
            } else {
                fclose(fmax);
            }
        }
    }
    
    /* Method 3: Try brightnessctl */
    cmd = g_strdup_printf("brightnessctl s %d%% 2>/dev/null", level);
    result = system(cmd);
    g_free(cmd);
    
    if (result == 0) return;
    
    /* Method 4: Try xrandr as fallback */
    float brightness = level / 100.0f;
    cmd = g_strdup_printf("xrandr --output $(xrandr | grep ' connected' | head -1 | awk '{print $1}') --brightness %.2f 2>/dev/null &", brightness);
    system(cmd);
    g_free(cmd);
}

/**
 * Timeout callback to apply brightness after debouncing.
 *
 * @param brightness Level as gpointer (cast to int).
 * @return G_SOURCE_REMOVE to run only once.
 */
static gboolean set_brightness_timeout(gpointer brightness)
{
    int level = GPOINTER_TO_INT(brightness);
    set_brightness(level);
    brightness_timeout_id = 0;
    return G_SOURCE_REMOVE;
}

/**
 * Callback for brightness slider value changes.
 * Uses a timeout to debounce rapid changes and prevent scroll conflicts.
 *
 * @param scale      The GtkScale widget.
 * @param user_data  User data (unused).
 */
static void on_brightness_changed(GtkScale *scale, gpointer user_data)
{
    (void)user_data;
    
    /* Cancel previous timeout if exists */
    if (brightness_timeout_id > 0) {
        g_source_remove(brightness_timeout_id);
    }
    
    /* Schedule brightness change after 50ms to debounce */
    int brightness = (int)gtk_range_get_value(GTK_RANGE(scale));
    brightness_timeout_id = g_timeout_add(50, set_brightness_timeout, GINT_TO_POINTER(brightness));
}

/**
 * Callback for View Connection Details button.
 *
 * @param btn  The button that was clicked.
 * @param data User data (unused).
 */
static void on_view_connection_details(GtkButton *btn, gpointer data)
{
    (void)data;
    GtkWidget *toplevel = gtk_widget_get_toplevel(GTK_WIDGET(btn));
    show_connection_details(toplevel);
}

/**
 * Callback for Ethernet Settings button.
 *
 * @param btn  The button that was clicked.
 * @param data User data (unused).
 */
static void on_ethernet_settings(GtkButton *btn, gpointer data)
{
    (void)data;
    GtkWidget *toplevel = gtk_widget_get_toplevel(GTK_WIDGET(btn));
    show_internet_settings(toplevel);
}

/**
 * Callback for Refresh Networks button.
 * Triggers a WiFi rescan and refreshes the network list.
 *
 * @param btn  The button that was clicked.
 * @param data User data (unused).
 */
static void on_refresh_networks(GtkButton *btn, gpointer data)
{
    (void)btn;
    (void)data;
    system("nmcli device wifi rescan 2>/dev/null &");
    sleep(1);
    
    /* Refresh the network list in the main window */
    if (network_window) {
        refresh_wifi_network_list(network_window);
    }
}

/**
 * Callback for View All Networks button.
 *
 * @param btn  The button that was clicked.
 * @param data User data (unused).
 */
static void on_view_all_networks(GtkButton *btn, gpointer data)
{
    (void)data;
    GtkWidget *toplevel = gtk_widget_get_toplevel(GTK_WIDGET(btn));
    show_wifi_list(toplevel);
}

/**
 * Updates CPU usage statistics.
 * Uses persistent CpuData structure to track deltas across calls.
 */
static void panel_update_cpu_usage(void)
{
    /* Use persistent CPU data structure to properly track deltas across calls */
    update_cpu_usage(&panel_cpu_data);
    panel_cpu_percent = panel_cpu_data.usage;
}

/**
 * Updates memory usage statistics.
 */
static void panel_update_mem_usage(void)
{
    MemData mem;
    update_mem_usage(&mem);
    panel_mem_total = mem.total;
    panel_mem_available = mem.available;
    panel_mem_used = panel_mem_total - panel_mem_available;
    if (panel_mem_total > 0) {
        panel_mem_percent = (panel_mem_used * 100) / panel_mem_total;
    } else {
        panel_mem_percent = 0;
    }
}

/**
 * Finds a window with a given PID using the _NET_WM_PID X11 property.
 *
 * @param display X11 display connection.
 * @param pid     Process ID to search for.
 * @return        Window ID if found, None otherwise.
 */
static Window find_window_by_pid(Display *display, pid_t pid)
{
    Atom net_client_list = XInternAtom(display, "_NET_CLIENT_LIST", True);
    Atom net_wm_pid = XInternAtom(display, "_NET_WM_PID", True);
    Atom actual_type;
    int actual_format;
    unsigned long num_items;
    unsigned long bytes_after;
    unsigned char *data = NULL;
    
    if (net_client_list == None || net_wm_pid == None)
        return None;
    
    Window root = DefaultRootWindow(display);
    
    if (XGetWindowProperty(display, root, net_client_list, 0, ~0L, False, XA_WINDOW,
                          &actual_type, &actual_format, &num_items, &bytes_after,
                          &data) != Success)
        return None;
    
    if (actual_type != XA_WINDOW || actual_format != 32) {
        XFree(data);
        return None;
    }
    
    Window *windows = (Window*)data;
    Window result = None;
    
    for (unsigned long i = 0; i < num_items; i++) {
        Window win = windows[i];
        
        /* Get the PID property of this window */
        unsigned char *pid_data = NULL;
        unsigned long pid_items;
        if (XGetWindowProperty(display, win, net_wm_pid, 0, 1, False, XA_CARDINAL,
                               &actual_type, &actual_format, &pid_items, &bytes_after,
                               &pid_data) == Success) {
            if (actual_type == XA_CARDINAL && actual_format == 32 && pid_items > 0) {
                unsigned long win_pid = *(unsigned long*)pid_data;
                if (win_pid == (unsigned long)pid) {
                    result = win;
                    XFree(pid_data);
                    break;
                }
            }
            if (pid_data)
                XFree(pid_data);
        }
    }
    
    XFree(data);
    return result;
}

/**
 * Updates the network window display with current connection status.
 *
 * @param vbox The main vertical box containing network widgets.
 */
static void update_network_window_display(GtkWidget *vbox)
{
    /* Refresh network status */
    check_internet_connectivity();
    check_wifi_status();
    
    /* Find and update labels in the vbox */
    GList *children = gtk_container_get_children(GTK_CONTAINER(vbox));
    for (GList *l = children; l != NULL; l = l->next) {
        GtkWidget *child = GTK_WIDGET(l->data);
        if (GTK_IS_LABEL(child)) {
            const gchar *name = gtk_widget_get_name(child);
            if (name == NULL) continue;
            
            if (g_strcmp0(name, "internet-label") == 0) {
                gchar *text = g_strdup_printf(
                    "🌐 Internet: %s",
                    is_internet_connected ? "🟢 Online" : "🔴 Offline"
                );
                gtk_label_set_text(GTK_LABEL(child), text);
                g_free(text);
            } else if (g_strcmp0(name, "wifi-label") == 0) {
                gchar *text = g_strdup_printf(
                    "📡 WiFi: %s %s",
                    is_wifi_connected ? "🟢 Connected" : "🔴 Disconnected",
                    (wifi_ssid && is_wifi_connected) ? wifi_ssid : ""
                );
                gtk_label_set_text(GTK_LABEL(child), text);
                g_free(text);
            }
        } else if (GTK_IS_BUTTON(child)) {
            const gchar *name = gtk_widget_get_name(child);
            if (name && g_strcmp0(name, "wifi-toggle-btn") == 0) {
                /* Update WiFi toggle button text based on WiFi state */
                const gchar *label = is_wifi_connected ? "🔴 Turn Off WiFi" : "🟢 Turn On WiFi";
                gtk_button_set_label(GTK_BUTTON(child), label);
            }
        }
    }
    g_list_free(children);
}

/**
 * Timer callback for periodic network window updates.
 *
 * @param user_data The vbox widget containing network displays.
 * @return G_SOURCE_CONTINUE to keep timer active.
 */
static gboolean network_window_update_timer(gpointer user_data)
{
    if (network_window != NULL && gtk_widget_get_visible(network_window)) {
        update_network_window_display(GTK_WIDGET(user_data));
        return G_SOURCE_CONTINUE;
    } else if (network_window == NULL) {
        return G_SOURCE_REMOVE;
    }
    return G_SOURCE_CONTINUE;
}

/**
 * Cleanup handler for network window destruction.
 *
 * @param widget The destroyed widget.
 * @param data   User data (unused).
 */
static void on_network_window_destroy(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;
    network_window = NULL;
}

/**
 * Applies custom CSS styling to the network window.
 *
 * @param window The GtkWindow to style.
 */
static void apply_network_window_css(GtkWidget *window)
{
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "window { background-color: #0b0f14; color: #ffffff; }"
        "button { background-color: #1a1f26; color: #a92d5a; border: 1px solid #a92d5a; padding: 10px 15px; margin: 2px; font-weight: bold; border-radius: 4px; }"
        "button:hover { background-color: #252d36; color: #472e57; border: 1px solid #472e57; }"
        "label { color: #a92d5a; font-size: 11px; font-weight: 500; }"
        "frame { border: 1px solid #a92d5a; border-radius: 4px; }"
        "frame > label { color: #a92d5a; }"
        "scrolledwindow { border: 1px solid #a92d5a; }"
        "scale { padding: 8px; }"
        "scale trough { border-radius: 4px; background-color: #1a1f26; border: 1px solid #a92d5a; }"
        "scale highlight { background-color: #a92d5a; }",
        -1, NULL);
    
    /* Apply CSS only to this specific window */
    gtk_style_context_add_provider(
        gtk_widget_get_style_context(window),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    
    g_object_unref(provider);
}

/**
 * Launches the network settings window/tab.
 *
 * @param button The button that was clicked.
 * @param data   User data (unused).
 *
 * @sideeffect Creates and displays network settings window.
 */
static void launch_network_tab(GtkButton *button, gpointer data)
{
    (void)button;
    (void)data;
    
    /* If window already exists, just show it */
    if (network_window != NULL) {
        gtk_window_present(GTK_WINDOW(network_window));
        return;
    }
    
    /* Create new network status window */
    network_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(network_window), "Settings");
    gtk_window_set_default_size(GTK_WINDOW(network_window), 380, 550);
    gtk_window_set_decorated(GTK_WINDOW(network_window), TRUE);
    gtk_window_set_type_hint(GTK_WINDOW(network_window), GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_keep_above(GTK_WINDOW(network_window), TRUE);
    gtk_window_set_modal(GTK_WINDOW(network_window), FALSE);
    gtk_window_set_transient_for(GTK_WINDOW(network_window), NULL);
    gtk_window_set_resizable(GTK_WINDOW(network_window), TRUE);
    
    /* Position at top-right corner (below network button) */
    GdkDisplay *gdisplay = gdk_display_get_default();
    GdkMonitor *monitor = gdk_display_get_primary_monitor(gdisplay);
    GdkRectangle geom;
    gdk_monitor_get_geometry(monitor, &geom);
    gtk_window_move(GTK_WINDOW(network_window), geom.width - 380, geom.y + 40);
    
    g_signal_connect(network_window, "destroy", G_CALLBACK(on_network_window_destroy), NULL);
    
    /* Apply CSS only to this window */
    apply_network_window_css(network_window);
    
    /* Main vertical box - this is the direct child of window */
    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(network_window), main_vbox);
    
    /* Title bar for the window */
    GtkWidget *title_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_name(title_bar, "settings-title-bar");
    gtk_widget_set_size_request(title_bar, -1, 30);
    gtk_box_pack_start(GTK_BOX(main_vbox), title_bar, FALSE, FALSE, 0);
    
    GtkWidget *title_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title_label), "<b>System Settings</b>");
    gtk_label_set_xalign(GTK_LABEL(title_label), 0.5);
    gtk_box_pack_start(GTK_BOX(title_bar), title_label, TRUE, TRUE, 10);
    
    GtkWidget *close_title_btn = gtk_button_new_with_label("✕");
    gtk_widget_set_size_request(close_title_btn, 25, 25);
    g_signal_connect_swapped(close_title_btn, "clicked", G_CALLBACK(gtk_widget_destroy), network_window);
    gtk_box_pack_end(GTK_BOX(title_bar), close_title_btn, FALSE, FALSE, 5);
    
    /* Separator under title bar */
    GtkWidget *title_sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(main_vbox), title_sep, FALSE, FALSE, 0);
    
    /* Scrolled window for the main content - THIS IS KEY FOR SCROLLING */
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), 
                                   GTK_POLICY_NEVER,      /* No horizontal scroll */
                                   GTK_POLICY_AUTOMATIC); /* Vertical scroll when needed */
    gtk_widget_set_hexpand(scroll, TRUE);
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_box_pack_start(GTK_BOX(main_vbox), scroll, TRUE, TRUE, 0);
    
    /* Main content box inside scroll */
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 15);
    gtk_widget_set_hexpand(vbox, TRUE);
    gtk_widget_set_vexpand(vbox, FALSE);
    gtk_container_add(GTK_CONTAINER(scroll), vbox);
    
    /* Store vbox reference for updates */
    g_object_set_data(G_OBJECT(network_window), "vbox", vbox);
    
    /* ============ INTERNET SECTION ============ */
    GtkWidget *inet_title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(inet_title), "<b>🌐 Internet</b>");
    gtk_label_set_xalign(GTK_LABEL(inet_title), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), inet_title, FALSE, FALSE, 0);
    
    /* Internet status */
    GtkWidget *inet_status = gtk_label_new("🌐 Internet: Loading...");
    gtk_widget_set_name(inet_status, "internet-label");
    gtk_label_set_xalign(GTK_LABEL(inet_status), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), inet_status, FALSE, FALSE, 0);
    
    /* Internet management frame */
    GtkWidget *inet_frame = gtk_frame_new("Connection Management");
    gtk_box_pack_start(GTK_BOX(vbox), inet_frame, FALSE, FALSE, 0);
    
    GtkWidget *inet_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(inet_vbox), 10);
    gtk_container_add(GTK_CONTAINER(inet_frame), inet_vbox);
    
    /* Connection info button */
    GtkWidget *conn_btn = gtk_button_new_with_label("📊 View Connection Details");
    g_signal_connect(conn_btn, "clicked", G_CALLBACK(on_view_connection_details), NULL);
    gtk_box_pack_start(GTK_BOX(inet_vbox), conn_btn, FALSE, FALSE, 0);
    
    /* Ethernet button */
    GtkWidget *eth_btn = gtk_button_new_with_label("🔗 Ethernet Settings");
    g_signal_connect(eth_btn, "clicked", G_CALLBACK(on_ethernet_settings), NULL);
    gtk_box_pack_start(GTK_BOX(inet_vbox), eth_btn, FALSE, FALSE, 0);
    
    /* ============ WiFi SECTION ============ */
    GtkWidget *wifi_sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), wifi_sep, FALSE, FALSE, 0);
    
    GtkWidget *wifi_title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(wifi_title), "<b>📡 WiFi Networks</b>");
    gtk_label_set_xalign(GTK_LABEL(wifi_title), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), wifi_title, FALSE, FALSE, 0);
    
    /* Current WiFi status */
    GtkWidget *current_wifi = gtk_label_new("📡 WiFi: Loading...");
    gtk_widget_set_name(current_wifi, "wifi-label");
    gtk_label_set_xalign(GTK_LABEL(current_wifi), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), current_wifi, FALSE, FALSE, 0);
    
    /* WiFi toggle button with correct initial state */
    gboolean wifi_enabled = is_wifi_enabled();
    GtkWidget *wifi_toggle = gtk_button_new_with_label(wifi_enabled ? "🔴 Turn Off WiFi" : "🟢 Turn On WiFi");
    gtk_widget_set_name(wifi_toggle, "wifi-toggle-btn");
    g_signal_connect(wifi_toggle, "clicked", G_CALLBACK(toggle_wifi_simple), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), wifi_toggle, FALSE, FALSE, 0);
    
    /* WiFi networks frame - this needs to expand */
    GtkWidget *networks_frame = gtk_frame_new("Available Networks");
    gtk_widget_set_hexpand(networks_frame, TRUE);
    gtk_widget_set_vexpand(networks_frame, TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), networks_frame, TRUE, TRUE, 0);
    
    /* Inner scrolled window for network list */
    GtkWidget *net_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(net_scroll), 
                                   GTK_POLICY_AUTOMATIC, 
                                   GTK_POLICY_AUTOMATIC);
    gtk_widget_set_hexpand(net_scroll, TRUE);
    gtk_widget_set_vexpand(net_scroll, TRUE);
    gtk_container_add(GTK_CONTAINER(networks_frame), net_scroll);
    
    /* Create connectable network list */
    GtkWidget *net_list = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_container_set_border_width(GTK_CONTAINER(net_list), 10);
    
    if (!wifi_enabled) {
        GtkWidget *wifi_off = gtk_label_new("WiFi is disabled");
        gtk_box_pack_start(GTK_BOX(net_list), wifi_off, TRUE, TRUE, 0);
    } else {
        /* Get WiFi networks */
        FILE *fp = popen("nmcli -t -f SSID,SIGNAL,SECURITY device wifi list --rescan yes 2>/dev/null", "r");
        if (fp) {
            char line[512];
            while (fgets(line, sizeof(line), fp)) {
                line[strcspn(line, "\n")] = 0;
                if (strlen(line) < 2) continue;
                
                char *ssid = strtok(line, ":");
                char *signal_str = strtok(NULL, ":");
                char *security = strtok(NULL, ":");
                
                if (!ssid || strcmp(ssid, "SSID") == 0 || strlen(ssid) == 0) continue;
                
                int signal = signal_str ? atoi(signal_str) : 0;
                
                /* Determine signal icon */
                const char *signal_icon = "🔴";
                if (signal >= 80) signal_icon = "🟢";
                else if (signal >= 60) signal_icon = "🟢";
                else if (signal >= 40) signal_icon = "🟡";
                else if (signal >= 20) signal_icon = "🟠";
                
                /* Security icon */
                const char *lock_icon = "";
                if (security && strlen(security) > 0 && strcmp(security, "--") != 0) {
                    lock_icon = "🔒 ";
                }
                
                gchar *btn_label = g_strdup_printf("%s %s%s   %d%%", signal_icon, lock_icon, ssid, signal);
                GtkWidget *net_btn = gtk_button_new_with_label(btn_label);
                g_free(btn_label);
                
                /* Store network info */
                g_object_set_data_full(G_OBJECT(net_btn), "ssid", g_strdup(ssid), g_free);
                g_object_set_data_full(G_OBJECT(net_btn), "security", g_strdup(security ? security : ""), g_free);
                
                /* Connect to callback */
                g_signal_connect(net_btn, "clicked", G_CALLBACK(on_wifi_network_clicked), network_window);
                
                gtk_widget_set_margin_top(net_btn, 2);
                gtk_widget_set_margin_bottom(net_btn, 2);
                gtk_box_pack_start(GTK_BOX(net_list), net_btn, FALSE, FALSE, 0);
            }
            pclose(fp);
        }
    }
    
    gtk_container_add(GTK_CONTAINER(net_scroll), net_list);
    gtk_widget_set_name(net_list, "wifi-list");
    
    /* Refresh networks button */
    GtkWidget *refresh_btn = gtk_button_new_with_label("🔄 Refresh Networks");
    g_signal_connect(refresh_btn, "clicked", G_CALLBACK(on_refresh_networks), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), refresh_btn, FALSE, FALSE, 0);
    
    /* View all networks button */
    GtkWidget *view_all_btn = gtk_button_new_with_label("📡 View All Networks");
    g_signal_connect(view_all_btn, "clicked", G_CALLBACK(on_view_all_networks), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), view_all_btn, FALSE, FALSE, 0);
    
    /* ============ BRIGHTNESS SECTION ============ */
    GtkWidget *bright_sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), bright_sep, FALSE, FALSE, 5);
    
    GtkWidget *bright_title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(bright_title), "<b>☀️ Brightness</b>");
    gtk_label_set_xalign(GTK_LABEL(bright_title), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), bright_title, FALSE, FALSE, 0);
    
    /* Brightness slider - block scroll events to prevent conflict */
    int current_brightness = get_brightness();
    GtkWidget *brightness_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 10, 100, 1);
    gtk_range_set_value(GTK_RANGE(brightness_scale), current_brightness);
    gtk_scale_set_draw_value(GTK_SCALE(brightness_scale), TRUE);
    gtk_scale_set_value_pos(GTK_SCALE(brightness_scale), GTK_POS_RIGHT);
    gtk_widget_set_size_request(brightness_scale, 200, 30);
    
    /* Block scroll events on the slider to prevent interfering with window scrolling */
    g_signal_connect(brightness_scale, "scroll-event", G_CALLBACK(gtk_true), NULL);
    
    g_signal_connect(brightness_scale, "value-changed", G_CALLBACK(on_brightness_changed), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), brightness_scale, FALSE, FALSE, 5);
    
    /* ============ FOOTER ============ */
    GtkWidget *footer_sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), footer_sep, FALSE, FALSE, 0);
    
    /* Close button */
    GtkWidget *close_btn = gtk_button_new_with_label("Close");
    g_signal_connect_swapped(close_btn, "clicked", G_CALLBACK(gtk_widget_destroy), network_window);
    gtk_box_pack_start(GTK_BOX(vbox), close_btn, FALSE, FALSE, 0);
    
    /* Initial update */
    update_network_window_display(vbox);
    
    /* Start timer for updates */
    g_timeout_add(2000, network_window_update_timer, vbox);
    
    gtk_widget_show_all(network_window);
}

/**
 * Launches the tools window.
 * Checks for existing window and raises it if found, otherwise forks a new process.
 *
 * @param button The button that was clicked.
 * @param data   User data (unused).
 *
 * @sideeffect Forks a child process to launch blackline-tools.
 */
static void launch_tools(GtkButton *button, gpointer data)
{
    (void)button;
    (void)data;
    
    /* Check if the window still exists */
    if (tools_pid > 0) {
        Display *display = XOpenDisplay(NULL);
        if (display) {
            Window win = find_window_by_pid(display, tools_pid);
            if (win != None) {
                /* Window exists – raise it */
                XRaiseWindow(display, win);
                XMapRaised(display, win);
                XFlush(display);
                XCloseDisplay(display);
                return;
            }
            XCloseDisplay(display);
        }
        /* If we get here, the window is gone – reset PID */
        tools_pid = 0;
    }
    
    /* Launch new tools container */
    pid_t pid = fork();
    if (pid == 0) {
        /* Child process */
        execl("./blackline-tools", "blackline-tools", NULL);
        exit(0);
    } else if (pid > 0) {
        /* Parent – store the PID */
        tools_pid = pid;
    }
}

/**
 * Placeholder callback for buttons with no functionality.
 *
 * @param button The button that was clicked.
 * @param data   User data (unused).
 */
static void do_nothing(GtkButton *button, gpointer data)
{
    (void)button;
    (void)data;
}

/**
 * Formats network speed for display with appropriate units.
 *
 * @param speed_bytes Speed in bytes per second.
 * @return Pointer to static string buffer containing formatted speed.
 */
static const char* format_speed(double speed_bytes)
{
    static char buffer[64];
    double speed = speed_bytes;
    if (speed < 1024) {
        snprintf(buffer, sizeof(buffer), "%.0f B/s", speed);
    } else if (speed < 1024 * 1024) {
        snprintf(buffer, sizeof(buffer), "%.1f KB/s", speed / 1024.0);
    } else {
        snprintf(buffer, sizeof(buffer), "%.1f MB/s", speed / (1024.0 * 1024.0));
    }
    return buffer;
}

/**
 * Timer callback for periodic system statistics updates.
 * Updates CPU, memory, network, and battery displays.
 *
 * @param user_data The CPU label widget (used to access other widgets via object data).
 * @return G_SOURCE_CONTINUE to keep timer active.
 */
static gboolean update_system_stats(gpointer user_data)
{
    GtkWidget *cpu_label = GTK_WIDGET(user_data);
    GtkWidget *mem_label = GTK_WIDGET(g_object_get_data(G_OBJECT(cpu_label), "mem-label"));
    GtkWidget *upload_label = GTK_WIDGET(g_object_get_data(G_OBJECT(cpu_label), "upload-label"));
    GtkWidget *download_label = GTK_WIDGET(g_object_get_data(G_OBJECT(cpu_label), "download-label"));
    GtkWidget *battery_label = GTK_WIDGET(g_object_get_data(G_OBJECT(cpu_label), "battery-label"));
    GtkWidget *network_btn = GTK_WIDGET(g_object_get_data(G_OBJECT(cpu_label), "network-btn"));
    
    if (!cpu_label || !mem_label || !upload_label || !download_label || !battery_label || !network_btn) {
        return G_SOURCE_CONTINUE;
    }
    
    /* Update CPU using wrapper */
    panel_update_cpu_usage();
    
    /* Smooth CPU display with moving average */
    panel_cpu_history[panel_cpu_history_index] = panel_cpu_percent;
    panel_cpu_history_index = (panel_cpu_history_index + 1) % 5;
    
    float avg_cpu = 0;
    for (int i = 0; i < 5; i++) {
        avg_cpu += panel_cpu_history[i];
    }
    avg_cpu /= 5;
    
    char cpu_text[64];
    if (avg_cpu < 10) {
        snprintf(cpu_text, sizeof(cpu_text), "CPU:  %.1f%%", avg_cpu);
    } else {
        snprintf(cpu_text, sizeof(cpu_text), "CPU: %.1f%%", avg_cpu);
    }
    gtk_label_set_text(GTK_LABEL(cpu_label), cpu_text);
    
    /* Update Memory using wrapper */
    panel_update_mem_usage();
    
    char mem_text[64];
    snprintf(mem_text, sizeof(mem_text), "RAM: %d%%", panel_mem_percent);
    gtk_label_set_text(GTK_LABEL(mem_label), mem_text);
    
    /* Update Network stats */
    network_stats_update();
    double upload_speed = network_stats_get_upload() * 1024.0;  /* Convert KB/s to B/s */
    double download_speed = network_stats_get_download() * 1024.0;

    char upload_text[64];
    char download_text[64];

    snprintf(upload_text, sizeof(upload_text), "↑ %s", format_speed(upload_speed));
    snprintf(download_text, sizeof(download_text), "↓ %s", format_speed(download_speed));

    gtk_label_set_text(GTK_LABEL(upload_label), upload_text);
    gtk_label_set_text(GTK_LABEL(download_label), download_text);
    
    /* Update Battery (every 15 ticks = 30 seconds) */
    static int battery_counter = 0;
    if (battery_counter++ % 15 == 0) {
        battery_percent = get_battery_percent();
        battery_charging = check_battery_charging();
        
        char battery_text[64];
        if (battery_percent >= 0) {
            const char *icon = "🔋";
            if (battery_charging) {
                icon = "🔌";
            } else if (battery_percent <= 15) {
                icon = "🪫";
            }
            snprintf(battery_text, sizeof(battery_text), "%s %d%%", icon, battery_percent);
        } else {
            snprintf(battery_text, sizeof(battery_text), "🔋 N/A");
        }
        gtk_button_set_label(GTK_BUTTON(battery_label), battery_text);
    }
    
    return G_SOURCE_CONTINUE;
}

/**
 * Timer callback for clock updates.
 *
 * @param label The GtkLabel to update with current time.
 * @return G_SOURCE_CONTINUE to keep timer active.
 */
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

/**
 * Applies custom CSS styling to the panel window.
 *
 * @param window The GtkWindow to style.
 */
static void apply_panel_css(GtkWidget *window)
{
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "window { background-color: #0b0f14; color: #a92d5a; border-bottom: 1px solid #a92d5a; }"
        "button { background-color: #1e2429; color: #a92d5a; border: 1px solid #a92d5a; padding: 2px 8px; margin: 2px; border-radius: 3px; }"
        "button:hover { background-color: #2a323a; border: 1px solid #a92d5a; }"
        "label { color: #a92d5a; padding: 0 4px; font-size: 11px; }"
        "#stats-box { margin-right: 8px; }"
        "#stats-box label { font-family: monospace; }",
        -1, NULL);
    
    /* Apply CSS only to this panel window */
    gtk_style_context_add_provider(
        gtk_widget_get_style_context(window),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    
    g_object_unref(provider);
}

/**
 * Application activation callback.
 * Creates the main panel window with system monitoring widgets.
 *
 * @param app        The GtkApplication instance.
 * @param user_data  User data passed during signal connection (unused).
 *
 * @sideeffect Creates the panel UI and starts periodic update timers.
 */
static void activate(GtkApplication *app, gpointer user_data)
{
    (void)app;
    (void)user_data;
    
    /* Initialize the minimized container */
    minimized_container_initialize();
    
    /* Initialize network stats */
    network_stats_init();
    
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "BlackLine Panel");
    gtk_window_set_default_size(GTK_WINDOW(window), -1, 35);
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
    gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_DOCK);
    
    /* Apply CSS only to the panel window */
    apply_panel_css(window);
    
    /* Full width */
    GdkDisplay *gdisplay = gdk_display_get_default();
    GdkMonitor *monitor = gdk_display_get_primary_monitor(gdisplay);
    GdkRectangle geom;
    gdk_monitor_get_geometry(monitor, &geom);
    gtk_window_set_default_size(GTK_WINDOW(window), geom.width, 35);
    gtk_window_move(GTK_WINDOW(window), 0, 0);
    
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_container_set_border_width(GTK_CONTAINER(box), 2);
    gtk_container_add(GTK_CONTAINER(window), box);
    
    /* Left section with buttons */
    GtkWidget *left_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_start(GTK_BOX(box), left_box, FALSE, FALSE, 0);
    
    /* BlackLine button */
    GtkWidget *btn1 = gtk_button_new_with_label("BlackLine");
    g_signal_connect(btn1, "clicked", G_CALLBACK(do_nothing), NULL);
    gtk_box_pack_start(GTK_BOX(left_box), btn1, FALSE, FALSE, 2);
    
    /* Tools button */
    GtkWidget *btn2 = gtk_button_new_with_label("Tools");
    g_signal_connect(btn2, "clicked", G_CALLBACK(launch_tools), NULL);
    gtk_box_pack_start(GTK_BOX(left_box), btn2, FALSE, FALSE, 2);
    
    /* Minimized Apps button (📌) */
    GtkWidget *min_btn = minimized_container_get_toggle_button();
    gtk_box_pack_start(GTK_BOX(left_box), min_btn, FALSE, FALSE, 2);
    
    /* Center spacer */
    GtkWidget *spacer = gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(box), spacer, TRUE, TRUE, 0);
    
    /* System stats container (right side) */
    GtkWidget *stats_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_set_name(stats_box, "stats-box");
    gtk_box_pack_end(GTK_BOX(box), stats_box, FALSE, FALSE, 4);
    
    /* CPU Label */
    GtkWidget *cpu_label = gtk_label_new("CPU: 0.0%");
    gtk_box_pack_start(GTK_BOX(stats_box), cpu_label, FALSE, FALSE, 2);
    
    /* Memory Label */
    GtkWidget *mem_label = gtk_label_new("RAM: 0%");
    gtk_box_pack_start(GTK_BOX(stats_box), mem_label, FALSE, FALSE, 2);
    
    /* Separator */
    GtkWidget *sep1 = gtk_label_new("|");
    gtk_box_pack_start(GTK_BOX(stats_box), sep1, FALSE, FALSE, 2);
    
    /* Upload Label */
    GtkWidget *upload_label = gtk_label_new("↑ 0 KB/s");
    gtk_box_pack_start(GTK_BOX(stats_box), upload_label, FALSE, FALSE, 2);
    
    /* Download Label */
    GtkWidget *download_label = gtk_label_new("↓ 0 KB/s");
    gtk_box_pack_start(GTK_BOX(stats_box), download_label, FALSE, FALSE, 2);
    
    /* Separator */
    GtkWidget *sep2 = gtk_label_new("|");
    gtk_box_pack_start(GTK_BOX(stats_box), sep2, FALSE, FALSE, 2);
    
    /* Battery Label Button */
    GtkWidget *battery_label = gtk_button_new_with_label("🔋 0%");
    gtk_button_set_relief(GTK_BUTTON(battery_label), GTK_RELIEF_NONE);
    gtk_box_pack_start(GTK_BOX(stats_box), battery_label, FALSE, FALSE, 2);
    
    /* Separator */
    GtkWidget *sep3 = gtk_label_new("|");
    gtk_box_pack_start(GTK_BOX(stats_box), sep3, FALSE, FALSE, 2);
        
    /* Network Status Button with Gear icon */
    GtkWidget *network_btn = gtk_button_new_with_label("⚙️ Gear");
    gtk_button_set_relief(GTK_BUTTON(network_btn), GTK_RELIEF_NONE);
    g_signal_connect(network_btn, "clicked", G_CALLBACK(launch_network_tab), NULL);
    gtk_box_pack_start(GTK_BOX(stats_box), network_btn, FALSE, FALSE, 2);

    /* Separator */
    GtkWidget *sep4 = gtk_label_new("|");
    gtk_box_pack_start(GTK_BOX(stats_box), sep4, FALSE, FALSE, 2);
    
    /* Clock with date */
    GtkWidget *clock = gtk_label_new(NULL);
    update_clock(clock);
    gtk_box_pack_start(GTK_BOX(stats_box), clock, FALSE, FALSE, 2);
    
    /* Store references for stats update */
    g_object_set_data(G_OBJECT(cpu_label), "mem-label", mem_label);
    g_object_set_data(G_OBJECT(cpu_label), "upload-label", upload_label);
    g_object_set_data(G_OBJECT(cpu_label), "download-label", download_label);
    g_object_set_data(G_OBJECT(cpu_label), "battery-label", battery_label);
    g_object_set_data(G_OBJECT(cpu_label), "network-btn", network_btn);
    
    /* Update timers */
    g_timeout_add_seconds(1, update_clock, clock);
    g_timeout_add(2000, update_system_stats, cpu_label);  /* Update every 2 seconds */
    
    gtk_widget_show_all(window);
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
    GtkApplication *app = gtk_application_new("org.blackline.panel", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    
    /* Cleanup all modules */
    network_stats_cleanup();
    if (wifi_ssid != NULL) {
        g_free(wifi_ssid);
    }
    wifi_list_cleanup();
    wifi_connect_cleanup();
    internet_settings_cleanup();
    
    return status;
}