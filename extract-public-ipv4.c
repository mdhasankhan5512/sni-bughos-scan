#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#define NUM_THREADS 12  // Number of threads to run in parallel (adjusted for 6-core CPU)

uint32_t ip_to_int(const char *ip) {
    uint32_t result = 0;
    int byte;
    while (*ip) {
        byte = 0;
        while (*ip >= '0' && *ip <= '9') {
            byte = byte * 10 + (*ip - '0');
            ip++;
        }
        result = (result << 8) | byte;

        if (*ip == '.') ip++;
    }
    return result;
}

void save_progress(uint32_t ip_int) {
    FILE *progress_file = fopen("progress.txt", "w");
    if (progress_file) {
        fprintf(progress_file, "%u\n", ip_int);
        fclose(progress_file);
    }
}

int ping_ip(const char *ip) {
    char command[50];
    snprintf(command, sizeof(command), "ping -c 1 -W 1 %s > /dev/null 2>&1", ip);
    return system(command);
}

void *ping_task(void *arg) {
    uint32_t ip = *((uint32_t *)arg);

    // Convert the current integer IP to string format
    char ip_str[16];
    snprintf(ip_str, sizeof(ip_str), "%u.%u.%u.%u", 
             (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, 
             (ip >> 8) & 0xFF, ip & 0xFF);

    // Ping the IP
    if (ping_ip(ip_str) == 0) {
        printf("Found live IP: %s\n", ip_str);
        FILE *file = fopen("publicips.txt", "a");
        if (file) {
            fprintf(file, "%s\n", ip_str);
            fclose(file);
        }
    }

    // Save progress periodically (every 10 seconds)
    if (ip % 100 == 0 || (ip % 1000 == 0)) {
        save_progress(ip);
        printf("Progress saved: Last scanned IP: %s\n", ip_str);
    }

    return NULL;
}

void scan_ips(uint32_t start_ip) {
    uint32_t ip = start_ip;
    uint32_t end_ip = 0xFFFFFFFF; // 255.255.255.255 as the last IP

    pthread_t threads[NUM_THREADS];
    int thread_count = 0;

    time_t last_saved = time(NULL); // Time to track last progress save

    while (ip <= end_ip) {
        // Create a thread for each IP in range
        pthread_create(&threads[thread_count], NULL, ping_task, (void *)&ip);

        // Wait for a thread to complete before creating another
        thread_count++;
        if (thread_count >= NUM_THREADS) {
            for (int i = 0; i < NUM_THREADS; i++) {
                pthread_join(threads[i], NULL);
            }
            thread_count = 0;
        }

        ip++;

        // Save progress every 10 seconds
        if (difftime(time(NULL), last_saved) >= 10) {
            save_progress(ip);
            printf("Progress saved: Last scanned IP: %u.%u.%u.%u\n", 
                   (ip >> 24) & 0xFF, (ip >> 16) & 0xFF,
                   (ip >> 8) & 0xFF, ip & 0xFF);
            last_saved = time(NULL);
        }
    }

    // Ensure any remaining threads finish
    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }
}

int main() {
    uint32_t start_ip = 0;
    FILE *progress_file = fopen("progress.txt", "r");

    if (progress_file) {
        fscanf(progress_file, "%u", &start_ip);
        fclose(progress_file);
        printf("Resuming from last scanned IP: %u.%u.%u.%u\n", 
               (start_ip >> 24) & 0xFF, (start_ip >> 16) & 0xFF,
               (start_ip >> 8) & 0xFF, start_ip & 0xFF);
    } else {
        printf("No progress file found. Starting from 0.0.0.0\n");
    }

    scan_ips(start_ip);

    return 0;
}
