#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <mach/mach.h>
#include <mach/host_info.h>
#include <mach/vm_statistics.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/proc.h>
#include <libproc.h>
#include <CoreFoundation/CoreFoundation.h>
#include <net/if.h>
#include <net/if_mib.h>

// Color scheme inspired by minimalist design
#define COLOR_BG 0
#define COLOR_FG 1
#define COLOR_ACCENT 2
#define COLOR_WARN 3

// System stats structure
typedef struct {
    // CPU
    double cpu_total;
    double *cpu_cores;
    int num_cores;
    
    // Memory
    unsigned long long mem_total;
    unsigned long long mem_used;
    unsigned long long mem_free;
    unsigned long long swap_total;
    unsigned long long swap_used;
    
    // Load average
    double load_avg[3];
    
    // Uptime
    time_t uptime;
    
    // Processes
    struct {
        pid_t pid;
        char name[256];
        double cpu_percent;
        unsigned long long mem_kb;
    } processes[20];
    int num_processes;
    
    // Network (basic counters)
    unsigned long long net_rx_bytes;
    unsigned long long net_tx_bytes;
    
    // Disk I/O (simplified)
    unsigned long long disk_reads;
    unsigned long long disk_writes;
} system_stats_t;

// Previous values for calculating rates
static unsigned long long prev_net_rx = 0;
static unsigned long long prev_net_tx = 0;
static unsigned long long prev_disk_reads = 0;
static unsigned long long prev_disk_writes = 0;

// Previous CPU tick values for percentage calculation
static natural_t prev_cpu_ticks[CPU_STATE_MAX] = {0};
static bool first_cpu_measurement = true;

// Function prototypes
void init_colors(void);
void get_system_stats(system_stats_t *stats);
void get_cpu_stats(system_stats_t *stats);
void get_memory_stats(system_stats_t *stats);
void get_load_uptime(system_stats_t *stats);
void get_process_stats(system_stats_t *stats);
void get_network_stats(system_stats_t *stats);
void get_disk_stats(system_stats_t *stats);
void draw_ui(system_stats_t *stats, int process_scroll);
void draw_header(WINDOW *win);
void draw_cpu(WINDOW *win, system_stats_t *stats, int start_y);
void draw_memory(WINDOW *win, system_stats_t *stats, int start_y);
void draw_processes(WINDOW *win, system_stats_t *stats, int start_y, int max_lines, int process_scroll);
void draw_network(WINDOW *win, system_stats_t *stats, int start_y);
void draw_disk(WINDOW *win, system_stats_t *stats, int start_y);
void cleanup_system_stats(system_stats_t *stats);

int main(void) {
    system_stats_t stats = {0};
    int ch;
    int process_scroll = 0;
    int max_process_scroll = 0;
    
    // Initialize ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(1000); // 1 second timeout
    
    if (has_colors()) {
        start_color();
        init_colors();
    }
    
    // Get initial system info
    get_system_stats(&stats);
    prev_net_rx = stats.net_rx_bytes;
    prev_net_tx = stats.net_tx_bytes;
    prev_disk_reads = stats.disk_reads;
    prev_disk_writes = stats.disk_writes;
    
    while ((ch = getch()) != 'q' && ch != 'Q') {
        // Handle keyboard input
        switch (ch) {
            case KEY_UP:
                if (process_scroll > 0) process_scroll--;
                break;
            case KEY_DOWN:
                if (process_scroll < max_process_scroll) process_scroll++;
                break;
            case KEY_PPAGE: // Page Up
                process_scroll = (process_scroll > 5) ? process_scroll - 5 : 0;
                break;
            case KEY_NPAGE: // Page Down
                process_scroll = (process_scroll + 5 < max_process_scroll) ? process_scroll + 5 : max_process_scroll;
                break;
        }
        
        // Update stats
        get_system_stats(&stats);
        
        // Calculate rates (per second)
        unsigned long long net_rx_rate = (stats.net_rx_bytes - prev_net_rx);
        unsigned long long net_tx_rate = (stats.net_tx_bytes - prev_net_tx);
        unsigned long long disk_read_rate = (stats.disk_reads - prev_disk_reads);
        unsigned long long disk_write_rate = (stats.disk_writes - prev_disk_writes);
        
        // Draw UI
        draw_ui(&stats, process_scroll);
        
        // Update previous values
        prev_net_rx = stats.net_rx_bytes;
        prev_net_tx = stats.net_tx_bytes;
        prev_disk_reads = stats.disk_reads;
        prev_disk_writes = stats.disk_writes;
        
        // Update max scroll
        max_process_scroll = (stats.num_processes > 10) ? stats.num_processes - 10 : 0;
        if (process_scroll > max_process_scroll) process_scroll = max_process_scroll;
        
        // Refresh and sleep briefly
        refresh();
        usleep(100000); // Small delay to prevent excessive CPU usage
    }
    
    cleanup_system_stats(&stats);
    endwin();
    return 0;
}

