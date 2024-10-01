#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <fcntl.h>    // For O_* constants
#include <sys/stat.h> // For mode constants
#include <time.h>
#include <unistd.h>

#include "message.h"

#define QUEUE_TM "/TM"
#define QUEUE_PERMISSIONS 0660
#define MAX_SIZE sizeof(message)

// TODO gcc tm.c -lrt -o tm

message get_ram_total(message msg);
message get_ram_free(message msg);
message get_ram_usage(message msg);
message get_cpu_temp(message msg);
message get_cpu_usage(message msg);

int main()
{
    mqd_t mq_rd, mq_wr;
    struct mq_attr attr;
    message msg;

    // Set the queue attributes
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_SIZE;
    attr.mq_curmsgs = 0;

    // Open the message queue
    mq_rd = mq_open(QUEUE_TM, O_CREAT | O_RDONLY, QUEUE_PERMISSIONS, &attr);
    mq_wr = mq_open(QUEUE_TM, O_CREAT | O_WRONLY, QUEUE_PERMISSIONS, &attr);

    if (mq_rd == (mqd_t)-1)
    {
        perror("mq_open");
        exit(1);
    }

    // Get the attributes of the queue
    if (mq_getattr(mq_rd, &attr) == -1)
    {
        perror("mq_getattr");
        exit(1);
    }

    printf("Receiver: Waiting for messages...\n");

    while (1)
    {
        // Receive the message
        ssize_t bytes_read = mq_receive(mq_rd, (char *)&msg, MAX_SIZE, NULL);
        if (bytes_read == -1)
        {
            perror("mq_receive");
            exit(1);
        }

        // Print the received message
        printf("\tTM Function\n");
        printf("Received type ID: %d\n", msg.type);
        printf("Received module ID: %u\n", msg.module_id);
        printf("Received request ID: %u\n", msg.request_id);

        printf("---------Send Back---------\n");

        //! Send back function
        if (msg.request_id == 0)
        {
            printf("Request ID : 0\n");
            msg = get_ram_total(msg);
        }
        else if (msg.request_id == 1)
        {
            printf("Request ID : 0\n");
            msg = get_ram_free(msg);
        }
        else if (msg.request_id == 2)
        {
            printf("Request ID : 0\n");
            msg = get_ram_usage(msg);
        }
        else if (msg.request_id == 3)
        {
            printf("Request ID : 0\n");
            msg = get_cpu_temp(msg);
        }
        else if (msg.request_id == 4)
        {
            printf("Request ID : 0\n");
            msg = get_cpu_usage(msg);
        }
        if (mq_send(mq_wr, (const char *)&msg, sizeof(message), 0) == -1)
        {
            perror("mq_send");
            return 1;
        }

        printf("------------------------------------------\n");
    }

    // Close the queue
    if (mq_close(mq_rd) == -1)
    {
        perror("mq_close");
        exit(1);
    }

    // Unlink the queue
    if (mq_unlink(QUEUE_TM) == -1)
    {
        perror("mq_unlink");
        exit(1);
    }

    return 0;
}

message get_ram_total(message msg)
{
    unsigned long mem_total = 0;
    char buffer[256];

    FILE *fp = fopen("/proc/meminfo", "r");
    if (fp == NULL)
    {
        perror("Unable to open /proc/meminfo");
    }

    while (fgets(buffer, sizeof(buffer), fp))
    {
        if (sscanf(buffer, "MemTotal: %lu kB", &mem_total) == 1)
        {
            break; // Stop reading after we've found the MemTotal line
        }
    }

    printf("Before formula : %lu\n", mem_total);
    fclose(fp);
    mem_total = mem_total / 1024; // Convert kB to MB
    printf("After formula : %lu MB\n", mem_total);

    msg.type = 1;
    msg.val_high = mem_total;
    return msg;
}

message get_ram_free(message msg)
{
    unsigned long mem_free = 0;
    char buffer[256];

    FILE *fp = fopen("/proc/meminfo", "r");
    if (fp == NULL)
    {
        perror("Unable to open /proc/meminfo");
    }

    while (fgets(buffer, sizeof(buffer), fp))
    {
        if (sscanf(buffer, "MemFree: %lu kB", &mem_free) == 1)
        {
            break; // Stop reading after we've found the MemTotal line
        }
    }

    printf("Before formula : %lu\n", mem_free);
    fclose(fp);
    mem_free = mem_free / 1024; // Convert kB to MB
    printf("After formula : %lu MB\n", mem_free);

    msg.type = 1;
    msg.val_high = mem_free;
    return msg;
}

message get_ram_usage(message msg)
{
    unsigned long mem_available = 0;
    char buffer[256];

    FILE *fp = fopen("/proc/meminfo", "r");
    if (fp == NULL)
    {
        perror("Unable to open /proc/meminfo");
    }

    while (fgets(buffer, sizeof(buffer), fp))
    {
        if (sscanf(buffer, "MemAvailable: %lu kB", &mem_available) == 1)
        {
            break; // Stop reading after we've found the MemTotal line
        }
    }

    printf("Before formula : %lu\n", mem_available);
    fclose(fp);
    mem_available = mem_available / 1024; // Convert kB to MB
    printf("After formula : %lu MB\n", mem_available);

    msg.type = 1;
    msg.val_high = mem_available;
    return msg;
}

message get_cpu_temp(message msg)
{
    unsigned long tmp;
    char buffer[256];
    int temp;

    FILE *fp = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (fp == NULL)
    {
        perror("Unable to open /sys/class/thermal/thermal_zone0/temp");
    }

    fscanf(fp, "%d", &temp);
    fclose(fp);

    tmp = temp / 1000.0;
    printf("TEMP : %u\n", tmp);

    msg.type = 1;
    msg.val_high = tmp;
    return msg;
}

message get_cpu_usage(message msg)
{
    long double a[4], b[4];
    long double cpuload;
    FILE *fp;

    fp = fopen("/proc/stat", "r");
    fscanf(fp, "%*s %Lf %Lf %Lf %Lf", &a[0], &a[1], &a[2], &a[3]);
    fclose(fp);

    sleep(1);

    fp = fopen("/proc/stat", "r");
    fscanf(fp, "%*s %Lf %Lf %Lf %Lf", &b[0], &b[1], &b[2], &b[3]);
    fclose(fp);

    long double loadavg = ((b[0] + b[1] + b[2]) - (a[0] + a[1] + a[2])) /
                          ((b[0] + b[1] + b[2] + b[3]) - (a[0] + a[1] + a[2] + a[3]));

    cpuload = loadavg * 100.0;

    printf("CPU UASAGE : %Lf\n", cpuload);

    msg.val_high = cpuload;
    return msg;
}
