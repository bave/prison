#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include <arpa/inet.h>

#include <event.h>

#include <libcage/cage.hpp>

#include <iostream>
#include <string>
#include <set>

#include <grp.h>
#include <pwd.h>

#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

#include "../base64.hpp"
#include "header.hpp"

#define DEBUG
#ifdef DEBUG
  #define D(X) X
#else
  #define D(X)
#endif

extern char *optarg;
extern int optind;
extern int optopt;
extern int opterr;
extern int optreset;

typedef boost::char_separator<char> char_separator;
typedef boost::tokenizer<char_separator> tokenizer;
typedef boost::escaped_list_separator<char> esc_separator;
typedef boost::tokenizer<esc_separator> esc_tokenizer;
typedef boost::unordered_map<int, boost::shared_ptr<event> > sock2ev_type;
typedef boost::unordered_map<std::string, boost::shared_ptr<libcage::cage> > name2node_type;

bool  is_daemon = false;
int instance_identifier = 0;
const char* nsock_path = NULL;

std::multimap<int,int> id_mapper;
static bool id_mapper_fixed_erase(int key, int value);
//static bool id_mapper_exist_match(int key, int value)
std::string bin2hex(const char* buf, unsigned int size);

#ifdef __APPLE__
extern "C" int getgrouplist_2(const char*, gid_t, gid_t**);
#endif

sock2ev_type   sock2ev;
name2node_type name2node;

// rdp_csock -> rdp_connect side socket
void callback_csock_accept(int fd, short ev, void *arg);
void callback_csock_read(int fd, short ev, void *arg);

// rdp_lsock -> rdp_listen side socket
void callback_lsock_accept(int fd, short ev, void *arg);
void callback_lsock_read(int fd, short ev, void *arg);

// cli_sock
void callback_accept(int fd, short ev, void *arg);
void callback_read(int fd, short ev, void *arg);
bool start_listen(const char* path);


// utils function
void usage(char *cmd);
int bind_safe(int sock_fd, const char* path, bool forceful);
int create_named_socket(const char* path);

// cli processing
void do_command(int sockfd, std::string command);
void replace(std::string &str, std::string from, std::string to);

void process_set_id(int sockfd, esc_tokenizer::iterator &it,
                    const esc_tokenizer::iterator &end);

void process_get_id(int sockfd, esc_tokenizer::iterator &it,
                    const esc_tokenizer::iterator &end);

void process_new(int sockfd, esc_tokenizer::iterator &it,
                 const esc_tokenizer::iterator &end);

void process_delete(int sockfd, esc_tokenizer::iterator &it,
                    const esc_tokenizer::iterator &end);

void process_join(int sockfd, esc_tokenizer::iterator &it,
                  const esc_tokenizer::iterator &end);

void process_put(int sockfd, esc_tokenizer::iterator &it,
                 const esc_tokenizer::iterator &end);

void process_get(int sockfd, esc_tokenizer::iterator &it,
                 const esc_tokenizer::iterator &end);

void process_rdp_listen(int sockfd, esc_tokenizer::iterator &it,
                        const esc_tokenizer::iterator &end);

void process_rdp_connect(int sockfd, esc_tokenizer::iterator &it,
                         const esc_tokenizer::iterator &end);

static const char* const SUCCEEDED_NEW         = "200";
static const char* const SUCCEEDED_DELETE      = "201";
static const char* const SUCCEEDED_JOIN        = "202";
static const char* const SUCCEEDED_PUT         = "203";
static const char* const SUCCEEDED_GET         = "204";
static const char* const SUCCEEDED_SET_ID      = "205";
static const char* const SUCCEEDED_RDP_LISTEN  = "206";
static const char* const SUCCEEDED_RDP_CONNECT = "207";
static const char* const SUCCEEDED_GET_ID      = "208";

static const char* const ERR_UNKNOWN_COMMAND   = "400";
static const char* const ERR_INVALID_STATEMENT = "401";
static const char* const ERR_CANNOT_OPEN_PORT  = "402";
static const char* const ERR_ALREADY_EXIST     = "403";
static const char* const ERR_DEL_NO_SUCH_NODE  = "404";
static const char* const ERR_JOIN_NO_SUCH_NODE = "405";
static const char* const ERR_JOIN_FAILED       = "406";
static const char* const ERR_CREAT_RDP_FAILURE = "407";
static const char* const ERR_NO_SUCH_NODE      = "408";
static const char* const ERR_GET_FAILURE       = "409";

/*
  escape character: "\"

  reserved words:   words in small letters, and numbers

  variables:        words in capital letters

  new,NODE_NAME,PORT_NUMBER |
  new,NODE_NAME,PORT_NUMBER,global
  -> 200,new,NODE_NAME,PORT_NUMBER |
     400 | 401,COMMENT |
     402 | 403,new,NODE_NAME,PORT_NUMBER,COMMENT

  delete,NODE_NAME
  -> 201,delete,NODE_NAME |
     404,delete,NODE_NAME,COMMENT

  set_id,NODE_NAME,IDNETIFIER
  -> 205,set_id,NODE_NAME,IDENTIFIER |
     400 | 401,COMMENT

  get_id,NODE_NAME
  -> 208,get_id,NODE_NAME,IDENTIFIER |
     400 | 401,COMMENT

  join,NODE_NAME,HOST,PORT
  -> 202,join,NODE_NAME,HOST,PORT
     400 | 401,COMMENT
     405 | 406,join,NODE_NAME,HOST,PORT,COMMENT

  put,NODE_NAME,KEY,VALUE,TTL | 
  put,NODE_NAME,KEY,VALUE,TTL,unique
  -> 203,put,NODE_NAME,KEY,VALUE,TTL |
     400 | 401,COMMENT |
     408,put,NODE_NAME,KEY,VALUE,TTL,COMMENT

  get,NODE_NAME,key
  -> 204,get,NODE_NAME,KEY,VALUE1,VALUE2,VALUE3,... |
     400 | 401,COMMENT |
     408,get,NODE_NAME,KEY,COMMENT |
     409,get,NODE_NAME,KEY

  rdp_listen,NODE_NAME,SOCK_NAME,RDP_DPORT
  -> 206,rdp_listen,NODE_NAME,SOCK_NAME,RDP_DPORT |
     400 | 401,COMMENT |
     407,rdp_listen,NODE_NAME,SOCK_NAME,RDP_DPORT |
     408,rdp_listen,NODE_NAME,SOCK_NAME,RDP_DPORT

  rdp_connect,NODE_NAME,SOCK_NAME,RDP_DADDR,RDP_DPORT,
  -> 206,rdp_connect,NODE_NAME,SOCK_NAME,RDP_DADDR,RDP_DPORT |
     400 | 401,COMMENT |
     407,rdp_connect,NODE_NAME,SOCK_NAME,RDP_DADDR,RDP_DPORT |
     408,rdp_connect,NODE_NAME,SOCK_NAME,RDP_DADDR,RDP_DPORT
 */

/* cage_rdp to named_socket protocol header
 * max size is lo0_mtu (16384byte - macosx 10.6.7)
 * max size is lo0_mtu (16436byte - linux 2.6.32-30(ubuntu 10.4))
 *
 * Flow_Type 16bit
 * 1   -> 00000001 : F_RDP_CONNECT_T2B
 * 2   -> 00000010 : F_RDP_CONNECT_B2T
 * 4   -> 00000100 : F_RDP_LISTEN_T2B
 * 8   -> 00001000 : F_RDP_LISTEN_B2T
 * 16  -> 00010000 : RESERVE
 * 32  -> 00100000 : RESERVE
 * 64  -> 01000000 : RESERVE
 * 128 -> 10000000 : RESERVE
 * :
 *
 * - comment
 *     T2B -> Top to Bottom
 *     B2T -> Bottom to Top
 *
 * Mesg_Type 16bit
 * 1   -> 00000001 : M_RDP_DATA
 * 2   -> 00000010 : M_RDP_ACCEPT
 * 4   -> 00000100 : M_RDP_CONNECT
 * 8   -> 00001000 : M_RDP_CLOSED
 * 16  -> 00010000 : RESERVE
 * 32  -> 00100000 : RESERVE
 * 64  -> 01000000 : RESERVE
 * 128 -> 10000000 : RESERVE
 * :
 *
 *
 * Packet Format of Types which are Flow_Type and Mesg_Type
 *
 * F_RDP_CONNECT_T2B + M_RDP_CONNECT
 * F_RDP_LISTEN_T2B  + M_RDP_ACCEPT
 *  -> nothing 
 *
 * F_RDP_CONNECT_B2T + M_RDP_CONNECT
 * F_RDP_LISTEN_B2T  + M_RDP_ACCEPT
 * F_RDP_CONNECT_B2T + M_RDP_CLOSED
 * F_RDP_LISTEN_B2T  + M_RDP_CLOSED
 * :
 *  -> |----------------------------------------|
 *     | XXXXXXXX          | XXXXXXXXX          |
 *     |----------------------------------------|
 *     | rdp_descriptor_handler (4byte)         |
 *     |----------------------------------------|
 *     : peer_node_id (20byte)                  : x 5 layer
 *     |----------------------------------------|
 *     : own_node_id (20byte)                   : x 5 layer
 *     |----------------------------------------|
 *
 * F_RDP_CONNECT_T2B + M_RDP_DATA
 * F_RDP_CONNECT_B2T + M_RDP_DATA
 * F_RDP_LISTEN_B2T  + M_RDP_DATA
 * F_RDP_LISTEN_T2B  + M_RDP_DATA
 *  -> |----------------------------------------|
 *     | XXXXXXXX          | XXXXXXXXX          |
 *     |----------------------------------------|
 *     | rdp_descriptor_handler (4byte)         |
 *     |----------------------------------------|
 *     | payload                                |
 *     :                                        :
 *     |                                        |
 *
 *
 */

 

