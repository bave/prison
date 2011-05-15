#import <Cocoa/Cocoa.h>

#include "../ps.h"

int main(int argc, char** argv)
{
    id pool = [NSAutoreleasePool new];

    id ps = [PrisonSock new];

    // configuration
    [ps set_node_name:@"prison"];
    [ps set_sock_path:@"/tmp/sock_cage"];
    //[ps set_sock_path:@"/tmp/prison/sock_cage"];

    id peer_id = @"78e305742ac743ecf22d8139502576ac8709e0ad";
    id peer_port= @"1000";

    // create prison socket
    BOOL retval = [ps ps_create];
    if (retval) {
        int handler = [ps ps_connect:peer_id :peer_port];

        // -------------------------------------------------------------
        // ps_sendto message(objc) sample
        // XXX
        // -------------------------------------------------------------

        // -------------------------------------------------------------
        // ps_recvfrom message(objc) sample
        for (;;) {
            // blocking..
            id psb = [ps ps_recvfrom];

            // error and close check..
            if (psb == nil) {
                // exceptional!!
                //   remote ps_server process is not servicing
                //   local prison/cage is not servicing
                break;
            } else if ([psb payload] == nil) {
                //  server side is closed or shutdowned by remote host!!
                break;
            } 

            // message receive processing... 
            NSLog(@"handler:%d", [psb handler]);
            NSLog(@"payload:%s", [[psb payload] bytes]);
        }
        // -------------------------------------------------------------
        [ps ps_close:handler];
    } else {
        NSLog(@"cant create prisonsock..");
    }
    [ps ps_destroy];
    [ps release];
    [pool drain];
    return 0;
}
