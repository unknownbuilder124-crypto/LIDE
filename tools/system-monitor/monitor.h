#ifndef LIDE_MONITOR_H
#define LIDE_MONITOR_H

#include <gtk/gtk.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <fcntl.h>

#define _GNU_SOURCE  // For DT_DIR
#define HISTORY_SIZE 60  // Move define to header

// CPU data
typedef struct {
    guint64 user, nice, system, idle, iowait, irq, softirq, steal;
    guint64 total_prev, idle_prev;
    double usage;
} CpuData;

// Memory data
typedef struct {
    guint64 total;
    guint64 free;
    guint64 available;
    guint64 cached;
    double percent;
} MemData;

// Process entry
typedef struct {
    pid_t pid;
    char name[256];
    double cpu_percent;
    double mem_percent;
    guint64 vm_rss; // in pages
} ProcessEntry;

// Global data for graphs (declare as extern so other files can access)
extern double cpu_history[HISTORY_SIZE];
extern int cpu_history_index;
extern double mem_history[HISTORY_SIZE];
extern int mem_history_index;

// Function prototypes
void update_cpu_usage(CpuData *cpu);
void update_mem_usage(MemData *mem);
GList* get_process_list(void);
void free_process_list(GList *list);

// Drawing helpers
gboolean draw_cpu_graph(GtkWidget *widget, cairo_t *cr, gpointer data);
gboolean draw_mem_bar(GtkWidget *widget, cairo_t *cr, gpointer data);

#endif