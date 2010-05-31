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

int main(int argc, char** argv)
{
    id pool = [NSAutoreleasePool new];

    struct ip* ip;
    struct tcphdr* tcp;
    uint16_t ip_csum =0xf98d;
    uint16_t tcp_csum=0xa18c;
    uint8_t buf[]={0x45,0x00,0x00,0x40,0x26,0x0a,0x40,0x00,0x40,0x06,0x00,0x00,0x96,0x41,0xce,0x5d,
                   0x96,0x41,0x20,0x40,
                   0xfa,0x58,0x1b,0x39,0x89,0x18,0x70,0x98,0x00,0x00,0x00,0x00,0xb0,0x02,0xff,0xff,
                   0x00,0x00,0x00,0x00,
                   0x02,0x04,0x05,0xb4,0x01,0x03,0x03,0x03,0x01,0x01,0x08,0x0a,0x03,0x1d,0x67,0xf2,
                   0x00,0x00,0x00,0x00,0x04,0x02,0x00,0x00};

    printf("buf:%lu\n", sizeof(buf));
    printf("ip_sum:%x\n", ip_csum);
    printf("tcp_sum:%x\n", tcp_csum);

    id pbuf = [[[PacketBuffer alloc] init] autorelease];
    [pbuf withBytes:buf :sizeof(buf)];
    [pbuf sync];
    ip = (struct ip*)[pbuf getL3];
    tcp = (struct tcphdr*)[pbuf getL4];
    printf("pbuf_ip_sum:%x\n", htons(ip->ip_sum));
    printf("pbuf_tcp_sum:%x\n", htons(tcp->th_sum));


    [pool drain];
}
