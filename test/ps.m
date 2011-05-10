#import <Cocoa/Cocoa.h>

#include "../ps.h"
#include "../category.h"

int main(int argc, char** argv)
{
    id pool = [NSAutoreleasePool new];

    id ps = [PrisonSock new];

    // nat1
    [ps ps_create];
    [ps set_node_name:@"prison"];
    [ps set_sock_path:@"/tmp/prison/sock_cage"];
    int handler = [ps ps_connect:@"53bf72753754587c39acd9c89cacb46232ea2951" :@"1000"];
    for (;;) {
        id psb = [ps ps_recvfrom];
        NSLog(@"handler:%d", [psb handler]);
        NSLog(@"payload:%s", [[psb payload] bytes]);
    }

    [ps ps_close:handler];
    [ps ps_destroy];
    [pool drain];
    return 0;
}
    //printf("sock_fd:%d\n",[ps sock_fd]);
    //printf("sock_fd:%d\n",[ps cage_fd]);
    //ssize_t ssize = [ps ps_sendto:handler :buffer];
    //ssize_t rsize = [ps ps_recvfrom:handler :buffer];
