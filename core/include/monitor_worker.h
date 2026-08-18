#ifndef MONITOR_WORKER_H
#define MONITOR_WORKER_H

typedef struct {
    int cores;
    double cpu_percent;
    double ram_total_gib;
    double ram_used_gib;
    double ram_percent;
    double disk_total_gib;
    double disk_used_gib;
    double disk_percent;
    char date[20];
    char boot_time[20];
    char error[160];
} MonitorSnapshot;

/* Lazily create the worker. Returns 0 on success and -1 on failure. */
int monitor_worker_start(void);

/*
 * Queue one refresh. Returns 1 when queued, 0 when one is already running,
 * and -1 if the worker is unavailable.
 */
int monitor_worker_request_refresh(void);

/* Copy a completed snapshot to the caller without blocking on Python work. */
int monitor_worker_poll(MonitorSnapshot *out);

/* Stop and join the worker. The worker owns Python shutdown. */
void monitor_worker_shutdown(void);

#endif
