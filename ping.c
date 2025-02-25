#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "ping.h"

#define PACKET_SIZE 1500

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

t_ping_request_data
send_ping (int socket_fd, struct sockaddr* addr, int icmp_seq)
{
    t_icmp_packet icmp_packet;
    t_ping_request_data output;

    bzero(&icmp_packet, sizeof(icmp_packet));
    icmp_packet.type = 8;
    icmp_packet.identifier = 42;
    icmp_packet.seq_num = htobe16(icmp_seq);

    if(sendto(socket_fd, &icmp_packet, sizeof(icmp_packet), 0, addr, sizeof(struct sockaddr_in)) == -1)
    {
        printf("error send %d: %s\n", errno, strerror(errno));
    }

    output.data_size = sizeof(icmp_packet);

    return (output);
}

t_ping_reply_data
recieve_ping_reply (int socket_fd)
{
    t_ping_reply_data output;

    struct msghdr header;
    uint8_t buffer[PACKET_SIZE];
    struct iovec iov;
    struct sockaddr srcAddress;
    uint8_t ctrlDataBuffer[CMSG_SPACE(sizeof(uint8_t))];

    struct cmsghdr* cmsg;
    int ttl;

    iov.iov_base = buffer;
    iov.iov_len = PACKET_SIZE;

    bzero(ctrlDataBuffer, sizeof(ctrlDataBuffer));

    bzero(&header, sizeof(struct msghdr));
    header.msg_name = &srcAddress;
    header.msg_namelen = sizeof(srcAddress);
    header.msg_iov = &iov;
    header.msg_iovlen = 1;
    header.msg_control = ctrlDataBuffer;
    header.msg_controllen = sizeof(ctrlDataBuffer);

    recvmsg(socket_fd, &header, 0);

    ttl = -1;
    cmsg = CMSG_FIRSTHDR(&header);
    while (cmsg)
    {
        if (cmsg->cmsg_level == IPPROTO_IP
            && cmsg->cmsg_type == IP_TTL)
        {
            memcpy(&ttl, CMSG_DATA(cmsg), sizeof(ttl));
        }
        cmsg = CMSG_NXTHDR(&header, cmsg);
    }

    output.ttl = ttl;
    memcpy(&output.srcAddress, &((struct sockaddr_in*)header.msg_name)->sin_addr, sizeof(struct in_addr));
    output.seq_num = be16toh(*(uint16_t*)&buffer[6]);

    return output;
}