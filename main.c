#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <locale.h>
#include <float.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "ping.h"

#define ICMP_SENT_PACKET_SIZE 56

#define IS_IP 0
#define IS_FQDN 1

typedef struct s_packets_list
{
    t_ping_request_data *request_data;
    t_ping_reply_data *reply_data;
    struct s_packets_list *next;
} t_packets_list;

typedef struct s_ping_data
{
    char *fqdn;
    char target_ip_str[INET_ADDRSTRLEN];
    int socket_fd;
    int input_type;
    struct sockaddr *addr;
    t_packets_list *packets_list;
} t_ping_data;

int stop_loop;

void clean_up(t_ping_data *ping_data)
{
    t_packets_list *packet;

    packet = ping_data->packets_list;
    while (packet)
    {
        free(packet->reply_data);
        free(packet->request_data);
        packet = packet->next;
    }
    free(ping_data->addr);
    close(ping_data->socket_fd);
}

int ft_sqrt(int nb)
{
    int i;

    i = 0;
    if (nb <= 0)
        return (0);
    while (i * i < nb && i <= 46340)
        i++;
    if (i * i == nb)
        return (i);
    else
        return (0);
}

double sqrt(double x)
{
    double j;
    int i;
    int k;

    i = 0;
    while (i * i < x && i <= 46340)
        i++;

    j = (double)i;
    k = 0;
    while (k < 10)
    {
        j = (x / j + j) / 2;
        if (j * j == x)
            return (j);
        k++;
    }
    return (j);
}

void print_ping_stats(t_ping_data *ping_data)
{
    t_packets_list *packets_list;
    uint32_t nb_packet_requests;
    uint32_t nb_packet_replies;
    uint32_t percent_packets_lost;
    uint32_t count;
    char *host;
    double round_trip_min;
    double round_trip_max;
    double round_trip_average;
    double rount_trip_old_average;
    double m2;
    double round_trip_variance;
    double round_trip_std_deviation;
    double round_trip_time;

    nb_packet_requests = 0;
    nb_packet_replies = 0;
    round_trip_average = 0.0;
    round_trip_variance = 0.0;
    round_trip_std_deviation = 0.0;
    m2 = 0.0;
    round_trip_min = DBL_MAX;
    round_trip_max = 0.0;
    count = 1;

    if (ping_data->input_type == IS_FQDN)
        host = ping_data->fqdn;
    else
        host = ping_data->target_ip_str;
    printf("--- %s ping statistics ---\n", host);

    packets_list = ping_data->packets_list;
    while (packets_list)
    {
        if (packets_list->request_data)
            nb_packet_requests++;
        if (packets_list->reply_data)
        {
            nb_packet_replies++;
            round_trip_time = packets_list->reply_data->round_trip_time;

            if (round_trip_time > round_trip_max)
                round_trip_max = round_trip_time;
            if (round_trip_time < round_trip_min)
                round_trip_min = round_trip_time;

            rount_trip_old_average = round_trip_average;
            round_trip_average = round_trip_average + (round_trip_time - round_trip_average) / count;

            round_trip_variance = round_trip_variance + (round_trip_time - round_trip_variance) / count;

            m2 = m2 + (round_trip_time - rount_trip_old_average) * (round_trip_time - round_trip_average);
            round_trip_variance = m2 / count;
            round_trip_std_deviation = sqrt(round_trip_variance);
        }

        count++;
        packets_list = packets_list->next;
    }

    if (nb_packet_replies == 0)
        percent_packets_lost = 100;
    else
        percent_packets_lost = (1 - nb_packet_replies / (double)nb_packet_requests) * 100;

    printf("%d packets transmitted, %d packets received, %d%% packet loss\n",
           nb_packet_requests,
           nb_packet_replies,
           percent_packets_lost);

    if (nb_packet_replies != 0)
        printf("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n",
               round_trip_min,
               round_trip_average,
               round_trip_max,
               round_trip_std_deviation);
}

struct sockaddr *
getAddrFromIP(char *ip)
{
    struct sockaddr_in *addr;

    if(!(addr = malloc(sizeof(struct sockaddr_in))))
        return (NULL);
    bzero(addr, sizeof(struct sockaddr_in));
    addr->sin_family = AF_INET;
    addr->sin_port = 0;

    if (inet_pton(AF_INET, ip, &addr->sin_addr) == 1)
        return ((struct sockaddr *)addr);
    free(addr);
    return (NULL);
}

