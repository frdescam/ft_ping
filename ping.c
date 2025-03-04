#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "ping.h"

#define DATA_SIZE 40
#define PACKET_SIZE 1500

typedef struct s_icmp_packet
{
    // Header 8 bytes
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t identifier;
    uint16_t seq_num;
    // Payload 56 bytes
    //  Timestamp 8 bytes + 8 bytes padding ?
    uint32_t timestamp_sec;
    uint32_t padding1;
    uint32_t timestamp_usec;
    uint32_t padding2;
    //  Actual payload 40 bytes
    uint8_t payload[DATA_SIZE];
} t_icmp_packet;

t_ping_request_data
send_ping (int socket_fd, struct sockaddr* addr, int icmp_seq)
{
    t_icmp_packet icmp_packet;
    t_ping_request_data output = {0};
    struct timeval tv_send;
    int i;

    bzero(&icmp_packet, sizeof(icmp_packet));
    icmp_packet.type = 8;
    icmp_packet.seq_num = htobe16(icmp_seq);

    i = 0;
    while (i < DATA_SIZE)
    {
        icmp_packet.payload[i] = i;
        i++;
    }

    gettimeofday(&tv_send, NULL);

    icmp_packet.timestamp_sec = (uint32_t)tv_send.tv_sec;
    icmp_packet.timestamp_usec = (uint32_t)tv_send.tv_usec;

    if(sendto(socket_fd, &icmp_packet, sizeof(icmp_packet), 0, addr, sizeof(struct sockaddr_in)) == -1)
    {
        printf("error send %d: %s\n", errno, strerror(errno));
    }

    return (output);
}

double
compute_round_trip_time (struct timeval* tv_send, struct timeval* tv_recv)
{
    struct timeval round_trip_time;
    double output;

    timersub(tv_send, tv_recv, &round_trip_time);

    output = (double)round_trip_time.tv_sec * 1000 + (double)round_trip_time.tv_usec / 1000;

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
    ssize_t reply_size;
    struct timeval tv_recv;
    struct timeval tv_send;

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

    reply_size = recvmsg(socket_fd, &header, 0);
    gettimeofday(&tv_recv, NULL);
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
    output.srcAddress = *(struct sockaddr *)(&((struct sockaddr_in*)header.msg_name)->sin_addr);
    output.seq_num = be16toh(*(uint16_t*)&buffer[6]);

    tv_send.tv_sec = *(uint32_t*)(&buffer[8]);
    tv_send.tv_usec = *(uint32_t*)(&buffer[16]);
    output.round_trip_time = compute_round_trip_time(&tv_recv, &tv_send);

    output.reply_size = reply_size;

    return (output);
}