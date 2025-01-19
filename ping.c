#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "ping.h"

typedef struct s_icmp_packet
{
    int8_t type;
    int8_t code;
    int16_t checksum;
    int16_t identifier;
    int16_t seq_num;
    int64_t padding1;
    int32_t padding2;
} t_icmp_packet;

t_ping_request_data send_ping(int socket_fd, struct sockaddr *addr)
{
    t_icmp_packet icmp_packet;
    t_ping_request_data output;

    bzero(&icmp_packet, sizeof(icmp_packet));
    icmp_packet.type = 8;
    icmp_packet.identifier = 42;

    if(sendto(socket_fd, &icmp_packet, sizeof(icmp_packet), 0, addr, sizeof(struct sockaddr_in)) == -1)
    {
        printf("error send %d: %s\n", errno, strerror(errno));
    }

    output.data_size = sizeof(icmp_packet);

    return (output);
}

t_ping_reply_data recieve_ping_reply(int socket_fd)
{
    t_icmp_packet response_packet;
    t_ping_reply_data output;

    bzero(&response_packet, sizeof(response_packet));

    recv(socket_fd, &response_packet, sizeof(response_packet), 0);

    output.val = response_packet.checksum;

    return output;
}