struct sockaddr *
getAddrFromFQDN(char *fqdn)
{
    struct addrinfo *addr_list;
    struct sockaddr_in *output_addr;
    struct addrinfo hints;
    int status;

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    if ((status = getaddrinfo(fqdn, NULL, &hints, &addr_list)) != 0)
        return (NULL);

    if(!(output_addr = malloc(sizeof(struct sockaddr_in))))
        return (NULL);
    memcpy(output_addr, addr_list->ai_addr, addr_list->ai_addrlen);
    freeaddrinfo(addr_list);

    return ((struct sockaddr *)output_addr);
}

void print_ping_reply_data(t_ping_reply_data *data, int input_type)
{
    char src_address_str[INET_ADDRSTRLEN];

    (void)input_type;
    inet_ntop(AF_INET, &data->srcAddress, src_address_str, sizeof(src_address_str));
    printf("%ld bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms\n", data->reply_size, src_address_str, data->seq_num, data->ttl, data->round_trip_time);
}

void sigint_handler()
{
    stop_loop = 1;
}

int main(int argc, char **argv)
{
    t_ping_data ping_data;
    int enable;
    struct timeval tv_recv_timeout;
    int seq_num;
    t_packets_list *current_packet;
    t_packets_list *prev_packet;
    t_ping_request_data *ping_request_data;
    t_ping_reply_data *ping_reply_data;

    if (argc != 2)
        return (-1);

    setlocale(LC_ALL, "");

    ping_data.input_type = IS_IP;
    if (!(ping_data.addr = getAddrFromIP(argv[1])))
    {
        clean_up(&ping_data);
        exit(-1);
    }

    if (!ping_data.addr)
    {
        ping_data.input_type = IS_FQDN;
        if (!(ping_data.addr = getAddrFromFQDN(argv[1])))
        {
            clean_up(&ping_data);
            exit(-1);
        }
    }
    inet_ntop(AF_INET, &((struct sockaddr_in *)ping_data.addr)->sin_addr, ping_data.target_ip_str, sizeof(ping_data.target_ip_str));
    ping_data.fqdn = argv[1];

    if ((ping_data.socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP)) == -1)
    {
        fprintf(stderr, "Error socket\n");
        clean_up(&ping_data);
        exit(-1);
    }

    enable = 1;
    if (setsockopt(ping_data.socket_fd, IPPROTO_IP, IP_RECVTTL, &enable, sizeof(enable)) != 0)
    {
        fprintf(stderr, "Error socket opt");
        clean_up(&ping_data);
        exit(-1);
    }

    tv_recv_timeout.tv_sec = 1;
    tv_recv_timeout.tv_usec = 0;
    if(setsockopt(ping_data.socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv_recv_timeout, sizeof(tv_recv_timeout)) != 0)
    {
        fprintf(stderr, "Error socket opt");
        clean_up(&ping_data);
        exit(-1);
    }

    printf("PING %s (%s): %d data bytes\n", ping_data.fqdn, ping_data.target_ip_str, ICMP_SENT_PACKET_SIZE);

    signal(SIGINT, sigint_handler);
    prev_packet = NULL;
    ping_data.packets_list = NULL;
    stop_loop = 0;
    seq_num = 0;
    while (!stop_loop)
    {
        if(!(current_packet = malloc(sizeof(t_packets_list))))
        {
            clean_up(&ping_data);
            exit(-1);
        }
        if (!(ping_request_data = send_ping(ping_data.socket_fd, ping_data.addr, seq_num)))
        {
            fprintf(stderr, "Error send");
            clean_up(&ping_data);
            exit(-1);
        }
        current_packet->request_data = ping_request_data;

        ping_reply_data = recieve_ping_reply(ping_data.socket_fd);
        if (ping_reply_data)
        {
            print_ping_reply_data(ping_reply_data, ping_data.input_type);
            sleep(1);
        }

        current_packet->reply_data = ping_reply_data;

        if (prev_packet)
            prev_packet->next = current_packet;
        else
            ping_data.packets_list = current_packet;

        prev_packet = current_packet;
        seq_num++;
    }

    print_ping_stats(&ping_data);

    clean_up(&ping_data);
}