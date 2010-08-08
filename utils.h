#ifndef __RAPRINS_UTILS_H_
#define __RAPRINS_UTILS_H_

#import <Cocoa/Cocoa.h>

#ifdef __MARC__
#include <AvailabilityMacros.h>
#endif

#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>

enum retFlag { success, fail };

// Macro Code
#define FW_PREFIX(X) htonl((unsigned int)(0xFFFFFFFF << (32-X)))
#define INT3 __asm__ __volatile__("int3");

#ifdef __MACH__
#import <servers/bootstrap.h>
#define SNOW    DEPRECATED_IN_MAC_OS_X_VERSION_10_6_AND_LATER
#define LEOPARD DEPRECATED_IN_MAC_OS_X_VERSION_10_5_AND_LATER
#define s6_addr8  __u6_addr.__u6_addr8
#define s6_addr16 __u6_addr.__u6_addr16
#define s6_addr32 __u6_addr.__u6_addr32
#endif

#ifdef __linux__
#ifndef bool
#define bool BOOL
#endif
#ifndef treu
#define true 1
#endif
#ifndef false
#define false 1
#endif
#endif

// prototype --------------------------------------------------------------------

// string spliter...
NSArray* array_split(NSString* string, NSString* delimiter);

// transform numerical to address
const char* ip_ntoa(NSString* ns_str, NSData* ns_data);
// transform address to numerical
NSData* ip_aton(NSString* type, NSString* addr);


// directory management
NSString* currentdir(void);
#ifdef __MACH__
bool mkdir(NSString* dir);
#endif

// memory 
void memdump(void *mem, int i);
bool memswap(void* s1, void* s2, size_t size);

//compare
// fcomp's flag is @"A.B.C.D." & @"B"
bool fcomp(NSString* flags, NSString* flag);
bool ip4comp(NSString* addr1, NSString* addr2);

// for ipv6 code
void fillscopeid(struct sockaddr_in6 *sin6);

// now ipv4 only
// 127.1.1.1 255.0.0.0 -> 127.0.0.0
NSString* addrmask(NSString* addr, NSString* mask);

// implementation --------------------------------------------------------------

#ifdef __MACH__
bool mkdir(NSString* dir)
{
    id pool = [NSAutoreleasePool new];
    bool ret;

    NSFileManager* manager;
    manager = [NSFileManager defaultManager];

    #ifdef LEOPARD
    ret = [manager createDirectoryAtPath:dir
             withIntermediateDirectories:false
                              attributes:nil
                                   error:nil];
    #else
    ret = [manager createDirectoryAtPath:dir attributes:nil];
    #endif


    [pool drain];
    return true;
}
#endif

NSString* currentdir(void)
{
    id pool = [NSAutoreleasePool new];

    NSFileManager* manager;
    NSString* path;
    manager = [NSFileManager defaultManager];
    path = [[NSString alloc]initWithFormat:@"%@/",[manager currentDirectoryPath]];
    [pool drain];
    [path autorelease];

    return path;
}


bool ip4comp(NSString* addr1, NSString* addr2)
{
    id pool = [NSAutoreleasePool new];

    if ((addr1 == nil) & (addr2 == nil)) {
        [pool drain];
        return true;
    }
    if (addr1 == nil) {
        [pool drain];
        return false;
    }
    if (addr2 == nil) {
        [pool drain];
        return false;
    }


    NSData* ns_addr = ip_aton(@"IPv4" , addr1);
    struct in_addr a1;
    memcpy(&a1, [ns_addr bytes], [ns_addr length]);

    NSData* ns_mask = ip_aton(@"IPv4" , addr2);
    struct in_addr a2;
    memcpy(&a2, [ns_mask bytes], [ns_mask length]);

    if (a1.s_addr == a2.s_addr) {
        [pool drain];
        return true;
    }
    [pool drain];
    return false;
}

NSString* addrmask(NSString* addr, NSString* mask)
{
    if ((addr==nil) || (mask==nil)) {
        return nil;
    }


    NSData* ns_addr = ip_aton(@"IPv4" , addr);
    struct in_addr a;
    memcpy(&a, [ns_addr bytes], [ns_addr length]);

    NSData* ns_mask = ip_aton(@"IPv4" , mask);
    struct in_addr m;
    memcpy(&m, [ns_mask bytes], [ns_mask length]);

    struct in_addr t;
    t.s_addr = a.s_addr & m.s_addr;

    ns_addr = [NSData dataWithBytes:&t length:sizeof(t)];
    return [NSString stringWithUTF8String:ip_ntoa(@"IPv4", ns_addr)];
}

bool fcomp(NSString* flags, NSString* flag) {
    id pool = [NSAutoreleasePool new];
    NSArray* array;
    array = array_split(flags, @".");
    size_t i;
    for (i=0; i<[array count]; i++) {
        if([[array objectAtIndex:i] isEqualToString:flag]) {
            [pool drain];
            return true;
        }
    }
    [pool drain];
    return false;
}


bool memswap(void* s1, void* s2, size_t size) {
    uint8_t* tmp;
    tmp = (uint8_t*)malloc(size);
    if (tmp == NULL) {
        return false;
    }
    memcpy(tmp, s1, size);
    memcpy(s1,  s2, size);
    memcpy(s2, tmp, size);
    free(tmp);
    return true;
}


