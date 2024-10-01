

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <fcntl.h>    // For O_* constants
#include <sys/stat.h> // For mode constants
#include <pthread.h>

#include <libsocketcan.h>
#include <csp/csp.h>
#include <csp/drivers/can_socketcan.h>

#include "message.h"

#define SERVER_ADDR 1  //* Server Address
#define SERVER_PORT 10 //* Server Port
#define CLIENT_ADDR 2  //* Address address

bool done = false;

#define QUEUE_NAME "/message_dispatcher"
#define QUEUE_TM "/TM"
#define QUEUE_TC "/TC"
#define QUEUE_PERMISSIONS 0660
#define MAX_SIZE sizeof(message)

// TODO : gcc message_dispatcher.c -lcsp -lpthread -o md

void *do_route(void *arg)
{
    while (!done)
    {
        csp_route_work(); //* Route packet from the incoming router queue
    }
}

message SendtoTM(message msg)
{
    printf("Send TM to Module ID : %d Request ID : %d\n", msg.module_id, msg.request_id);

    mqd_t mq_wr, mq_rd;
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(message);
    attr.mq_curmsgs = 0;

    mq_rd = mq_open(QUEUE_TM, O_CREAT | O_RDONLY, QUEUE_PERMISSIONS, &attr);
    mq_wr = mq_open(QUEUE_TM, O_CREAT | O_WRONLY, QUEUE_PERMISSIONS, &attr); //! Config Queue to send

    // Send the message
    if (mq_send(mq_wr, (const char *)&msg, sizeof(message), 0) == -1)
    {
        perror("mq_send");
    }

    // Close the queue
    if (mq_close(mq_wr) == -1)
    {
        perror("mq_close");
    }

    printf("------Receive from TM------\n");
    ssize_t bytes_read = mq_receive(mq_rd, (char *)&msg, MAX_SIZE, NULL);
    if (bytes_read == -1)
    {
        perror("mq_receive");
        exit(1);
    }

    //! Print the received message
    printf("Received type ID: %u\n", msg.type);
    printf("Received module ID: %u\n", msg.module_id);
    printf("Received request ID: %u\n", msg.request_id);

    if (msg.request_id == 0)
    {
        printf("Ram total : %u\n", msg.val_high);
    }
    else if (msg.request_id == 1)
    {
        printf("Ram free : %u\n", msg.val_high);
    }
    else if (msg.request_id == 2)
    {
        printf("Ram usage : %u\n", msg.val_high);
    }
    else if (msg.request_id == 3)
    {
        printf("CPU Temperature : %u\n", msg.val_high);
    }
    else if (msg.request_id == 4)
    {
        printf("CPU Usage : %u\n", msg.val_high);
    }
    else
    {
        printf("Not have request ID\n");
    }

    return msg;
}

message SendtoTC(message msg)
{
    printf("Send TC to Module ID : %d Request ID : %d\n", msg.module_id, msg.request_id);

    mqd_t mq_wr, mq_rd;
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(message);
    attr.mq_curmsgs = 0;

    mq_rd = mq_open(QUEUE_TC, O_CREAT | O_RDONLY, QUEUE_PERMISSIONS, &attr);
    mq_wr = mq_open(QUEUE_TC, O_CREAT | O_WRONLY, QUEUE_PERMISSIONS, &attr); //! Config Queue to send

    // Send the message
    if (mq_send(mq_wr, (const char *)&msg, sizeof(message), 0) == -1)
    {
        perror("mq_send");
    }

    // Close the queue
    if (mq_close(mq_wr) == -1)
    {
        perror("mq_close");
    }

    printf("------Receive from TC------\n");
    ssize_t bytes_read = mq_receive(mq_rd, (char *)&msg, MAX_SIZE, NULL);
    if (bytes_read == -1)
    {
        perror("mq_receive");
        exit(1);
    }

    //! Print the received message
    printf("Received type ID: %u\n", msg.type);
    printf("Received module ID: %u\n", msg.module_id);
    printf("Received request ID: %u\n", msg.request_id);

    if (msg.request_id == 0)
    {
        printf("Ram total : %u\n", msg.val_high);
    }
    else if (msg.request_id == 1)
    {
        printf("Ram free : %u\n", msg.val_high);
    }
    else if (msg.request_id == 2)
    {
        printf("Ram usage : %u\n", msg.val_high);
    }
    else if (msg.request_id == 3)
    {
        printf("CPU Temperature : %u\n", msg.val_high);
    }
    else if (msg.request_id == 4)
    {
        printf("CPU Usage : %u\n", msg.val_high);
    }
    else
    {
        printf("Not have request ID\n");
    }

    return msg;
}

