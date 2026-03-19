#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#define CONFIG_DIR "/.config/blackline"
#define NETWORK_CONFIG_FILE "/network_settings.conf"

static GtkCssProvider *internet_settings_provider = NULL;
static char *config_path = NULL;

// Structure to hold network settings
typedef struct {
    char interface[32];
    char method[16];  // "dhcp" or "static"
    char ip_address[32];
    char netmask[32];
    char gateway[32];
    char dns[32];
} NetworkSettings;

static NetworkSettings current_settings = {
    .interface = "",
    .method = "dhcp",
    .ip_address = "",
    .netmask = "",
    .gateway = "",
    .dns = ""
};

// Forward declaration for callback
static void on_method_toggled(GtkToggleButton *btn, gpointer data);

// Get config directory path
static const char* get_config_dir(void)
{
    static char path[512];
    const char *home = getenv("HOME");
    if (!home) home = "/root";
    snprintf(path, sizeof(path), "%s%s", home, CONFIG_DIR);
    return path;
}

// Get config file path
static const char* get_config_file(void)
{
    static char path[512];
    snprintf(path, sizeof(path), "%s%s", get_config_dir(), NETWORK_CONFIG_FILE);
    return path;
}

// Ensure config directory exists
static void ensure_config_dir(void)
{
    const char *dir = get_config_dir();
    struct stat st = {0};
    if (stat(dir, &st) == -1) {
        mkdir(dir, 0700);
    }
}

// Save network settings to file
static void save_network_settings(void)
{
    ensure_config_dir();
    FILE *f = fopen(get_config_file(), "w");
    if (!f) {
        g_printerr("Failed to save network settings\n");
        return;
    }
    
    fprintf(f, "interface=%s\n", current_settings.interface);
    fprintf(f, "method=%s\n", current_settings.method);
    fprintf(f, "ip_address=%s\n", current_settings.ip_address);
    fprintf(f, "netmask=%s\n", current_settings.netmask);
    fprintf(f, "gateway=%s\n", current_settings.gateway);
    fprintf(f, "dns=%s\n", current_settings.dns);
    
    fclose(f);
    printf("Network settings saved to %s\n", get_config_file());
}

// Load network settings from file
static void load_network_settings(void)
{
    FILE *f = fopen(get_config_file(), "r");
    if (!f) {
        printf("No saved network settings found, using defaults\n");
        return;
    }
    
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0;
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = line;
        char *value = eq + 1;
        
        if (strcmp(key, "interface") == 0) {
            strncpy(current_settings.interface, value, sizeof(current_settings.interface) - 1);
        } else if (strcmp(key, "method") == 0) {
            strncpy(current_settings.method, value, sizeof(current_settings.method) - 1);
        } else if (strcmp(key, "ip_address") == 0) {
            strncpy(current_settings.ip_address, value, sizeof(current_settings.ip_address) - 1);
        } else if (strcmp(key, "netmask") == 0) {
            strncpy(current_settings.netmask, value, sizeof(current_settings.netmask) - 1);
        } else if (strcmp(key, "gateway") == 0) {
            strncpy(current_settings.gateway, value, sizeof(current_settings.gateway) - 1);
        } else if (strcmp(key, "dns") == 0) {
            strncpy(current_settings.dns, value, sizeof(current_settings.dns) - 1);
        }
    }
    fclose(f);
    printf("Network settings loaded from %s\n", get_config_file());
}