int
main(int argc, char** argv)
{
    pid_t pid;

    int   opt;

    while ((opt = getopt(argc, argv, "dhn:")) != -1) {
        switch (opt) {
        case 'h':
            usage(argv[0]);
            return 0;
        case 'n':
            nsock_path = optarg;
            break;
        case 'd':
            is_daemon = true;
            break;
            /*
           case 'p':
           port = atoi(optarg);
           break;
           */
        }
    }

    if (is_daemon) {
        if ((pid = fork()) < 0) {
            //printf("cant fork!!\n");
            return -1;
        } else if (pid != 0) {
            //printf("parent process!!\n");
            exit(0);
        } else {
            //printf("forked process!!\n");
            setsid();
            int retval;
            retval = chdir("/");
            if (retval == 0) {
                // success
            } else {
                // error
            }
            umask(0);
        }
    }

    if (nsock_path == NULL) {
        nsock_path = "/tmp/sock_cage";
    }


    event_init();

    if (start_listen(nsock_path) == false) {
        perror("start_listen");
        return -1;
    }

    event_dispatch();

    return 0;
}

void
usage(char *cmd)
{
        printf("%s [-d] [-n name_socket]\n", cmd);
        printf("    -d: run as daemon\n");
        printf("    -h: show this help\n");
        printf("    -n: af_local socket, default is /tmp/sock_cage\n");
}

int
bind_safe(int sock_fd, const char* path, bool forceful)
{
    struct stat st;
    memset(&st, 0, sizeof(st));

    struct sockaddr_un bind_request;
    memset(&bind_request, 0, sizeof(bind_request));

    // path size checking
    int err = 0;

    if (strlen(path) >= sizeof(bind_request.sun_path)) {
        err = EINVAL;
    }

    // get parent diectory path
    char* path_dir = NULL;
    char* last_slash = NULL;
    if (err == 0) {
        path_dir = strdup(path);
        if (path_dir == NULL) {
            err = ENOMEM;
        }
    }

    if (err == 0) {
        last_slash = strrchr(path_dir, '/');
        if (last_slash == NULL) {
            err = EINVAL;
        } else {
            *last_slash = '\0';
        }
        err = stat(path_dir, &st);
        if (err != 0) {
            err = errno;
        }
    }

    // is Existent parent directory ?
    if ( (err == 0) && !S_ISDIR(st.st_mode) ) {
        err = EINVAL;
    }

    // writable checking...
    if (err == 0) {
        uid_t uid = geteuid();
        gid_t gid = getegid();

        if (st.st_mode & S_IWUSR && st.st_uid == uid) {
            goto safe_bind_writable;
        } else if (st.st_mode & S_IWOTH) {
            goto safe_bind_writable;
        } else if (st.st_mode & S_IWGRP && st.st_gid == gid) {
            goto safe_bind_writable;
        }

        struct passwd* pw;
        pw = getpwuid(uid);
        int number_of_groups = 0;
        gid_t* gid_buffer = NULL;

#ifdef __APPLE__
        number_of_groups = getgrouplist_2(pw->pw_name, gid, &gid_buffer);
#else
        getgrouplist(pw->pw_name, gid, gid_buffer, &number_of_groups);
        gid_buffer = (gid_t*)malloc(number_of_groups * sizeof(gid_t));
        memset(gid_buffer, 0, number_of_groups * sizeof(gid_t));
        getgrouplist(pw->pw_name, gid, gid_buffer, &number_of_groups);
#endif

        for (int i = 0; i < number_of_groups; i++) {
            if (st.st_mode & S_IWGRP && st.st_gid == gid_buffer[i]) {
                free(gid_buffer);
                goto safe_bind_writable;
            }
        }

        free(gid_buffer);
        err = EINVAL;
    }

safe_bind_writable:

    /*
    // writable checking...
    if (err == 0) {
        const char* mkdir_test_name = "/mkdir_test";
        int mkdir_test_size = strlen(path_dir) + strlen(mkdir_test_name) + 1;
        char* mkdir_test_path = (char*)malloc(mkdir_test_size);
        if (mkdir_test_path == NULL) {
                err = errno;
        }
        if (err == 0) {
            memset(mkdir_test_path, 0 , mkdir_test_size);
            strcat(mkdir_test_path, path_dir);
            strcat(mkdir_test_path, mkdir_test_name);
            //printf("%s\n", mkdir_test_path);
        }

        // permission : rwxr-xr-t
        static const mode_t mode_required = 
                   S_ISVTX | S_IRWXU | (S_IRGRP | S_IXGRP) | (S_IROTH | S_IXOTH);

        if (err == 0) {
            err = mkdir(mkdir_test_path, mode_required);
            if (err != 0) {
                err = errno;
            }
        }
        if (err == 0) {
            err = rmdir(mkdir_test_path);
            if (err != 0) {
                err = errno;
            }
        }
        if (err == 0) {
            free(mkdir_test_path);
        }
    }

    if ((err == 0) && (!(st.st_mode & S_ISTXT) || (st.st_uid != 0))) {
        err = EINVAL;
    }
    if ((err == 0) && (st.st_uid != geteuid())) {
        err = EINVAL;
    }
    if ((err == 0) && ((st.st_mode & ACCESSPERMS) != mode_required)) {
        err = EINVAL;
    }
    */

    // check file exist
    if (err == 0) {
        int ret = stat(path, &st);
        if (ret == 0) {
            // exist
            if (S_ISSOCK(st.st_mode)) {
                // socket
                if (forceful == true) {
                    unlink(path);
                } else {
                    err = EEXIST;
                }
            } else {
                // not socket
                err = EEXIST;
            }
        } else {
            // nothing
        }
    }

    if (err == 0) {
        uint32_t prev_mask = umask(0);
#ifdef __APPLE__
        bind_request.sun_len = sizeof(bind_request);
#endif
        bind_request.sun_family = AF_LOCAL;
        memcpy(bind_request.sun_path, path, strlen(path));
        err = bind(sock_fd, (struct sockaddr*)&bind_request, SUN_LEN(&bind_request));
        //perror("bind");
        if (err != 0) {
            err = errno;
        }
        umask(prev_mask);
    }

    free(path_dir);
    errno = err;
    return err;
}

bool
start_listen(const char* path)
{
    event *ev = new event;

    int err = 0;
    int on = 1;
    int sock_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (sock_fd == -1) { 
        err = errno;
        //perror("socket");
    } 

    if (err == 0) {
        err = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR,
                (char*)&on, sizeof(on));
        if (err != 0) {
            err = errno;
            //perror("setsockopt");
        }
    }

    if (err == 0) {
        err = bind_safe(sock_fd, path, true);
        if (err != 0) {
            //perror("bind_safe");
            err = errno;
            close(sock_fd);
            delete ev;
            return false;
        }
    }
    if (err == 0) {
        if (listen(sock_fd, 10) < 0) {
            err = errno;
            close(sock_fd);
            delete ev;
            //perror("listen");
            return false;
        }
    }

    if (is_daemon == false) {
        D(fprintf(stderr, "cage: preparations for listen\n"));
    }

    event_set(ev, sock_fd, EV_READ | EV_PERSIST, &callback_accept, NULL);
    event_add(ev, NULL);

    return true;
}

void
callback_accept(int sockfd, short ev, void *arg)
{
    if (ev == EV_READ) {
        D(std::cout << "function callback_cli_accept" << std::endl); 
        sockaddr_in saddr_in;
        socklen_t len = sizeof(saddr_in);
        int fd = accept(sockfd, (sockaddr*)&saddr_in, &len);

        boost::shared_ptr<event> readev(new event);

        sock2ev[fd] = readev;

        event_set(readev.get(), fd, EV_READ | EV_PERSIST, &callback_read, NULL);
        event_add(readev.get(), NULL);
    }
    return;
}

void
callback_read(int sockfd, short ev, void *arg)
{
    ssize_t size;
    char    buf[1024 * 64];
    memset(buf, 0, sizeof(buf));

    if (ev == EV_READ) {
        retry:
        size = recv(sockfd, buf, sizeof(buf), 0);

        if (size <= 0) {
            if (size == -1) {
                if (errno == EINTR) {
                    goto retry;
                }
                perror("recv");
            }
            D(std::cout << "function cli_close" << std::endl); 
            event_del(sock2ev[sockfd].get());
            sock2ev.erase(sockfd);
            shutdown(sockfd, SHUT_RDWR);
            close(sockfd);
            return;
        }

        //fprintf(stderr, "cage_read:%lu:%s\n", strlen(buf), buf);
        D(std::cout << "function callback_cli_read" << std::endl
                    << "    buf: " << buf << std::endl
                    << "    len: " << strlen(buf) << std::endl); 

        buf[size - 1] = '\0';
        std::string str(buf);

        replace(str, "\r\n", "\n");
        replace(str, "\r", "\n");

        char_separator sep("\n", "", boost::drop_empty_tokens);
        tokenizer      tokens(str, sep);

        for (tokenizer::iterator it = tokens.begin();
                it != tokens.end(); ++it) {
            do_command(sockfd, *it);
        }
    }
    return;
}

