#import <Cocoa/Cocoa.h>

#include "../rc.h"

int main(int argc, char *argv[])
{
    id pool = [NSAutoreleasePool new];
    RaprinsConfig* rc;
    RaprinsConfig* rc2;

    rc = [[RaprinsConfig alloc] init];
    rc2 = [[RaprinsConfig alloc] initWithConf:@"../raprins.conf"];

    if (rc == nil) NSLog(@"rc nilllll\n");
    if (rc2 == nil) NSLog(@"rc 2nilllll\n");

    [pool drain];
    return 0;
}

