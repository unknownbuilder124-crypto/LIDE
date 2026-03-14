#include "extensions.h"

// Define all available themes
static const BrowserTheme themes[] = {
    // 1. VoidFox Dark (Default) - Dark terminal-like theme
    {
        .name = "VoidFox Dark",
        .description = "Dark terminal-like theme with bright green accents",
        .bg_color = "#0b0f14",
        .text_color = "#ffffff",
        .entry_bg = "#1e2429",
        .entry_text = "#ffffff",
        .entry_border = "#00ff88",
        .button_bg = "#1e2429",
        .button_text = "#00ff88",
        .button_hover = "#2a323a",
        .title_bar_bg = "#0b0f14",
        .title_bar_border = "#00ff88",
        .bookmarks_bar_bg = "#1e2429"
    },
    // 2. Light Mode - Clean light theme
    {
        .name = "Light Mode",
        .description = "Clean light theme with neutral colors",
        .bg_color = "#f0f0f0",
        .text_color = "#000000",
        .entry_bg = "#ffffff",
        .entry_text = "#000000",
        .entry_border = "#888888",
        .button_bg = "#e0e0e0",
        .button_text = "#000000",
        .button_hover = "#d0d0d0",
        .title_bar_bg = "#e0e0e0",
        .title_bar_border = "#888888",
        .bookmarks_bar_bg = "#d0d0d0"
    },
    // 3. Cyberpunk - Vibrant cyan and magenta
    {
        .name = "Cyberpunk",
        .description = "Vibrant cyberpunk theme with cyan and magenta",
        .bg_color = "#0a0e27",
        .text_color = "#00ffff",
        .entry_bg = "#1a1e3f",
        .entry_text = "#00ffff",
        .entry_border = "#ff00ff",
        .button_bg = "#1a1e3f",
        .button_text = "#ff00ff",
        .button_hover = "#2a2e4f",
        .title_bar_bg = "#0a0e27",
        .title_bar_border = "#ff00ff",
        .bookmarks_bar_bg = "#1a1e3f"
    },
    // 4. Midnight Blue - Deep blue theme
    {
        .name = "Midnight Blue",
        .description = "Deep blue theme with light blue accents",
        .bg_color = "#0f1419",
        .text_color = "#e8f4f8",
        .entry_bg = "#1a2332",
        .entry_text = "#e8f4f8",
        .entry_border = "#4a9eff",
        .button_bg = "#1a2332",
        .button_text = "#4a9eff",
        .button_hover = "#243447",
        .title_bar_bg = "#0f1419",
        .title_bar_border = "#4a9eff",
        .bookmarks_bar_bg = "#1a2332"
    },
    // 5. Forest Green - Nature-inspired green theme
    {
        .name = "Forest Green",
        .description = "Nature-inspired green theme",
        .bg_color = "#1a2927",
        .text_color = "#c4e8d8",
        .entry_bg = "#2d4a3f",
        .entry_text = "#c4e8d8",
        .entry_border = "#52b788",
        .button_bg = "#2d4a3f",
        .button_text = "#52b788",
        .button_hover = "#3d5a4f",
        .title_bar_bg = "#1a2927",
        .title_bar_border = "#52b788",
        .bookmarks_bar_bg = "#2d4a3f"
    },
    // 6. Sunset Orange - Warm orange and red tones
    {
        .name = "Sunset Orange",
        .description = "Warm sunset theme with orange and amber",
        .bg_color = "#2d1f1f",
        .text_color = "#ffd6a5",
        .entry_bg = "#4a2c29",
        .entry_text = "#ffd6a5",
        .entry_border = "#ff8c42",
        .button_bg = "#4a2c29",
        .button_text = "#ff8c42",
        .button_hover = "#5a3c39",
        .title_bar_bg = "#2d1f1f",
        .title_bar_border = "#ff8c42",
        .bookmarks_bar_bg = "#4a2c29"
    },
    // 7. Purple Noir - Deep purple with lavender accents
    {
        .name = "Purple Noir",
        .description = "Deep purple theme with lavender accents",
        .bg_color = "#1a0f2e",
        .text_color = "#e8d5ff",
        .entry_bg = "#2d1b4e",
        .entry_text = "#e8d5ff",
        .entry_border = "#b19cd9",
        .button_bg = "#2d1b4e",
        .button_text = "#b19cd9",
        .button_hover = "#3d2b5e",
        .title_bar_bg = "#1a0f2e",
        .title_bar_border = "#b19cd9",
        .bookmarks_bar_bg = "#2d1b4e"
    },
    // 8. Nord - Popular Nord color scheme
    {
        .name = "Nord",
        .description = "Arctic, north-bluish color scheme",
        .bg_color = "#2e3440",
        .text_color = "#eceff4",
        .entry_bg = "#3b4252",
        .entry_text = "#eceff4",
        .entry_border = "#81a1c1",
        .button_bg = "#3b4252",
        .button_text = "#81a1c1",
        .button_hover = "#434c5e",
        .title_bar_bg = "#2e3440",
        .title_bar_border = "#81a1c1",
        .bookmarks_bar_bg = "#3b4252"
    },
    // 9. Dracula - Popular Dracula theme
    {
        .name = "Dracula",
        .description = "Dark theme inspired by Dracula color scheme",
        .bg_color = "#282a36",
        .text_color = "#f8f8f2",
        .entry_bg = "#44475a",
        .entry_text = "#f8f8f2",
        .entry_border = "#bd93f9",
        .button_bg = "#44475a",
        .button_text = "#bd93f9",
        .button_hover = "#565a77",
        .title_bar_bg = "#282a36",
        .title_bar_border = "#bd93f9",
        .bookmarks_bar_bg = "#44475a"
    },
    // 10. Gruvbox Dark - Retro groove dark theme
    {
        .name = "Gruvbox Dark",
        .description = "Retro groove dark theme with warm colors",
        .bg_color = "#282828",
        .text_color = "#ebdbb2",
        .entry_bg = "#3c3836",
        .entry_text = "#ebdbb2",
        .entry_border = "#b8bb26",
        .button_bg = "#3c3836",
        .button_text = "#b8bb26",
        .button_hover = "#504945",
        .title_bar_bg = "#282828",
        .title_bar_border = "#b8bb26",
        .bookmarks_bar_bg = "#3c3836"
    },
    // 11. Solarized Dark - Popular Solarized dark theme
    {
        .name = "Solarized Dark",
        .description = "Precision colors for machines and people",
        .bg_color = "#002b36",
        .text_color = "#839496",
        .entry_bg = "#073642",
        .entry_text = "#839496",
        .entry_border = "#268bd2",
        .button_bg = "#073642",
        .button_text = "#268bd2",
        .button_hover = "#1a3f52",
        .title_bar_bg = "#002b36",
        .title_bar_border = "#268bd2",
        .bookmarks_bar_bg = "#073642"
    },
    // 12. Ocean Blue - Fresh ocean blue theme
    {
        .name = "Ocean Blue",
        .description = "Fresh ocean blue theme with aqua accents",
        .bg_color = "#0d1b2a",
        .text_color = "#c8d5e0",
        .entry_bg = "#1b3a52",
        .entry_text = "#c8d5e0",
        .entry_border = "#1dd3b0",
        .button_bg = "#1b3a52",
        .button_text = "#1dd3b0",
        .button_hover = "#2a4a62",
        .title_bar_bg = "#0d1b2a",
        .title_bar_border = "#1dd3b0",
        .bookmarks_bar_bg = "#1b3a52"
    }
};

