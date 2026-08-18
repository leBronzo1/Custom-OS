#include "monitor_worker.h"
#include "python_bridge.h"
#include "cJSON.h"

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static SDL_Thread *s_thread = NULL;
static SDL_mutex *s_mutex = NULL;
static SDL_cond *s_wakeup = NULL;

static int s_stop_requested = 0;
static int s_refresh_requested = 0;
static int s_refresh_in_flight = 0;
static int s_result_ready = 0;
static MonitorSnapshot s_completed_snapshot;

static int json_number(cJSON *object, const char *name, double *out) {
    cJSON *value = cJSON_GetObjectItem(object, name);
    if (!cJSON_IsNumber(value)) return 0;
    *out = value->valuedouble;
    return 1;
}

static int json_string(cJSON *object, const char *name, char *out, size_t out_size) {
    cJSON *value = cJSON_GetObjectItem(object, name);
    if (!cJSON_IsString(value) || !value->valuestring) return 0;
    snprintf(out, out_size, "%s", value->valuestring);
    return 1;
}

static void collect_snapshot(MonitorSnapshot *snapshot) {
    memset(snapshot, 0, sizeof(*snapshot));

    if (!python_bridge_init()) {
        snprintf(snapshot->error, sizeof(snapshot->error),
                 "Could not start the Python monitor.");
        return;
    }

    char *json = python_bridge_get_system_info();
    if (!json) {
        snprintf(snapshot->error, sizeof(snapshot->error),
                 "Could not read system information.");
        return;
    }

    cJSON *root = cJSON_Parse(json);
    free(json);
    if (!root) {
        snprintf(snapshot->error, sizeof(snapshot->error),
                 "Python returned invalid monitor data.");
        return;
    }

    cJSON *cpu = cJSON_GetObjectItem(root, "cpu");
    cJSON *memory = cJSON_GetObjectItem(root, "memory");
    cJSON *disk = cJSON_GetObjectItem(root, "disk");
    double cores = 0;
    double memory_total = 0, memory_used = 0;
    double disk_total = 0, disk_used = 0;

    int valid = cJSON_IsObject(cpu) && cJSON_IsObject(memory) && cJSON_IsObject(disk)
        && json_number(cpu, "cores", &cores)
        && json_number(cpu, "usage", &snapshot->cpu_percent)
        && json_number(memory, "total", &memory_total)
        && json_number(memory, "used", &memory_used)
        && json_number(memory, "percent", &snapshot->ram_percent)
        && json_number(disk, "total", &disk_total)
        && json_number(disk, "used", &disk_used)
        && json_number(disk, "percent", &snapshot->disk_percent)
        && json_string(root, "current_time", snapshot->date, sizeof(snapshot->date))
        && json_string(root, "boot_time", snapshot->boot_time, sizeof(snapshot->boot_time));

    if (valid) {
        snapshot->cores = (int)cores;
        snapshot->ram_total_gib = memory_total / (1024.0 * 1024.0 * 1024.0);
        snapshot->ram_used_gib = memory_used / (1024.0 * 1024.0 * 1024.0);
        snapshot->disk_total_gib = disk_total / (1024.0 * 1024.0 * 1024.0);
        snapshot->disk_used_gib = disk_used / (1024.0 * 1024.0 * 1024.0);
    } else {
        memset(snapshot, 0, sizeof(*snapshot));
        snprintf(snapshot->error, sizeof(snapshot->error),
                 "Python returned incomplete monitor data.");
    }

    cJSON_Delete(root);
}

static int monitor_thread_main(void *unused) {
    (void)unused;

    for (;;) {
        SDL_LockMutex(s_mutex);
        while (!s_stop_requested && !s_refresh_requested)
            SDL_CondWait(s_wakeup, s_mutex);

        if (s_stop_requested) {
            SDL_UnlockMutex(s_mutex);
            break;
        }

        s_refresh_requested = 0;
        s_refresh_in_flight = 1;
        SDL_UnlockMutex(s_mutex);

        MonitorSnapshot snapshot;
        collect_snapshot(&snapshot);

        SDL_LockMutex(s_mutex);
        s_completed_snapshot = snapshot;
        s_result_ready = 1;
        s_refresh_in_flight = 0;
        SDL_UnlockMutex(s_mutex);
    }

    /* Python is initialized, called, and finalized only on this thread. */
    python_bridge_shutdown();
    return 0;
}

int monitor_worker_start(void) {
    if (s_thread) return 0;

    s_mutex = SDL_CreateMutex();
    if (!s_mutex) {
        fprintf(stderr, "[monitor] Could not create mutex: %s\n", SDL_GetError());
        return -1;
    }

    s_wakeup = SDL_CreateCond();
    if (!s_wakeup) {
        fprintf(stderr, "[monitor] Could not create condition variable: %s\n", SDL_GetError());
        SDL_DestroyMutex(s_mutex);
        s_mutex = NULL;
        return -1;
    }

    s_stop_requested = 0;
    s_refresh_requested = 0;
    s_refresh_in_flight = 0;
    s_result_ready = 0;
    s_thread = SDL_CreateThread(monitor_thread_main, "system-monitor", NULL);
    if (!s_thread) {
        fprintf(stderr, "[monitor] Could not create worker: %s\n", SDL_GetError());
        SDL_DestroyCond(s_wakeup);
        SDL_DestroyMutex(s_mutex);
        s_wakeup = NULL;
        s_mutex = NULL;
        return -1;
    }

    return 0;
}

int monitor_worker_request_refresh(void) {
    if (!s_thread || !s_mutex) return -1;

    SDL_LockMutex(s_mutex);
    if (s_stop_requested) {
        SDL_UnlockMutex(s_mutex);
        return -1;
    }

    if (s_refresh_requested || s_refresh_in_flight) {
        SDL_UnlockMutex(s_mutex);
        return 0;
    }

    s_refresh_requested = 1;
    SDL_CondSignal(s_wakeup);
    SDL_UnlockMutex(s_mutex);
    return 1;
}

int monitor_worker_poll(MonitorSnapshot *out) {
    if (!out || !s_thread || !s_mutex) return 0;

    SDL_LockMutex(s_mutex);
    if (!s_result_ready) {
        SDL_UnlockMutex(s_mutex);
        return 0;
    }

    *out = s_completed_snapshot;
    s_result_ready = 0;
    SDL_UnlockMutex(s_mutex);
    return 1;
}

void monitor_worker_shutdown(void) {
    if (!s_thread) return;

    SDL_LockMutex(s_mutex);
    s_stop_requested = 1;
    SDL_CondSignal(s_wakeup);
    SDL_UnlockMutex(s_mutex);

    SDL_WaitThread(s_thread, NULL);
    SDL_DestroyCond(s_wakeup);
    SDL_DestroyMutex(s_mutex);

    s_thread = NULL;
    s_wakeup = NULL;
    s_mutex = NULL;
    s_stop_requested = 0;
    s_refresh_requested = 0;
    s_refresh_in_flight = 0;
    s_result_ready = 0;
}