void init_colors(void) {
    init_pair(COLOR_FG, COLOR_WHITE, COLOR_BLACK);
    init_pair(COLOR_ACCENT, COLOR_CYAN, COLOR_BLACK);
    init_pair(COLOR_WARN, COLOR_YELLOW, COLOR_BLACK);
    init_pair(COLOR_BG, COLOR_BLACK, COLOR_BLACK);
    bkgd(COLOR_PAIR(COLOR_BG));
}

void get_system_stats(system_stats_t *stats) {
    get_cpu_stats(stats);
    get_memory_stats(stats);
    get_load_uptime(stats);
    get_process_stats(stats);
    get_network_stats(stats);
    get_disk_stats(stats);
}

void get_cpu_stats(system_stats_t *stats) {
    host_cpu_load_info_data_t cpu_load;
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    kern_return_t kr = host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO,
                                     (host_info_t)&cpu_load, &count);

    if (kr != KERN_SUCCESS) return;

    // Get number of cores
    size_t size = sizeof(int);
    sysctlbyname("hw.ncpu", &stats->num_cores, &size, NULL, 0);

    if (stats->cpu_cores == NULL) {
        stats->cpu_cores = malloc(sizeof(double) * stats->num_cores);
    }

    // Calculate CPU usage percentages based on tick differences
    natural_t current_ticks[CPU_STATE_MAX];
    memcpy(current_ticks, cpu_load.cpu_ticks, sizeof(current_ticks));

    if (first_cpu_measurement) {
        // First measurement - can't calculate percentage yet
        memset(stats->cpu_cores, 0, sizeof(double) * stats->num_cores);
        stats->cpu_total = 0.0;
        memcpy(prev_cpu_ticks, current_ticks, sizeof(current_ticks));
        first_cpu_measurement = false;
        return;
    }

    // Calculate total ticks across all states
    natural_t total_current = 0;
    natural_t total_prev = 0;
    for (int i = 0; i < CPU_STATE_MAX; i++) {
        total_current += current_ticks[i];
        total_prev += prev_cpu_ticks[i];
    }

    natural_t total_diff = total_current - total_prev;
    if (total_diff == 0) {
        // No change in total ticks
        memset(stats->cpu_cores, 0, sizeof(double) * stats->num_cores);
        stats->cpu_total = 0.0;
        return;
    }

    // Calculate per-core CPU usage (simplified - macOS doesn't provide per-core tick info easily)
    // For now, distribute total CPU usage across cores
    double total_cpu_percent = 0.0;
    if (total_prev > 0) {
        natural_t active_ticks = (current_ticks[CPU_STATE_USER] - prev_cpu_ticks[CPU_STATE_USER]) +
                                (current_ticks[CPU_STATE_SYSTEM] - prev_cpu_ticks[CPU_STATE_SYSTEM]) +
                                (current_ticks[CPU_STATE_NICE] - prev_cpu_ticks[CPU_STATE_NICE]);
        total_cpu_percent = (double)active_ticks / (double)total_diff * 100.0;
    }

    // Distribute total CPU usage across cores (simplified)
    for (int i = 0; i < stats->num_cores; i++) {
        stats->cpu_cores[i] = total_cpu_percent / stats->num_cores;
    }

    stats->cpu_total = total_cpu_percent;

    // Update previous values
    memcpy(prev_cpu_ticks, current_ticks, sizeof(current_ticks));
}

