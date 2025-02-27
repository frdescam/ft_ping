#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "ping.h"

#define IS_IP 0
#define IS_FQDN 1

typedef struct s_global_data
{
    char* fqdn;
    char target_ip_str[INET_ADDRSTRLEN];
    int socket_fd;
    int input_type;
    struct sockaddr* addr;
} t_global_data;

int stop_loop;

struct sockaddr*
getAddrFromIP (char *ip)
{
    struct sockaddr_in* addr;

    addr = malloc(sizeof(struct sockaddr_in));
    bzero(addr, sizeof(struct sockaddr_in));
    addr->sin_family = AF_INET;
    addr->sin_port = 0;

    if (inet_pton(AF_INET, ip, &addr->sin_addr) == 1)
        return ((struct sockaddr *)addr);
    free(addr);
    return (NULL);
}

struct sockaddr*
getAddrFromFQDN(char *fqdn)
{
    struct addrinfo* addr_list;
    struct sockaddr_in* output_addr;
    struct addrinfo hints;
    int status;

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    if ((status = getaddrinfo(fqdn, NULL, &hints, &addr_list)) != 0)
        return (NULL);

    output_addr = malloc(sizeof(struct sockaddr_in));
    memcpy(output_addr, addr_list->ai_addr, addr_list->ai_addrlen);
    freeaddrinfo(addr_list);

    return ((struct sockaddr *)output_addr);
}

void
print_ping_reply_data (t_ping_reply_data data, int input_type)
{
    char src_address_str[INET_ADDRSTRLEN];

    (void)input_type;
    inet_ntop(AF_INET, &data.srcAddress, src_address_str, sizeof(src_address_str));
    printf("%ld bytes from %s: icmp_seq=%d ttl=%d time=%f ms\n", data.reply_size, src_address_str, data.seq_num, data.ttl, 0.0);
}

void
sigint_handler (__attribute__((unused)) int signal)
{
    stop_loop = 1;
}

int
main (int argc, char **argv)
{   
    t_global_data global_data;
    int enable;
    int seq_num;
    t_ping_request_data ping_request_data;
    t_ping_reply_data ping_reply_data;

    if (argc != 2)
        return (-1);

    global_data.input_type = IS_IP;
    global_data.addr = getAddrFromIP(argv[1]);

    if (!global_data.addr)
    {
        global_data.input_type = IS_FQDN;
        global_data.addr = getAddrFromFQDN(argv[1]);
        if (!global_data.addr)
            return (-1);
    }
    inet_ntop(AF_INET, &((struct sockaddr_in*)global_data.addr)->sin_addr, global_data.target_ip_str, sizeof(global_data.target_ip_str));
    global_data.fqdn = argv[1];

    if ((global_data.socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP)) == -1)
    {
        printf("error socket\n");
        exit(1);
    }

    enable = 1;
    setsockopt(global_data.socket_fd, IPPROTO_IP, IP_RECVTTL, &enable, sizeof(enable));

    printf("PING %s (%s): %d data bytes\n", global_data.fqdn, global_data.target_ip_str, 56);

    signal(SIGINT, sigint_handler);
    stop_loop = 0;
    seq_num = 0;
    while (!stop_loop)
    {
        ping_request_data = send_ping(global_data.socket_fd, global_data.addr, seq_num);
        (void)ping_request_data;
        // TODO : add to stats

        ping_reply_data = recieve_ping_reply(global_data.socket_fd);
        print_ping_reply_data(ping_reply_data, global_data.input_type);

        seq_num++;
        sleep(1);
    }

    printf("--- %s ping statistics ---\n", "host or ip");
    // TODO : print stats

    free(global_data.addr);
    close(global_data.socket_fd);
}