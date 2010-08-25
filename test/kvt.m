#import <Cocoa/Cocoa.h>

#include "../kvt.h"

ResourceConfig* rc;
NetInfo* ni;
bool is_verbose;


int main(int argc, char** argv)
{
    id pool = [NSAutoreleasePool new];
    is_verbose = true;
    keyValueTable* kvt;
    @try {
        ni = [NetInfo new];
        rc = [[ResourceConfig alloc] initWithConf:@"../rc.plist"];

        kvt = [[keyValueTable alloc] init:@"../bin/cage"];
        [kvt setLocalDB:[rc getLocalDB]];
        NSLog(@"string:%@",[kvt ip4key:@"test0.aris.p2p"]);
        NSLog(@"string_nothing:%@",[kvt ip4key:@"test4.aris.p2p"]);
        NSLog(@"string:%@",[kvt port4key:@"test0.aris.p2p"]);
        NSLog(@"int:%d",[[kvt port4key:@"test0.aris.p2p"] intValue]);

    } @catch (id err) {
        NSLog(@"err:%@", err);
    }


    [kvt release];
    [pool drain];
    return 0;
}
