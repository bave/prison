
#define __PRISON__

#import <Cocoa/Cocoa.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gpg.h"

extern GPGME* gpg;

@interface event : NSObject
{
    int cage_sock_fd;
    NSFileHandle* sock_handle;
}

- (id)init:(char*)arg1 :(char*)arg2 :(char*)arg3;
- (void)dealloc;
- (void)_sock_handler:(NSNotification*)notify;
- (void)_recv_put:(NSArray*)element_array;
- (void)_recv_get:(NSArray*)element_array;
@end


@implementation event

- (int)sendMessage:(NSString*)message
{
    return send(cage_sock_fd, [message UTF8String], [message length], 0);
}

- (void)_sock_handler:(NSNotification*)notify
{
    id pool = [NSAutoreleasePool new];
    if (![[notify userInfo] objectForKey:@"NSFileHandleError"])
    {
        NSData *data;
        const NSStringEncoding* encode = [NSString availableStringEncodings];
        data = [[notify userInfo] objectForKey:NSFileHandleNotificationDataItem];

        NSString* buffer = [[NSString alloc] initWithData:data encoding:*encode];
        [buffer autorelease];

        NSArray* buffer_array;
        buffer_array = [buffer componentsSeparatedByString:@"\n"];
        [buffer_array autorelease];

        unsigned int i;
        for (i=0; i<[buffer_array count]; i++)
        {
            id element = [buffer_array objectAtIndex:i];
            if ([element length] == 0) { continue; }

            /*
            if ([element hasSuffix:@"\n"]) {
                NSRange range = NSMakeRange(0 ,[element length]-1);
                element = [NSString stringWithString:[element substringWithRange:range]];
            } 
            */

            NSArray* element_array = [element componentsSeparatedByString:@","];
            id code = [element_array objectAtIndex:0];

            if ([code isEqualToString:@"203"]) { 
                [self _recv_put:element_array];
            }

            if ([code isEqualToString:@"204"]) { 
                [self _recv_get:element_array];
            }
        }

    }
    [sock_handle readInBackgroundAndNotify];
    [pool drain];
    return;
}

- (void)_recv_put:(NSArray*)element_array
{
    /*
    id command = [element_array objectAtIndex:1];
    id node = [element_array objectAtIndex:2];
    id key = [element_array objectAtIndex:3];
    id value = [element_array objectAtIndex:4];
    id ttl = [element_array objectAtIndex:5];
    if ([element_array count] == 7) {
        BOOL unique = YES;
    }
    */
    return;
}

- (void)_recv_get:(NSArray*)element_array
{
    id command = [element_array objectAtIndex:1];
    id node = [element_array objectAtIndex:2];
    id key = [element_array objectAtIndex:3];
    id value = [element_array objectAtIndex:4];
    TCHK_START(get);
    NSLog(@"%@\n", [gpg verify:[GPGME appendMessageFrame:value]]);
    //NSLog(@"%@\n", value);
    TCHK_END(get);
    return;
}

- (void)dealloc {
    // --------------
    // release coding
    // --------------
    close(cage_sock_fd);
    [super dealloc];
}

- (id)init:(char*)arg1 :(char*)arg2 :(char*)arg3
{
    self = [super init];
    if(self != nil) {
        // --------------
        // initial coding
        // --------------


        // argv[1] : local_socket
        // argv[2] : seed ip
        // argv[3] : seed port

        char buffer[65535];
        struct sockaddr_un conn_request;

        cage_sock_fd = socket(AF_LOCAL, SOCK_STREAM, 0);

        memset(&conn_request, 0, sizeof(conn_request));
        conn_request.sun_len = sizeof(conn_request);
        NSString* cis = [NSString stringWithFormat:@"%s", arg1];
        memcpy(conn_request.sun_path, [cis UTF8String], [cis length]);
        conn_request.sun_family = AF_LOCAL;
        connect(cage_sock_fd, (struct sockaddr*)&conn_request, SUN_LEN(&conn_request));

        NSString* message = [NSString stringWithFormat:@"new,test,11111,global\n"];
        NSLog(@"%@", message);

        memset(buffer, 0, sizeof(buffer));
        memcpy(buffer, [message UTF8String], [message length]);
        send(cage_sock_fd, buffer, (socklen_t)sizeof(buffer), 0);

        memset(buffer, 0, sizeof(buffer));
        recv(cage_sock_fd, buffer, sizeof(buffer), 0);
        NSLog(@"%@", [NSString stringWithUTF8String:buffer]);

        message = [NSString stringWithFormat:@"join,test,%s,%s,global\n", arg2, arg3];
        NSLog(@"%@", message);

        memset(buffer, 0, sizeof(buffer));
        memcpy(buffer, [message UTF8String], [message length]);
        send(cage_sock_fd, buffer, (socklen_t)sizeof(buffer), 0);

        memset(buffer, 0, sizeof(buffer));
        recv(cage_sock_fd, buffer, sizeof(buffer), 0);

        NSLog(@"%@", [NSString stringWithUTF8String:buffer]);


        sock_handle = [[NSFileHandle alloc] initWithFileDescriptor:cage_sock_fd];
        [[NSNotificationCenter defaultCenter]
            addObserver:self
            selector:@selector(_sock_handler:)
            name:NSFileHandleReadCompletionNotification
            object:sock_handle
        ];
        [sock_handle readInBackgroundAndNotify];


    }
    return self;
}

@end

/*
int main(int argc, char** argv) {
    id pool = [NSAutoreleasePool new];
    [NSApplication sharedApplication];

    gpg = [[GPGME alloc] initWithDir:@"/Users/bayve/work/gpg"];

    id e = [[event alloc] init:"/tmp/sock_cage" :"150.65.179.151" :"12000"];    

    DISPATCH_START(loop)
        int port;
        for (port=12001; port<=13000; port++) {
            usleep(100000);
            [e sendMessage:[NSString stringWithFormat:@"get,test,dev%d.p2p\n", port]];
        }
    DISPATCH_END
    DISPATCH(loop);

    NSLog(@"start run");
    [NSApp run];
    [pool drain];
    return 0;
}
*/







