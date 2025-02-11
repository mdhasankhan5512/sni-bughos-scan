#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <string.h>

#define PROGRESS_FILE "progress.txt"
#define OUTPUT_FILE "ipv4.txt"
#define BUFFER_SIZE 100000  // Adjusted to prevent overflow

bool is_private_ip(uint32_t ip) {
    uint8_t octet1 = (ip >> 24) & 0xFF;
    uint8_t octet2 = (ip >> 16) & 0xFF;

    return (octet1 == 10) ||
           (octet1 == 172 && octet2 >= 16 && octet2 <= 31) ||
           (octet1 == 192 && octet2 == 168);
}

uint32_t load_progress() {
    FILE *file = fopen(PROGRESS_FILE, "r");
    uint32_t last_ip = 0;
    if (file) {
        if (fscanf(file, "%u", &last_ip) != 1) {
            last_ip = 0; // If reading fails, start from 0
        }
        fclose(file);
    } else {
        printf("Progress file is unavailable. Starting from 0.0.0.0\n");
    }
    return last_ip;
}

void save_progress(uint32_t ip) {
    FILE *file = fopen(PROGRESS_FILE, "w");
    if (file) {
        fprintf(file, "%u", ip);
        fclose(file);
    }
}

void print_progress(uint32_t current_ip) {
    static const uint32_t total_ips = 0xFFFFFFFF; // Total number of IPv4 addresses
    double percentage = ((double)current_ip / total_ips) * 100.0;
    printf("Progress: %.2f%%\n", percentage);
}

int main() {
    uint32_t start_ip = load_progress();
    FILE *output = fopen(OUTPUT_FILE, "a");
    if (!output) {
        perror("Error opening output file");
        return 1;
    }

    char buffer[BUFFER_SIZE]; // Smaller buffer to avoid stack overflow
    size_t buf_index = 0;
    char ip_str[INET_ADDRSTRLEN];

    for (uint32_t ip = start_ip; ip <= 0xFFFFFFFF; ip++) {
        if (is_private_ip(ip)) continue;

        struct in_addr addr;
        addr.s_addr = htonl(ip);
        if (inet_ntop(AF_INET, &addr, ip_str, INET_ADDRSTRLEN) == NULL) {
            perror("inet_ntop failed");
            continue;
        }

        int written = snprintf(buffer + buf_index, sizeof(buffer) - buf_index, "%s\n", ip_str);
        if (written < 0 || buf_index + written >= sizeof(buffer)) {
            fwrite(buffer, 1, buf_index, output);
            fflush(output);
            buf_index = 0;
            written = snprintf(buffer, sizeof(buffer), "%s\n", ip_str); // Retry writing the current IP
        }
        buf_index += written;

        if (ip % 1000000 == 0) {
            save_progress(ip);
            print_progress(ip); // Print progress every 1,000,000 IPs
        }

        // Handle overflow: Terminate loop when ip reaches 0xFFFFFFFF
        if (ip == 0xFFFFFFFF) break;
    }

    if (buf_index > 0) {
        fwrite(buffer, 1, buf_index, output);
        fflush(output);
    }

    fclose(output);
    remove(PROGRESS_FILE);
    return 0;
}