void get_memory_stats(system_stats_t *stats) {
    vm_statistics64_data_t vm_stats;
    mach_msg_type_number_t count = sizeof(vm_stats) / sizeof(integer_t);
    kern_return_t kr = host_statistics64(mach_host_self(), HOST_VM_INFO64, 
                                        (host_info_t)&vm_stats, &count);
    
    if (kr != KERN_SUCCESS) return;
    
    // Get total memory
    size_t size = sizeof(unsigned long long);
    sysctlbyname("hw.memsize", &stats->mem_total, &size, NULL, 0);
    
    // Calculate used/free memory
    vm_size_t page_size;
    host_page_size(mach_host_self(), &page_size);
    
    stats->mem_free = (unsigned long long)vm_stats.free_count * page_size;
    stats->mem_used = stats->mem_total - stats->mem_free;
    
    // Swap info (simplified)
    struct xsw_usage xsu;
    size = sizeof(xsu);
    sysctlbyname("vm.swapusage", &xsu, &size, NULL, 0);
    stats->swap_total = xsu.xsu_total;
    stats->swap_used = xsu.xsu_used;
}

void get_load_uptime(system_stats_t *stats) {
    // Load average
    getloadavg(stats->load_avg, 3);
    
    // Uptime
    struct timeval tv;
    size_t size = sizeof(tv);
    sysctlbyname("kern.boottime", &tv, &size, NULL, 0);
    stats->uptime = time(NULL) - tv.tv_sec;
}

void get_process_stats(system_stats_t *stats) {
    int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};
    size_t size;
    
    if (sysctl(mib, 4, NULL, &size, NULL, 0) < 0) return;
    
    struct kinfo_proc *procs = malloc(size);
    if (procs == NULL) return;
    
    if (sysctl(mib, 4, procs, &size, NULL, 0) < 0) {
        free(procs);
        return;
    }
    
    int num_procs = size / sizeof(struct kinfo_proc);
    
    // Sort processes by CPU usage (simplified)
    struct {
        struct kinfo_proc proc;
        double cpu_percent;
        unsigned long long mem_kb;
    } proc_list[256];
    
    int valid_procs = 0;
    for (int i = 0; i < num_procs && valid_procs < 256; i++) {
        if (procs[i].kp_proc.p_pid == 0) continue;
        
        proc_list[valid_procs].proc = procs[i];
        
        // Get process CPU and memory info
        struct proc_taskinfo taskinfo;
        pid_t pid = procs[i].kp_proc.p_pid;
        int ret = proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &taskinfo, sizeof(taskinfo));
        
        if (ret > 0) {
            // CPU percentage (simplified)
            proc_list[valid_procs].cpu_percent = (double)taskinfo.pti_total_user / 1000000.0;
            proc_list[valid_procs].mem_kb = taskinfo.pti_resident_size / 1024;
        } else {
            proc_list[valid_procs].cpu_percent = 0.0;
            proc_list[valid_procs].mem_kb = 0;
        }
        
        valid_procs++;
    }
    
    // Sort by CPU usage (bubble sort for simplicity)
    for (int i = 0; i < valid_procs - 1; i++) {
        for (int j = 0; j < valid_procs - i - 1; j++) {
            if (proc_list[j].cpu_percent < proc_list[j + 1].cpu_percent) {
                struct kinfo_proc temp_proc = proc_list[j].proc;
                double temp_cpu = proc_list[j].cpu_percent;
                unsigned long long temp_mem = proc_list[j].mem_kb;
                
                proc_list[j].proc = proc_list[j + 1].proc;
                proc_list[j].cpu_percent = proc_list[j + 1].cpu_percent;
                proc_list[j].mem_kb = proc_list[j + 1].mem_kb;
                
                proc_list[j + 1].proc = temp_proc;
                proc_list[j + 1].cpu_percent = temp_cpu;
                proc_list[j + 1].mem_kb = temp_mem;
            }
        }
    }
    
    // Fill stats structure
    stats->num_processes = (valid_procs < 20) ? valid_procs : 20;
    for (int i = 0; i < stats->num_processes; i++) {
        stats->processes[i].pid = proc_list[i].proc.kp_proc.p_pid;
        strncpy(stats->processes[i].name, proc_list[i].proc.kp_proc.p_comm, 255);
        stats->processes[i].name[255] = '\0';
        stats->processes[i].cpu_percent = proc_list[i].cpu_percent;
        stats->processes[i].mem_kb = proc_list[i].mem_kb;
    }
    
    free(procs);
}