void
replace(std::string &str, std::string from, std::string to)
{
        for (std::string::size_type pos = str.find(from);
             pos != std::string::npos;
             pos = str.find(from, from.length() + pos)) {
                str.replace(pos, from.length(), to);
        }
}

void
do_command(int sockfd, std::string command)
{
    esc_separator sep("\\", ",", "\"");
    esc_tokenizer tokens(command, sep);

    esc_tokenizer::iterator it = tokens.begin();

    if (it == tokens.end())
        return;

    if (*it == "new") {
        D(std::cout << "process new" << std::endl);
        process_new(sockfd, ++it, tokens.end());
    } else if (*it == "delete") {
        D(std::cout << "process delete" << std::endl);
        process_delete(sockfd, ++it, tokens.end());
    } else if (*it == "set_id") {
        D(std::cout << "process set_id" << std::endl;);
        process_set_id(sockfd, ++it, tokens.end());
    } else if (*it == "get_id") {
        D(std::cout << "process get_id" << std::endl;);
        process_get_id(sockfd, ++it, tokens.end());
    } else if (*it == "join") {
        D(std::cout << "process join" << std::endl;);
        process_join(sockfd, ++it, tokens.end());
    } else if (*it == "put") {
        D(std::cout << "process put" << std::endl);
        process_put(sockfd, ++it, tokens.end());
    } else if (*it == "get") {
        D(std::cout << "process get" << std::endl);
        process_get(sockfd, ++it, tokens.end());
    } else if (*it == "rdp_listen") {
        D(std::cout << "process rdp_listen" << std::endl);
        process_rdp_listen(sockfd, ++it, tokens.end());
    } else if (*it == "rdp_connect") {
        D(std::cout << "process rdp_connect" << std::endl);
        process_rdp_connect(sockfd, ++it, tokens.end());
    } else {
        D(std::cout << "unknown command: " << *it << std::endl);
        char result[1024*64];
        // format: 400,COMMENT
        snprintf(result, sizeof(result), 
                "400,unknown command. cannot recognize '%s'\n", it->c_str());
        send(sockfd, result, strlen(result), 0);
    }
}

void
process_set_id(int sockfd, esc_tokenizer::iterator &it,
               const esc_tokenizer::iterator &end)
{
    name2node_type::iterator it_n2n;
    std::string node_name;
    std::string esc_node_name;
    char result[1024 * 64];

    // read node_name
    if (it == end || it->length() == 0) {
        // there is no port number
        // format: 401,COMMENT
        snprintf(result, sizeof(result),
                "401,node name is required\n");
        send(sockfd, result, strlen(result), 0);
        return;
    }

    node_name = *it;
    esc_node_name = *it;
    replace(esc_node_name, ",", "\\,");

    it_n2n = name2node.find(node_name);
    if (it_n2n == name2node.end()) {
        // invalid node name
        // format: 404,delete,NODE_NAME,COMMENT
        snprintf(result, sizeof(result),
                "%s,delete,%s,no such node named '%s'\n",
                ERR_DEL_NO_SUCH_NODE, esc_node_name.c_str(),
                esc_node_name.c_str());
        send(sockfd, result, strlen(result), 0);
        return;
    }

    ++it;

    // read port number
    if (it == end) {
        // there is no identifier
        // format: 401,COMMENT
        snprintf(result, sizeof(result),
                "401,identifier is required\n");
        send(sockfd, result, strlen(result), 0);
        return;
    }

    D(std::cout << "function process_set_id"
                << "    c_str: " << it->c_str() << std::endl
                << "    size : " << it->size() << std::endl); 


    it_n2n->second->set_id(it->c_str(), it->size());

    // format: 205,set_id,NODE_NAME,IDENTIFIER
    snprintf(result, sizeof(result), "%s,set_id,%s,%s\n",
            SUCCEEDED_SET_ID, esc_node_name.c_str(), it->c_str());
    send(sockfd, result, strlen(result), 0);
}

void
process_get_id(int sockfd, esc_tokenizer::iterator &it,
               const esc_tokenizer::iterator &end)
{
    name2node_type::iterator it_n2n;
    std::string node_name;
    std::string esc_node_name;
    char result[1024 * 64];

    // read node_name
    if (it == end || it->length() == 0) {
        // there is no port number
        // format: 401,COMMENT
        snprintf(result, sizeof(result),
                "401,node name is required\n");
        send(sockfd, result, strlen(result), 0);
        return;
    }

    node_name = *it;
    esc_node_name = *it;
    replace(esc_node_name, ",", "\\,");


    it_n2n = name2node.find(node_name);
    if (it_n2n == name2node.end()) {
        // invalid node name
        // format: 404,delete,NODE_NAME,COMMENT
        snprintf(result, sizeof(result),
                "%s,delete,%s,no such node named '%s'\n",
                ERR_DEL_NO_SUCH_NODE, esc_node_name.c_str(),
                esc_node_name.c_str());
        send(sockfd, result, strlen(result), 0);
        return;
    }

    //if get id is binary
    /*
    uint8_t addr[CAGE_ID_LEN];
    it_n2n->second->get_id(addr);
    std::vector<uint8_t> v;
    v.assign(addr,addr+sizeof(addr));
    std::string addr_str;
    base64::encode(v, addr_str);
    */

    //if get id is std::string (HEX)
    std::string addr_str = it_n2n->second->get_id_str();

    D(std::cout << "    node_name: " << esc_node_name <<std::endl);
    D(std::cout << "    node_id  : " << addr_str <<std::endl);

    // format: 208,get_id,NODE_NAME,IDENTIFIER
    snprintf(result, sizeof(result), "%s,get_id,%s,%s\n",
            SUCCEEDED_GET_ID, esc_node_name.c_str(), addr_str.c_str());
    send(sockfd, result, strlen(result), 0);
}

void
process_new(int sockfd, esc_tokenizer::iterator &it,
            const esc_tokenizer::iterator &end)
{
    std::string node_name;
    std::string esc_node_name;

    int port;
    char result[1024 * 64];
    bool is_global = false;

    // read node_name
    if (it == end || it->length() == 0) {
        // there is no port number
        // format: 401,COMMENT
        snprintf(result, sizeof(result),
                "401,node name is required\n");
        send(sockfd, result, strlen(result), 0);
        return;
    }

    node_name = *it;
    esc_node_name = *it;
    replace(esc_node_name, ",", "\\,");
    ++it;


    // read port number
    if (it == end) {
        // there is no port number
        // format: 401,COMMENT
        snprintf(result, sizeof(result),
                "401,port number is required\n");
        send(sockfd, result, strlen(result), 0);
        return;
    }

    try {
        port = boost::lexical_cast<int>(*it);
    } catch (boost::bad_lexical_cast) {
        // no integer
        // format: 401,COMMENT
        std::string esc_port = *it;
        replace(esc_port, ",", "\\,");
        snprintf(result, sizeof(result),
                "401,'%s' is not a valid number\n",
                esc_port.c_str());
        send(sockfd, result, strlen(result), 0);
        return;
    }

    ++it;


    // read option
    if (it != end) {
        if (*it == "global")
            is_global = true;
    }

    D(std::cout << "    node_name: " << node_name << "\n"
                << "    port     : " << port << "\n"
                << "    is_global: " << is_global << std::endl);

    // check whether the node_name has been used already or not
    if (name2node.find(node_name) != name2node.end()) {
        // the node name has been already used
        // format: 403,new,NODE_NAME,PORT_NUMBER,COMMENT
        snprintf(result, sizeof(result),
                "%s,new,%s,%d,the node name '%s' exists already\n",
                ERR_ALREADY_EXIST, esc_node_name.c_str(), port,
                esc_node_name.c_str());
        send(sockfd, result, strlen(result), 0);
        return;
    }

    boost::shared_ptr<libcage::cage> c(new libcage::cage);

    // set timeout limit
    c->rdp_set_max_retrans(16);

    // open port
    if (c->open(PF_INET, port) == false) {
        // cannot open port
        // format: 402,new,NODE_NAME,PORT_NUMBER,COMMENT
        snprintf(result, sizeof(result),
                "%s,new,%s,%d,cannot open port(%d)\n",
                ERR_CANNOT_OPEN_PORT, esc_node_name.c_str(),
                port, port);
        send(sockfd, result, strlen(result), 0);
        return;
    }

    // set as global
    if (is_global) {
        c->set_global();
    }

    name2node[node_name] = c;

    // send result
    // format: 200,new,NODE_NAME,PORT_NUMBER
    snprintf(result, sizeof(result), "%s,new,%s,%d\n", 
            SUCCEEDED_NEW, esc_node_name.c_str(), port);

    send(sockfd, result, strlen(result), 0);

    return;
}

