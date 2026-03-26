#include "passwords.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PASSWORDS_FILE "passwords.txt"

static GList *passwords = NULL;

/* Forward declarations */
static void copy_to_clipboard(GtkButton *button);
static void show_add_password_dialog(GtkButton *button, BrowserWindow *browser);
//gboolean on_close_tab_clicked(GtkButton *button, BrowserWindow *browser);

/**
 * Toggles password visibility in an entry widget.
 *
 * @param toggle The check button that was toggled.
 * @param entry  The GtkEntry widget containing the password.
 */
static void on_show_password_toggle(GtkToggleButton *toggle, GtkEntry *entry)

{
    gtk_entry_set_visibility(entry, gtk_toggle_button_get_active(toggle));
}

/**
 * Copies password text to system clipboard.
 * Changes button label to "✓" briefly as visual feedback.
 *
 * @param button The button that was clicked.
 */
static void copy_to_clipboard(GtkButton *button)

{
    const char *text = g_object_get_data(G_OBJECT(button), "password");
    if (text) {
        GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
        gtk_clipboard_set_text(clipboard, text, -1);
        
        /* Show brief feedback */
        gtk_button_set_label(button, "✓");
        g_timeout_add_seconds(1, (GSourceFunc)gtk_button_set_label, button);
    }
}

/**
 * Displays a dialog to add a new password entry.
 *
 * @param button  The button that was clicked (unused).
 * @param browser BrowserWindow instance for parent window.
 *
 * @sideeffect Adds password to list and saves to file on confirmation.
 */
static void show_add_password_dialog(GtkButton *button, BrowserWindow *browser)

{
    (void)button;
    
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Add Password",
                                                    GTK_WINDOW(browser->window),
                                                    GTK_DIALOG_MODAL,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_Add", GTK_RESPONSE_ACCEPT,
                                                    NULL);
    
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 200);
    
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 10);
    gtk_box_pack_start(GTK_BOX(content), grid, TRUE, TRUE, 0);
    
    GtkWidget *site_label = gtk_label_new("Site:");
    gtk_label_set_xalign(GTK_LABEL(site_label), 1.0);
    gtk_grid_attach(GTK_GRID(grid), site_label, 0, 0, 1, 1);
    
    GtkWidget *site_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(site_entry), "example.com");
    gtk_grid_attach(GTK_GRID(grid), site_entry, 1, 0, 1, 1);
    
    GtkWidget *username_label = gtk_label_new("Username:");
    gtk_label_set_xalign(GTK_LABEL(username_label), 1.0);
    gtk_grid_attach(GTK_GRID(grid), username_label, 0, 1, 1, 1);
    
    GtkWidget *username_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(username_entry), "user@example.com");
    gtk_grid_attach(GTK_GRID(grid), username_entry, 1, 1, 1, 1);
    
    GtkWidget *password_label = gtk_label_new("Password:");
    gtk_label_set_xalign(GTK_LABEL(password_label), 1.0);
    gtk_grid_attach(GTK_GRID(grid), password_label, 0, 2, 1, 1);
    
    GtkWidget *password_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(password_entry), "Enter password");
    gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);
    gtk_grid_attach(GTK_GRID(grid), password_entry, 1, 2, 1, 1);
    
    gtk_widget_show_all(dialog);
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        const char *site = gtk_entry_get_text(GTK_ENTRY(site_entry));
        const char *username = gtk_entry_get_text(GTK_ENTRY(username_entry));
        const char *password = gtk_entry_get_text(GTK_ENTRY(password_entry));
        
        if (site && *site && username && *username && password && *password) {
            PasswordEntry *entry = g_new(PasswordEntry, 1);
            entry->site = g_strdup(site);
            entry->username = g_strdup(username);
            entry->password = g_strdup(password);
            
            passwords = g_list_append(passwords, entry);
            save_passwords();
            
            /* Show success message */
            GtkWidget *msg = gtk_message_dialog_new(GTK_WINDOW(browser->window),
                                                   GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_INFO,
                                                   GTK_BUTTONS_OK,
                                                   "Password saved successfully!\nClose and reopen the Passwords tab to see it.");
            gtk_dialog_run(GTK_DIALOG(msg));
            gtk_widget_destroy(msg);
        }
    }
    
    gtk_widget_destroy(dialog);
}

/**
 * Creates and displays the saved passwords tab.
 * Shows all stored credentials with options to show/hide and copy passwords.
 *
 * @param browser BrowserWindow instance.
 *
 * @sideeffect Adds a new tab to the notebook with password list.
 */
void show_passwords_tab(BrowserWindow *browser)

