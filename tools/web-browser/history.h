#ifndef VOIDFOX_HISTORY_H
#define VOIDFOX_HISTORY_H

#include <gtk/gtk.h>
#include "voidfox.h"

// History entry structure
typedef struct {
    char *title;
    char *url;
    time_t timestamp;
} HistoryEntry;

// Function prototypes
void add_to_history(const char *url, const char *title);
void show_history_tab(BrowserWindow *browser);
void save_history(void);
void load_history(void);

#endif