void
process_delete(int sockfd, esc_tokenizer::iterator &it,
               const esc_tokenizer::iterator &end)
{
    name2node_type::iterator it_n2n;
    std::string node_name;
    std::string esc_node_name;

    char result[1024 * 64];


    if (it == end) {
        // there is no node_name
        // format: 401,COMMENT
        snprintf(result, sizeof(result),
                "401,node name is required\n");
        send(sockfd, result, strlen(result), 0);
        return;
    }

    node_name = *it;
    esc_node_name = *it;
    replace(esc_node_name, ",", "\\,");

    it_n2n = name2node.find(node_name);
    if (it_n2n == name2node.end()) {
        // invalid node name
        // format: 404,delete,NODE_NAME,COMMENT
        snprintf(result, sizeof(result),
                "%s,delete,%s,no such node named '%s'\n",
                ERR_DEL_NO_SUCH_NODE, esc_node_name.c_str(),
                esc_node_name.c_str());
        send(sockfd, result, strlen(result), 0);
        return;
    }

    D(std::cout << "    node_name: " << node_name << std::endl);

    name2node.erase(it_n2n);

    // send result
    // format: 201,delete,NODE_NAME
    snprintf(result, sizeof(result),
            "%s,delete,%s\n",
            SUCCEEDED_DELETE, esc_node_name.c_str());
    send(sockfd, result, strlen(result), 0);
}

class func_join
{
public:
    std::string esc_node_name;
    std::string esc_host;
    int         port;
    int         sockfd;

    void operator() (bool is_join) {
        sock2ev_type::iterator it;
        char result[1024 * 64];

        it = sock2ev.find(sockfd);
        if (it == sock2ev.end()) {
            return;
        }

        if (is_join) {
            // send result of the success
            // format: 202,join,NODE_NAME,HOST,PORT
            snprintf(result, sizeof(result),
                    "%s,join,%s,%s,%d\n",
                    SUCCEEDED_JOIN, esc_node_name.c_str(),
                    esc_host.c_str(), port);
            send(sockfd, result, strlen(result), 0);
        } else {
            // send result of the fail
            // format: 406,join,NODE_NAME,HOST,PORT,COMMENT
            snprintf(result, sizeof(result),
                    "%s,join,%s,%s,%d,failed in connecting to '%s:%d'\n",
                    ERR_JOIN_FAILED, esc_node_name.c_str(),
                    esc_host.c_str(), port, esc_host.c_str(),
                    port);
            send(sockfd, result, strlen(result), 0);
        }
    }
};

void process_join(int sockfd, esc_tokenizer::iterator &it,
                  const esc_tokenizer::iterator &end)
{
    func_join   func;
    std::string node_name;
    std::string esc_node_name;
    std::string host;
    std::string esc_host;

    int port;
    char result[1024 * 64];

    name2node_type::iterator it_n2n;

    if (it == end) {
        // there is no node_name
        // format: 401,COMMENT
        snprintf(result, sizeof(result),
                "401,node name is required\n");
        send(sockfd, result, strlen(result), 0);
        return;
    }

    node_name = *it;
    esc_node_name = node_name;
    replace(esc_node_name, ",", "\\,");

    ++it;
    if (it == end) {
        // there is no host
        // format: 401,COMMENT
        snprintf(result, sizeof(result),
                "401,host is required\n");
        send(sockfd, result, strlen(result), 0);
        return;
    }

    host = *it;
    esc_host = host;
    replace(esc_host, ",", "\\,");

    ++it;
    if (it == end) {
        // there is no port number
        // format: 401,COMMENT
        snprintf(result, sizeof(result),
                "401,port number is required\n");
        send(sockfd, result, strlen(result), 0);
        return;
    }


    try {
        port = boost::lexical_cast<int>(*it);
    } catch (boost::bad_lexical_cast) {
        // no integer
        // format: 401,COMMENT
        std::string esc_port = *it;
        replace(esc_port, ",", "\\,");
        snprintf(result, sizeof(result),
                "401,'%s' is not a valid number\n",
                esc_port.c_str());
        send(sockfd, result, strlen(result), 0);
        return;
    }

    it_n2n = name2node.find(node_name);
    if (it_n2n == name2node.end()) {
        // invalid node name
        // format: 405,join,NODE_NAME,HOST,PORT,COMMENT
        snprintf(result, sizeof(result),
                "%s,join,%s,%s,%d,no such node named '%s'\n",
                ERR_JOIN_NO_SUCH_NODE, esc_node_name.c_str(),
                host.c_str(), port, esc_node_name.c_str());
        send(sockfd, result, strlen(result), 0);
        return;
    }

    D(std::cout << "    node_name: " << node_name << "\n"
                << "    host     : " << host << "\n"
                << "    port     : " << port << std::endl);

        func.esc_node_name = esc_node_name;
    func.esc_host      = esc_host;
    func.port          = port;
    func.sockfd        = sockfd;

    // join to the host:port
    it_n2n->second->join(host, port, func);
}

void process_put(int sockfd, esc_tokenizer::iterator &it,
        const esc_tokenizer::iterator &end)
{
    std::string node_name;
    std::string esc_node_name;
    std::string key, value;
    std::string esc_key, esc_value;

    uint16_t ttl;
    bool is_unique = false;
    char result[1024 * 64];

    if (it == end) {
        // there is no node_name
        // format: result,401,COMMENT
        snprintf(result, sizeof(result),
                "401,node name is required\n");
        send(sockfd, result, strlen(result), 0);
        return;
    }

    node_name = *it;
    esc_node_name = node_name;
    replace(esc_node_name, ",", "\\,");


    ++it;
    if (it == end) {
        // there is no key
        // format: 401,COMMENT
        snprintf(result, sizeof(result),
                "401,key is required\n");
        send(sockfd, result, strlen(result), 0);
        return;
    }

    key = *it;
    esc_key = key;
    replace(esc_key, ",", "\\,");


    ++it;
    if (it == end) {
        // there is no value
        // format: result,401,COMMENT
        snprintf(result, sizeof(result),
                "401,value is required\n");
        send(sockfd, result, strlen(result), 0);
        return;
    }

    value = *it;
    esc_value = value;
    replace(esc_value, ",", "\\,");


    ++it;
    if (it == end) {
        // there is no ttl
        // format: 401,COMMENT
        snprintf(result, sizeof(result),
                "401,ttl is required\n");
        send(sockfd, result, strlen(result), 0);
        return;
    }


    try {
        ttl = boost::lexical_cast<uint16_t>(*it);
    } catch (boost::bad_lexical_cast) {
        // no integer
        // format: 401,COMMENT
        std::string esc_ttl = *it;
        replace(esc_ttl, ",", "\\,");
        snprintf(result, sizeof(result),
                "401,'%s' is not a valid number\n",
                esc_ttl.c_str());
        send(sockfd, result, strlen(result), 0);
        return;
    }

    ++it;
    if (it != end && *it == "unique")
        is_unique = true;


    name2node_type::iterator it_n2n;
    it_n2n = name2node.find(node_name);
    if (it_n2n == name2node.end()) {
        // invalid node name
        // format: 407,put,NODE_NAME,KEY,VALUE,TTL,COMMENT
        snprintf(result, sizeof(result),
                "%s,put,%s,%s,%s,%d,no such node named '%s'\n",
                ERR_NO_SUCH_NODE, esc_node_name.c_str(),
                esc_key.c_str(), esc_value.c_str(), ttl,
                esc_node_name.c_str());
        send(sockfd, result, strlen(result), 0);
        return;
    }

    it_n2n->second->put(key.c_str(), key.length(),
            value.c_str(), value.length(), ttl, is_unique);

    D(std::cout << "    node_name: " << node_name << std::endl
                << "    key      : " << key << std::endl
                << "    value    : " << value << std::endl
                << "    TTL      : " << ttl << std::endl);


    // send result of the success
    if (is_unique) {
        // format: 203,put,NODE_NAME,KEY,VALUE,TTL,unique
        snprintf(result, sizeof(result),
                "%s,put,%s,%s,%s,%d,unique\n",
                SUCCEEDED_PUT, esc_node_name.c_str(),
                esc_key.c_str(), esc_value.c_str(), ttl);
    } else {
        // format: 203,put,NODE_NAME,KEY,VALUE,TTL
        snprintf(result, sizeof(result),
                "%s,put,%s,%s,%s,%d\n",
                SUCCEEDED_PUT, esc_node_name.c_str(),
                esc_key.c_str(), esc_value.c_str(), ttl);
    }

    //fprintf(stderr, "cage_put:%lu:%s\n", strlen(result), result);
    send(sockfd, result, strlen(result), 0);
}

class func_get
{
public:
    std::string esc_node_name;
    std::string esc_key;
    int         sockfd;

