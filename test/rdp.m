#import <Cocoa/Cocoa.h>

#include "../rdp.h"

int main(int argc, char** argv)
{
    id pool = [NSAutoreleasePool new];
    id s_rdp = [SockRDP new];
    [s_rdp rdp_socket:@"/tmp/rdp_test"];
    [s_rdp rdp_close];
    [pool drain];
    return 0;
}
