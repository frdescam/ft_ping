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
    (void)input_type;
    printf("%d\n", data.val);
}

void
sigint_handler (int signal)
{
    (void)signal;
    stop_loop = 1;
}

int
main (int argc, char **argv)
{   
    int socket_fd;
    int input_type;
    struct sockaddr* addr;
    t_ping_request_data ping_request_data;
    t_ping_reply_data ping_reply_data;

    if (argc != 2)
        return (-1);

    input_type = IS_IP;
    addr = getAddrFromIP(argv[1]);

    if (!addr)
    {
        input_type = IS_FQDN;
        addr = getAddrFromFQDN(argv[1]);
        if (!addr)
            return (-1);
    }

    if ((socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP)) == -1)
    {
        printf("error socket\n");
        exit(1);
    }

    printf("PING %s (%s): %d data bytes\n", "host", "ip", 42);

    stop_loop = 0;
    while (!stop_loop)
    {
        signal(SIGINT, sigint_handler);

        ping_request_data = send_ping(socket_fd, addr);
        (void)ping_request_data;
        // TODO : add to stats

        ping_reply_data = recieve_ping_reply(socket_fd);
        print_ping_reply_data(ping_reply_data, input_type);

        sleep(1);
    }

    printf("--- %s ping statistics ---\n", "host or ip");
    // TODO : print stats

    free(addr);
    close(socket_fd);
}