    void operator() (bool is_get, libcage::dht::value_set_ptr vset)
    {
        sock2ev_type::iterator it;
        char                   result[1024 * 1024];

        it = sock2ev.find(sockfd);
        if (it == sock2ev.end()) {
            return;
        }

        if (is_get) {
            std::string values;
            libcage::dht::value_set::iterator it;

            for (it = vset->begin(); it != vset->end(); ++it) {
                boost::shared_array<char> tmp(new char[it->len + 1]);
                std::string value;

                memcpy(tmp.get(), it->value.get(), it->len);

                tmp[it->len] = '\0';

                value = tmp.get();
                replace(value, ",", "\\,");

                values += ",";
                values += value;
            }

            // send value
            // format: 204,get,NODE_NAME,KEY,VALUE
            snprintf(result,
                    sizeof(result),
                    "%s,get,%s,%s%s\n",
                    SUCCEEDED_GET,
                    esc_node_name.c_str(),
                    esc_key.c_str(),
                    values.c_str());

            send(sockfd, result, strlen(result), 0);
            return;

        } else {
            // send result of fail
            // format: 409,get,NODE_NAME,KEY
            snprintf(result,
                    sizeof(result),
                    "%s,get,%s,%s\n",
                    ERR_GET_FAILURE,
                    esc_node_name.c_str(),
                    esc_key.c_str());

            send(sockfd, result, strlen(result), 0);
        }
    }
};

void process_get(int sockfd, esc_tokenizer::iterator &it,
                 const esc_tokenizer::iterator &end)
{
    func_get    func;
    std::string node_name;
    std::string esc_node_name;
    std::string key;
    std::string esc_key;

    char result[1024 * 64];

    if (it == end) {
        // there is no node_name
        // format: 401,COMMENT
        snprintf(result, sizeof(result),
                "401,node name is required\n");
        send(sockfd, result, strlen(result), 0);
        return;
    }

    node_name = *it;
    esc_node_name = node_name;
    replace(esc_node_name, ",", "\\,");


    ++it;
    if (it == end) {
        // there is no key
        // format: 401,COMMENT
        snprintf(result, sizeof(result),
                "401,key is required\n");
        send(sockfd, result, strlen(result), 0);
        return;
    }

    key = *it;
    esc_key = key;
    replace(esc_key, ",", "\\,");


    name2node_type::iterator it_n2n;
    it_n2n = name2node.find(node_name);
    if (it_n2n == name2node.end()) {
        // invalid node name
        // format: 408,NODE_NAME,get,KEY,COMMENT
        snprintf(result, sizeof(result),
                "%s,get,%s,%s,no such node named '%s'\n",
                ERR_NO_SUCH_NODE, esc_node_name.c_str(),
                esc_key.c_str(), esc_node_name.c_str());
        send(sockfd, result, strlen(result), 0);
        return;
    }

    D(std::cout << "    node_name: " << node_name << "\n"
            << "    key: " << key << std::endl);

    func.esc_node_name = esc_node_name;
    func.esc_key       = esc_key;
    func.sockfd        = sockfd;
    it_n2n->second->get(key.c_str(), key.length(), func);
}

class func_rdp_listen
{
public:
    func_rdp_listen* dup_instance;
    int nsockfd; // listen socket
    int connfd;  // connection socket rdp_liste <-> ./un
    int instance_identifier;
    int listen_desc;
    uint8_t node_id[CAGE_ID_LEN];
    std::string esc_node_id;
    std::string esc_node_name;
    std::string esc_sock_name;
    std::string esc_rdp_port;
    libcage::cage &m_cage;

    func_rdp_listen(libcage::cage &c) : m_cage(c) { }
    void operator() (int desc, libcage::rdp_addr addr, libcage::rdp_event event);
};

void
func_rdp_listen::operator() (int desc, libcage::rdp_addr addr, libcage::rdp_event event)
{
    D(std::cout << "function func_rdp_listen" << std::endl
                << "    node_name : " << esc_node_name << std::endl);

    switch (event) {
        case libcage::CONNECTED:
        {
            // dont use in func_rdp_listen.
            D(std::cout << "    CONNECTED" << std::endl);
            break;
        }
        case libcage::ACCEPTED:
        {
            D(std::cout << "    ACCEPT"
                        << std::endl
                        << "    accept_desc: "
                        << desc
                        << std::endl
                        << "    own_addr   : "
                        << esc_node_id
                        << std::endl
                        << "    peer_addr  : "
                        << addr.did->to_string()
                        << std::endl);

            id_mapper.insert(std::pair<int,int>(instance_identifier, desc));

            struct _long_header  l_header;
            memset(&l_header, 0, sizeof(l_header));
            l_header.f_type = F_RDP_LISTEN_B2T;
            l_header.m_type = M_RDP_ACCEPT;
            l_header.descriptor = desc;
            addr.did->to_binary(l_header.peer_addr, CAGE_ID_LEN);
            memcpy(l_header.own_addr, node_id, CAGE_ID_LEN);
            send(connfd, &l_header, sizeof(l_header), 0);
            break;
        }
        case libcage::READY2READ:
        {
            int len;
            char recv_buf[1024 * 64];
            char send_buf[1024 * 64 + sizeof(struct _long_header)];
            char* ptr_send_buf = send_buf;

            len = sizeof(recv_buf);
            memset(recv_buf, 0, sizeof(recv_buf));
            memset(send_buf, 0, sizeof(send_buf));

            m_cage.rdp_receive(desc, recv_buf, &len);

            D(std::cout << "    READY2READ"
                        << std::endl
                        << "    desc: "
                        << desc
                        << std::endl
                        << "    len : "
                        << len
                        << std::endl
                        << "    buf : "
                        << recv_buf
                        << std::endl);

            if (len == 0) {
                id_mapper_fixed_erase(instance_identifier, desc);
                m_cage.rdp_close(desc);
                break;
            }

            struct _short_header s_header;
            s_header.f_type = F_RDP_LISTEN_B2T;
            s_header.m_type = M_RDP_DATA;
            s_header.descriptor = desc;

            int send_len = len + sizeof(s_header);
            memcpy(ptr_send_buf, &s_header, sizeof(s_header));
            memcpy(ptr_send_buf+sizeof(s_header), recv_buf, len);
            send(connfd, send_buf, send_len, 0);

            /* string header send
            const char* ptr_peer_address = addr.did->to_string().c_str();
            const char* ptr_own_address = esc_node_id.c_str();
            int addr_len = strlen(ptr_peer_address);
            int send_len = len + addr_len + addr_len + 4;
            memcpy(ptr_send_buf, ptr_peer_address, addr_len);
            memcpy(ptr_send_buf+addr_len, ptr_own_address, addr_len);
            memcpy(ptr_send_buf+addr_len+addr_len, &desc, 4);
            memcpy(ptr_send_buf+addr_len+addr_len+4, recv_buf, len);
            */

            break;
        }
        case libcage::BROKEN:
        {
            // retransmit failed phase
            D(std::cout << "    BROKEN" << std::endl
                        << "    close desc: " << desc << std::endl);
            id_mapper_fixed_erase(instance_identifier, desc);

            struct _long_header  l_header;
            l_header.f_type = F_RDP_LISTEN_B2T;
            l_header.m_type = M_RDP_CLOSED;
            l_header.descriptor = desc;
            addr.did->to_binary(l_header.peer_addr, CAGE_ID_LEN);
            memcpy(l_header.own_addr, node_id, CAGE_ID_LEN);
            send(connfd, &l_header, sizeof(l_header), 0);
            m_cage.rdp_close(desc);
            break;
        }
        case libcage::RESET:
        {
            // CLOSE phase
            D(std::cout << "    RESET" << std::endl
                        << "    close desc: " << desc << std::endl);
            id_mapper_fixed_erase(instance_identifier, desc);

            struct _long_header  l_header;
            l_header.f_type = F_RDP_LISTEN_B2T;
            l_header.m_type = M_RDP_CLOSED;
            l_header.descriptor = desc;
            addr.did->to_binary(l_header.peer_addr, CAGE_ID_LEN);
            memcpy(l_header.own_addr, node_id, CAGE_ID_LEN);
            send(connfd, &l_header, sizeof(l_header), 0);
            m_cage.rdp_close(desc);
            break;
        }
        case libcage::FAILED:
        {
            // SYN_SENT & SYN_RCVD timeout phase
            D(std::cout << "    FAILED" << std::endl
                        << "    close desc: " << desc << std::endl);
            id_mapper_fixed_erase(instance_identifier, desc);

            struct _long_header  l_header;
            l_header.f_type = F_RDP_LISTEN_B2T;
            l_header.m_type = M_RDP_TIMEOUT;
            l_header.descriptor = desc;
            addr.did->to_binary(l_header.peer_addr, CAGE_ID_LEN);
            memcpy(l_header.own_addr, node_id, CAGE_ID_LEN);
            send(connfd, &l_header, sizeof(l_header), 0);
            m_cage.rdp_close(desc);
            break;
        }
        default:
        {
            D(std::cout << "    default" << std::endl);
            break;
        }
    }
}


