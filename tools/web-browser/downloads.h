#ifndef VOIDFOX_DOWNLOADS_H
#define VOIDFOX_DOWNLOADS_H

#include <gtk/gtk.h>
#include "voidfox.h"

// Download item structure
typedef struct {
    char *filename;
    char *url;
    char *destination;
    double progress;
    int status; // 0: pending, 1: downloading, 2: complete, 3: failed
} DownloadItem;

// Function prototypes
void show_downloads_tab(BrowserWindow *browser);
void add_download(const char *url, const char *filename);
void save_downloads(void);
void load_downloads(void);

#endif