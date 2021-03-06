#ifndef __PRISON_SOCKET_H__
#define __PRISON_SOCKET_H__

// -- memo --
// this source code has 2 class PrisonSockBuffer and PrisonSock
// prison socket's max frame size is 16384 bytes


#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "utils.h"
#include "category.h"
#include "cage/header.hpp"

#import <Cocoa/Cocoa.h>


@interface PrisonSockBuffer : NSObject
{
    int m_type;
    int handler;
    NSData* payload;
}

#ifdef __MACH__

@property (getter=m_type, setter=set_m_type:, assign, readwrite) int m_type;
@property (getter=handler, setter=set_handler:, assign, readwrite) int handler;
@property (getter=payload, setter=set_payload:, copy, readwrite) NSData* payload;

#else

- (int)m_type;
- (void)set_m_type:(int)set;
- (int)handler;
- (void)set_handler:(int)set;
- (NSData*)payload;
- (void)set_payload:(NSData*)set;

#endif

- (id)init;
- (void)dealloc;

@end


@implementation PrisonSockBuffer

#ifdef __MACH__

@synthesize m_type;
@synthesize handler;
@synthesize payload;

#else

- (int)m_type
{
    return m_type;
}

- (void)set_m_type:(int)set
{
    m_type = set;
    return;
}

- (int)handler;
{
    return handler;
}

- (void)set_handler:(int)set
{
    handler = set;
    return;
}

- (NSData*)payload
{
    return payload;
}

- (void)set_payload:(NSData*)set
{
    payload = [set copy];
    return;
}

#endif

- (id)init
{
    self = [super init];
    if (self != nil) {
        handler = 0;
        payload = nil;
    }
    return self;
}

- (void)dealloc;
{
    [super dealloc];
    [payload release];
    return;
}

@end


@interface PrisonSock : NSObject
{
    int sock_fd;
    int cage_fd;
    BOOL already_use;

    NSData*   own_addr_bin;
    NSString* own_addr_str;
    NSString* connect_port;
    NSString* listen_port;

    NSString* node_name;
    NSString* sock_path;
    NSString* work_dir;

    NSMutableDictionary* ps_handler;
}

// XXX setter いらない？
#ifdef __MACH__

@property (getter=sock_fd, setter=set_sock_fd:, assign, readwrite) int sock_fd;
@property (getter=cage_fd, setter=set_cage_fd:, assign, readwrite) int cage_fd;

#else

- (int)sock_fd;
- (void)set_sock_fd:(int)set;
- (int)cage_fd;
- (void)set_cage_fd:(int)set;

#endif

- (id)init;
- (void)dealloc;
- (void)set_node_name:(NSString*)node;
- (void)set_sock_path:(NSString*)nsock;
- (BOOL)ps_create;
- (BOOL)ps_close:(int)handler;
- (BOOL)ps_destroy;
- (BOOL)ps_listen:(NSString*)psport;
- (int)ps_connect:(NSString*)psid :(NSString*)psport;

- (NSString*)get_own_addr;
- (NSString*)get_peer_addr:(int)handler;

- (PrisonSockBuffer*)ps_recvfrom;
- (int)ps_sendto:(PrisonSockBuffer*)psbuf;

- (NSString*)ps_lookup:(NSString*)psname;

@end


@implementation PrisonSock

#ifdef __MACH__

@synthesize sock_fd;
@synthesize cage_fd;

#else

- (int)sock_fd
{
    return sock_fd;
}

- (void)set_sock_fd:(int)set
{
    sock_fd = set;
    return;
}

- (int)cage_fd
{
    return cage_fd;
}

- (void)set_cage_fd:(int)set
{
    cage_fd = set;
    return;
}

#endif

- (NSString*)ps_lookup:(NSString*)psname
{
    NSString* message;
    message   = [NSString stringWithFormat:@"get,%@,%@\n",
                                           node_name,
                                           psname];

    ssize_t rsize;
    char buf[1024 * 64];
    memset(buf, 0, sizeof(buf));

    rsize = send(cage_fd, [message UTF8String], [message length], 0);
    if (rsize == -1) {
        return nil;
    }

    rsize = recv(cage_fd, buf, sizeof(buf), 0);
    if (rsize == -1) {
        return nil;
    }

    NSString* element_string = [NSString stringWithUTF8String:buf];
    NSArray* element_array = [element_string componentsSeparatedByString:@","];

    if ([[element_array objectAtIndex:0] isEqualToString:@"204"]) {
        return [[element_array objectAtIndex:4] trim];
    } else {
        errno = ECONNREFUSED;
        return nil;
    }
}

