#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <fcntl.h>    // For O_* constants
#include <sys/stat.h> // For mode constants
#include <time.h>
#include <dirent.h> // Functions to read directory entries.
#include <unistd.h>

#include "message.h"

#define QUEUE_TC "/TC"
#define QUEUE_PERMISSIONS 0660
#define MAX_SIZE sizeof(message)

// TODO gcc tc.c -lrt -o tc

message cleardata(message msg, const char *dir_path)
{

    struct stat st;               // Structure to store file status information
    struct dirent *entry;         // Structure to store directory entry information
    DIR *dir = opendir(dir_path); // Open the directory specified by dir_path
    int ret = 0;                  // Variable to store the return status

    if (!dir)
    {
        perror("opendir"); // Print error if directory cannot be opened
    }

    while ((entry = readdir(dir)) != NULL)
    {
        // printf("Name : %s\n", entry->d_name);
        // printf("Hello\n");                                              // Read entries in the directory
        char path[1024];                                                // Buffer to store the full path
        snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name); // Construct the full path
        // printf("Path now : %s\n", path);
        if (stat(path, &st) == -1)
        {                   // Get file status information
            perror("stat"); // Print error if stat fails
            ret = -1;
            continue;
        }

        // Skip the current directory (.) and parent directory (..) entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        if (S_ISDIR(st.st_mode))
        { // Check if the entry is a directory
            // Recursively delete contents of subdirectories
            // printf("Delete DIR\n");
            cleardata(msg, path);

            // Remove the empty directory
            if (rmdir(path) == -1)
            {
                perror("rmdir"); // Print error if directory cannot be removed
                ret = -1;
            }
        }
        else
        {
            // Remove files
            // printf("Delete FIle\n");
            if (unlink(path) == -1)
            {
                perror("unlink"); // Print error if file cannot be removed
                ret = -1;
            }
        }
    }

    closedir(dir); // Close the directory

    if (ret == 0)
    {
        printf("All contents of the directory '%s' have been cleared.\n", dir_path);
        msg.type = 3;
        msg.val_high = 1;
        return msg;
    }

    else
    {
        printf("Failed to clear all contents of the directory '%s'.\n", dir_path);
        msg.request_id = 3;
        msg.val = 2;
        return msg;
    }
}

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
    mq_rd = mq_open(QUEUE_TC, O_CREAT | O_RDONLY, QUEUE_PERMISSIONS, &attr);
    mq_wr = mq_open(QUEUE_TC, O_CREAT | O_WRONLY, QUEUE_PERMISSIONS, &attr);

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
        printf("\tTC Function\n");
        printf("Received type ID: %d\n", msg.type);
        printf("Received module ID: %u\n", msg.module_id);
        printf("Received request ID: %u\n", msg.request_id);

        printf("---------Send Back---------\n");

        //! Send back function
        if (msg.request_id == 0)
        {
            printf("Request ID : 0\n");
            const char *dir_path = "/home/testforcsp/csp_messagedispatch_test/test"; // Hardcoded path to the directory
            msg = cleardata(msg, dir_path);
        }
        else if (msg.request_id == 1)
        {
            printf("Request ID : 0\n");
        }
        else if (msg.request_id == 2)
        {
            printf("Request ID : 0\n");
        }
        else if (msg.request_id == 3)
        {
            printf("Request ID : 0\n");
        }
        else if (msg.request_id == 4)
        {
            printf("Request ID : 0\n");
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
    if (mq_unlink(QUEUE_TC) == -1)
    {
        perror("mq_unlink");
        exit(1);
    }

    return 0;
}
