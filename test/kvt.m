#import <Cocoa/Cocoa.h>

#include "../kvt.h"


int main()
{
    id pool = [NSAutoreleasePool new];

    keyValueTable* kvt;
    kvt = [keyValueTable new];
    [kvt setPath:@"../data.conf"];
    //[kvt _debug_dict];
    //[kvt _debug_path];
    NSLog(@"string:%@",[kvt ip4key:@"http.aris.p2p"]);
    NSLog(@"string_nothing:%@",[kvt ip4key:@"hhttp.aris.p2p"]);
    NSLog(@"string:%@",[kvt port4key:@"http.aris.p2p"]);
    NSLog(@"int:%d",[[kvt port4key:@"http.aris.p2p"] intValue]);

    [kvt release];
    [pool drain];
    return 0;
}