- (NSString*)get_own_addr
{
    return own_addr_str;
}

- (NSString*)get_peer_addr:(int)handler
{
    return [ps_handler objectForKey:[NSNumber numberWithInt:handler]];
}

- (void)set_node_name:(NSString*)node
{
    [node_name release];
    node_name = [NSString stringWithString:node];
    return;
}

- (void)set_sock_path:(NSString*)nsock
{
    [sock_path release];
    sock_path = [NSString stringWithString:nsock];
    return;
}


- (BOOL)ps_listen:(NSString*)psport
{
    if (already_use) {
        errno = EISCONN;
        return NO;
    }

    // cant use following rdp_ports in this porgram.
    // because,, rdp_port 100 is cage_store_port and rdp_port 101 is cage_get_port.
    int psport_int = [psport intValue];
    if (psport_int == 0) {
        errno = EADDRNOTAVAIL;
        return NO;
    } else if (psport_int == 100) {
        errno = EADDRINUSE;
        return NO;
    } else if (psport_int == 101) {
        errno = EADDRINUSE;
        return NO;
    }

    // listen messsage format
    //rdp_listen,NODE_NAME,SOCK_NAME,RDP_DPORT
    NSString* sock_name;
    sock_name = [NSString stringWithFormat:@"%@psListen_%@",
                                           work_dir,
                                           psport];
    NSString* message;
    message   = [NSString stringWithFormat:@"rdp_listen,%@,%@,%@\n",
                                           node_name,
                                           sock_name,
                                           psport];

    ssize_t rsize;
    char buf[1024 * 64];
    memset(buf, 0, sizeof(buf));

    rsize = send(cage_fd, [message UTF8String], [message length], 0);
    if (rsize == -1) {
        return NO;
    }

    rsize = recv(cage_fd, buf, sizeof(buf), 0);
    if (rsize == -1) {
        return NO;
    }

    NSString* element_string = [NSString stringWithUTF8String:buf];
    NSArray* element_array = [element_string componentsSeparatedByString:@","];

    if ([[element_array objectAtIndex:0] isEqualToString:@"206"]) {
        // success : rdp_listen..
        struct sockaddr_un conn_request;
        memset(&conn_request, 0, sizeof(conn_request));
        sock_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
        if (sock_fd <= 0) { 
            sock_fd = 0;
            return NO;
        } 

        #ifdef __MACH__
        conn_request.sun_len = sizeof(conn_request);
        #endif
        conn_request.sun_family = AF_LOCAL;
        memcpy(conn_request.sun_path, [sock_name UTF8String], [sock_name length]);

        int ret;
        ret = connect(sock_fd, (struct sockaddr*)&conn_request, SUN_LEN(&conn_request));
        if (ret != 0) {
            close(sock_fd);
            sock_fd = 0;
            return NO;
        }
        already_use = YES;
        listen_port = [psport copy];
    } else {
        return NO;
    }

    return YES;
}

