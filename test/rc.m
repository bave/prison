#import <Cocoa/Cocoa.h>

#include "../rc.h"

int main(int argc, char *argv[])
{
    id pool = [NSAutoreleasePool new];
    bool ret;
    ResourceConfig* rc;

    NSLog(@"\n");
    //rc = [[ResourceConfig alloc] init];
    ret = true;
    rc = [[ResourceConfig alloc] initWithConf:@"../rc.plist"];

    //NSLog(@"rc :%@\n", rc);

    if (rc == nil) {
        NSLog(@"nilllll\n");
        ret = false;
    }    

    if (ret == true) {
        NSLog(@"path:\n%@\n", [rc getPath]);
        NSLog(@"path:\n%@\n", [rc getRunDir]);
        NSLog(@"path:\n%@\n", [rc getPurityPath]);
        NSLog(@"path:\n%@\n", [rc getInternal]);
        NSLog(@"path:\n%@\n", [rc getExternal]);
        NSLog(@"path:\n%@\n", [rc getSeedHost]);
        NSLog(@"path:\n%@\n", [rc getSeedPort]);
        NSLog(@"path:\n%@\n", [rc getLocalDB]);
    }

    [pool drain];
    return 0;
}

