#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <libsocketcan.h>

#include <csp/csp.h>
#include <csp/drivers/can_socketcan.h>

#include "message.h"

#define SERVER_ADDR 1  //* Server Address
#define SERVER_PORT 10 //* Server Port
#define CLIENT_ADDR 2  //* Address address

bool done = false;

// TODO : gcc sender.c -lcsp -lpthread -o sd

void *do_route(void *arg)
{
    while (!done)
    {
        csp_route_work(); //* Route packet from the incoming router queue
    }
}

message input_message()
{
    message msg;

    while (1)
    {
        printf("Enter type id: ");
        if (scanf("%u", &msg.type) != 1)
        {
            fprintf(stderr, "Error reading module_id\n");
        }

        if (msg.type == 0 || msg.type == 2)
        {
            break;
        }
        printf("-------- Can't send your TCC type --------\n");
    }

    printf("Enter module_id: ");
    if (scanf("%u", &msg.module_id) != 1)
    {
        fprintf(stderr, "Error reading module_id\n");
    }

    printf("Enter request_id: ");
    if (scanf("%u", &msg.request_id) != 1)
    {
        fprintf(stderr, "Error reading request_id\n");
    }

    return msg;
}

void receive_message_from_receiver()
{
    printf("Waiting Message return !!!!!\n");
    csp_iface_t *iface;      //* For set interface
    csp_socket_t sock = {0}; //* Create socket with no specific socket options, e.g. accepts CRC32, HMAC, etc. if enabled during compilation
    message msg;

    int ret;
    int bitrate = 500000;

    csp_bind(&sock, CSP_ANY);       //* Bind socket to all ports, e.g. all incoming connections will be handled here
    csp_listen(&sock, SERVER_PORT); //* Create a backlog of 10 connections, i.e. up to 10 new connections can be queued

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
        printf("Value : %u\n", msg.val_high);
        printf("----------------------------\n");
        break;

    default: //* Destination doesn't match with server port
        printf("wrong port\n");
        exit(1);
        break;
    }

    csp_buffer_free(req); //* Clear data in req
    csp_close(conn);      //* Close an open connection
}

void *send_message(void *arg) //* Thread fucntion
{

    csp_conn_t *conn;     //* For store connection structure
    csp_packet_t *packet; //* Packet for store data to send to server
    csp_packet_t *reply;  //* Packet for store reply from server

    int ret;          //* For check error
    int data_to_send; //* For store data to send

    csp_conn_print_table(); //* Print connection table
    csp_iflist_print();

    while (1)
    {
        int num;
        message msg = input_message();

        data_to_send = 20000;       //! Set data to send !//
        packet = csp_buffer_get(0); //* Get free buffer from task context

        //*(int *)(packet->data) = data_to_send; //* Store value of data_to_send in packet to send to Server
        *(message *)(packet->data) = msg; //* Store value of data_to_send in packet to send to Server

        packet->length = 4; //* Set data length (Bytes) MAX : 251

        conn = csp_connect(CSP_PRIO_NORM, SERVER_ADDR, SERVER_PORT, 0, CSP_O_NONE); //* Connect to server

        csp_send(conn, packet); //* Send packet to server

        printf("------------------------------------\n");
        receive_message_from_receiver();
        csp_close(conn); //* Close an open connection
    }
}

int main(int argc, char *argv[])
{
    int ret;
    pthread_t router_thread, sender_thread;
    int bitrate = 500000; //* Bitrate config
    csp_iface_t *iface;   //* For set interface

    csp_init(); //* Start CSP

    ret = csp_can_socketcan_open_and_add_interface("can0", CSP_IF_CAN_DEFAULT_NAME, CLIENT_ADDR, bitrate, false, &iface);
    if (ret != CSP_ERR_NONE)
    {
        csp_print("open failed\n");
        exit(1);
    }

    iface->addr = CLIENT_ADDR; //* Set client's address
    iface->is_default = 1;

    ret = pthread_create(&router_thread, NULL, do_route, NULL); //* Start route thread
    if (ret != 0)
    {
        printf("pthread failed\n");
        exit(1);
    }

    ret = pthread_create(&sender_thread, NULL, send_message, NULL); //* Start route thread
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