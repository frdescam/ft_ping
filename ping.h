#ifndef PING_H
# define PING_H

typedef struct s_ping_reply_data
{
    int val;
}   t_ping_reply_data;

typedef struct s_ping_request_data
{
    int data_size;
}   t_ping_request_data;

t_ping_request_data send_ping(int socket_fd, struct sockaddr *addr);
t_ping_reply_data recieve_ping_reply(int fd);

#endif