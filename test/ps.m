#import <Cocoa/Cocoa.h>

#include "../ps.h"

int main(int argc, char** argv)
{
    id pool = [NSAutoreleasePool new];

    id ps = [PrisonSock new];
    [ps set_node_name:@"prison"];
    [ps set_sock_path:@"/tmp/sock_cage"];

    [ps create];

    sleep(2);

    [ps close];

    [pool drain];
    return 0;
}