void process_rdp_listen(int sockfd, esc_tokenizer::iterator &it,
                        const esc_tokenizer::iterator &end)
{
    std::string node_name;
    std::string esc_node_name;
    std::string sock_name;
    std::string esc_sock_name;
    std::string rdp_port;
    std::string esc_rdp_port;

    char result[1024 * 64];

    if (it == end) {
        // there is no node_name
        // format: 401,COMMENT
        snprintf(result, sizeof(result), "401,node name is required\n");
        send(sockfd, result, strlen(result), 0);
        return;
    }

    node_name = *it;
    esc_node_name = node_name;
    replace(esc_node_name, ",", "\\,");

    ++it;
    if (it == end) {
        // there is no socket_name
        // format: 401,COMMENT
        snprintf(result, sizeof(result), "401,socket name is required\n");
        send(sockfd, result, strlen(result), 0);
        return;
    }

    sock_name = *it;
    esc_sock_name = sock_name;
    replace(esc_sock_name, ",", "\\,");

    ++it;
    if (it == end) {
        // there is no rdp_port
        // format: 401,COMMENT
        snprintf(result, sizeof(result), "401,rdp port is required\n");
        send(sockfd, result, strlen(result), 0);
        return;
    }

    rdp_port = *it;
    esc_rdp_port = rdp_port;
    replace(esc_rdp_port, ",", "\\,");

    // -- processing part
    // get node name in node list
    name2node_type::iterator it_n2n;
    it_n2n = name2node.find(node_name);

    if (it_n2n == name2node.end()) {
        // invalid node name
        // format: 408,NODE_NAME,get,KEY,COMMENT
        snprintf(result, sizeof(result),
                "%s,rdp_listen,%s,%s,%s,no such node named '%s'\n",
                ERR_NO_SUCH_NODE, esc_node_name.c_str(),
                esc_sock_name.c_str(), esc_rdp_port.c_str(),
                esc_node_name.c_str());

        send(sockfd, result, strlen(result), 0);
        return;
    }

    // named_socket create..
    int nsockfd;
    nsockfd = create_named_socket(esc_sock_name.c_str());
    if (nsockfd == -1) {
        snprintf(result, sizeof(result),
                "401,can't use %s\n", esc_sock_name.c_str());
        send(sockfd, result, strlen(result), 0);
        return;
    }

    // dispatch processing in libcage
    // XXX
    //boost::shared_ptr<func_rdp_listen> opaque(new func_rdp_listen(*it_n2n->second.get()));
    func_rdp_listen* opaque = new func_rdp_listen(*it_n2n->second.get());
   
    opaque->dup_instance = opaque;
    it_n2n->second->get_id(opaque->node_id);
    opaque->esc_node_id         = it_n2n->second->get_id_str();
    opaque->esc_node_name       = esc_node_name;
    opaque->esc_sock_name       = esc_sock_name;
    opaque->esc_rdp_port        = esc_rdp_port;
    opaque->nsockfd             = nsockfd;
    opaque->listen_desc         = 0;
    opaque->instance_identifier = instance_identifier;
    instance_identifier++;

    // -- debug message --
    D(std::cout << "    node_name   : " << node_name << std::endl
                << "    sock_name   : " << sock_name << std::endl
                << "    rdp_own_port: " << rdp_port  << std::endl);

    // set to ev for writing event callback
    boost::shared_ptr<event> readev(new event);
    sock2ev[nsockfd] = readev;
    event_set(readev.get(), nsockfd, EV_READ | EV_PERSIST,
              &callback_lsock_accept, (void*)opaque);
    event_add(readev.get(), NULL);

    // format: 206,rdp_listen,NODE_NAME,SOCK_NAME,PORT_NUMBER
    snprintf(result, sizeof(result),
            "%s,rdp_listen,%s,%s,%s\n",
            SUCCEEDED_RDP_LISTEN, esc_node_name.c_str(),
            esc_sock_name.c_str(), esc_rdp_port.c_str());
    send(sockfd, result, strlen(result), 0);
    return;
}

void callback_lsock_accept(int fd, short ev, void* arg)
{
    func_rdp_listen* opaque;
    opaque = (func_rdp_listen*)arg;

    if (ev == EV_READ) {
        struct sockaddr_storage sa_storage;
        socklen_t sa_len = sizeof(sa_storage);
        memset(&sa_storage, 0, sizeof(sa_storage));

        int conn_fd;
        conn_fd = accept(fd, (struct sockaddr*)&sa_storage, &sa_len);
        opaque->connfd = conn_fd;

        uint16_t num_port = atoi(opaque->esc_rdp_port.c_str());
        opaque->listen_desc = opaque->m_cage.rdp_listen(num_port, *opaque);
        id_mapper.insert(std::pair<int,int>(opaque->instance_identifier, opaque->listen_desc));

        D(std::cout << "function callback_lsock_accept" 
                    << std::endl
                    << "    node_name      : "
                    << opaque->esc_node_name
                    << std::endl
                    << "    node_id        : "
                    << opaque->esc_node_id
                    << std::endl
                    << "    rdp_peer_port  : "
                    << opaque->esc_rdp_port
                    << std::endl
                    << "    rdp_listen_desc: "
                    << opaque->listen_desc
                    << std::endl);


        // -- close listen socket --
        event_del(sock2ev[opaque->nsockfd].get());
        sock2ev.erase(opaque->nsockfd);
        shutdown(opaque->nsockfd, SHUT_RDWR);
        close(opaque->nsockfd);
        unlink(opaque->esc_sock_name.c_str());

        boost::shared_ptr<event> readev(new event);
        sock2ev[conn_fd] = readev;

        event_set(readev.get(), conn_fd, EV_READ | EV_PERSIST,
                  &callback_lsock_read, (void*)opaque);
        event_add(readev.get(), NULL);
    } 
    return;
}

void callback_lsock_read(int fd, short ev, void* arg)
{
    func_rdp_listen* opaque;
    opaque = (func_rdp_listen*)arg;

    D(std::cout << "function callback_lsock_read" << "\n"
                << "    node_name      : " << opaque->esc_node_name << "\n"
                << "    rdp_listen_port: " << opaque->esc_rdp_port << "\n" 
                << "    listen_desc    : " << opaque->listen_desc << std::endl);

    char buf[1024 * 64];
    memset(buf, 0, sizeof(buf));

    int rsize;
    rsize = recv(fd, buf, sizeof(buf), 0);

    if (rsize > 0) {
        // -- protocol processing --
        uint16_t* ptr;
        ptr = (uint16_t*)buf;
        uint16_t f_type = *ptr;
        ptr++;
        uint16_t m_type = *ptr;
        ptr++;
        uint32_t desc = *(uint32_t*)ptr;
        D(printf("    accept_desc    : %d\n", desc));

        if (m_type == M_RDP_DATA) {
            // -- data type --
            rsize = rsize - sizeof(struct _short_header);
            char* sbuf = buf + sizeof(struct _short_header);
            int retval = opaque->m_cage.rdp_send(desc, sbuf, rsize);
            if (retval >= 0) {
                D(printf("    rdp_send success\n"));
            } else {
                D(printf("    rdp_send failed\n"));
            }
        } else {
            // -- message type --
            D(printf("    f_type:%s\n", trans_f_type(f_type)));
            D(printf("    m_type:%s\n", trans_m_type(m_type)));
            if (f_type == F_RDP_LISTEN_T2B && m_type == M_RDP_CLOSED) {
                // -- delete event descriptor of cage_rdp --
                struct _long_header* ptr_l_header = (struct _long_header*)buf;
                if (id_mapper_fixed_erase(opaque->instance_identifier, ptr_l_header->descriptor)) {
                    opaque->m_cage.rdp_close(ptr_l_header->descriptor);
                    ptr_l_header->f_type = F_RDP_LISTEN_B2T;
                    ptr_l_header->m_type = M_RDP_CLOSED;
                    //memcpy(ptr_l_header->peer_addr, opaque->peer_id, CAGE_ID_LEN);
                    //memcpy(ptr_l_header->own_addr, opaque->node_id, CAGE_ID_LEN);
                    send(opaque->connfd, ptr_l_header, sizeof(struct _long_header), 0);
                    D(printf("    Peer socket Close\n"));
                    D(printf("    rdp_message_send: M_RDP_CLOSED\n"));
                } else {
                    ptr_l_header->f_type = F_RDP_LISTEN_B2T;
                    ptr_l_header->m_type = M_RDP_ERROR;
                    //memcpy(ptr_l_header->peer_addr, opaque->peer_id, CAGE_ID_LEN);
                    //memcpy(ptr_l_header->own_addr, opaque->node_id, CAGE_ID_LEN);
                    send(opaque->connfd, ptr_l_header, sizeof(struct _long_header), 0);
                    D(printf("    rdp_message_send: M_RDP_ERROR\n"));
                }
            } else {
                ;
            }
        }

    } else {
        // peer socket close
        D(printf("    Peer socket Close\n"));
        std::pair<std::multimap<int,int>::iterator,std::multimap<int,int>::iterator> it;
        it = id_mapper.equal_range(opaque->instance_identifier);
        for (std::multimap<int,int>::iterator iter = it.first; iter != it.second; iter++) {
            opaque->m_cage.rdp_close(iter->second);
        }
        id_mapper.erase(opaque->instance_identifier);
        event_del(sock2ev[fd].get());
        sock2ev.erase(fd);
        shutdown(fd, SHUT_RDWR);
        close(fd);
        delete opaque->dup_instance;
    }
    return;
}

class func_rdp_connect
{
public:
    int nsockfd;      // listen socket
    int connfd;       // connection socket rdp_connect <-> un
    int connect_desc; // coonnect description
    uint8_t node_id[CAGE_ID_LEN];
    uint8_t peer_id[CAGE_ID_LEN];
    std::string esc_node_id;
    std::string esc_node_name;
    std::string esc_sock_name;
    std::string esc_rdp_port;
    std::string esc_rdp_addr;
    libcage::cage &m_cage;

