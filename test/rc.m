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
        NSLog(@"PrisonName:\n%@\n", [rc getPrisonName]);
        NSLog(@"passwd:\n%@\n", [rc getPasswd]);
        NSLog(@"path:\n%@\n", [rc getPath]);
        NSLog(@"RunDir:\n%@\n", [rc getRunDir]);
        NSLog(@"PurityPath:\n%@\n", [rc getPurityPath]);
        NSLog(@"InternalPort:\n%@\n", [rc getInternal]);
        NSLog(@"ExternalPort:\n%@\n", [rc getExternal]);
        NSLog(@"SeedHost:\n%@\n", [rc getSeedHost]);
        NSLog(@"SeedPort:\n%@\n", [rc getSeedPort]);
        NSLog(@"LovalDB:\n%@\n", [rc getLocalDB]);
    }

    [pool drain];
    return 0;
}

