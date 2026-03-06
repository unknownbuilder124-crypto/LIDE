#include "monitor.h"

// Read CPU times from /proc/stat
static void read_cpu_times(guint64 *user, guint64 *nice, guint64 *system, guint64 *idle,
                           guint64 *iowait, guint64 *irq, guint64 *softirq, guint64 *steal)
                           
                           {
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) return;

    char buf[256];
    if (fgets(buf, sizeof(buf), fp)) 
    {
        sscanf(buf, "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
               user, nice, system, idle, iowait, irq, softirq, steal);
    }
    fclose(fp);
}

void update_cpu_usage(CpuData *cpu) 

{
    guint64 user, nice, system, idle, iowait, irq, softirq, steal;
    read_cpu_times(&user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);

    guint64 total = user + nice + system + idle + iowait + irq + softirq + steal;
    guint64 idle_all = idle + iowait; // iowait is idle time waiting for I/O

    if (cpu->total_prev != 0) {
        guint64 total_diff = total - cpu->total_prev;
        guint64 idle_diff = idle_all - cpu->idle_prev;
        cpu->usage = 100.0 * (total_diff - idle_diff) / total_diff;
        if (cpu->usage < 0) cpu->usage = 0;
    } else {
        cpu->usage = 0;
    }

    cpu->total_prev = total;
    cpu->idle_prev = idle_all;
}

// Drawing function for CPU graph
gboolean draw_cpu_graph(GtkWidget *widget, cairo_t *cr, gpointer data) 

{
    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);
    int width = alloc.width;
    int height = alloc.height;

    // Background
    cairo_set_source_rgb(cr, 0x0b/255.0, 0x0f/255.0, 0x14/255.0); // #0b0f14
    cairo_paint(cr);

    // Draw grid
    cairo_set_source_rgb(cr, 0x88/255.0, 0x88/255.0, 0x88/255.0); // gray
    cairo_set_line_width(cr, 1);
    for (int i = 0; i < 5; i++) {
        int y = height * (i+1) / 5;
        cairo_move_to(cr, 0, y);
        cairo_line_to(cr, width, y);
        cairo_stroke(cr);
    }

    // Draw graph line
    if (cpu_history_index > 0) 
    {
        double step = (double)width / HISTORY_SIZE;
        cairo_set_source_rgb(cr, 0x00/255.0, 0xff/255.0, 0x88/255.0); // #00ff88
        cairo_set_line_width(cr, 2);
        int start = cpu_history_index; 
        int first = 1;
        for (int i = 0; i < HISTORY_SIZE; i++) {
            int idx = (cpu_history_index + i) % HISTORY_SIZE;
            double value = cpu_history[idx];
            double x = i * step;
            double y = height - (value / 100.0) * height; // value is percent
            if (y < 0) y = 0;
            if (y > height) y = height;
            if (first) {
                cairo_move_to(cr, x, y);
                first = 0;
            } else {
                cairo_line_to(cr, x, y);
            }
        }
        cairo_stroke(cr);
    }

    return FALSE;
}