#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>
#include <mqueue.h>

typedef struct
{

    //* type 0 = TM_Request
    //* type 1 = TM_Return
    //* type 2 = TC_Request
    //* type 3 = TC_Return
    u_int8_t type;
    u_int8_t module_id;
    u_int8_t request_id;
    u_int8_t param;
    u_int8_t val;
    u_int16_t val_high;

} __attribute__((packed)) message;

// static struct mq_attr attr = {
//     .mq_flags = 0,
//     .mq_maxmsg = 10,
//     .mq_msgsize = sizeof(message),
//     .mq_curmsgs = 0,
// };

// int a = 1;
// extern struct mq_attr attr;

#endif
