#ifndef PING_H
# define PING_H

#include <sys/socket.h>

typedef struct s_ping_reply_data
{
    int ttl;
    struct sockaddr srcAddress;
    uint16_t seq_num;
    ssize_t reply_size;
}   t_ping_reply_data;

typedef struct s_ping_request_data
{
    int data_size;
}   t_ping_request_data;

t_ping_request_data
send_ping (int socket_fd, struct sockaddr* addr, int icmp_seq);

t_ping_reply_data
recieve_ping_reply (int fd);

#endif