#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "../pbuf.h"

void memdump(void *mem, size_t i)
{
    int j, max;
    int *memi = (int *)mem;
    printf("start memory dump %p *****\n", mem);
    max = i / 16 + (i % 16 ? 1 : 0);
    for (j = 0; j < max; j++) {
        printf("%p : %08x %08x %08x %08x\n",
                memi, htonl(*(memi)), htonl(*(memi+1)), htonl(*(memi+2)), htonl(*(memi+3)));
        memi += 4;
    }
    printf("end memory dump *****\n");
    return;
}

int open_divert(uint16_t port)
{
    struct sockaddr_in bind_port;
    int fd;

    fd = socket(PF_INET, SOCK_RAW, IPPROTO_DIVERT);
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    bzero(&bind_port, sizeof(bind_port));

    bind_port.sin_family = PF_INET;
    bind_port.sin_port   = htons(port);

    if (bind(fd, (struct sockaddr*)&bind_port, sizeof(bind_port)) < 0) {
        close(fd);
        perror("bind");
        return -1;
    }

    return fd;
}

void read_loop(int fd)
{
    struct sockaddr_in  sin;
    struct ip          *ip;
    struct tcphdr* tcp;
    socklen_t sin_len;
    ssize_t   size;
    uint8_t   buf[1024 * 100];

    sin_len = sizeof(sin);
    for(;;) {
        id pool = [NSAutoreleasePool new];
        size = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr*)&sin, &sin_len);
        if (size < 0) {
            perror("recvfrom");
            exit(-1);
        }

        id pbuf = [[[PacketBuffer alloc] init] autorelease];
        [pbuf withBytes:buf :size];
        //printf("if addr: %s\n", inet_ntoa(sin.sin_addr));

        if (sin.sin_addr.s_addr != INADDR_ANY) {
            printf("recv\n");
            memdump(&sin, sizeof(sin));
            printf("len:%d\n",sin.sin_len);       //16
            printf("family:%d\n",sin.sin_family); //AF_INET
            printf("port:%d\n", sin.sin_port); // ipfw rule number
            printf("s_addr:%s\n", inet_ntoa(sin.sin_addr));
            printf("if_name:%s\n", sin.sin_zero);


            /*
            uint8_t  sin_len;
            uint8_t  sin_family;
            uint16_t sin_port;
            struct  in_addr {
                in_addr_t s_addr;
            } sin_addr;
            char    sin_zero[8];
            printf("\n");
            */

            //sin.sin_addr.s_addr=inet_addr("127.0.0.1");
            //sin.sin_addr.s_addr=inet_addr("0.0.0.0");
            memset(&sin, 0, sizeof(sin));

            ip = (struct ip*)buf;
            //ip->ip_src.s_addr=inet_addr("");
            ip->ip_dst.s_addr=inet_addr("127.0.0.1");
            //ip->ip_sum = 0x0000;

            tcp = (struct tcphdr*)(buf + 20);
            //tcp->th_sum = 0x0000;

            //uint8_t* tcp_op = (bufptr + 20 + 20);
        } else {
            //send
            printf("send\n");
            memdump(&sin, sizeof(sin));
            memset(&sin, 0, sizeof(sin));
            //sin.sin_addr.s_addr=inet_addr("150.65.206.93");


            ip = (struct ip*)buf;
            tcp = (struct tcphdr*)(buf + 20);
            ip->ip_src.s_addr=inet_addr("150.65.206.93");
            ip->ip_dst.s_addr=inet_addr("150.65.32.64");
        }

        [pbuf sync];
        /*
        printf("recv %lu[bytes]\n", size);
        printf("src: %s\n", inet_ntoa(ip->ip_src));
        printf("dst: %s\n", inet_ntoa(ip->ip_dst));
        printf("protocol: %d\n\n", ip->ip_p);
        */

        sendto(fd, buf, size, 0, (struct sockaddr*)&sin, sin_len);
        //sendto(fd, [pbuf bytes], [pbuf length], 0, (struct sockaddr*)&sin, sin_len);

        [pool drain];
    }
}


int main(int argc, char *argv[])
{
    int fd;

    if (argc < 2) {
        printf("usage: ./a.out port\n");
        return -1;
    }

    fd = open_divert(atoi(argv[1]));
    if (fd < 0)
        return -1;

    read_loop(fd);

    return 0;
}