{
    GtkWidget *tab_content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(tab_content), 20);
    
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='20000' weight='bold'>Saved Passwords</span>");
    gtk_box_pack_start(GTK_BOX(tab_content), title, FALSE, FALSE, 0);
    
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(tab_content), scrolled, TRUE, TRUE, 0);
    
    GtkWidget *listbox = gtk_list_box_new();
    gtk_container_add(GTK_CONTAINER(scrolled), listbox);
    
    for (GList *l = passwords; l; l = l->next) {
        PasswordEntry *entry = l->data;
        
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *grid = gtk_grid_new();
        gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
        gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
        gtk_container_add(GTK_CONTAINER(row), grid);
        
        GtkWidget *site_label = gtk_label_new(entry->site);
        gtk_label_set_xalign(GTK_LABEL(site_label), 0.0);
        gtk_widget_set_size_request(site_label, 150, -1);
        gtk_grid_attach(GTK_GRID(grid), site_label, 0, 0, 1, 1);
        
        GtkWidget *username_label = gtk_label_new(entry->username);
        gtk_label_set_xalign(GTK_LABEL(username_label), 0.0);
        gtk_widget_set_size_request(username_label, 150, -1);
        gtk_grid_attach(GTK_GRID(grid), username_label, 1, 0, 1, 1);
        
        GtkWidget *password_entry = gtk_entry_new();
        gtk_entry_set_text(GTK_ENTRY(password_entry), entry->password);
        gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);
        gtk_editable_set_editable(GTK_EDITABLE(password_entry), FALSE);
        gtk_widget_set_size_request(password_entry, 150, -1);
        gtk_grid_attach(GTK_GRID(grid), password_entry, 2, 0, 1, 1);
        
        GtkWidget *show_btn = gtk_check_button_new_with_label("Show");
        g_signal_connect(show_btn, "toggled", G_CALLBACK(on_show_password_toggle), password_entry);
        gtk_grid_attach(GTK_GRID(grid), show_btn, 3, 0, 1, 1);
        
        GtkWidget *copy_btn = gtk_button_new_with_label("Copy");
        g_object_set_data_full(G_OBJECT(copy_btn), "password", g_strdup(entry->password), g_free);
        g_signal_connect(copy_btn, "clicked", G_CALLBACK(copy_to_clipboard), NULL);
        gtk_grid_attach(GTK_GRID(grid), copy_btn, 4, 0, 1, 1);
        
        gtk_list_box_insert(GTK_LIST_BOX(listbox), row, -1);
    }
    
    /* Add password button */
    GtkWidget *add_btn = gtk_button_new_with_label("Add Password");
    g_signal_connect(add_btn, "clicked", G_CALLBACK(show_add_password_dialog), browser);
    gtk_box_pack_start(GTK_BOX(tab_content), add_btn, FALSE, FALSE, 0);
    
    gtk_widget_show_all(tab_content);
    
    /* Create tab label with close button */
    GtkWidget *tab_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *tab_label = gtk_label_new("Passwords");
    GtkWidget *close_btn = gtk_button_new_from_icon_name("window-close", GTK_ICON_SIZE_MENU);
    gtk_button_set_relief(GTK_BUTTON(close_btn), GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(close_btn, "Close tab");
    
    gtk_box_pack_start(GTK_BOX(tab_box), tab_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(tab_box), close_btn, FALSE, FALSE, 0);
    gtk_widget_show_all(tab_box);
    
    g_object_set_data_full(G_OBJECT(close_btn), "tab-child", tab_content, NULL);
    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_close_tab_clicked), browser);
    
    int page_num = gtk_notebook_append_page(GTK_NOTEBOOK(browser->notebook), tab_content, tab_box);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(browser->notebook), page_num);
}

/**
 * Adds a password entry to the list.
 *
 * @param site     Website domain.
 * @param username Username or email for the site.
 * @param password Password for the site.
 *
 * @sideeffect Appends to global passwords list and saves to file.
 */
void add_password(const char *site, const char *username, const char *password)

{
    PasswordEntry *entry = g_new(PasswordEntry, 1);
    entry->site = g_strdup(site);
    entry->username = g_strdup(username);
    entry->password = g_strdup(password);
    
    passwords = g_list_append(passwords, entry);
    save_passwords();
}

/**
 * Saves all passwords to disk.
 * Format: site|username|password per line.
 *
 * @sideeffect Writes to PASSWORDS_FILE.
 */
void save_passwords(void)

{
    FILE *f = fopen(PASSWORDS_FILE, "w");
    if (!f) return;
    
    for (GList *l = passwords; l; l = l->next) {
        PasswordEntry *entry = l->data;
        fprintf(f, "%s|%s|%s\n", entry->site, entry->username, entry->password);
    }
    
    fclose(f);
}

/**
 * Loads passwords from disk.
 * Reads PASSWORDS_FILE and populates the passwords list.
 *
 * @sideeffect Appends loaded passwords to global passwords list.
 * @note Call during application initialization to restore saved credentials.
 */
void load_passwords(void)

{
    FILE *f = fopen(PASSWORDS_FILE, "r");
    if (!f) return;
    
    char line[2048];
    while (fgets(line, sizeof(line), f)) {
        char *newline = strchr(line, '\n');
        if (newline) *newline = '\0';
        
        char *first_sep = strchr(line, '|');
        if (!first_sep) continue;
        *first_sep = '\0';
        
        char *second_sep = strchr(first_sep + 1, '|');
        if (!second_sep) continue;
        *second_sep = '\0';
        
        char *site = line;
        char *username = first_sep + 1;
        char *password = second_sep + 1;
        
        PasswordEntry *entry = g_new(PasswordEntry, 1);
        entry->site = g_strdup(site);
        entry->username = g_strdup(username);
        entry->password = g_strdup(password);
        
        passwords = g_list_append(passwords, entry);
    }
    
    fclose(f);
}