message send_tcc_function(message msg)
{
    if (msg.type == 0)
    {
        printf("--------Send to TM function--------\n");
        msg = SendtoTM(msg);
    }

    else if (msg.type == 2)
    {
        printf("--------Send to TC function--------\n");
        msg = SendtoTC(msg);
    }

    return msg;
}

void send_message_to_sender(message msg)
{
    csp_conn_t *conn;     //* For store connection structure
    csp_packet_t *packet; //* Packet for store data to send to server
    csp_packet_t *reply;  //* Packet for store reply from server

    int ret;          //* For check error
    int data_to_send; //* For store data to send

    //? Send Here
    int num;

    data_to_send = 20000;       //! Set data to send !//
    packet = csp_buffer_get(0); //* Get free buffer from task context

    //*(int *)(packet->data) = data_to_send; //* Store value of data_to_send in packet to send to Server
    *(message *)(packet->data) = msg; //* Store value of data_to_send in packet to send to Server

    packet->length = 16; //* Set data length (Bytes) MAX : 251

    conn = csp_connect(CSP_PRIO_NORM, CLIENT_ADDR, SERVER_PORT, 0, CSP_O_NONE); //* Connect to server

    csp_send(conn, packet); //* Send packet to server
    printf("Send message!!!\n");
    printf("-----------------\n");
    csp_close(conn); //* Close an open connection
}

void *receive_message(void *arg)
{
    csp_iface_t *iface;      //* For set interface
    csp_socket_t sock = {0}; //* Create socket with no specific socket options, e.g. accepts CRC32, HMAC, etc. if enabled during compilation
    message msg;

    int ret;
    int bitrate = 500000;

    ret = csp_can_socketcan_open_and_add_interface("can0", CSP_IF_CAN_DEFAULT_NAME, SERVER_ADDR, bitrate, false, &iface);

    if (ret != CSP_ERR_NONE)
    {
        printf("open failed\n");
        exit(1);
    }
    iface->addr = SERVER_ADDR; //* Set Server's address
    iface->is_default = 1;

    csp_conn_print_table(); //* Print connection table
    csp_iflist_print();
    csp_bind(&sock, CSP_ANY);       //* Bind socket to all ports, e.g. all incoming connections will be handled here
    csp_listen(&sock, SERVER_PORT); //* Create a backlog of 10 connections, i.e. up to 10 new connections can be queued

    while (!done) //* While loop for waiting client to connect
    {
        csp_packet_t *req; //* Create packet for store data to send from client
        csp_conn_t *conn;  //* For store connection structure

        conn = csp_accept(&sock, CSP_MAX_TIMEOUT); //! Accept/Waiting new connection
        req = csp_read(conn, CSP_MAX_TIMEOUT);     //* Read packet from connection

        switch (csp_conn_dport(conn)) //* Check destination port of connection
        {
        case 10: //* Destination match with server port

            memcpy(&msg, req->data, sizeof(message));
            printf("Type ID : %u\n", msg.type);
            printf("Module ID : %u\n", msg.module_id);
            printf("Request ID : %u\n", msg.request_id);
            printf("----------------------------\n");
            break;

        default: //* Destination doesn't match with server port
            printf("wrong port\n");
            exit(1);
            break;
        }

        //! Send to TCC Function
        msg = send_tcc_function(msg);

        //! Send function
        send_message_to_sender(msg);

        csp_buffer_free(req); //* Clear data in req
        csp_close(conn);      //* Close an open connection
    }
}

int main(int argc, char *argv[])
{

    pthread_t router_thread, receive_thread; //* For use Pthread

    int ret;

    csp_init(); //* Start CSP

    //! If error to start CAN will exit this code !//

    ret = pthread_create(&router_thread, NULL, do_route, NULL); //* Start route thread
    if (ret != 0)
    {
        printf("pthread failed\n");
        exit(1);
    }

    ret = pthread_create(&receive_thread, NULL, receive_message, NULL); //* Start route thread
    if (ret != 0)
    {
        printf("pthread failed\n");
        exit(1);
    }

    while (1)
    {
    }

    return 0;
}