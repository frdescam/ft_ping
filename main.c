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

int getAddrFromArgs(int argc, char **argv, struct addrinfo **addr)
{
    struct addrinfo hints;

    if (argc != 2)
    {
        printf("error arguments count\n");
        exit(1);
    }

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(argv[1], NULL, &hints, addr) != 0)
    {
        if (!(addr = malloc(sizeof(struct addrinfo))))
        {
            exit(1);
        }
        bzero(addr, sizeof(struct addrinfo));
        (*addr)->ai_family = AF_INET;
        if (inet_aton(argv[1], (struct in_addr *)&(*addr)->ai_addr) == 0)
        {
            printf("error: argument is not an ip nor a FQDN\n");
            exit(1);
        }
        return (IS_IP);
    }
    return (IS_FQDN);
}

void print_ping_reply_data(t_ping_reply_data data, int input_type)
{
    (void)input_type;
    printf("%d\n", data.val);
}

void sigint_handler(int signal)
{
    (void)signal;
    stop_loop = 1;
}

int main(int argc, char **argv)
{
    int input_type;
    int socket_fd;
    struct addrinfo *addr;
    t_ping_request_data ping_request_data;
    t_ping_reply_data ping_reply_data;

    input_type = getAddrFromArgs(argc, argv, &addr);

    if ((socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP)) == -1)
    {
        printf("error socket\n");
        exit(1);
    }

    // char rspace[40];

    // memset (rspace, 0, sizeof (rspace));
    // rspace[0] = 7;
    // rspace[1] = sizeof (rspace) - 1;
    // rspace[2] = 4;

    // setsockopt(socket_fd, IPPROTO_IP, 4, rspace, sizeof(rspace));


    printf("PING %s (%s): %d data bytes\n", "host", "ip", 42);

    stop_loop = 0;
    while (!stop_loop)
    {
        signal(SIGINT, sigint_handler);

        ping_request_data = send_ping(socket_fd, addr->ai_addr);
        (void)ping_request_data;
        // TODO : add to stats

        ping_reply_data = recieve_ping_reply(socket_fd);
        print_ping_reply_data(ping_reply_data, input_type);

        sleep(1);
    }

    printf("--- %s ping statistics ---\n", "host or ip");
    // TODO : print stats


    close(socket_fd);
}