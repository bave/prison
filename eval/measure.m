#import <Cocoa/Cocoa.h>

#include <stdlib.h>
#include <resolv.h>
#include <nameser.h>

#include <time.h>
#include <sys/time.h>



#include "../name.h"

#define SA struct sockaddr

void usage(void);

int main(int argc, char *argv[])
{
    if (argc != 3) usage();

    id pool = [[NSAutoreleasePool alloc] init];
    id n = [NamePacket new];

    srand((unsigned)time(NULL));
    unsigned int ns_id = (rand()&0x0000FFFF);

    NSString* request = [NSString stringWithUTF8String:argv[1]];


    [n n_set_id:ns_id];
    [n n_set_flags:RA];
    [n n_create_rr_questionA:request];
    [n n_build_payload];

    ssize_t len;
    socklen_t sin_size = sizeof(struct sockaddr_in);

    int sockfd;
    struct sockaddr_in sin1;
    struct sockaddr_in sin2;
    unsigned char buf[BUFSIZ];

    memset(&sin1, 0, sizeof(sin1));
    memset(&sin2, 0, sizeof(sin2));
    memset(buf, 0, BUFSIZ);

    sin1.sin_family = AF_INET;
    sin1.sin_port = htons(53);
    inet_pton(AF_INET, argv[2], &sin1.sin_addr);

    /*
    struct timeval {
        time_t       tv_sec;
        suseconds_t  tv_usec;
    };
    */
    struct timeval prev;
    struct timeval current;

    gettimeofday(&prev, NULL);
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    sendto(sockfd, [n n_payload], [n n_payload_size], 0, (SA*)&sin1, sizeof(sin1));
    gettimeofday(&current, NULL);

    //printf("sec:%lu, usec:%d\n", prev.tv_sec, prev.tv_usec);
    //printf("sec:%lu, usec:%d\n", current.tv_sec, current.tv_usec);

    time_t sec = current.tv_sec - prev.tv_sec;
    suseconds_t usec = current.tv_usec - prev.tv_usec;

    printf("sec:%lu usec:%d ", sec, usec);

    re:
    len = recvfrom(sockfd, buf, BUFSIZ, 0, (SA*)&sin2, &sin_size); 

    if (len == -1) { exit(1); }

    unsigned int name_id;
    ns_msg ns_handle;
    memset(&ns_handle, 0, sizeof(ns_handle));
    ns_initparse((const unsigned char*)buf,  len, &ns_handle);
    name_id = ns_msg_id(ns_handle);
    if (name_id != ns_id) {
        goto re;
    }

    int count;
    ns_rr rr;
    memset(&rr, 0, sizeof(rr));
    count = ns_msg_count(ns_handle, ns_s_an);

    if (count == 0) {
        printf("%s:nxdomain\n", argv[1]);
    }
    
    int i;
    int type;
    //char rdata_buffer[BUFSIZ];
    for (i=0; i<count; i++) {
        ns_parserr(&ns_handle, ns_s_an, i, &rr);
        printf("%s", argv[1]);
        printf(" ");
        /*
        memset(rdata_buffer, '\0', BUFSIZ);
        ret = ns_name_uncompress(
            ns_msg_base(ns_handle),
            ns_msg_end(ns_handle),
            ns_rr_rdata(rr),
            rdata_buffer,
            BUFSIZ);
        printf("%s", rdata_buffer);
        */

        uint32_t rbuf;
        struct in_addr* addr;
        memcpy(&rbuf, ns_rr_rdata(rr), sizeof(rbuf));
        addr = (struct in_addr*)&rbuf;
        printf("%s", inet_ntoa(*addr));
        printf(" ");
        type = ns_rr_type(rr);
        // type 1 : a
        if(type == 1) {
            printf("A");
            printf(" ");
        }
        printf("\n");
        /*
        // type 28: aaaa
        if(type == 28) {
            printf(":AAAA\n");
        }
        */
    }
    [pool drain];

    return 0;
}

void usage(void){
    printf("name [hostname] [dns server]\n");
    exit(-1);
}
