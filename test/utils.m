#import <Cocoa/Cocoa.h>

#include "../utils.h"
#include "../category.h"

int main() {
    int i;
    id pool = [[NSAutoreleasePool alloc] init];

    NSArray* array;
    NSString* str = @"123.456.789.abc";
    array = array_split(str, @".");

    NSLog(@"%@\n" , array);
    NSLog(@"%@\n" ,[str componentsSeparatedByString:@"."]);


    for (i=0; i<[array icount]; i++) {
        NSLog(@"%@\n", [array objectAtIndex:i]);
    }

    NSData* addr;
    addr = ip_aton(@"IPv4", @"127.0.0.1");
    struct in_addr a;
    memcpy(&a, [addr bytes], [addr length]);
    printf("0x%x\n", htonl((uint32_t)a.s_addr));

    addr = ip_aton(@"IPv6", @"fe80::1");
    struct in6_addr b;
    memcpy(&b, [addr bytes], [addr length]);
    printf("0x%x\n", htonl((uint32_t)b.s6_addr32[0]));
    printf("0x%x\n", htonl((uint32_t)b.s6_addr32[1]));
    printf("0x%x\n", htonl((uint32_t)b.s6_addr32[2]));
    printf("0x%x\n", htonl((uint32_t)b.s6_addr32[3]));

    addr = [NSData dataWithBytes:&a length:sizeof(a)];
    NSLog(@"%s", ip_ntoa(@"IPv4", addr));
    NSLog(@"%@", [NSString stringWithUTF8String:ip_ntoa(@"IPv4", addr)]);
    addr = [NSData dataWithBytes:&b length:sizeof(b)];
    NSLog(@"%s", ip_ntoa(@"IPv6", addr));
    NSLog(@"%@", [NSString stringWithUTF8String:ip_ntoa(@"IPv6", addr)]);

    NSLog(@"%@", addrmask(@"127.0.0.1", @"255.255.255.0"));

    NSLog(@"%d", ip4comp(@"127.0.0.1", @"127.0.0.1"));
    NSLog(@"%d", ip4comp(@"127.0.0.1", @"127.0.0.2"));

    a.s_addr = htonl(0xc0a80a01); 
    addr = [NSData dataWithBytes:&a length:sizeof(a)];
    NSLog(@"%s", ip_ntoa(@"IPv4", addr));
    a.s_addr = htonl(0x000aa8c0);
    addr = [NSData dataWithBytes:&a length:sizeof(a)];
    NSLog(@"%s", ip_ntoa(@"IPv4", addr));
    a.s_addr = htonl(0x00ffffff);
    addr = [NSData dataWithBytes:&a length:sizeof(a)];
    NSLog(@"%s", ip_ntoa(@"IPv4", addr));
    a.s_addr = htonl(0x000aa8c0);
    addr = [NSData dataWithBytes:&a length:sizeof(a)];
    NSLog(@"%s", ip_ntoa(@"IPv4", addr));
    a.s_addr = htonl(0x00ffffff);
    addr = [NSData dataWithBytes:&a length:sizeof(a)];
    NSLog(@"%s", ip_ntoa(@"IPv4", addr));
    a.s_addr = htonl(0xc0a80aff);
    addr = [NSData dataWithBytes:&a length:sizeof(a)];
    NSLog(@"%s", ip_ntoa(@"IPv4", addr));
    a.s_addr = htonl(0xc0a80aff);
    addr = [NSData dataWithBytes:&a length:sizeof(a)];
    NSLog(@"%s", ip_ntoa(@"IPv4", addr));

    NSLog(@"%@\n", currentdir());
    //NSLog(@"%d\n", mkdir(@"tetete"));
    NSLog(@"A    - www.wide.ad.jp:%@\n", getHost2Addr(@"IPv4", @"www.wide.ad.jp"));
    NSLog(@"AAAA - www.wide.ad.jp:%@\n", getHost2Addr(@"IPv6", @"www.wide.ad.jp"));


    NSLog(@"%d\n", is_local(@"172.16.1.1"));
    NSLog(@"%d\n", is_global(@"172.16.1.1"));

    [pool release];
    return 0;
}