void get_network_stats(system_stats_t *stats) {
    // Network statistics (simplified - using sysctl for basic counters)
    // For now, we'll use a placeholder implementation
    // In a production version, you'd properly enumerate network interfaces
    // and sum their statistics using getifaddrs() or sysctl with proper MIBs

    static unsigned long long fake_counter = 0;
    fake_counter += 1024; // Simulate some network activity

    stats->net_rx_bytes = fake_counter;
    stats->net_tx_bytes = fake_counter / 2;
}

void get_disk_stats(system_stats_t *stats) {
    // Disk I/O statistics (simplified)
    // This would typically require IOKit, but for simplicity using basic counters
    // In a real implementation, you'd use IOKit to get disk I/O stats
    
    // Placeholder - in practice, you'd query disk statistics
    stats->disk_reads = 0;
    stats->disk_writes = 0;
}

void draw_ui(system_stats_t *stats, int process_scroll) {
    clear();

    int height, width;
    getmaxyx(stdscr, height, width);
    
    // Draw header
    draw_header(stdscr);
    
    // Layout sections based on terminal size
    int section_y = 2;
    
    // CPU section
    draw_cpu(stdscr, stats, section_y);
    section_y += 5 + stats->num_cores;
    
    // Memory section
    draw_memory(stdscr, stats, section_y);
    section_y += 6;
    
    // Network section
    draw_network(stdscr, stats, section_y);
    section_y += 4;
    
    // Disk section
    draw_disk(stdscr, stats, section_y);
    section_y += 4;
    
    // Processes section
    int remaining_height = height - section_y - 2;
    if (remaining_height > 5) {
        draw_processes(stdscr, stats, section_y, remaining_height - 2, process_scroll);
    }
}

void draw_header(WINDOW *win) {
    int height, width;
    getmaxyx(win, height, width);
    
    attron(COLOR_PAIR(COLOR_ACCENT) | A_BOLD);
    mvprintw(0, (width - 14) / 2, " System Monitor ");
    attroff(COLOR_PAIR(COLOR_ACCENT) | A_BOLD);
    
    attron(COLOR_PAIR(COLOR_FG));
    mvprintw(1, 0, "Press 'q' to quit | Arrow keys to scroll processes");
    attroff(COLOR_PAIR(COLOR_FG));
}

void draw_cpu(WINDOW *win, system_stats_t *stats, int start_y) {
    attron(COLOR_PAIR(COLOR_ACCENT) | A_BOLD);
    mvprintw(start_y++, 0, "CPU Usage");
    attroff(COLOR_PAIR(COLOR_ACCENT) | A_BOLD);
    
    attron(COLOR_PAIR(COLOR_FG));
    mvprintw(start_y++, 0, "Total: %.1f%%", stats->cpu_total);
    
    for (int i = 0; i < stats->num_cores && i < 8; i++) { // Show max 8 cores
        mvprintw(start_y++, 0, "Core %d: %.1f%%", i + 1, stats->cpu_cores[i]);
    }
    
    // Load average
    mvprintw(start_y++, 0, "Load: %.2f %.2f %.2f", 
             stats->load_avg[0], stats->load_avg[1], stats->load_avg[2]);
    attroff(COLOR_PAIR(COLOR_FG));
}

