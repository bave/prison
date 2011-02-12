
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

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
    char buffer[BUFSIZ];
    for (;;) {
        printf("send:"); 
        memset(buffer, 0, sizeof(buffer));
        fgets(buffer, sizeof(buffer), stdin);
        send(sock_fd, buffer, strlen(buffer), 0);
        memset(buffer, 0, sizeof(buffer));
        recv(sock_fd, buffer, sizeof(buffer), 0);
        printf("recv:%s", buffer); 
    }

    close(sock_fd);
    return 0;
}