- (int)ps_connect:(NSString*)psid :(NSString*)psport
{
    // success return is prison_sock_handler_number.
    // failure return is -1, and check errno.

    if (already_use) {
        errno = EISCONN;
        return -1;
    }

    // cant use following rdp_ports in this porgram.
    // because,, rdp_port 100 is cage_store_port and rdp_port 101 is cage_get_port.
    int psport_int = [psport intValue];
    if (psport_int == 0) {
        errno = EADDRNOTAVAIL;
        return -1;
    } else if (psport_int == 100) {
        errno = EADDRINUSE;
        return -1;
    } else if (psport_int == 101) {
        errno = EADDRINUSE;
        return -1;
    }

    // psid is sha1 hashing digest valuse 
    // CAGE_ID_LEN
    if (psid == nil) {
        errno = EADDRNOTAVAIL;
        return -1;
    } else if (psport == nil) {
        errno = EADDRNOTAVAIL;
        return -1;
    } else if ([psid length] != 40) {
        errno = EADDRNOTAVAIL;
        return -1;
    } 

    // connect messsage format
    //rdp_connect,NODE_NAME,SOCK_NAME,RDP_DPORT,RDP_DADDR
    NSString* sock_name;
    sock_name = [NSString stringWithFormat:@"%@psConnect_%@_%@",
                                           work_dir,
                                           psport,
                                           psid];
    NSString* message;
    message   = [NSString stringWithFormat:@"rdp_connect,%@,%@,%@,%@\n",
                                           node_name,
                                           sock_name,
                                           psport,
                                           psid];

    //NSLog(@"%@", message);
    ssize_t rsize;
    char buf[1024 * 64];
    memset(buf, 0, sizeof(buf));

    rsize = send(cage_fd, [message UTF8String], [message length], 0);
    if (rsize == -1) {
        return -1;
    }

    rsize = recv(cage_fd, buf, sizeof(buf), 0);
    //NSLog(@"recv:%s\n", buf);
    if (rsize == -1) {
        return -1;
    }

    NSString* element_string = [NSString stringWithUTF8String:buf];
    NSArray* element_array = [element_string componentsSeparatedByString:@","];

    if ([[element_array objectAtIndex:0] isEqualToString:@"207"]) {
        // success : rdp_connect..
        struct sockaddr_un conn_request;
        memset(&conn_request, 0, sizeof(conn_request));
        sock_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
        if (sock_fd <= 0) { 
            sock_fd = 0;
            return -1;
        } 

        #ifdef __APPLE__
        conn_request.sun_len = sizeof(conn_request);
        #endif
        conn_request.sun_family = AF_LOCAL;
        memcpy(conn_request.sun_path, [sock_name UTF8String], [sock_name length]);

        int ret;
        ret = connect(sock_fd, (struct sockaddr*)&conn_request, SUN_LEN(&conn_request));
        if (ret != 0) {
            close(sock_fd);
            sock_fd = 0;
            return -1;
        }

        //printf("debug:send\n");
        memset(buf, 0, sizeof(buf));
        rsize = recv(sock_fd, buf, sizeof(buf), 0);
        if (rsize == -1) {
            close(sock_fd);
            sock_fd = 0;
            return -1;
        }

        //ref to cage/header.hpp of struct _short_header.
        uint16_t* ptr;
        ptr = (uint16_t*)buf;
        uint16_t f_type = *ptr;
        ptr++;
        uint16_t m_type = *ptr;
        ptr++;
        uint32_t handler = (uint32_t)*ptr;
        if (f_type == F_RDP_CONNECT_B2T && m_type == M_RDP_CONNECT) {
            //printf("debug:return handler\n");
            errno = 0;
            struct _long_header* lh = (struct _long_header*)buf;
            ssize_t s = sizeof(lh->peer_addr);
            [ps_handler setObject:[NSData dataWithBytes:lh->peer_addr length:s]
                           forKey:[NSNumber numberWithInt:handler]];
            already_use = YES;
            connect_port = [psport copy];
            return handler;
        } else if (f_type == F_RDP_CONNECT_B2T && m_type == M_RDP_TIMEOUT) {
            //printf("debug:RDP_TIMEOUT\n");
            close(sock_fd);
            sock_fd = 0;
            errno = ETIMEDOUT;
            return -1;
        } else if (f_type == F_RDP_CONNECT_B2T && m_type == M_RDP_ERROR) {
            //printf("debug:RDP_ERROR\n");
            close(sock_fd);
            sock_fd = 0;
            errno = ECONNREFUSED;
            return -1;
        } else if (f_type == F_RDP_CONNECT_B2T && m_type == M_RDP_CLOSED) {
            //printf("debug:RDP_CLOSED\n");
            close(sock_fd);
            sock_fd = 0;
            errno = ECONNREFUSED;
            return -1;
        }

    } else {
        // failure : cant rdp_connect..
        errno = ECONNREFUSED;
        return -1;
    }
    errno = ECONNREFUSED;
    return -1;
}

