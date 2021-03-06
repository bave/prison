
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/select.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

void list(void);

int main(int argc, char** argv)
{
    const char* path = NULL;
    if (argc == 1) {
        path = "/tmp/sock_cage";
    }
    if (argc == 2) {
        path = argv[1];
    }
    if (argc >= 3) {
        printf("usage\n");
        printf("cli only,,, default sock_path is /tmp/sock_cage\n");
        printf("cli [socket_path]\n");
        return 1;
    }

    int err = 0;
    int sock_fd;
    sock_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (sock_fd == -1) { 
        err = errno;
        perror("socket");
    } 

    /*
    if (err == 0) {
        int on = 1;
        err = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));
        if (err != 0) {
            err = errno;
            perror("setsockopt");
        }
    }
    */

    struct sockaddr_un conn_request;
    memset(&conn_request, 0, sizeof(conn_request));

    if (err == 0) {
        #ifndef __linux__
        conn_request.sun_len = sizeof(conn_request);
        #endif
        conn_request.sun_family = AF_LOCAL;
        memcpy(conn_request.sun_path, path, strlen(path));
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

    int ret_system;
    ret_system = system("sh -c \"tput clear\"");
    if (ret_system != 0) { 
        printf("Cant Clear this terminal.. Why??\n");
    }
    printf("Now connecting to %s!!\n", path);
    char buffer[65535];

    fd_set fds;
    struct timeval t;
    t.tv_sec=1;
    t.tv_usec=0;
    int sel;

    for (;;) {

        printf("send_message:");
        fflush(stdout);

        memset(buffer, '\0', sizeof(buffer));

SELECT_LOOP:
        t.tv_sec=1;
        t.tv_usec=0;
        FD_ZERO(&fds);
        FD_SET(fileno(stdin), &fds);
        FD_SET(sock_fd, &fds);
        sel = select(sock_fd+1, &fds, NULL, NULL, &t);
        if (sel == 0) {
            goto SELECT_LOOP;
        } else if (FD_ISSET(sock_fd, &fds)) {
            printf("cage process closed");
            close(sock_fd);
            return 0;
        }

        char* ret = NULL;
        ret = fgets(buffer, sizeof(buffer), stdin);

        if (ret == NULL) {
            fflush(stdin);
            break;
        }

        if (strcmp(buffer, "\n") == 0) {
            fflush(stdin);
            continue;
        }

        if (strcmp(buffer, "list\n") == 0) {
            list();
            fflush(stdin);
            continue;
        }

        if (strcmp(buffer, "?\n") == 0) {
            list();
            fflush(stdin);
            continue;
        }

        if (strcmp(buffer, "clear\n") == 0) {

            ret_system = system("sh -c \"tput clear\"");
            if (ret_system != 0) { 
                printf("Cant Clear this terminal.. Why??\n");
            }
            printf("Cant Clear this terminal.. Why??\n");
            fflush(stdin);
            continue;
        }

        if (strcmp(buffer, "quit\n") == 0) {
            break;
        }

        if (strcmp(buffer, "exit\n") == 0) {
            break;
        }

        send(sock_fd, buffer, strlen(buffer), 0);
        memset(buffer, 0, sizeof(buffer));
        ssize_t rsize = recv(sock_fd, buffer, sizeof(buffer), 0);
        if (rsize == 0) {
            printf("cage process closed");
            close(sock_fd);
            return 0;
        }
        
        printf("recv_message:%s", buffer); 
    }
    //leave:
    close(sock_fd);
    return 0;
}

void list(void) {

    const char* usage_list =""
    "\n"
    "console-command:list, claer, quit, exit\n"
    "\n"
    "escape character: \"\\\"\n"
    "reserved words: words in small letters, and numbers\n"
    "variables     : words in capital letters\n"
    "\n"
    "new,NODE_NAME,PORT_NUMBER | new,NODE_NAME,PORT_NUMBER,global\n"
    "-> 200,new,NODE_NAME,PORT_NUMBER |\n"
    //"   400 | 401,COMMENT |\n"
    //"   402 | 403,new,NODE_NAME,PORT_NUMBER,COMMENT\n"
    "\n"
    "delete,NODE_NAME\n"
    "-> 201,delete,NODE_NAME |\n"
    //"   404,delete,NODE_NAME,COMMENT\n"
    "\n"
    "set_id,NODE_NAME,IDNETIFIER\n"
    "-> 205,set_id,NODE_NAME,IDENTIFIER |\n"
    "\n"
    "get_id,NODE_NAME\n"
    "-> 208,get_id,NODE_NAME,IDENTIFIER |\n"
    //"   400 | 401,COMMENT\n"
    "\n"
    "join,NODE_NAME,HOST,PORT\n"
    "-> 202,join,NODE_NAME,HOST,PORT\n"
    //"   400 | 401,COMMENT\n"
    //"   405 | 406,join,NODE_NAME,HOST,PORT,COMMENT\n"
    "\n"
    "put,NODE_NAME,KEY,VALUE,TTL | put,NODE_NAME,KEY,VALUE,TTL,unique\n"
    "-> 203,put,NODE_NAME,KEY,VALUE,TTL\n"
    //"   400,COMMENT\n"
    //"   401,COMMENT\n"
    //"   407,put,NODE_NAME,KEY,VALUE,TTL,COMMENT\n"
    "\n"
    "get,NODE_NAME,key\n"
    "-> 204,get,NODE_NAME,KEY,VALUE1,VALUE2,VALUE3,...\n"
    //"   400,COMMENT\n"
    //"   401,COMMENT\n"
    //"   408,get,KEY,COMMENT\n"
    //"   409,get,KEY\n"
    "\n"
    "rdp_listen,NODE_NAME,SOCK_NAME,RDP_SPORT\n"
    "-> 206,rdp_listen,NODE_NAME,SOCK_NAME,RDP_SPORT\n"
    //"400,COMMENT\n"
    //"401,COMMENT\n"
    //"407,rdp_listen,NODE_NAME,SOCK_NAME,PORT\n"
    //"408,rdp_listen,NODE_NAME,SOCK_NAME,PORT\n"
    "\n"
    "rdp_connect,NODE_NAME,SOCK_NAME,RDP_DPORT,RDP_DADDR\n"
    "-> 207,rdp_listen,NODE_NAME,SOCK_NAME,RDP_DPORT,RDP_DADDR\n"
    "\n";
    printf("%s", usage_list);
    return;
}