// Apply CSS to a specific widget only
static void apply_widget_css(GtkWidget *widget)
{
    if (internet_settings_provider == NULL) {
        internet_settings_provider = gtk_css_provider_new();
        gtk_css_provider_load_from_data(internet_settings_provider,
            "window { background-color: #0b0f14; color: #ffffff; }"
            "button { background-color: #1a1f26; color: #a92d5a; border: 1px solid #a92d5a; padding: 8px 15px; margin: 2px; font-weight: bold; border-radius: 4px; }"
            "button:hover { background-color: #252d36; color: #472e57; border: 1px solid #472e57; }"
            "label { color: #a92d5a; font-size: 11px; font-weight: 500; }"
            "entry { background-color: #1a1f26; color: #a92d5a; border: 1px solid #a92d5a; padding: 6px; border-radius: 4px; }"
            "combobox { background-color: #1a1f26; color: #a92d5a; border: 1px solid #a92d5a; }"
            "combobox entry { background-color: #1a1f26; color: #a92d5a; }"
            "frame { border: 1px solid #a92d5a; border-radius: 4px; }",
            -1, NULL);
    }
    
    gtk_style_context_add_provider(
        gtk_widget_get_style_context(widget),
        GTK_STYLE_PROVIDER(internet_settings_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

// Get current network interfaces
static char** get_network_interfaces(int *count)
{
    FILE *fp = popen("ls /sys/class/net/ 2>/dev/null | grep -v lo", "r");
    if (!fp) return NULL;
    
    char **interfaces = NULL;
    char line[256];
    int num = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0;
        interfaces = realloc(interfaces, (num + 1) * sizeof(char*));
        interfaces[num] = strdup(line);
        num++;
    }
    pclose(fp);
    
    *count = num;
    return interfaces;
}

// Get current connection method for an interface
static const char* get_connection_method(const char *iface)
{
    static char method[32] = "dhcp";
    char cmd[256];
    char result[256] = {0};
    
    snprintf(cmd, sizeof(cmd), "nmcli -t -f ipv4.method connection show '%s' 2>/dev/null | head -1", iface);
    FILE *fp = popen(cmd, "r");
    if (fp) {
        if (fgets(result, sizeof(result), fp)) {
            result[strcspn(result, "\n")] = 0;
            strcpy(method, result);
        }
        pclose(fp);
    }
    return method;
}

// Callback for method toggle
static void on_method_toggled(GtkToggleButton *btn, gpointer data)
{
    GtkWidget *frame = GTK_WIDGET(data);
    gtk_widget_set_sensitive(frame, !gtk_toggle_button_get_active(btn));
}

// Show internet settings dialog
void show_internet_settings(GtkWidget *parent_window)
{
    // Load saved settings first
    load_network_settings();
    
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Internet Settings",
        GTK_WINDOW(parent_window),
        GTK_DIALOG_MODAL,
        "Apply",
        GTK_RESPONSE_APPLY,
        "Close",
        GTK_RESPONSE_CLOSE,
        NULL
    );
    
    gtk_window_set_default_size(GTK_WINDOW(dialog), 500, 400);
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
    
    // Apply CSS to dialog
    apply_widget_css(dialog);
    
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 15);
    gtk_container_add(GTK_CONTAINER(content), vbox);
    
    // Title
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<b>🔧 Internet Connection Settings</b>");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), title, FALSE, FALSE, 0);
    apply_widget_css(title);
    
    // Interface selection
    GtkWidget *iface_frame = gtk_frame_new("Network Interface");
    gtk_box_pack_start(GTK_BOX(vbox), iface_frame, FALSE, FALSE, 0);
    apply_widget_css(iface_frame);
    
    GtkWidget *iface_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(iface_box), 10);
    gtk_container_add(GTK_CONTAINER(iface_frame), iface_box);
    
    int iface_count = 0;
    char **interfaces = get_network_interfaces(&iface_count);
    
    GtkWidget *iface_combo = gtk_combo_box_text_new();
    int active_idx = 0;
    for (int i = 0; i < iface_count; i++) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(iface_combo), interfaces[i]);
        if (strcmp(interfaces[i], current_settings.interface) == 0) {
            active_idx = i;
        }
        free(interfaces[i]);
    }
    free(interfaces);
    
    if (iface_count > 0) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(iface_combo), active_idx);
    }
    gtk_box_pack_start(GTK_BOX(iface_box), iface_combo, FALSE, FALSE, 0);
    apply_widget_css(iface_combo);
    
    // Connection method frame
    GtkWidget *method_frame = gtk_frame_new("Connection Method");
    gtk_box_pack_start(GTK_BOX(vbox), method_frame, FALSE, FALSE, 0);
    apply_widget_css(method_frame);
    
    GtkWidget *method_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(method_box), 10);
    gtk_container_add(GTK_CONTAINER(method_frame), method_box);
    
    GSList *method_group = NULL;
    GtkWidget *dhcp_radio = gtk_radio_button_new_with_label(method_group, "Automatic (DHCP)");
    method_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(dhcp_radio));
    gtk_box_pack_start(GTK_BOX(method_box), dhcp_radio, FALSE, FALSE, 0);
    apply_widget_css(dhcp_radio);
    
    GtkWidget *static_radio = gtk_radio_button_new_with_label(method_group, "Manual (Static IP)");
    method_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(static_radio));
    gtk_box_pack_start(GTK_BOX(method_box), static_radio, FALSE, FALSE, 0);
    apply_widget_css(static_radio);
    
    // Set active based on saved settings
    if (strcmp(current_settings.method, "static") == 0) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(static_radio), TRUE);
    } else {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dhcp_radio), TRUE);
    }
    
    // Static IP settings frame
    GtkWidget *static_frame = gtk_frame_new("Static IP Configuration");
    gtk_box_pack_start(GTK_BOX(vbox), static_frame, FALSE, FALSE, 0);
    apply_widget_css(static_frame);
    
    GtkWidget *static_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(static_grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(static_grid), 10);
    gtk_container_set_border_width(GTK_CONTAINER(static_grid), 10);
    gtk_container_add(GTK_CONTAINER(static_frame), static_grid);
    
    // IP Address
    GtkWidget *ip_label = gtk_label_new("IP Address:");
    gtk_grid_attach(GTK_GRID(static_grid), ip_label, 0, 0, 1, 1);
    apply_widget_css(ip_label);
    
    GtkWidget *ip_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(ip_entry), current_settings.ip_address);
    gtk_entry_set_placeholder_text(GTK_ENTRY(ip_entry), "192.168.1.100");
    gtk_grid_attach(GTK_GRID(static_grid), ip_entry, 1, 0, 1, 1);
    apply_widget_css(ip_entry);
    
    // Netmask
    GtkWidget *netmask_label = gtk_label_new("Netmask:");
    gtk_grid_attach(GTK_GRID(static_grid), netmask_label, 0, 1, 1, 1);
    apply_widget_css(netmask_label);
    
    GtkWidget *netmask_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(netmask_entry), current_settings.netmask);
    gtk_entry_set_placeholder_text(GTK_ENTRY(netmask_entry), "255.255.255.0");
    gtk_grid_attach(GTK_GRID(static_grid), netmask_entry, 1, 1, 1, 1);
    apply_widget_css(netmask_entry);
    
    // Gateway
    GtkWidget *gateway_label = gtk_label_new("Gateway:");
    gtk_grid_attach(GTK_GRID(static_grid), gateway_label, 0, 2, 1, 1);
    apply_widget_css(gateway_label);
    
    GtkWidget *gateway_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(gateway_entry), current_settings.gateway);
    gtk_entry_set_placeholder_text(GTK_ENTRY(gateway_entry), "192.168.1.1");
    gtk_grid_attach(GTK_GRID(static_grid), gateway_entry, 1, 2, 1, 1);
    apply_widget_css(gateway_entry);
    
    // DNS
    GtkWidget *dns_label = gtk_label_new("DNS Server:");
    gtk_grid_attach(GTK_GRID(static_grid), dns_label, 0, 3, 1, 1);
    apply_widget_css(dns_label);
    
    GtkWidget *dns_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(dns_entry), current_settings.dns);
    gtk_entry_set_placeholder_text(GTK_ENTRY(dns_entry), "8.8.8.8");
    gtk_grid_attach(GTK_GRID(static_grid), dns_entry, 1, 3, 1, 1);
    apply_widget_css(dns_entry);
    
    // Connect callback for method toggle (using proper function)
    g_signal_connect(dhcp_radio, "toggled", G_CALLBACK(on_method_toggled), static_frame);
    
    // Set initial sensitivity
    gtk_widget_set_sensitive(static_frame, strcmp(current_settings.method, "static") == 0);
    
    gtk_widget_show_all(dialog);
    
    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_APPLY) {
        const char *iface = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(iface_combo));
        gboolean use_dhcp = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dhcp_radio));
        
        // Save current settings
        strncpy(current_settings.interface, iface ? iface : "", sizeof(current_settings.interface) - 1);
        strncpy(current_settings.method, use_dhcp ? "dhcp" : "static", sizeof(current_settings.method) - 1);
        
        if (!use_dhcp) {
            // Save static IP settings
            const char *ip = gtk_entry_get_text(GTK_ENTRY(ip_entry));
            const char *netmask = gtk_entry_get_text(GTK_ENTRY(netmask_entry));
            const char *gateway = gtk_entry_get_text(GTK_ENTRY(gateway_entry));
            const char *dns = gtk_entry_get_text(GTK_ENTRY(dns_entry));
            
            strncpy(current_settings.ip_address, ip, sizeof(current_settings.ip_address) - 1);
            strncpy(current_settings.netmask, netmask, sizeof(current_settings.netmask) - 1);
            strncpy(current_settings.gateway, gateway, sizeof(current_settings.gateway) - 1);
            strncpy(current_settings.dns, dns, sizeof(current_settings.dns) - 1);
        }
        
        // Save to file
        save_network_settings();
        
        if (use_dhcp) {
            // Apply DHCP
            gchar *cmd = g_strdup_printf("nmcli connection modify '%s' ipv4.method auto && nmcli connection up '%s' 2>/dev/null", 
                                         iface, iface);
            int result = system(cmd);
            g_free(cmd);
            
            if (result == 0) {
                GtkWidget *success = gtk_message_dialog_new(
                    GTK_WINDOW(parent_window),
                    GTK_DIALOG_MODAL,
                    GTK_MESSAGE_INFO,
                    GTK_BUTTONS_OK,
                    "DHCP Enabled"
                );
                gtk_message_dialog_format_secondary_text(
                    GTK_MESSAGE_DIALOG(success),
                    "Interface %s set to automatic (DHCP) mode", iface
                );
                apply_widget_css(success);
                gtk_dialog_run(GTK_DIALOG(success));
                gtk_widget_destroy(success);
            }
        } else {
            // Apply static IP
            const char *ip = gtk_entry_get_text(GTK_ENTRY(ip_entry));
            const char *netmask = gtk_entry_get_text(GTK_ENTRY(netmask_entry));
            const char *gateway = gtk_entry_get_text(GTK_ENTRY(gateway_entry));
            const char *dns = gtk_entry_get_text(GTK_ENTRY(dns_entry));
            
            if (strlen(ip) > 0 && strlen(netmask) > 0) {
                gchar *cmd = g_strdup_printf(
                    "nmcli connection modify '%s' ipv4.method manual ipv4.addresses %s/%s "
                    "ipv4.gateway %s ipv4.dns '%s' && nmcli connection up '%s' 2>/dev/null",
                    iface, ip, netmask, gateway ? gateway : "", dns ? dns : "8.8.8.8", iface);
                int result = system(cmd);
                g_free(cmd);
                
                if (result == 0) {
                    GtkWidget *success = gtk_message_dialog_new(
                        GTK_WINDOW(parent_window),
                        GTK_DIALOG_MODAL,
                        GTK_MESSAGE_INFO,
                        GTK_BUTTONS_OK,
                        "Static IP Applied"
                    );
                    gtk_message_dialog_format_secondary_text(
                        GTK_MESSAGE_DIALOG(success),
                        "Static IP configuration applied to %s", iface
                    );
                    apply_widget_css(success);
                    gtk_dialog_run(GTK_DIALOG(success));
                    gtk_widget_destroy(success);
                }
            }
        }
    }
    
    gtk_widget_destroy(dialog);
}

// Clean up provider
void internet_settings_cleanup(void)
{
    if (internet_settings_provider) {
        g_object_unref(internet_settings_provider);
        internet_settings_provider = NULL;
    }
}