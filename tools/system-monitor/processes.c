#include "monitor.h"
#include <sys/stat.h>  

// Helper: read a line from /proc/[pid]/stat
static int read_proc_stat(pid_t pid, char *buf, size_t size) 

{
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    FILE *fp = fopen(path, "r");
    if (!fp) return -1;
    if (!fgets(buf, size, fp)) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return 0;
}

// Extract fields from stat (comm, utime, stime, rss)
static int parse_stat(const char *buf, char *comm, size_t comm_size,
                      unsigned long long *utime, unsigned long long *stime,
                      long *rss) {
    // Format: pid (comm) state ppid pgrp session tty_nr tpgid flags minflt cminflt majflt cmajflt utime stime cutime cstime priority nice num_threads itrealvalue starttime vsize rss rsslim ...
    const char *p = buf;
    // skip pid
    while (*p && *p != ' ') p++;
    while (*p && *p == ' ') p++;
    if (*p != '(') return -1;
    p++; // skip '('
    const char *comm_start = p;
    while (*p && *p != ')') p++;
    if (*p != ')') return -1;
    size_t len = p - comm_start;
    if (len >= comm_size) len = comm_size - 1;
    strncpy(comm, comm_start, len);
    comm[len] = '\0';
    p++; // skip ')'
    // skip to field 14 (utime)
    int field = 3; // consumed pid and comm, next is state
    while (field < 14 && *p) {
        while (*p && *p == ' ') p++;
        while (*p && *p != ' ') p++;
        field++;
    }
    if (field < 14) return -1;
    if (sscanf(p, "%llu", utime) != 1) return -1;
    // skip to stime
    while (*p && *p != ' ') p++;
    while (*p && *p == ' ') p++;
    if (sscanf(p, "%llu", stime) != 1) return -1;
    // skip to field 24 (rss)
    for (int i = 16; i <= 23; i++) {
        while (*p && *p != ' ') p++;
        while (*p && *p == ' ') p++;
    }
    if (sscanf(p, "%ld", rss) != 1) return -1;
    return 0;
}

// Check if path is a directory (more portable than d_type)
static int is_directory(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    return 0;
}

// Read total memory from /proc/meminfo (used for mem%)
static guint64 get_total_memory(void) {
    static guint64 total = 0;
    if (total != 0) return total;
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) return 0;
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "MemTotal: %llu kB", &total) == 1) break;
    }
    fclose(fp);
    return total;
}

GList* get_process_list(void) {
    GList *list = NULL;
    DIR *dir = opendir("/proc");
    if (!dir) return NULL;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip non-numeric entries
        pid_t pid = atoi(entry->d_name);
        if (pid <= 0) continue;

        // Build full path and check if it's a directory
        char path[256];
        snprintf(path, sizeof(path), "/proc/%s", entry->d_name);
        if (!is_directory(path)) continue;

        char stat_buf[1024];
        if (read_proc_stat(pid, stat_buf, sizeof(stat_buf)) != 0) continue;

        char comm[256];
        unsigned long long utime, stime;
        long rss_pages;
        if (parse_stat(stat_buf, comm, sizeof(comm), &utime, &stime, &rss_pages) != 0) continue;

        ProcessEntry *p = g_new(ProcessEntry, 1);
        p->pid = pid;
        strncpy(p->name, comm, sizeof(p->name)-1);
        p->name[sizeof(p->name)-1] = '\0';
        p->cpu_percent = 0.0; // Placeholder – full CPU% calculation would require history

        // Memory percent: rss (pages) * page size / total memory
        long page_size = sysconf(_SC_PAGESIZE);
        guint64 total_mem = get_total_memory() * 1024; // convert kB to bytes
        if (total_mem > 0) {
            p->mem_percent = 100.0 * (rss_pages * page_size) / total_mem;
        } else {
            p->mem_percent = 0;
        }

        list = g_list_append(list, p);
    }
    closedir(dir);
    return list;
}

void free_process_list(GList *list) {
    for (GList *l = list; l != NULL; l = l->next) {
        g_free(l->data);
    }
    g_list_free(list);
}