    func_rdp_connect(libcage::cage &c) : m_cage(c) { }
    void operator() (int desc, libcage::rdp_addr addr, libcage::rdp_event ev);
    //func_rdp_connect &operator= (const func_rdp_connect& rhs);
};

/*
func_rdp_connect &operator= (const func_rdp_connect& rhs)
{
    nsockfd       = rhs.nsockfd;
    connfd        = rhs.connfd;
    connect_desc  = rhs.connect_desc;
    esc_node_name = rhs.esc_node_name;
    esc_sock_name = rhs.esc_sock_name;
    esc_rdp_port  = rhs.esc_rdp_port;
    esc_rdp_addr  = rhs.esc_rdp_addr;
    dup_instance  = rhs.this;
    memcpy(node_id, rhs.node_id, CAGE_ID_LEN);
    return *this;
}
*/

void
func_rdp_connect::operator() (int desc,
                              libcage::rdp_addr addr,
                              libcage::rdp_event ev)
{
    D(std::cout << "function func_rdp_connect" << std::endl
                << "    node_name    : " << esc_node_name << std::endl);
    switch (ev) {
        case libcage::CONNECTED:
        {
            D(std::cout << "    CONNECTED"
                        << std::endl
                        << "    connect_desc : "
                        << desc
                        << std::endl
                        << "    rdp_own_addr : "
                        << esc_node_id 
                        << std::endl
                        << "    rdp_peer_addr: "
                        << addr.did->to_string()
                        << std::endl);

            struct _long_header  l_header;
            l_header.f_type = F_RDP_CONNECT_B2T;
            l_header.m_type = M_RDP_CONNECT;
            l_header.descriptor = desc;
            addr.did->to_binary(l_header.peer_addr, CAGE_ID_LEN);
            memcpy(l_header.own_addr, node_id, CAGE_ID_LEN);
            send(connfd, &l_header, sizeof(l_header), 0);

            connect_desc = desc;
            addr.did->to_binary(peer_id, CAGE_ID_LEN);
            boost::shared_ptr<event> readev(new event);
            sock2ev[connfd] = readev;

            event_set(readev.get(), connfd, EV_READ | EV_PERSIST,
                      &callback_csock_read, this);
            event_add(readev.get(), NULL);
            break;
        }
        case libcage::ACCEPTED:
        {
            // dont be called by anywhere
            D(std::cout << "    ACCEPTED" << std::endl);
            break;
        }
        case libcage::READY2READ:
        {
            D(std::cout << "    READY2READ" << std::endl);
            // XXX
            int len;
            char recv_buf[1024 * 64];
            char send_buf[1024 * 64 + sizeof(struct _long_header)];
            char* ptr_send_buf = send_buf;

            len = sizeof(recv_buf);
            memset(recv_buf, 0, sizeof(recv_buf));
            memset(send_buf, 0, sizeof(send_buf));

            m_cage.rdp_receive(desc, recv_buf, &len);

            D(std::cout << "    READY2READ"
                        << std::endl
                        << "    desc: "
                        << desc
                        << std::endl
                        << "    len : "
                        << len
                        << std::endl
                        << "    buf : "
                        << recv_buf
                        << std::endl);

            if (len == 0) {
                struct _long_header  l_header;
                l_header.f_type = F_RDP_CONNECT_B2T;
                l_header.m_type = M_RDP_CLOSED;
                l_header.descriptor = desc;
                addr.did->to_binary(l_header.peer_addr, CAGE_ID_LEN);
                memcpy(l_header.own_addr, node_id, CAGE_ID_LEN);
                send(connfd, &l_header, sizeof(l_header), 0);

                event_del(sock2ev[connfd].get());
                sock2ev.erase(connfd);
                shutdown(connfd, SHUT_RDWR);
                close(connfd);
                m_cage.rdp_close(desc);
                break;
            }

            struct _short_header s_header;
            s_header.f_type = F_RDP_CONNECT_B2T;
            s_header.m_type = M_RDP_DATA;
            s_header.descriptor = desc;

            int send_len = len + sizeof(s_header);
            memcpy(ptr_send_buf, &s_header, sizeof(s_header));
            memcpy(ptr_send_buf+sizeof(s_header), recv_buf, len);
            send(connfd, send_buf, send_len, 0);

            /* string header send
            const char* ptr_peer_address = addr.did->to_string().c_str();
            const char* ptr_own_address = esc_node_id.c_str();
            int addr_len = strlen(ptr_peer_address);
            int send_len = len + addr_len + addr_len + 4;
            memcpy(ptr_send_buf, ptr_peer_address, addr_len);
            memcpy(ptr_send_buf+addr_len, ptr_own_address, addr_len);
            memcpy(ptr_send_buf+addr_len+addr_len, &desc, 4);
            memcpy(ptr_send_buf+addr_len+addr_len+4, recv_buf, len);
            */

            break;
        }
        case libcage::BROKEN:
        {
            D(std::cout << "    BROKEN"
                        << std::endl
                        << "    close conn_fd     : "
                        << connfd
                        << std::endl
                        << "    close connect_desc: "
                        << desc 
                        << std::endl);

            struct _long_header  l_header;
            l_header.f_type = F_RDP_CONNECT_B2T;
            l_header.m_type = M_RDP_CLOSED;
            l_header.descriptor = desc;
            addr.did->to_binary(l_header.peer_addr, CAGE_ID_LEN);
            memcpy(l_header.own_addr, node_id, CAGE_ID_LEN);
            send(connfd, &l_header, sizeof(l_header), 0);

            event_del(sock2ev[connfd].get());
            sock2ev.erase(connfd);
            shutdown(connfd, SHUT_RDWR);
            close(connfd);
            m_cage.rdp_close(desc);
            break;
        }
        case libcage::RESET:
        {
            D(std::cout << "    RESET"
                        << std::endl
                        << "    close conn_fd     : "
                        << connfd
                        << std::endl
                        << "    close connect_desc: "
                        << desc 
                        << std::endl);
            struct _long_header  l_header;
            l_header.f_type = F_RDP_CONNECT_B2T;
            l_header.m_type = M_RDP_CLOSED;
            l_header.descriptor = desc;
            addr.did->to_binary(l_header.peer_addr, CAGE_ID_LEN);
            memcpy(l_header.own_addr, node_id, CAGE_ID_LEN);
            send(connfd, &l_header, sizeof(l_header), 0);

            event_del(sock2ev[connfd].get());
            sock2ev.erase(connfd);
            shutdown(connfd, SHUT_RDWR);
            close(connfd);
            m_cage.rdp_close(desc);
            break;
        }
        case libcage::FAILED:
        {
            D(std::cout << "    FAILED"
                        << std::endl
                        << "    close conn_fd     : "
                        << connfd
                        << std::endl
                        << "    close connect_desc: "
                        << desc 
                        << std::endl);

            struct _long_header  l_header;
            l_header.f_type = F_RDP_CONNECT_B2T;
            l_header.m_type = M_RDP_TIMEOUT;
            l_header.descriptor = desc;
            addr.did->to_binary(l_header.peer_addr, CAGE_ID_LEN);
            memcpy(l_header.own_addr, node_id, CAGE_ID_LEN);
            send(connfd, &l_header, sizeof(l_header), 0);

            shutdown(connfd, SHUT_RDWR);
            close(connfd);
            m_cage.rdp_close(desc);
            break;
        }
        default:
        {
            D(std::cout << "    default" << std::endl);
        }
    }
}


