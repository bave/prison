#import <Cocoa/Cocoa.h>

#include "../ps.h"

int main(int argc, char** argv)
{
    id pool = [NSAutoreleasePool new];

    id ps = [[PrisonSock new] autorelease];

    [ps set_node_name:@"prison"];
    [ps set_sock_path:@"/tmp/sock_cage"];

    id peer_port= @"1000";

    BOOL retval = [ps ps_create];
    if (retval) {
        // listen
        [ps ps_listen:peer_port];
        NSLog(@"listen start");

        for (;;) {
            id psb = [ps ps_recvfrom];
            NSLog(@"assert event");

            int m_type = [psb m_type];
            int handler = [psb handler];
            id payload = [psb payload];

            if (m_type == M_RDP_DATA) {
                //data
                NSLog(@"handler %d's data is %@", handler, payload);
                [ps ps_close:handler];
            } else if (m_type == M_RDP_ACCEPT) {
                NSLog(@"accept handler is %d", handler);
            } else if (m_type == M_RDP_CONNECT) { 
                NSLog(@"unused..(M_RDP_CONNECT)");
            } else if (m_type == M_RDP_TIMEOUT) {
                NSLog(@"timeout handler is %d", handler);
            } else if (m_type == M_RDP_ERROR)   {
                NSLog(@"error handler is %d", handler);
            } else if (m_type == M_RDP_CLOSED)   {
                NSLog(@"close handler is %d", handler);
            } else {
            }
        }
    } else {
        NSLog(@"cant create prisonsock..");
    }

    [ps ps_destroy];

    [pool drain];
    return 0;
}