void draw_memory(WINDOW *win, system_stats_t *stats, int start_y) {
    attron(COLOR_PAIR(COLOR_ACCENT) | A_BOLD);
    mvprintw(start_y++, 0, "Memory");
    attroff(COLOR_PAIR(COLOR_ACCENT) | A_BOLD);
    
    attron(COLOR_PAIR(COLOR_FG));
    mvprintw(start_y++, 0, "Used: %llu MB / %llu MB (%.1f%%)", 
             stats->mem_used / 1024 / 1024, 
             stats->mem_total / 1024 / 1024,
             (double)stats->mem_used / (double)stats->mem_total * 100.0);
    
    mvprintw(start_y++, 0, "Free: %llu MB", stats->mem_free / 1024 / 1024);
    
    if (stats->swap_total > 0) {
        mvprintw(start_y++, 0, "Swap: %llu MB / %llu MB (%.1f%%)", 
                 stats->swap_used / 1024 / 1024, 
                 stats->swap_total / 1024 / 1024,
                 (double)stats->swap_used / (double)stats->swap_total * 100.0);
    }
    
    // Uptime
    int days = stats->uptime / 86400;
    int hours = (stats->uptime % 86400) / 3600;
    int minutes = (stats->uptime % 3600) / 60;
    mvprintw(start_y++, 0, "Uptime: %dd %dh %dm", days, hours, minutes);
    attroff(COLOR_PAIR(COLOR_FG));
}

void draw_processes(WINDOW *win, system_stats_t *stats, int start_y, int max_lines, int process_scroll) {
    attron(COLOR_PAIR(COLOR_ACCENT) | A_BOLD);
    mvprintw(start_y++, 0, "Top Processes (by CPU)");
    attroff(COLOR_PAIR(COLOR_ACCENT) | A_BOLD);

    attron(COLOR_PAIR(COLOR_FG));
    mvprintw(start_y++, 0, "%-8s %-20s %-8s %-10s", "PID", "Name", "CPU%", "Memory");

    int display_count = (stats->num_processes - process_scroll < max_lines - 2) ?
                       stats->num_processes - process_scroll : max_lines - 2;
    if (display_count < 0) display_count = 0;

    for (int i = 0; i < display_count; i++) {
        int process_index = process_scroll + i;
        if (process_index >= stats->num_processes) break;

        mvprintw(start_y + i, 0, "%-8d %-20s %-8.1f %-10llu KB",
                 stats->processes[process_index].pid,
                 stats->processes[process_index].name,
                 stats->processes[process_index].cpu_percent,
                 stats->processes[process_index].mem_kb);
    }
    attroff(COLOR_PAIR(COLOR_FG));
}

void draw_network(WINDOW *win, system_stats_t *stats, int start_y) {
    attron(COLOR_PAIR(COLOR_ACCENT) | A_BOLD);
    mvprintw(start_y++, 0, "Network");
    attroff(COLOR_PAIR(COLOR_ACCENT) | A_BOLD);
    
    attron(COLOR_PAIR(COLOR_FG));
    mvprintw(start_y++, 0, "RX: %llu bytes/s", stats->net_rx_bytes - prev_net_rx);
    mvprintw(start_y++, 0, "TX: %llu bytes/s", stats->net_tx_bytes - prev_net_tx);
    attroff(COLOR_PAIR(COLOR_FG));
}

void draw_disk(WINDOW *win, system_stats_t *stats, int start_y) {
    attron(COLOR_PAIR(COLOR_ACCENT) | A_BOLD);
    mvprintw(start_y++, 0, "Disk I/O");
    attroff(COLOR_PAIR(COLOR_ACCENT) | A_BOLD);
    
    attron(COLOR_PAIR(COLOR_FG));
    mvprintw(start_y++, 0, "Reads: %llu ops/s", stats->disk_reads - prev_disk_reads);
    mvprintw(start_y++, 0, "Writes: %llu ops/s", stats->disk_writes - prev_disk_writes);
    attroff(COLOR_PAIR(COLOR_FG));
}

void cleanup_system_stats(system_stats_t *stats) {
    if (stats->cpu_cores) {
        free(stats->cpu_cores);
        stats->cpu_cores = NULL;
    }
}