- (PrisonSockBuffer*)ps_recvfrom;
{
    // retval is ,,,
    // nil : exceptional
    // psb : handler : 1 - 0xFFFFFFFF
    //     : payload : nil or NSData

    PrisonSockBuffer* psb = [[PrisonSockBuffer new] autorelease];
    ssize_t rsize;
    char buf[sizeof(struct _short_header)];
    memset(buf, 0, sizeof(buf));
    rsize = recv(sock_fd, buf, sizeof(buf), 0);
    if (rsize <= 0) {
        return nil;
    } 
    struct _short_header* sh = (struct _short_header*)buf;

    char* extbuf = NULL;
    uint32_t extbuf_size = sh->m_size - sizeof(struct _short_header);
    extbuf = (char*)malloc(extbuf_size);
    rsize = recv(sock_fd, extbuf, extbuf_size, 0);
    if (rsize <= 0) {
        free(extbuf);
        return nil;
    } else if (rsize != extbuf_size) {
        printf("rsize : %d\n, extbuf_size : %d\n", (int)rsize, extbuf_size);
        exit(-1);
    }

    if (sh->f_type == F_RDP_CONNECT_B2T && sh->m_type != M_RDP_DATA) {
        [psb set_m_type:sh->m_type];
        [psb set_handler:sh->descriptor];
        [psb set_payload:nil];
        free(extbuf);
        return psb;
    } else if (sh->f_type == F_RDP_LISTEN_B2T && sh->m_type != M_RDP_DATA) { 
        if (sh->m_type == M_RDP_ACCEPT) {
            errno = 0;
            struct _addr_header* ah = (struct _addr_header*)extbuf;
            [ps_handler setObject:[NSData dataWithBytes:ah->peer_addr length:CAGE_ID_LEN]
                           forKey:[NSNumber numberWithInt:sh->descriptor]];
        }

        [psb set_m_type:sh->m_type];
        [psb set_handler:sh->descriptor];
        [psb set_payload:nil];
        free(extbuf);
        return psb;
    } else if (sh->m_type == M_RDP_DATA) {
        // M_RDP_DATA
        [psb set_m_type:sh->m_type];
        [psb set_handler:sh->descriptor];
        [psb set_payload:[NSData dataWithBytes:extbuf length:extbuf_size]];
        return psb;
    } else {
        free(extbuf);
        return nil;
    }
}

- (int)ps_sendto:(PrisonSockBuffer*)psbuf
{
    if (psbuf == nil) {
        return 0;
    }

    int desc = [psbuf handler];

    if ([ps_handler objectForKey:[NSNumber numberWithInt:desc]]) {
        // correct descriptor
        struct _short_header sh;
        sh.f_type = F_RDP_CONNECT_T2B;
        sh.m_type = M_RDP_DATA;
        sh.descriptor = desc;
        sh.m_size = sizeof(sh) + [[psbuf payload] length];

        id send_psbuf = [[NSMutableData new] autorelease];
        [send_psbuf appendBytes:&sh length:sizeof(sh)];
        [send_psbuf appendData:[psbuf payload]];

        ssize_t ssize = send(sock_fd, [send_psbuf bytes], [send_psbuf length], 0);
        int result = ssize - sizeof(struct _short_header);
        if (result <= 0) {
            return 0;
        } else {
            return result;
        }
    } else {
        // error descriptor
        return 0;
    }
}

- (BOOL)ps_close:(int)handler
{
    id peer = [ps_handler objectForKey:[NSNumber numberWithInt:handler]];
    if (peer != nil) {
        //printf("debug: in ps_close\n");
        if (connect_port != nil) {
            // connect socket
            //printf("debug: in connect\n");
            already_use = NO;
            if (sock_fd >= 0) {
                shutdown(sock_fd, SHUT_RDWR);
                close(sock_fd);
                sock_fd = 0;
            } 
            if (own_addr_bin != nil) {
                [own_addr_bin release];
                own_addr_bin = nil;
            }
            if (own_addr_str != nil) {
                [own_addr_str release];
                own_addr_str = nil;
            }
            if (connect_port != nil) {
                [connect_port release];
                connect_port =nil;
            }
            if (listen_port != nil) {
                [listen_port  release];
                listen_port = nil;
            }
            [ps_handler removeAllObjects];
            return YES;
        } else if (listen_port != nil) {
            // listen socket
            // TODO
            printf("ps_close listen side!!");
            ssize_t rsize;
            struct _long_header lh;
            lh.f_type = F_RDP_LISTEN_T2B;
            lh.m_type = M_RDP_CLOSED;
            lh.descriptor = handler;
            lh.m_size = sizeof(lh);
            memcpy(lh.peer_addr, [peer bytes], [peer length]);
            memcpy(lh.own_addr, [own_addr_bin bytes], [own_addr_bin length]);
            rsize = send(sock_fd, &lh, sizeof(lh), 0);
            if (rsize == -1) {
                errno = EBADF;
                return NO;
            }
            printf("    send close message!!");
            [ps_handler removeObjectForKey:[NSNumber numberWithInt:handler]];
            return YES;
            /*
            ps_close_listen_recv:
            memset(&lh, 0, sizeof(lh));
            rsize = recv(sock_fd, &lh, sizeof(lh), 0);
            printf("    recv closed message!!");
            if (rsize == -1) {
                errno = EBADF;
                return NO;
            }
            if (lh.f_type == F_RDP_LISTEN_B2T && lh.m_type == M_RDP_CLOSED) {
                [ps_handler removeObjectForKey:[NSNumber numberWithInt:handler]];
                return YES;
            } else if (lh.f_type == F_RDP_LISTEN_B2T && lh.m_type == M_RDP_ERROR) {
                errno = EBADF;
                return NO;
            } else {
                goto ps_close_listen_recv;
            }
            */
        }
    } else {
        errno = EBADF;
        return NO;
    }
    errno = EBADF;
    return NO;
}

