#include "voidfox.h"

/* Tab utility functions */

/**
 * Gets the currently active browser tab.
 *
 * @param browser BrowserWindow instance.
 * @return BrowserTab pointer for the active tab, or NULL if none.
 */
BrowserTab* get_current_tab(BrowserWindow *browser)
{
    if (!browser || !browser->notebook) return NULL;
    
    int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(browser->notebook));
    if (current_page < 0) return NULL;
    
    GtkWidget *current_page_widget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(browser->notebook), current_page);
    if (!current_page_widget) return NULL;
    
    return (BrowserTab *)g_object_get_data(G_OBJECT(current_page_widget), "browser-tab");
}

/**
 * Duplicates the current tab.
 *
 * @param browser BrowserWindow instance.
 *
 * @sideeffect Creates a new tab with the same URL and history as current tab.
 */
void duplicate_tab(BrowserWindow *browser)
{
    BrowserTab *current = get_current_tab(browser);
    if (!current || !current->web_view) return;
    
    const char *url = webkit_web_view_get_uri(current->web_view);
    if (url && *url) {
        new_tab(browser, url);
    }
}

/**
 * Pins a tab to the left side of the tab bar.
 * Pinned tabs are smaller and persist across sessions.
 *
 * @param browser BrowserWindow instance.
 * @param tab     The tab to pin.
 */
void pin_tab(BrowserWindow *browser, BrowserTab *tab)
{
    (void)browser;
    if (!tab || tab->is_pinned) return;
    
    tab->is_pinned = TRUE;
    /* TODO: Implement tab pinning UI changes */
}

/**
 * Unpins a previously pinned tab.
 *
 * @param browser BrowserWindow instance.
 * @param tab     The tab to unpin.
 */
void unpin_tab(BrowserWindow *browser, BrowserTab *tab)
{
    (void)browser;
    if (!tab || !tab->is_pinned) return;
    
    tab->is_pinned = FALSE;
    /* TODO: Implement tab unpinning UI changes */
}

/**
 * Mutes or unmutes audio in the current tab.
 *
 * @param browser BrowserWindow instance.
 * @param mute    TRUE to mute, FALSE to unmute.
 */
void mute_tab(BrowserWindow *browser, gboolean mute)
{
    BrowserTab *current = get_current_tab(browser);
    if (!current || !current->web_view) return;
    
    current->is_muted = mute;
    webkit_web_view_set_is_muted(current->web_view, mute);
    
    /* Update tab label to show mute icon */
    const char *title = webkit_web_view_get_title(current->web_view);
    char *label_text;
    if (mute) {
        label_text = g_strdup_printf("🔇 %s", title ? title : "New Tab");
    } else {
        label_text = g_strdup_printf("%s", title ? title : "New Tab");
    }
    gtk_label_set_text(GTK_LABEL(current->tab_label), label_text);
    g_free(label_text);
}

/**
 * Reloads all tabs.
 *
 * @param browser BrowserWindow instance.
 *
 * @sideeffect Refreshes every open tab.
 */
void reload_all_tabs(BrowserWindow *browser)
{
    if (!browser || !browser->notebook) return;
    
    int n_pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(browser->notebook));
    for (int i = 0; i < n_pages; i++) {
        GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(browser->notebook), i);
        if (!page) continue;
        
        BrowserTab *tab = (BrowserTab *)g_object_get_data(G_OBJECT(page), "browser-tab");
        if (tab && tab->web_view) {
            webkit_web_view_reload(tab->web_view);
        }
    }
}

/**
 * Closes all tabs except the current one.
 *
 * @param browser BrowserWindow instance.
 */
void close_other_tabs(BrowserWindow *browser)
{
    if (!browser || !browser->notebook) return;
    
    int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(browser->notebook));
    if (current_page < 0) return;
    
    GtkWidget *current_tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(browser->notebook), current_page);
    if (!current_tab) return;
    
    int n_pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(browser->notebook));
    for (int i = n_pages - 1; i >= 0; i--) {
        if (i == current_page) continue;
        
        GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(browser->notebook), i);
        if (page) {
            gtk_notebook_remove_page(GTK_NOTEBOOK(browser->notebook), i);
        }
    }
}

/**
 * Closes tabs to the right of the current tab.
 *
 * @param browser BrowserWindow instance.
 */
void close_tabs_to_right(BrowserWindow *browser)
{
    if (!browser || !browser->notebook) return;
    
    int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(browser->notebook));
    if (current_page < 0) return;
    
    int n_pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(browser->notebook));
    for (int i = n_pages - 1; i > current_page; i--) {
        GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(browser->notebook), i);
        if (page) {
            gtk_notebook_remove_page(GTK_NOTEBOOK(browser->notebook), i);
        }
    }
}

/**
 * Restores the last closed tab.
 *
 * @param browser BrowserWindow instance.
 *
 * @sideeffect Reopens the most recently closed tab with its history.
 */
void restore_last_closed_tab(BrowserWindow *browser)
{
    /* TODO: Implement tab history for restoring closed tabs */
    (void)browser;
}