#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>

#include <iostream>
#include <string>

#include "header.hpp"

struct _thread_arg {
    int sock;
    struct sockaddr_un addr;
};


uint32_t handler;
uint16_t flow;
char dst[CAGE_ID_LEN];
char src[CAGE_ID_LEN];

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
        #ifndef __linux__
        conn_request.sun_len = sizeof(conn_request);
        #endif
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
    flow = 0;
    int len;
    char buffer[BUFSIZ];
    struct _short_header s_header;
    struct _long_header l_header;

    char* retval;
    char* ptr_fgetting = buffer+sizeof(s_header);

    for (;;) {
        printf("send:"); 

        memset(buffer, 0, sizeof(buffer));
        memset(&s_header, 0, sizeof(s_header));

        s_header.descriptor = handler;
        if (flow == 0) {
            s_header.f_type = (F_RDP_LISTEN_T2B & F_RDP_CONNECT_T2B);
        } else {
            s_header.f_type = flow;
        }
        s_header.m_type = M_RDP_DATA;
        memcpy(buffer, &s_header, sizeof(s_header));

        retval = fgets(ptr_fgetting, sizeof(buffer)-sizeof(s_header), stdin);

        if (retval == NULL) {
            break;
        }

        if (*ptr_fgetting == '\n') {
            continue;
        }

        if (strncmp(ptr_fgetting, "FLOW\n", sizeof("FLOW")) == 0) {
            FLOW:
            char tmp_buf[BUFSIZ];
            memset(tmp_buf, 0, sizeof(tmp_buf));
            printf("flow_type:"); 
            retval = fgets(tmp_buf, sizeof(tmp_buf), stdin);

            if (retval == NULL) {
                break;
            }

            if (*tmp_buf == '\n') {
                goto FLOW;
            }

            flow = (uint16_t)atoi(tmp_buf);
            continue;
        }

        if (strncmp(ptr_fgetting, "HANDLER\n", sizeof("HANDLER")) == 0) {
            ESC:
            char tmp_buf[BUFSIZ];
            memset(tmp_buf, 0, sizeof(tmp_buf));
            printf("destination handler:"); 
            retval = fgets(tmp_buf, sizeof(tmp_buf), stdin);

            if (retval == NULL) {
                break;
            }

            if (*tmp_buf == '\n') {
                goto ESC;
            }

            handler = atoi(tmp_buf);
            continue;
        }

        if (strncmp(ptr_fgetting, "MESSAGE\n", sizeof("MESSAGE")) == 0) {
            MESSAGE:
            char tmp_buf[BUFSIZ];
            memset(tmp_buf, 0, sizeof(tmp_buf));

            printf("message_type:"); 
            retval = fgets(tmp_buf, sizeof(tmp_buf), stdin);

            if (retval == NULL) {
                break;
            }

            if (*tmp_buf == '\n') {
                goto MESSAGE;
            }

            memset(&l_header, 0, sizeof(l_header));
            l_header.descriptor = handler;
            if (flow == 0) {
                l_header.f_type = (F_RDP_LISTEN_T2B & F_RDP_CONNECT_T2B);
            } else {
                l_header.f_type = flow;
            }
            l_header.m_type = atoi(tmp_buf);
            memcpy(l_header.peer_addr, dst, CAGE_ID_LEN);
            memcpy(l_header.own_addr, src, CAGE_ID_LEN);
            memcpy(buffer, &l_header, sizeof(l_header));

            std::string src_addr;
            std::string dst_addr;
            src_addr = bin2hex((const char*)(l_header.peer_addr), 20);
            dst_addr = bin2hex((const char*)(l_header.own_addr), 20);
            printf("    f_type  :%s\n", trans_f_type(l_header.f_type));
            printf("    m_type  :%s\n", trans_m_type(l_header.m_type));
            printf("    handler :%u\n", l_header.descriptor);
            printf("    src_addr:%s\n", src_addr.c_str());
            printf("    dst_addr:%s\n", dst_addr.c_str());
            len = sizeof(s_header) + strlen(ptr_fgetting);
            printf("    size is %lu\n", send(sock_fd, buffer, len, 0));
        } else { 
            len = sizeof(s_header) + strlen(ptr_fgetting);
            printf("    send is %lu\n", send(sock_fd, buffer, len, 0));
        }
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
            uint16_t f_type = *ptr;
            ptr++;
            uint16_t m_type = *ptr;

            if (M_RDP_DATA == m_type)
            {
                struct _short_header* s_header = (struct _short_header*)buffer;
                printf("\nrecv:\n");
                printf("    f_type  :%s\n", trans_f_type(f_type));
                printf("    m_type  :%s\n", trans_m_type(m_type));
                printf("    handler :%u\n", s_header->descriptor);
                printf("    size    :%lu\n", rsize);
                printf("    buffer  :%s", buffer+sizeof(struct _short_header)); 
                handler = s_header->descriptor;
            }
            else 
            {
                struct _long_header* l_header = (struct _long_header*)buffer;
                std::string src_addr;
                std::string dst_addr;
                src_addr = bin2hex((const char*)(l_header->peer_addr), 20);
                dst_addr = bin2hex((const char*)(l_header->own_addr),  20);
                printf("\nrecv:\n");
                printf("    f_type  :%s\n", trans_f_type(f_type));
                printf("    m_type  :%s\n", trans_m_type(m_type));
                printf("    handler :%u\n", l_header->descriptor);
                printf("    src_addr:%s\n", src_addr.c_str());
                printf("    dst_addr:%s\n", dst_addr.c_str());
                printf("    size    :%lu\n", rsize);
                memcpy(dst, l_header->peer_addr, CAGE_ID_LEN);
                memcpy(src, l_header->own_addr, CAGE_ID_LEN);
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
    const char *hexstr[16] = {"0", "1", "2", "3",
                              "4", "5", "6", "7",
                              "8", "9", "a", "b",
                              "c", "d", "e", "f"};

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