void process_rdp_connect(int sockfd, esc_tokenizer::iterator &it,
                         const esc_tokenizer::iterator &end)
{
    std::string node_name;
    std::string esc_node_name;
    std::string sock_name;
    std::string esc_sock_name;
    std::string rdp_port;
    std::string esc_rdp_port;
    std::string rdp_addr;
    std::string esc_rdp_addr;

    char result[1024 * 64];

    if (it == end) {
        // there is no node_name
        // format: 401,COMMENT
        snprintf(result, sizeof(result), "401,node name is required\n");
        send(sockfd, result, strlen(result), 0);
        return;
    }

    node_name = *it;
    esc_node_name = node_name;
    replace(esc_node_name, ",", "\\,");

    ++it;
    if (it == end) {
        // there is no socket_name
        // format: 401,COMMENT
        snprintf(result, sizeof(result), "401,socket name is required\n");
        send(sockfd, result, strlen(result), 0);
        return;
    }

    sock_name = *it;
    esc_sock_name = sock_name;
    replace(esc_sock_name, ",", "\\,");

    ++it;
    if (it == end) {
        // there is no rdp_port
        // format: 401,COMMENT
        snprintf(result, sizeof(result), "401,rdp addr is required\n");
        send(sockfd, result, strlen(result), 0);
        return;
    }

    rdp_port = *it;
    esc_rdp_port = rdp_port;
    replace(esc_rdp_port, ",", "\\,");

    ++it;
    if (it == end) {
        // there is no rdp_addr
        // format: 401,COMMENT
        snprintf(result, sizeof(result), "401,rdp address is required\n");
        send(sockfd, result, strlen(result), 0);
        return;
    }

    rdp_addr = *it;
    esc_rdp_addr = rdp_addr;
    replace(esc_rdp_addr, ",", "\\,");

    // -- processing part
    // get node name in node list
    name2node_type::iterator it_n2n;
    it_n2n = name2node.find(node_name);
    if (it_n2n == name2node.end()) {
        // invalid node name
        // format: 408,NODE_NAME,sock_name,,COMMENT
        snprintf(result, sizeof(result),
                "%s,rdp_connect,%s,no such node named\n",
                ERR_NO_SUCH_NODE, esc_node_name.c_str());
        send(sockfd, result, strlen(result), 0);
        return;
    }

    // named_socket create..
    int nsockfd;
    nsockfd = create_named_socket(esc_sock_name.c_str());
    if (nsockfd == -1) {
        snprintf(result, sizeof(result),
                "401,can't use %s\n", esc_sock_name.c_str());
        send(sockfd, result, strlen(result), 0);
        return;
    }

    // -- debug message --
    D(std::cout << "    node_name: " << node_name << "\n"
                << "    sock_name: " << sock_name << "\n"
                << "    rdp_port : " << rdp_port  << std::endl);


    // dispatch processing in libcage
    // XXX
    //boost::shared_ptr<func_rdp_connect> opaque(new func_rdp_connect(*it_n2n->second.get()));
    func_rdp_connect* opaque = new func_rdp_connect(*it_n2n->second.get());
  
    it_n2n->second->get_id(opaque->node_id);
    opaque->esc_node_id   = it_n2n->second->get_id_str();
    opaque->esc_node_name = esc_node_name;
    opaque->esc_sock_name = esc_sock_name;
    opaque->esc_rdp_port  = esc_rdp_port;
    opaque->esc_rdp_addr  = esc_rdp_addr;
    opaque->nsockfd       = nsockfd;

    // set to ev for writing event callback
    boost::shared_ptr<event> readev(new event);
    sock2ev[nsockfd] = readev;
    event_set(readev.get(), nsockfd, EV_READ | EV_PERSIST,
              &callback_csock_accept, (void*)opaque);
    event_add(readev.get(), NULL);

    // format: 207,rdp_connect,NODE_NAME,SOCK_NAME,PORT_NUMBER
    snprintf(result, sizeof(result),
            "%s,rdp_connect,%s,%s,%s,%s\n",
            SUCCEEDED_RDP_CONNECT, esc_node_name.c_str(),
            esc_sock_name.c_str(), esc_rdp_port.c_str(),
            esc_rdp_addr.c_str());
    send(sockfd, result, strlen(result), 0);

    return;
}

void callback_csock_accept(int fd, short ev, void *arg)
{
    func_rdp_connect* opaque;
    opaque = (func_rdp_connect*)arg;

    if (ev == EV_READ) {

        /*
        std::vector<uint8_t>v;
        base64::decode(opaque->esc_rdp_addr, v);
        uint8_t* mem = (uint8_t*)malloc(v.size());
        uint8_t* seeker = mem;
        memset(mem, 0, v.size());

        std::vector<uint8_t>::const_iterator it;
        for (it = v.begin(); it != v.end(); ++it) {
            *seeker = *it;
            seeker++;
        }

        libcage::id_ptr id(new libcage::uint160_t);
        id->from_binary(mem, v.size());
        free(mem);
        */


        libcage::id_ptr cage_id(new libcage::uint160_t);
        cage_id->from_string(opaque->esc_rdp_addr);

        uint16_t rdp_d_port;
        rdp_d_port = atoi(opaque->esc_rdp_port.c_str());

        uint16_t rdp_s_port;
        rdp_s_port = 0;

        int conn_fd;
        struct sockaddr_storage sa_storage;
        socklen_t sa_len = sizeof(sa_storage);
        memset(&sa_storage, 0, sizeof(sa_storage));
        conn_fd = accept(fd, (struct sockaddr*)&sa_storage, &sa_len);
        opaque->connfd = conn_fd;
        event_del(sock2ev[opaque->nsockfd].get());
        sock2ev.erase(opaque->nsockfd);
        shutdown(opaque->nsockfd, SHUT_RDWR);
        close(opaque->nsockfd);
        unlink(opaque->esc_sock_name.c_str());

        // -- debug message --
        D(std::cout << "function callback_csock_accept" << "\n"
                    << "    node_name   : " << opaque->esc_node_name << "\n"
                    << "    node_id     : " << opaque->esc_node_id << "\n"
                    << "    rdp_dst_id  : " << cage_id->to_string() << "\n"
                    << "    rdp_dst_port: " << rdp_d_port << "\n"
                    << "    rdp_src_port: " << rdp_s_port << "\n"
                    << "    conn_fd     : " << conn_fd << std::endl);

        /*
        switch (opaque->m_cage.rdp_get_desc_state(opaque->connect_desc)) {
            case libcage::OPEN:
            {
                D(std::cout << "    libcage::OPEN" << std::endl);
                break;
            }
            case libcage::CLOSED:
            {
                D(std::cout << "    libcage::CLOSED" << std::endl);
                break;
            }
            case libcage::LISTEN:
            {
                D(std::cout << "    libcage::LISTEN" << std::endl);
                break;
            }
            case libcage::SYN_SENT:
            {
                D(std::cout << "    libcage::SYN_SENT" << std::endl);
                break;
            }
            case libcage::SYN_RCVD:
            {
                D(std::cout << "    libcage::SYN_RCVD" << std::endl);
                break;
            }
            case libcage::CLOSE_WAIT_PASV:
            {
                D(std::cout << "    libcage::CLOSE_WAIT_PASV" << std::endl);
                break;
            }
            case libcage::CLOSE_WAIT_ACTIVE:
            {
                D(std::cout << "    libcage::CLOSE_WAIT" << std::endl);
                break;
            }
            default:
            { 
                D(std::cout << "    libcage::STATU:default" << std::endl);
                //opaque->m_cage.rdp_close(opaque->connect_desc);
                return;
            }
        }
        */

        opaque->connect_desc = opaque->m_cage.rdp_connect(rdp_s_port, cage_id, rdp_d_port, *opaque);
        D(std::cout << "    rdp_connect_desc: " << opaque->connect_desc << std::endl);
        delete opaque;

        /*
        boost::shared_ptr<event> readev(new event);
        sock2ev[conn_fd] = readev;

        event_set(readev.get(), conn_fd, EV_READ | EV_PERSIST,
                  &callback_csock_read, (void*)opaque);
        event_add(readev.get(), NULL);
        */
    } 
    return;
}

void callback_csock_read(int fd, short ev, void *arg)
{
    func_rdp_connect* opaque;
    opaque = (func_rdp_connect*)arg;

    char buf[1024 * 64];
    memset(buf, 0, sizeof(buf));

    int rsize;
    rsize = recv(fd, buf, sizeof(buf), 0);

    // --- protocol processing ---
    // XXX
    rsize = rsize - sizeof(struct _short_header);
    char* sbuf = buf + sizeof(struct _short_header);
    // ---------------------------

    D(std::cout << "function callback_csock_read" << "\n"
                << "    node_name       : " << opaque->esc_node_name << "\n"
                << "    rdp_peer_port   : " << opaque->esc_rdp_port << "\n" 
                << "    rdp_perr_addr   : " << opaque->esc_rdp_addr << "\n"
                << "    rdp_connect_desc: " << opaque->connect_desc << std::endl);

    if (rsize > 0) {
        int retval = opaque->m_cage.rdp_send(opaque->connect_desc, sbuf, rsize);
        if (retval >= 0) {
            D(printf("    rdp_send success\n"));
        } else {
            D(printf("    rdp_send failed\n"));
        }
    } else {
        // peer socket close
        D(printf("    Peer socket Close\n"));
        event_del(sock2ev[fd].get());
        sock2ev.erase(fd);
        shutdown(fd, SHUT_RDWR);
        close(fd);
        opaque->m_cage.rdp_close(opaque->connect_desc);
    }
    return;
}

int create_named_socket(const char* path)
{
        int err = 0;

        int sock_fd;
        sock_fd = socket(AF_LOCAL, SOCK_STREAM, 0);

        if (sock_fd == -1) { 
            err = errno;
        } 

        if (err == 0) {
            int on = 1;
            int ret;
            ret = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));
            if (ret != 0) {
                err = errno;
            }
        }

        if (err == 0) {
            int ret;
            ret = bind_safe(sock_fd, path, false);
            if (ret != 0) {
                err = errno;
                close(sock_fd);
            }
        }

        if (err == 0) {
            if (listen(sock_fd, 10) < 0) {
                err = errno;
                close(sock_fd);
            }
        }

        if (err == 0) {
            return sock_fd;
        } else {
            errno = err;
            return -1;
        }
}

static bool id_mapper_fixed_erase(int key, int value)
{
    int ret_flag = false;
    std::pair<std::multimap<int,int>::iterator,std::multimap<int,int>::iterator> it;
    it = id_mapper.equal_range(key);
    for (std::multimap<int,int>::iterator iter = it.first; iter != it.second; iter++) {
        if (iter->second == value) {
            id_mapper.erase(iter);
            ret_flag = true;
        }
    }
    return ret_flag;
}

/*
static bool id_mapper_exist_match(int key, int value)
{
    std::pair<std::multimap<int,int>::iterator,std::multimap<int,int>::iterator> it;
    it = id_mapper.equal_range(key);
    for (std::multimap<int,int>::iterator iter = it.first; iter != it.second; iter++) {
        if (iter->second == value) {
            return true;
        }
    }
    return false;
}
*/

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

