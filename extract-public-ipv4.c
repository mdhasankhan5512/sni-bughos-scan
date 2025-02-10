#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

#define OUTPUT_FILE "publicips.txt"
#define PROGRESS_FILE "progress.txt"
#define PING_COUNT 1
#define TIMEOUT 1
#define SAVE_INTERVAL 10 // Save progress every 10 seconds

// Function to check if an IP is public
int is_public_ip(unsigned long ip_num) {
    // Private IP ranges
    if ((ip_num >= 0x0A000000 && ip_num <= 0x0AFFFFFF) ||  // 10.0.0.0/8
        (ip_num >= 0xAC100000 && ip_num <= 0xAC1FFFFF) ||  // 172.16.0.0/12
        (ip_num >= 0xC0A80000 && ip_num <= 0xC0A8FFFF)) {  // 192.168.0.0/16
        return 0;
    }
    return 1;
}

// Function to ping an IP
int ping_ip(const char *ip) {
    char command[64];
    snprintf(command, sizeof(command), "ping -c %d -W %d %s > /dev/null 2>&1", PING_COUNT, TIMEOUT, ip);
    return system(command) == 0;
}

// Function to save progress (last scanned IP) as integer
void save_progress(unsigned long last_ip) {
    FILE *file = fopen(PROGRESS_FILE, "w");
    if (file) {
        fprintf(file, "%lu\n", last_ip);  // Save IP as an integer
        fclose(file);
    }
}

// Function to load the progress (last scanned IP) as integer
int load_progress(unsigned long *last_ip) {
    FILE *file = fopen(PROGRESS_FILE, "r");
    if (file) {
        fscanf(file, "%lu", last_ip);  // Read IP from file as an integer
        fclose(file);
        return 1;  // Progress file exists and is read
    }
    return 0;  // No progress file found
}

// Function to convert integer to human-readable IP
void int_to_ip(unsigned long ip_num, char *ip_str) {
    struct in_addr addr;
    addr.s_addr = htonl(ip_num);  // Convert to network byte order
    strcpy(ip_str, inet_ntoa(addr));  // Convert to string
}

// Function to scan public IPs
void scan_ips() {
    FILE *file = fopen(OUTPUT_FILE, "a");  // Open in append mode
    if (!file) {
        perror("Failed to open output file");
        exit(EXIT_FAILURE);
    }

    unsigned long last_ip = 0;  // Default to start from 0.0.0.0
    if (!load_progress(&last_ip)) {  // If no progress file, start from 0.0.0.0
        printf("No progress file found. Starting from 0.0.0.0.\n");
    } else {
        char last_ip_str[16];
        int_to_ip(last_ip, last_ip_str);
        printf("Resuming from last scanned IP: %s\n", last_ip_str);
    }

    time_t last_save_time = time(NULL);

    for (; last_ip <= 0xFFFFFFFF; last_ip++) { // 1.0.0.0 to 255.255.255.255
        char ip[16];
        int_to_ip(last_ip, ip);

        if (is_public_ip(last_ip) && ping_ip(ip)) {
            printf("%s is online\n", ip);
            fprintf(file, "%s\n", ip);
            fflush(file);
        }

        // Save progress every SAVE_INTERVAL seconds
        if (time(NULL) - last_save_time >= SAVE_INTERVAL) {
            save_progress(last_ip);  // Save progress as integer
            printf("Progress saved: Last scanned IP: %s\n", ip);  // Print in human-readable format
            last_save_time = time(NULL);  // Update the time of last save
        }
    }

    fclose(file);
}

int main() {
    scan_ips();
    return 0;
}