static const int num_themes = sizeof(themes) / sizeof(themes[0]);

// Function to apply CSS for a theme
void apply_theme(BrowserWindow *browser, const BrowserTheme *theme) {
    if (!browser || !theme) return;

    GtkCssProvider *provider = gtk_css_provider_new();

    gchar *css = g_strdup_printf(
        "window { background-color: %s; color: %s; }\n"
        "entry { background-color: %s; color: %s; border: 1px solid %s; }\n"
        "button { background-color: %s; color: %s; border: none; }\n"
        "button:hover { background-color: %s; }\n"
        "notebook { background-color: %s; }\n"
        "#title-bar { background-color: %s; border-bottom: 2px solid %s; }\n"
        "#bookmarks-bar { background-color: %s; padding: 2px; }\n",
        theme->bg_color,
        theme->text_color,
        theme->entry_bg,
        theme->entry_text,
        theme->entry_border,
        theme->button_bg,
        theme->button_text,
        theme->button_hover,
        theme->bg_color,
        theme->title_bar_bg,
        theme->title_bar_border,
        theme->bookmarks_bar_bg
    );

    gtk_css_provider_load_from_data(provider, css, -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
    g_free(css);
}

// Callback when a theme button is clicked
static void on_theme_selected(GtkButton *button, gpointer user_data) {
    BrowserWindow *browser = (BrowserWindow *)user_data;
    const BrowserTheme *theme = (const BrowserTheme *)g_object_get_data(G_OBJECT(button), "theme-data");

    if (theme) {
        apply_theme(browser, theme);
        g_print("Theme applied: %s\n", theme->name);
    }
}

static void on_close_tab_clicked(GtkButton *button, BrowserWindow *browser)
{
    GtkWidget *tab_child = g_object_get_data(G_OBJECT(button), "tab-child");
    if (tab_child) {
        int page_num = gtk_notebook_page_num(GTK_NOTEBOOK(browser->notebook), tab_child);
        if (page_num != -1) {
            gtk_notebook_remove_page(GTK_NOTEBOOK(browser->notebook), page_num);
        }
    }
}

// Show themes tab with all available themes
void show_themes_tab(BrowserWindow *browser) {
    GtkWidget *tab_content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(tab_content), 15);

    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='20000' weight='bold'>Browser Themes</span>");
    gtk_box_pack_start(GTK_BOX(tab_content), title, FALSE, FALSE, 0);

    GtkWidget *subtitle = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(subtitle), "<span size='11000'>Select a theme to customize your browser appearance</span>");
    gtk_box_pack_start(GTK_BOX(tab_content), subtitle, FALSE, FALSE, 0);

    // Create scrolled window for themes
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    // Container for theme grid
    GtkWidget *themes_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(themes_container), 10);

    // Add theme buttons in a grid
    for (int i = 0; i < num_themes; i++) {
        GtkWidget *theme_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

        // Color preview
        GtkWidget *color_preview = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_widget_set_size_request(color_preview, 40, 40);
        GtkCssProvider *preview_provider = gtk_css_provider_new();
        gchar *preview_css = g_strdup_printf(
            "box { background-color: %s; border: 2px solid %s; }",
            themes[i].bg_color,
            themes[i].entry_border
        );
        gtk_css_provider_load_from_data(preview_provider, preview_css, -1, NULL);
        gtk_style_context_add_provider(gtk_widget_get_style_context(color_preview),
                                       GTK_STYLE_PROVIDER(preview_provider),
                                       GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        g_free(preview_css);
        g_object_unref(preview_provider);
        gtk_box_pack_start(GTK_BOX(theme_box), color_preview, FALSE, FALSE, 0);

        // Theme info
        GtkWidget *info_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
        GtkWidget *theme_name = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(theme_name),
                            g_strdup_printf("<span weight='bold'>%s</span>", themes[i].name));
        gtk_label_set_xalign(GTK_LABEL(theme_name), 0.0);
        gtk_box_pack_start(GTK_BOX(info_box), theme_name, FALSE, FALSE, 0);

        GtkWidget *theme_desc = gtk_label_new(themes[i].description);
        gtk_label_set_xalign(GTK_LABEL(theme_desc), 0.0);
        gtk_label_set_line_wrap(GTK_LABEL(theme_desc), TRUE);
        gtk_box_pack_start(GTK_BOX(info_box), theme_desc, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(theme_box), info_box, TRUE, TRUE, 0);

        // Apply button
        GtkWidget *apply_btn = gtk_button_new_with_label("Apply");
        gtk_widget_set_size_request(apply_btn, 80, -1);
        g_object_set_data(G_OBJECT(apply_btn), "theme-data", (gpointer)&themes[i]);
        g_signal_connect(apply_btn, "clicked", G_CALLBACK(on_theme_selected), browser);
        gtk_box_pack_end(GTK_BOX(theme_box), apply_btn, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(themes_container), theme_box, FALSE, FALSE, 0);

        // Add separator between themes
        if (i < num_themes - 1) {
            GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
            gtk_box_pack_start(GTK_BOX(themes_container), sep, FALSE, FALSE, 0);
        }
    }

    GtkWidget *viewport = gtk_viewport_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(viewport), themes_container);
    gtk_container_add(GTK_CONTAINER(scrolled), viewport);
    gtk_box_pack_start(GTK_BOX(tab_content), scrolled, TRUE, TRUE, 0);

    gtk_widget_show_all(tab_content);

    // Create tab label with close button
    GtkWidget *tab_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *tab_label = gtk_label_new("Themes");
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

void show_extensions_tab(BrowserWindow *browser)
{
    GtkWidget *tab_content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_container_set_border_width(GTK_CONTAINER(tab_content), 20);

    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='20000' weight='bold'>Extensions and Themes</span>");
    gtk_box_pack_start(GTK_BOX(tab_content), title, FALSE, FALSE, 0);

    // No extensions installed message
    GtkWidget *message = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(message),
        "<span size='12000'>No extensions installed</span>\n\n"
        "<span>Extension support coming soon!</span>");
    gtk_box_pack_start(GTK_BOX(tab_content), message, TRUE, TRUE, 0);

    // Get more extensions button
    GtkWidget *get_more_btn = gtk_button_new_with_label("Get more extensions");
    g_signal_connect_swapped(get_more_btn, "clicked", G_CALLBACK(load_url), browser);
    gtk_box_pack_start(GTK_BOX(tab_content), get_more_btn, FALSE, FALSE, 0);

    gtk_widget_show_all(tab_content);

    // Create tab label with close button
    GtkWidget *tab_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *tab_label = gtk_label_new("Extensions");
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
