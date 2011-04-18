#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include <iostream>
#include <string>

#include "header.hpp"

struct _thread_arg {
    int sock;
    struct sockaddr_un addr;
};


int handler;

void *thread_callback(void *opaque);
std::string bin2hex(const char* buf, unsigned int size);



int main(int argc, char** argv)
{
    if (argc != 2) {
        printf("usage : un [socket_path]\n");
        return 1;
    }

    int err = 0;
    int on = 1;
    int sock_fd;

    sock_fd = socket(AF_LOCAL, SOCK_STREAM, 0);

    if (sock_fd == -1) { 
        err = errno;
        perror("socket");
    } 

    if (err == 0) {
        err = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));
        if (err != 0) {
            err = errno;
            perror("setsockopt");
        }
    }

    struct sockaddr_un conn_request;
    memset(&conn_request, 0, sizeof(conn_request));

    if (err == 0) {
        conn_request.sun_len = sizeof(conn_request);
        conn_request.sun_family = AF_LOCAL;
        memcpy(conn_request.sun_path, argv[1], strlen(argv[1]));
    }

    if (err == 0) {
        err=connect(sock_fd, (struct sockaddr*)&conn_request, SUN_LEN(&conn_request));
        if (err != 0) {
            err = errno;
            perror("connect");
            close(sock_fd);
            return 1;
        }

    }

    pthread_t t_id;
    struct _thread_arg *thread_arg;
    thread_arg = (struct _thread_arg *)malloc(sizeof(struct _thread_arg));
    thread_arg->sock =  sock_fd;
    memcpy(&conn_request, &thread_arg->addr, sizeof(struct sockaddr_un));
    pthread_create(&t_id, 0, thread_callback, (void *)thread_arg);

    handler = 0;
    int len;
    char buffer[BUFSIZ];
    struct _short_header
    {
        uint16_t f_type;
        uint16_t m_type;
        uint32_t descriptor;
    }s_header;
    char* ptr_fgetting = buffer+sizeof(s_header);

    for (;;) {
        printf("send:"); 

        memset(buffer, 0, sizeof(buffer));
        memset(&s_header, 0, sizeof(s_header));

        s_header.descriptor = htonl(handler);
        s_header.f_type = htons(F_RDP_LISTEN_T2B & F_RDP_CONNECT_T2B);
        s_header.m_type = htons(M_RDP_DATA);
        memcpy(buffer, &s_header, sizeof(s_header));

        fgets(ptr_fgetting, sizeof(buffer)-sizeof(s_header), stdin);
        if (*ptr_fgetting == '\0') {
            break;
        }

        len = sizeof(s_header) + strlen(ptr_fgetting);
        printf("    send_size is %lu\n", send(sock_fd, buffer, len, 0));
                
    }

    close(sock_fd);
    return 0;
}

void* thread_callback(void *opaque)
{
    pthread_detach(pthread_self());
    struct _thread_arg *thread_arg;
    thread_arg = (struct _thread_arg*)opaque;
    char buffer[BUFSIZ];

    ssize_t rsize;
    for (;;) {
        memset(buffer, 0, sizeof(buffer));
        rsize = recv(thread_arg->sock, buffer, sizeof(buffer), 0);

        if (rsize <= 0) {
            break;
        } else {

            uint16_t* ptr;
            ptr = (uint16_t*)buffer;
            uint16_t f_type = ntohs(*ptr);
            ptr++;
            uint16_t m_type = ntohs(*ptr);

            if (M_RDP_DATA == m_type)
            {
                struct _short_header* s_header = (struct _short_header*)buffer;
                printf("\nrecv:\n");
                printf("    size    :%lu\n", rsize);
                printf("    f_type  :%s\n", trans_f_type(f_type));
                printf("    m_type  :%s\n", trans_m_type(m_type));
                printf("    handler :%d\n", s_header->descriptor);
                printf("    buffer  :%s", buffer+sizeof(struct _short_header)); 
                handler = s_header->descriptor;
            }
            else 
            {
                struct _long_header* l_header = (struct _long_header*)buffer;
                const char* src_addr;
                const char* dst_addr;
                src_addr = bin2hex((const char*)&(l_header->peer_addr), 20).c_str();
                dst_addr = bin2hex((const char*)&(l_header->own_addr),  20).c_str();
                printf("\nrecv:\n");
                printf("    size    :%lu\n", rsize);
                printf("    f_type  :%s\n", trans_f_type(f_type));
                printf("    m_type  :%s\n", trans_m_type(m_type));
                printf("    src_addr:%s\n", src_addr);
                printf("    dst_addr:%s\n", dst_addr);
                printf("    handler :%d\n", l_header->descriptor);
                handler = l_header->descriptor;
            }
        }
    }
    free(opaque);
    exit(0);
    return 0;
}

std::string bin2hex(const char* buf, unsigned int size)
{
    const char *hexstr[16] = {"0", "1", "2", "3", "4",
                              "5", "6", "7", "8", "9",
                              "a", "b", "c", "d", "e", "f"};

    std::string str = "";
    char* p = (char*)buf;

    uint32_t i;
    for (i = 0; i < size; i++) {
            uint8_t t1, t2;
            t1 = (0x00f0 & (p[i])) >> 4;
            t2 = 0x000f & (p[i]);
            str += hexstr[t1];
            str += hexstr[t2];
    }

    return str;
}
