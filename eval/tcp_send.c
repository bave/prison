#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>


// time measurement macro
#define TCHK_START(name)           \
    struct timeval name##_prev;    \
    struct timeval name##_current; \
    gettimeofday(&name##_prev, NULL)

#define TCHK_END(name)                                                             \
gettimeofday(&name##_current, NULL);                                               \
time_t name##_sec;                                                                 \
suseconds_t name##_usec;                                                           \
if (name##_current.tv_sec == name##_prev.tv_sec) {                                 \
    name##_sec = name##_current.tv_sec - name##_prev.tv_sec;                       \
    name##_usec = name##_current.tv_usec - name##_prev.tv_usec;                    \
} else if (name ##_current.tv_sec != name##_prev.tv_sec) {                         \
    int name##_carry = 1000000;                                                    \
    name##_sec = name##_current.tv_sec - name##_prev.tv_sec;                       \
    if (name##_prev.tv_usec > name##_current.tv_usec) {                            \
        name##_usec = name##_carry - name##_prev.tv_usec + name##_current.tv_usec; \
        name##_sec--;                                                              \
        if (name##_usec > name##_carry) {                                          \
            name##_usec = name##_usec - name##_carry;                              \
            name##_sec++;                                                          \
        }                                                                          \
    } else {                                                                       \
        name##_usec = name##_current.tv_usec - name##_prev.tv_usec;                \
    }                                                                              \
}                                                                                  \
printf("%s: sec:%lu usec:%06d\n", #name, name##_sec, name##_usec); 

void usage (void);

int main( int argc, char **argv)
{

    int errno;
    int sockfd;
    struct addrinfo info; 
    struct addrinfo *res; 
    int i;

    if (argc != 4) usage();

    memset(&info, 0, sizeof(info));
    info.ai_socktype = SOCK_STREAM; 
    errno = getaddrinfo(argv[1], argv[2], &info, &res);

    if (errno) { 
        printf("you set bad addr,so exit!\n");
        return 1;
    }

    printf("port_number:%d\n", atoi(argv[2]));
    printf("hostname:%s\n", (argv[1]));

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    int len;
    char buf[BUFSIZ];
    socklen_t sin_size = sizeof(struct sockaddr_storage);  
    struct sockaddr_storage saddr;

    connect(sockfd, res->ai_addr, res->ai_addrlen); 
    //while(1){
    for(i=0; i<100; i++) {
        memset(buf, 0, BUFSIZ);
        TCHK_START(tcp_ping);
        send(sockfd, argv[3], strlen(argv[3]), 0);
        len = recv(sockfd, buf, BUFSIZ, 0); 
        TCHK_END(tcp_ping);
        //printf("recv: %s\n", buf);
        sleep(1);
    }
    close(sockfd);
    freeaddrinfo(res);
    return 0;

}

    void
usage()
{
    printf("usage\n");
    printf("send_tcp [IP_address] [port_number] [data_string]\n");
    exit(1);	
}

