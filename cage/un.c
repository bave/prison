#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

struct _thread_arg {
    int sock;
    struct sockaddr_un addr;
};

void *thread_callback(void *opaque);



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

    char buffer[BUFSIZ];
    for (;;) {
        printf("send:"); 
        memset(buffer, 0, sizeof(buffer));
        fgets(buffer, sizeof(buffer), stdin);
        send(sock_fd, buffer, strlen(buffer), 0);
        usleep(1000);
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
        if (rsize ==0) {
            break;
        }
        printf("\nrecv:%s\nsend:", buffer); 
    }

    free(opaque);
    exit(0);
    return 0;
}

