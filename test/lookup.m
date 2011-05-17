#import <Cocoa/Cocoa.h>

#include "../ps.h"

int main(int argc, char** argv)
{
    id pool = [NSAutoreleasePool new];

    id ps = [[PrisonSock new] autorelease];
    id peer_id = nil;

    // configuration
    [ps set_node_name:@"prison"];
    [ps set_sock_path:@"/tmp/sock_cage"];

    // create prison socket
    BOOL retval = [ps ps_create];
    if (retval) {
        if (argc != 2) { 
            peer_id = [ps ps_lookup:@"aris@ps"];
        } else {
            id tmp = [NSString stringWithUTF8String:argv[1]];
            peer_id = [ps ps_lookup:tmp];
        }
        NSLog(@"id:%@", peer_id);
        NSLog(@"length%d", [peer_id length]);
    }
    [ps ps_destroy];

    [pool drain];
    return 0;
}
