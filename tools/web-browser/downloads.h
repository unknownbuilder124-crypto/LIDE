#ifndef VOIDFOX_DOWNLOADS_H
#define VOIDFOX_DOWNLOADS_H

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include "voidfox.h"

/**

/*
 * downloads.h
 * 
 * Download manager interface definitions
 *
 * This module is part of the LIDE desktop environment system.
 * See the main window manager (wm/) and session management (session/)
 * for system architecture overview.

 * Download item structure.
 * Represents a single file download with its state and metadata.
 */
typedef struct {
    char *filename;          /* Local filename (extracted from URL or content-disposition) */
    char *url;               /* Source URL of the download */
    char *destination;       /* Full local file path where download is saved */
    double progress;         /* Download progress percentage (0-100) */
    guint64 received;        /* Bytes received so far */
    guint64 total;           /* Total bytes expected (0 if unknown) */
    int status;              /* 0: pending, 1: downloading, 2: complete, 3: failed */
    char *error_message;     /* Error message if status is failed */
    WebKitDownload *download;/* Reference to WebKitDownload object */
} DownloadItem;

/* Global download list (shared with download-stats.c) */
extern GList *downloads;           /* Linked list of DownloadItem pointers */
extern BrowserWindow *global_browser; /* Currently active browser window for UI updates */

/* Function prototypes */

/**
 * Creates and displays the downloads management tab.
 * Shows all current downloads with status, progress, and action buttons.
 *
 * @param browser BrowserWindow instance.
 *
 * @sideeffect Adds a new tab to the notebook with download list.
 */
void show_downloads_tab(BrowserWindow *browser);

/**
 * Adds a new download to the download manager.
 * Creates a DownloadItem, sets up destination path, and connects WebKit signals.
 *
 * @param download The WebKitDownload object.
 * @param browser  BrowserWindow instance for UI updates.
 *
 * @sideeffect Creates the downloads directory if needed.
 * @sideeffect Appends to global downloads list and saves to file.
 */
void add_download(WebKitDownload *download, BrowserWindow *browser);

/**
 * Updates or creates the downloads tab to reflect current state.
 * Refreshes the existing downloads tab if open, otherwise creates a new one.
 *
 * @param browser BrowserWindow instance.
 *
 * @sideeffect Refreshes downloads tab UI with latest data.
 */
void update_downloads_tab(BrowserWindow *browser);

/**
 * Saves all downloads to disk.
 * Persists download metadata to DOWNLOADS_FILE for restoration across sessions.
 *
 * @sideeffect Writes to downloads.txt in the current directory.
 */
void save_downloads(void);

/**
 * Loads downloads from disk.
 * Reads DOWNLOADS_FILE and populates the global downloads list.
 *
 * @sideeffect Appends loaded downloads to global downloads list.
 * @note Call during application initialization to restore previous downloads.
 */
void load_downloads(void);

/**
 * Gets the count of active downloads.
 * Counts all downloads with status == 1 (downloading).
 *
 * @return Number of downloads currently in progress.
 */
int get_active_download_count(void);

#endif /* VOIDFOX_DOWNLOADS_H */