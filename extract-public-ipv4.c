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
#define SAVE_INTERVAL 300 // 5 minutes

// Function to check if an IP is public
int is_public_ip(const char *ip) {
    struct in_addr addr;
    inet_aton(ip, &addr);
    unsigned long ip_num = ntohl(addr.s_addr);

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

// Function to save progress (last scanned IP)
void save_progress(const char *last_ip) {
    FILE *file = fopen(PROGRESS_FILE, "w");
    if (file) {
        fprintf(file, "%s\n", last_ip);  // Save IP as string
        fclose(file);
    }
}

// Function to load the progress (last scanned IP)
int load_progress(char *last_ip) {
    FILE *file = fopen(PROGRESS_FILE, "r");
    if (file) {
        fscanf(file, "%s", last_ip);  // Read IP from file
        fclose(file);
        return 1;  // Progress file exists and is read
    }
    return 0;  // No progress file found
}

// Function to scan public IPs
void scan_ips() {
    FILE *file = fopen(OUTPUT_FILE, "a");  // Open in append mode
    if (!file) {
        perror("Failed to open output file");
        exit(EXIT_FAILURE);
    }

    char last_ip[16] = "0.0.0.0";  // Default to start from 0.0.0.0
    if (!load_progress(last_ip)) {  // If no progress file, start from 0.0.0.0
        printf("No progress file found. Starting from 0.0.0.0.\n");
    } else {
        printf("Resuming from last scanned IP: %s\n", last_ip);
    }

    time_t last_save_time = time(NULL);

    struct in_addr addr;
    unsigned long i = inet_addr(last_ip);  // Convert last IP to integer

    for (; i <= 0xFFFFFFFF; i++) { // 1.0.0.0 to 255.255.255.255
        addr.s_addr = htonl(i);
        char ip[16];
        strcpy(ip, inet_ntoa(addr));

        if (is_public_ip(ip) && ping_ip(ip)) {
            printf("%s is online\n", ip);
            fprintf(file, "%s\n", ip);
            fflush(file);
        }

        // Save progress every SAVE_INTERVAL seconds
        if (time(NULL) - last_save_time >= SAVE_INTERVAL) {
            save_progress(ip);  // Save progress
            printf("Progress saved: Last scanned IP: %s\n", ip);
            last_save_time = time(NULL);  // Update the time of last save
        }
    }

    fclose(file);
}

int main() {
    scan_ips();
    return 0;
}