// same behavior
// [NSString componentsSeparatedByString:(NSString*)]
NSArray* array_split(NSString* string, NSString* delimiter) {

    NSCharacterSet* chSet;
    NSString* token;
    NSScanner* scanner;
    NSMutableArray* array;
    NSArray* output_array;

    id pool = [NSAutoreleasePool new];
    //NSString* string = @"123.234.345.456";

    array = [[[NSMutableArray alloc] init] autorelease];
    chSet = [NSCharacterSet characterSetWithCharactersInString:delimiter];

    scanner = [NSScanner scannerWithString:string];
    while(![scanner isAtEnd]) {
        if([scanner scanUpToCharactersFromSet:chSet intoString:&token]) {
            //NSLog(@"%@\n",token);
            [array addObject:token];
        }
        [scanner scanCharactersFromSet:chSet intoString:(NSString**)nil];
    }

    output_array = [[NSArray alloc] initWithArray:array];
    [pool release];
    [output_array autorelease];
    return output_array;
}

void fillscopeid(struct sockaddr_in6 *sin6)
{
    if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr)) {
        sin6->sin6_scope_id = ntohs(*(u_int16_t *)&sin6->sin6_addr.s6_addr[2]);
        sin6->sin6_addr.s6_addr[2] = 0;
        sin6->sin6_addr.s6_addr[3] = 0;
    }
    return;
}


NSData* ip_aton(NSString* type, NSString* addr)
{
    struct addrinfo hints;
    struct addrinfo* res;
    char* ip;
    ip = (char *)[addr UTF8String];

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_RAW;

    int err=0;
    if ([type isEqualToString:@"IPv4"]) { 
        struct sockaddr_in* sin;
        struct sockaddr_in sin_tmp;
        memset(&sin_tmp, 0, sizeof(sin_tmp));
        hints.ai_family = AF_INET;
        err = getaddrinfo(ip, NULL, &hints, &res);
        if (err>0) perror("getaddrinfo");
        sin = (struct sockaddr_in *)(res->ai_addr);
        memcpy(&sin_tmp, sin, sizeof(sin_tmp));
        freeaddrinfo(res);
        sin = &sin_tmp;
        return [NSData dataWithBytes:&sin->sin_addr length:sizeof(struct in_addr)];
    } 
    else if ([type isEqualToString:@"IPv6"]) {
        struct sockaddr_in6* sin6;
        struct sockaddr_in6 sin6_tmp;
        memset(&sin6_tmp, 0, sizeof(sin6_tmp));
        hints.ai_family = AF_INET6;
        err = getaddrinfo(ip, NULL, &hints, &res);
        if (err>0) perror("getaddrinfo");
        sin6 = (struct sockaddr_in6 *)(res->ai_addr);
        memcpy(&sin6_tmp, sin6, sizeof(sin6_tmp));
        freeaddrinfo(res);
        sin6 = &sin6_tmp;
        return [NSData dataWithBytes:&sin6->sin6_addr length:sizeof(struct in6_addr)];
    }
    else { return nil; }

    return nil;
}


const char* ip_ntoa(NSString* ns_str, NSData* ns_data)
{
    //ns_data -> sockaddr_storage

    struct sockaddr* sa;
    struct sockaddr_in* sin;
    struct sockaddr_in6* sin6;
    struct sockaddr_storage sast;
    
    static char host4[sizeof("123.456.789.abc")];
    static char host6[sizeof("1111:2222:3333:4444:5555:6666:7777:8888")];

    memset(&sast, 0 , sizeof(sast));
    memset(host4, 0 , sizeof(host4));
    memset(host6, 0 , sizeof(host6));

    sa    = (struct sockaddr*)&sast;
    sin  = (struct sockaddr_in*)&sast;
    sin6 = (struct sockaddr_in6*)&sast;

    if ([ns_str isEqualToString:@"IPv4"]) {
        sa->sa_family = AF_INET;
        memcpy(&(sin->sin_addr.s_addr), [ns_data bytes], sizeof(uint32_t));
    } 
    else if ([ns_str isEqualToString:@"IPv6"]) {
        sa->sa_family = AF_INET6;
        memcpy(&(sin6->sin6_addr), [ns_data bytes], sizeof(struct in6_addr));
    }
    else {
        return NULL;
    }


    int err=0;
    switch (sa->sa_family) {
        case AF_INET:
            err = getnameinfo(sa, sizeof(struct sockaddr_in),
                      host4, sizeof(host4), NULL, 0,  NI_NUMERICHOST);
            if (err>0) perror("getnameinfo");
            return (const char *)host4;
        case AF_INET6:
            fillscopeid((struct sockaddr_in6 *)sa);
            err = getnameinfo(sa, sizeof(struct sockaddr_in6),
                      host6, sizeof(host6), NULL, 0,  NI_NUMERICHOST);
            if (err>0) perror("getnameinfo");
            return (const char*)host6;
        default:
            return NULL;
    }
    return NULL;
}



void memdump(void *mem, int i)
{
    if(i>=2000) {
        printf("allocation memory size over\n");
        return;
    }
    int j;
    int max;
    int *memi;
    int *buf;
    buf = (int*)malloc(2000);
    memset(buf, 0, 2000);
    memcpy(buf, mem, i);
    memi = buf;

    printf("start memory dump %p *****\n", mem);
    max = i / 16 + (i % 16 ? 1 : 0);
    for (j = 0; j < max; j++) {
        printf("%p : %08x %08x %08x %08x\n",
                memi, htonl(*(memi)), htonl(*(memi+1)), htonl(*(memi+2)), htonl(*(memi+3)));
        memi += 4;
    }
    printf("end memory dump *****\n");
    free(buf);
    return;
}



#endif // __RAPRINS_UTILS_H
