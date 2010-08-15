#import <Cocoa/Cocoa.h>

#include "../rc.h"

int main(int argc, char *argv[])
{
    id pool = [NSAutoreleasePool new];
    RaprinsConfig* rc;

    //rc = [[RaprinsConfig alloc] init];
    rc = [[RaprinsConfig alloc] initWithConf:@"../rc.plist"];

    //NSLog(@"rc :%@\n", rc);

    if (rc == nil) {
        NSLog(@"rc is niiiiiil\n");
        [pool drain];
        return 0;
    }    

    NSLog(@"path:\n%@\n", [rc getPath]);
    NSLog(@"path:\n%@\n", [rc getRunDir]);
    NSLog(@"path:\n%@\n", [rc getInternal]);
    NSLog(@"path:\n%@\n", [rc getExternal]);
    NSLog(@"path:\n%@\n", [rc getSeedHost]);
    NSLog(@"path:\n%@\n", [rc getSeedPort]);
    NSLog(@"path:\n%@\n", [rc getLocalDB]);

    [pool drain];
    return 0;
}

