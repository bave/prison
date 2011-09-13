#import <Cocoa/Cocoa.h>

#include "../ps.h"

int main(int argc, char** argv)
{

    if (argc != 4) {
        printf("./a.out hoge@ps port_num message\n");
        return -1;
    }

    id pool = [NSAutoreleasePool new];

    id ps = [[PrisonSock new] autorelease];

    // configuration
    [ps set_node_name:@"prison"];
    [ps set_sock_path:@"/tmp/prison/sock_cage"];

    // create prison socket
    BOOL retval = [ps ps_create];
    if (retval) {
        // connect

        id peer_id = [ps ps_lookup:[NSString stringWithUTF8String:argv[1]]];
        id peer_port= [NSString stringWithUTF8String:argv[2]];
        NSLog(@"peer_id  :%s -> %@", argv[1],peer_id);
        NSLog(@"peer_port:%@", peer_port);

        int handler = [ps ps_connect:peer_id :peer_port];
        if (handler == -1) {
            NSLog(@"cant connect");
            goto finish;
        }

        // prison socket buffer
        id psb = nil;
        id m_data = [[NSMutableData new] autorelease];

        // -------------------------------------------------------------
/*
*/
//loop:
        // ps_sendto sample
        psb = [[PrisonSockBuffer new] autorelease];
        [psb set_handler:handler];
        [m_data appendData:[NSData dataWithBytes:argv[3] length:strlen(argv[3])]];
        [psb set_payload:m_data];
        uint32_t ssize = [ps ps_sendto:psb];
        if (ssize != [[psb payload] length]) { 
            printf("dont matching send size");
            exit(-1);
        }
        //sleep(1);
        // -------------------------------------------------------------

        // -------------------------------------------------------------
        // ps_recvfrom sample
        psb = [ps ps_recvfrom];
        NSLog(@"recv");
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
            NSLog(@"m_type   :%s", trans_m_type([psb m_type]));
            NSLog(@"handler  :%d", [psb handler]);
            NSLog(@"payload  :%s", [[psb payload] bytes]);
        }
        // -------------------------------------------------------------
        //goto loop;
        [ps ps_close:handler];
    } else {
        NSLog(@"cant create prisonsock..");
    }

finish:
    [ps ps_destroy];
    [pool drain];
    return 0;
}