-(BOOL)ps_destroy
{
    if (cage_fd <= 0) {
        return NO;
    }
    int retval;
    retval = close(cage_fd);
    if (retval == 0) {
        cage_fd = 0;
        if (sock_fd >= 0) {
            close(sock_fd);
            sock_fd = 0;
        } 
        already_use = NO;
        if (own_addr_bin != nil) {
            [own_addr_bin release];
            own_addr_bin = nil;
        }
        if (own_addr_str != nil) {
            [own_addr_str release];
            own_addr_str = nil;
        }
        if (connect_port != nil) {
            [connect_port release];
            connect_port =nil;
        }
        if (listen_port != nil) {
            [listen_port  release];
            listen_port = nil;
        }
        [ps_handler removeAllObjects];
        return YES;
    } else {
        return NO;
    }
}


- (id)init
{
    self = [super init];
    if (self != nil) {
        ps_handler = [NSMutableDictionary new];
        node_name = [NSString stringWithString:@"prison"];
        sock_path = [NSString stringWithString:@"/tmp/sock_cage"];
        work_dir  = [NSString stringWithString:@"/tmp/"];
        own_addr_bin = nil;
        own_addr_str = nil;
        connect_port = nil;
        listen_port = nil;
        sock_fd = 0;
        cage_fd = 0;
        already_use = NO;
    }
    return self;
}

- (void)dealloc;
{
    [super dealloc];
    [ps_handler release];
    [node_name release];
    [sock_path release];
    [work_dir  release];
    if (own_addr_str != nil) [own_addr_str release];
    if (own_addr_bin != nil) [own_addr_bin release];
    if (connect_port != nil) [connect_port release];
    if (listen_port  != nil) [listen_port release];
    if (sock_fd != 0) close(sock_fd);
    if (cage_fd != 0) close(cage_fd);
    return;
}

- (BOOL)ps_create
{
    // success return is YES.
    // failure return is NO.

    if (cage_fd != 0) {
        errno = EISCONN;
        return NO;
    }

    int err = 0;

    struct sockaddr_un conn_request;
    memset(&conn_request, 0, sizeof(conn_request));

    cage_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (cage_fd <= 0) { 
        err = errno;
    } 

    /*
    if (err == 0) {
        int on = 1;
        int ret = setsockopt(cage_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));
        if (ret != 0) {
            err = errno;
        }
    }
    */

    if (err == 0) { 
        // -- connect to cage_path --
        #ifdef __APPLE__
        conn_request.sun_len = sizeof(conn_request);
        #endif
        conn_request.sun_family = AF_LOCAL;
        memcpy(conn_request.sun_path, [sock_path UTF8String], [sock_path length]);
        int ret = connect(cage_fd, (struct sockaddr*)&conn_request, SUN_LEN(&conn_request));
        if (ret != 0) {
            err = errno;
            close(cage_fd);
            cage_fd = 0;
        }
    }

    if (err == 0) {
        // -- check own id --
        NSString* message;
        message   = [NSString stringWithFormat:@"get_id,%@\n", node_name];

        ssize_t rsize;
        char buf[1024 * 64];
        memset(buf, 0, sizeof(buf));

        rsize = send(cage_fd, [message UTF8String], [message length], 0);
        if (rsize == -1) {
            err = errno;
        }

        rsize = recv(cage_fd, buf, sizeof(buf), 0);
        if (rsize == -1) {
            err = errno;
        }

        if (err == 0) {
            NSString* element_string = [NSString stringWithUTF8String:buf];
            NSArray* element_array = [element_string componentsSeparatedByString:@","];
            if ([[element_array objectAtIndex:0] isEqualToString:@"205"]) {
                // success : get_id..
                own_addr_str = [[element_array objectAtIndex:3] copy];
                own_addr_bin = [[own_addr_str str2hex] copy];
            } else {
                // failure : get_id..
                // error reason is ,, cage_path or node_name...
                errno = ETIMEDOUT;
            }
        }
    }

    // -------------------
    if (err == 0) {
        errno = err;
        return YES;
    } else {
        errno = err;
        return NO;
    }
}


@end //implementation

#endif // __PRRISON_SOCKET_H__
