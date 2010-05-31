#import  <Cocoa/Cocoa.h>
#include "../ni.h"
#include "../utils.h"
#include "../category.h"

#include <unistd.h>

int
main(int argc, char** argv)
{
    id pool = [NSAutoreleasePool new];

    id ni = [NetInfo new];
    for (;;) {
        sleep(1);
        [ni getRT];
        printf("defaultIFname4:%s\n", [[ni defaultIFname4] UTF8String]);
        printf("defaultIFaddr4:%s\n", [[ni defaultIFaddr4] UTF8String]);
        printf("defaultIP4:%s\n", [[ni defaultIP4] UTF8String]);
        printf("defaultMask4:%s\n", [[ni defaultMask4] UTF8String]);
    }


    /*
    int i;
    for(i=0; i< [ni rt4_icount]; i++){
        NSLog(@"%@\n", [ni rt4_family:i]);
        NSLog(@"%@\n", [ni rt4_destination:i]);
        NSLog(@"%@\n", [ni rt4_netmask:i]);
        NSLog(@"%@\n", [ni rt4_gateway:i]);
        NSLog(@"%@\n", [ni rt4_flags:i]);
        NSLog(@"%@\n", [ni rt4_ifname:i]);
        NSLog(@"\n");
    }


    for(i=0; i< [ni rt6_icount]; i++){
        NSLog(@"%@\n", [ni rt6_family:i]);
        NSLog(@"%@\n", [ni rt6_destination:i]);
        NSLog(@"%@\n", [ni rt6_netmask:i]);
        NSLog(@"%@\n", [ni rt6_gateway:i]);
        NSLog(@"%@\n", [ni rt6_flags:i]);
        NSLog(@"%@\n", [ni rt6_ifname:i]);
        NSLog(@"\n");
    }


    for(i=0; i< [ni rt_icount]; i++){
        if (fcomp([ni rt_flags:i], @"G")) {
            NSLog(@"%@\n", [ni rt_family:i]);
            NSLog(@"%@\n", [ni rt_destination:i]);
            NSLog(@"%@\n", [ni rt_netmask:i]);
            NSLog(@"%@\n", [ni rt_gateway:i]);
            NSLog(@"%@\n", [ni rt_flags:i]);
            NSLog(@"%@\n", [ni rt_ifname:i]);
            NSLog(@"\n");
        }
    }


    NSLog(@"\n");
    NSLog(@"default4:%@\n", [ni defaultRoute4]);
    NSLog(@"default6:%@\n", [ni defaultRoute6]);
    printf("\n");
    printf("default4:%s\n", [[ni defaultRoute4] UTF8String]);
    printf("default6:%s\n", [[ni defaultRoute6] UTF8String]);
    printf("\n");


    [ni getETH];
    for(i=0; i< [ni eth_icount]; i++){
        NSLog(@"%@\n", [ni eth_ifname:i]);
        NSLog(@"%@\n", [ni eth_address:i]);
        NSLog(@"%@\n", [ni eth_flags:i]);
        NSLog(@"\n");
    }
    */


    [pool drain];
    return 0; 


}

