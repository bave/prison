#import <Cocoa/Cocoa.h>

#include "../ps.h"

int main(int argc, char** argv)
{
    id pool = [NSAutoreleasePool new];

    id ps = [[PrisonSock new] autorelease];

    // configuration
    [ps set_node_name:@"prison"];
    //[ps set_sock_path:@"/tmp/sock_cage"];
    [ps set_sock_path:@"/tmp/prison/sock_cage"];

    //set_id "aris" 

    id peer_id = [ps ps_lookup:@"prison@ps"];
    //id peer_id = @"78e305742ac743ecf22d8139502576ac8709e0ad";
    id peer_port= @"1000";

    // create prison socket
    BOOL retval = [ps ps_create];
    if (retval) {
        // connect
        int handler = [ps ps_connect:peer_id :peer_port];

        // prison socket buffer
        id psb = nil;

        // -------------------------------------------------------------
        // ps_sendto sample
        psb = [[PrisonSockBuffer new] autorelease];
        [psb set_handler:handler];
        [psb set_payload:[NSData dataWithBytes:"test\n" length:strlen("test\n")]];
        int size = [ps ps_sendto:psb];
        printf("send_size:%d\n", size);
        // -------------------------------------------------------------

        // -------------------------------------------------------------
        // ps_recvfrom sample
        psb = [ps ps_recvfrom];
        if (psb == nil) {
            // exceptional!!
            // - remote ps_server process is not servicing
            // - local prison/cage is not servicing
            NSLog(@"exceptional close");
        } else if ([psb payload] == nil) {
            // server side is closed or shutdowned by remote host!!
            NSLog(@"server side close");
        } else  { 
            // message receive processing... 
            NSLog(@"m_type:%d", [psb m_type]);
            NSLog(@"handler:%d", [psb handler]);
            NSLog(@"payload:%s", [[psb payload] bytes]);
        }
        // -------------------------------------------------------------
        [ps ps_close:handler];
    } else {
        NSLog(@"cant create prisonsock..");
    }

    [ps ps_destroy];

    [pool drain];
    return 0;
}
