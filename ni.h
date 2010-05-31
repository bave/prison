#ifndef __RAPRINS_RT_H_
#define __RAPRINS_RT_H_

#import  <Cocoa/Cocoa.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sysctl.h>

#include <net/route.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/ethernet.h>


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "utils.h"
#include "category.h"

#define ROUNDUP(a) ((a)>0?(1+(((a)-1)|(sizeof(uint32_t)-1))):sizeof(uint32_t))

struct _rt {
    NSString* rt_destination;
    NSString* rt_family;
    NSString* rt_gateway;
    NSString* rt_netmask;
    //NSString* rt_genmask;
    NSString* rt_ifname;
    //NSString* rt_ifmac;
    //NSString* rt_author;
    //NSString* rt_broadcast;
    NSString* rt_flags;
};

struct _eth {
    NSString* eth_ifname;
    NSString* eth_address;
    NSString* eth_flags;
};

@interface NetInfo : NSObject {
    NSMutableArray* eth_array;
    NSMutableArray* rt_array;
    NSMutableArray* rt4_array;
    NSMutableArray* rt6_array;
}

// public function
- (id)init;
- (void)dealloc;

- (bool)refresh;
- (bool)getRT;
- (bool)getETH;

- (int)eth_icount;
- (uint32_t)eth_count;
- (NSString*)eth_ifname:(uint32_t)i;
- (NSString*)eth_address:(uint32_t)i;
- (NSString*)eth_flags:(uint32_t)i;

- (int)rt_icount;
- (uint32_t)rt_count;
- (NSString*)rt_ifname:(uint32_t)i;
- (NSString*)rt_flags:(uint32_t)i;
- (NSString*)rt_family:(uint32_t)i;
- (NSString*)rt_gateway:(uint32_t)i;
- (NSString*)rt_netmask:(uint32_t)i;
- (NSString*)rt_destination:(uint32_t)i;

- (int)rt4_icount;
- (uint32_t)rt4_count;
- (NSString*)rt4_ifname:(uint32_t)i;
- (NSString*)rt4_flags:(uint32_t)i;
- (NSString*)rt4_family:(uint32_t)i;
- (NSString*)rt4_gateway:(uint32_t)i;
- (NSString*)rt4_netmask:(uint32_t)i;
- (NSString*)rt4_destination:(uint32_t)i;


- (int)rt6_icount;
- (uint32_t)rt6_count;
- (NSString*)rt6_ifname:(uint32_t)i;
- (NSString*)rt6_flags:(uint32_t)i;
- (NSString*)rt6_family:(uint32_t)i;
- (NSString*)rt6_gateway:(uint32_t)i;
- (NSString*)rt6_netmask:(uint32_t)i;
- (NSString*)rt6_destination:(uint32_t)i;

- (NSString*)defaultRoute4;
- (NSString*)defaultRoute6;

- (NSString*)netmask:(NSString*)ifname;

- (NSString*)defaultIFname4;
- (NSString*)defaultIFaddr4;
- (NSString*)defaultIP4;
- (NSString*)defaultMask4;
//- (NSString*)useIF_gateway4;
//- (NSString*)useIF_gateway4;
//- (NSString*)useIF_ip6;
//- (NSString*)useIF_maskv6;
//- (NSString*)useIF_gateway6;

// private function
- (void)_eth_process:(struct if_msghdr*)ifm;
- (void)_rt_process:(struct rt_msghdr2*)rtm;
- (void)_rt_getaddrs:(int)addrs :(struct sockaddr*)sa :(struct sockaddr**)rti_info;

@end


@implementation NetInfo

- (NSString*)defaultIFname4 {
    size_t i;
    for(i=0; i< [self rt4_count]; i++){
        if (fcomp([self rt4_flags:i], @"G")) {
            if([[self rt_destination:i] isEqualToString:@"0.0.0.0"] &&
               [[self rt_netmask:i] isEqualToString:@"0.0.0.0"])
            {
                return [self rt_ifname:i];
            }
        }
    }
    return nil;
}


- (NSString*)defaultIP4
{
    size_t i;
    for(i=0; i< [self rt4_count]; i++){
        if (fcomp([self rt4_flags:i], @"U") &&
            fcomp([self rt4_flags:i], @"H") &&
            fcomp([self rt4_flags:i], @"S") )
        {
            NSString* mask = [self netmask:[self defaultIFname4]];
            //NSLog(@"%@\n", mask);
            NSString* net1 = addrmask([self rt4_destination:i], mask);
            //NSLog(@"%@\n", net1);
            NSString* net2 = addrmask([self defaultRoute4], mask);
            //NSLog(@"%@\n", net2);
            if (ip4comp(net1, net2)) {
                return [self rt_destination:i];
            }
        }
    }
    return nil;
}


- (NSString*)defaultIFaddr4
{
    size_t i;
    for(i=0; i< [self eth_count]; i++){
        if ([[self defaultIFname4] isEqualToString:[self eth_ifname:i]]) {
            return [self eth_address:i];
        }

    }
    return nil;
}


- (NSString*)defaultMask4
{
    size_t i;
    for(i=0; i< [self rt4_count]; i++){
        if (fcomp([self rt4_flags:i], @"U") &&
            fcomp([self rt4_flags:i], @"H") &&
            fcomp([self rt4_flags:i], @"S") )
        {
            return [self netmask:[self defaultIFname4]];
        }
    }
    return nil;
}


- (NSString*)netmask:(NSString*)ifname
{
    size_t i;
    for(i=0; i< [self rt4_count]; i++){
        if (fcomp([self rt4_flags:i], @"U") &&
            fcomp([self rt4_flags:i], @"C") &&
            fcomp([self rt4_flags:i], @"S") )
        {
            if ([[self rt4_ifname:i] isEqualToString:ifname]) {
                return [self rt4_netmask:i];
            }
        }
    }
    return nil;
}


- (id)init
{
    self = [super init];
    if(self != nil) {
        // --------------
        // initial coding
        // --------------
        eth_array = [[NSMutableArray alloc] init];
        rt_array =  [[NSMutableArray alloc] init];
        rt4_array = [[NSMutableArray alloc] init];
        rt6_array = [[NSMutableArray alloc] init];
    }
    [self getRT];
    [self getETH];
    return self;
}

- (void)dealloc
{
    // --------------
    // release coding
    // --------------
    [eth_array release];
    [rt_array release];
    [rt4_array release];
    [rt6_array release];
    [super dealloc];
    return;
}

- (bool)getETH
{
/*
*/
    size_t i;
    for (i=0; i< [eth_array count]; i++) {
        NSData* d;
        struct _eth* eth;
        d = [eth_array objectAtIndex:i];
        eth = (struct _eth*)[d bytes];
        [eth->eth_ifname release];
        [eth->eth_address release];
        [eth->eth_flags release];
    }
    [eth_array removeAllObjects];

    size_t needed;
    int mib[6];
    u_char *buf;
    u_char *next;
    u_char *limit;
    struct if_msghdr *ifm;

    
    mib[0] = CTL_NET;
    mib[1] = AF_ROUTE;
    mib[2] = 0;
    mib[3] = AF_LINK;
    mib[4] = NET_RT_IFLIST;
    mib[5] = 0;

    if (sysctl(mib, 6, NULL, &needed, NULL, 0) < 0) {
        perror("sysctl");
        return false;
    }

    if ((buf = (u_char*)malloc(needed)) == 0) {
        perror("malloc");
        return false;
    }
    memset(buf, 0, sizeof(buf));

    if (sysctl(mib, 6, buf, &needed, NULL, 0) < 0) {
        perror("sysctl");
        free(buf);
        return false;
    }
    limit  = buf + needed;
    for (next = buf; next < limit; next += ifm->ifm_msglen) {
        ifm = (struct if_msghdr *)next;
        [self _eth_process:ifm];
    }
    free(buf);
    return true;
}

- (void)_eth_process:(struct if_msghdr*)ifm {
    id pool = [NSAutoreleasePool new];

    struct sockaddr_dl *sdl;
    struct _eth eth;
    eth.eth_ifname  = nil;
    eth.eth_address = nil;
    eth.eth_flags   = nil;


    if (ifm->ifm_type == RTM_IFINFO) {
        sdl = (struct sockaddr_dl*)(ifm + 1);
        //printf("%s\n", sdl->sdl_data);  // interface name
        eth.eth_ifname = [NSString stringWithUTF8String:sdl->sdl_data];

        //printf("%x\n", ifm->ifm_flags); // intarface state
        NSMutableString* flags = [NSMutableString new];

        // U:IFF_UP          0x1    interface is up
        if ((ifm->ifm_flags) & IFF_UP) {
            [flags appendString:@"U."];
        }
        //IFF_BROADCAST   0x2    broadcast address valid
        if ((ifm->ifm_flags) & IFF_BROADCAST) {
            [flags appendString:@"B."];
        }
        //IFF_DEBUG       0x4    turn on debugging
        if ((ifm->ifm_flags) & IFF_DEBUG) {
            [flags appendString:@"D."];
        }
        //IFF_LOOPBACK    0x8    is a loopback net
        if ((ifm->ifm_flags) & IFF_LOOPBACK) {
            [flags appendString:@"L."];
        }
        //IFF_POINTOPOINT 0x10   interface is point-to-point link
        if ((ifm->ifm_flags) & IFF_POINTOPOINT) {
            [flags appendString:@"P."];
        }
        //IFF_NOTRAILERS  0x20   obsolete: avoid use of trailers
        if ((ifm->ifm_flags) & IFF_NOTRAILERS) {
            [flags appendString:@"T."];
        }
        //IFF_RUNNING     0x40   resources allocated
        if ((ifm->ifm_flags) & IFF_RUNNING) {
            [flags appendString:@"R."];
        }
        //IFF_NOARP       0x80   no address resolution protocol
        if ((ifm->ifm_flags) & IFF_NOARP) {
            [flags appendString:@"A."];
        }
        //IFF_PROMISC     0x100  receive all packets
        if ((ifm->ifm_flags) & IFF_PROMISC) {
            [flags appendString:@"P."];
        }
        //IFF_ALLMULTI    0x200  receive all multicast packets
        if ((ifm->ifm_flags) & IFF_ALLMULTI) {
            [flags appendString:@"RM."];
        }
        //IFF_OACTIVE     0x400  transmission in progress
        if ((ifm->ifm_flags) & IFF_OACTIVE) {
            [flags appendString:@"O."];
        }
        //IFF_SIMPLEX     0x800  can't hear own transmissions
        if ((ifm->ifm_flags) & IFF_SIMPLEX) {
            [flags appendString:@"S."];
        }
        //IFF_LINK0       0x1000 per link layer defined bit
        //IFF_LINK1       0x2000 per link layer defined bit
        //IFF_LINK2       0x4000 per link layer defined bit
        //IFF_MULTICAST   0x8000 supports multicast
        if ((ifm->ifm_flags) & IFF_SIMPLEX) {
            [flags appendString:@"SM."];
        }
        eth.eth_flags = [NSString stringWithString:flags];

        //memdump(sdl->sdl_data, 12);

        struct  ether_addr ether;
        memcpy(&ether, LLADDR(sdl), ETHER_ADDR_LEN);
        eth.eth_address = [NSString stringWithUTF8String:ether_ntoa(&ether)];

        [eth.eth_ifname retain];
        [eth.eth_address retain];
        [eth.eth_flags retain];

        [eth_array addObject:[NSData dataWithBytes:&eth length:sizeof(eth)]];

        [flags release];
        
    } 
    /*
    NSLog(@"addr:%d\n",[eth.eth_address retainCount]);
    NSLog(@"flag:%d\n",[eth.eth_flags retainCount]);
    NSLog(@"name:%d\n",[eth.eth_ifname retainCount]);
    NSLog(@"\n");
    */
    [pool drain];
    return;
}



- (void)_rt_getaddrs:(int)addrs
                    :(struct sockaddr*)sa
                    :(struct sockaddr**)rti_info
{
    int i;
    for (i = 0; i < RTAX_MAX; i++) {
        if (addrs & (1 << i)) {
            rti_info[i] = sa;
            sa = (struct sockaddr *)(ROUNDUP(sa->sa_len) + (char *)sa);
        } else {
            rti_info[i] = NULL;
        }
    }
}



- (void)_rt_process:(struct rt_msghdr2*)rtm {

    id pool = [NSAutoreleasePool new];

    struct _rt rt;
    rt.rt_destination = nil;
    rt.rt_family      = nil;
    rt.rt_gateway     = nil;
    rt.rt_netmask     = nil;
    //rt.rt_genmask     = nil;
    rt.rt_ifname      = nil;
    //rt.rt_ifmac       = nil;
    //rt.rt_author      = nil;
    //rt.rt_broadcast   = nil;
    rt.rt_flags       = nil;

    struct sockaddr* sa;
    struct sockaddr* rti_info[RTAX_MAX];

    char ifname[IFNAMSIZ];
    memset(ifname, '\0', sizeof(ifname));

    sa = (struct sockaddr *)(rtm + 1);

    // check address faimly
    if (sa->sa_family == AF_INET) {
        rt.rt_family = @"IPv4";
    }
    else if (sa->sa_family == AF_INET6) {
        rt.rt_family = @"IPv6";
    }
    else {
        [pool drain];
        return;
    }

    if_indextoname(rtm->rtm_index, ifname);
    rt.rt_ifname = [NSString stringWithUTF8String:ifname];
    //NSLog(@"ifname:%@", rt.rt_ifname);


    [self _rt_getaddrs:(rtm->rtm_addrs) :sa :rti_info];


    if ((rtm->rtm_addrs & RTA_DST)) {
        NSData* data;
        data = nil;
        if (sa->sa_family == AF_INET) {
            struct sockaddr_in* sin;
            sin = (struct sockaddr_in*)rti_info[RTAX_DST];
            //printf("sin->sin_len:%d\n", sin->sin_len);
            if (sin->sin_len == 0) {
                rt.rt_destination = @"0.0.0.0";
            } else if (sin->sin_len <= 16 || sin->sin_len > 0) {
                data = [NSData dataWithBytes:&sin->sin_addr length:sizeof(struct in_addr)];
                rt.rt_destination = [NSString stringWithUTF8String:ip_ntoa(rt.rt_family, data)];
            } else {
            }
        }
        if (sa->sa_family == AF_INET6) {
            struct sockaddr_in6* sin6;
            sin6 = (struct sockaddr_in6*)rti_info[RTAX_DST];
            if (sin6->sin6_len == 0) {
                rt.rt_destination = @"0";
            } else if (sin6->sin6_len <= 28 || sin6->sin6_len > 0){
                data = [NSData dataWithBytes:&sin6->sin6_addr length:sizeof(struct in6_addr)];
                rt.rt_destination = [NSString stringWithUTF8String:ip_ntoa(rt.rt_family, data)];
            } else {
            }
        }
        //[data release];
    } else {
        //rt.rt_destination = nil;
    }
    //NSLog(@"Destination:%@", rt.rt_destination);

    if ((rtm->rtm_addrs)&RTA_NETMASK) {
        NSData* data;
        data = nil;
        if (sa->sa_family == AF_INET) {
            struct sockaddr_in* sin;
            sin = (struct sockaddr_in*)rti_info[RTAX_NETMASK];
            //printf("sin->sin_len:%d\n", sin->sin_len);
            if (sin->sin_len == 0) {
                rt.rt_netmask = @"0.0.0.0";
            } else if (sin->sin_len <= 16 || sin->sin_len > 0) {
                data = [NSData dataWithBytes:&sin->sin_addr length:sizeof(struct in_addr)];
                rt.rt_netmask = [NSString stringWithUTF8String:ip_ntoa(rt.rt_family, data)];
            } else {
            }
        }
        if (sa->sa_family == AF_INET6) {
            struct sockaddr_in6* sin6;
            sin6 = (struct sockaddr_in6*)rti_info[RTAX_NETMASK];
            if (sin6->sin6_len == 0) {
                rt.rt_netmask = @"0";
            } else if (sin6->sin6_len <= 28 || sin6->sin6_len > 0){
                data = [NSData dataWithBytes:&sin6->sin6_addr length:sizeof(struct in6_addr)];
                rt.rt_netmask = [NSString stringWithUTF8String:ip_ntoa(rt.rt_family, data)];
            } else {
            }
        }
        //[data release];
    } else {
        //rt.rt_netmask = nil;
    }
    //NSLog(@"NETMASK:%@", rt.rt_netmask);

    if ((rtm->rtm_addrs)&RTA_GATEWAY) {
        NSData* data;
        data=nil;
        if (rti_info[RTAX_GATEWAY]->sa_family == AF_INET) {
            struct sockaddr_in* sin;
            sin = (struct sockaddr_in*)rti_info[RTAX_GATEWAY];
            data = [NSData dataWithBytes:&sin->sin_addr length:sizeof(struct in_addr)];
            rt.rt_gateway = [NSString stringWithUTF8String:ip_ntoa(rt.rt_family, data)];
        }
        if (rti_info[RTAX_GATEWAY]->sa_family == AF_INET6) {
            struct sockaddr_in6* sin6;
            sin6 = (struct sockaddr_in6*)rti_info[RTAX_GATEWAY];
            data = [NSData dataWithBytes:&sin6->sin6_addr length:sizeof(struct in6_addr)];
            rt.rt_gateway = [NSString stringWithUTF8String:ip_ntoa(rt.rt_family, data)];
        }
        if (rti_info[RTAX_GATEWAY]->sa_family == AF_LINK) {
            struct sockaddr_dl *sdl;
            sdl = (struct sockaddr_dl*)rti_info[RTAX_GATEWAY];
            if (sdl->sdl_nlen == 0 && sdl->sdl_alen == 0 && sdl->sdl_slen == 0) {
                rt.rt_gateway = [NSString stringWithFormat:@"link#%d",sdl->sdl_index];
            } else { 
                //rt.rt_gateway = [NSString stringWithUTF8String:link_ntoa(sdl)];
                struct  ether_addr ether;
                memcpy(&ether, LLADDR(sdl), ETHER_ADDR_LEN);
                rt.rt_gateway = [NSString stringWithUTF8String:ether_ntoa(&ether)];
            }
        }
        //[data release];
    } else {
        //rt.rt_gateway = nil;
    }
    //NSLog(@"GATEWAY:%@", rt.rt_gateway);


    NSMutableString* flags = [NSMutableString new];
    // 1 RTF_PROTO1    Protocol specific routing flag #1
    if ((rtm->rtm_flags) & RTF_PROTO1) {
        [flags appendString:@"1."];
    }
    // 2 RTF_PROTO2    Protocol specific routing flag #2
    if ((rtm->rtm_flags) & RTF_PROTO2) {
        [flags appendString:@"2."];
    }
    // 3 RTF_PROTO3    Protocol specific routing flag #3
    if ((rtm->rtm_flags) & RTF_PROTO3) {
        [flags appendString:@"3."];
    }
    // B RTF_BLACKHOLE Just discard packets (during updates)
    if ((rtm->rtm_flags) & RTF_BLACKHOLE) {
        [flags appendString:@"B."];
    }
    // b RTF_BROADCAST The route represents a broadcast address
    if ((rtm->rtm_flags) & RTF_BROADCAST) {
        [flags appendString:@"b."];
    }
    // C RTF_CLONING   Generate new routes on use
    if ((rtm->rtm_flags) & RTF_CLONING) {
        [flags appendString:@"C."];
    }
    // c RTF_PRCLONING Protocol-specified generate new routes on use
    if ((rtm->rtm_flags) & RTF_PRCLONING) {
        [flags appendString:@"c."];
    }
    // D RTF_DYNAMIC   Created dynamically (by redirect)
    if ((rtm->rtm_flags) & RTF_DYNAMIC) {
        [flags appendString:@"D."];
    }
    // G RTF_GATEWAY   Destination requires forwarding by intermediary
    if ((rtm->rtm_flags) & RTF_GATEWAY) {
        [flags appendString:@"G."];
        if ((rt.rt_netmask == nil) && rt.rt_destination) {
            [flags release];
            [pool drain];
            return;
        }
    }
    // H RTF_HOST      Host entry (net otherwise)
    if ((rtm->rtm_flags) & RTF_HOST) {
        [flags appendString:@"H."];
    }
    // I RTF_IFSCOPE   Route is associated with an interface scope
    if ((rtm->rtm_flags) & RTF_IFSCOPE) {
        [flags appendString:@"I."];
    }
    // L RTF_LLINFO    Valid protocol to link address translation
    if ((rtm->rtm_flags) & RTF_LLINFO) {
        [flags appendString:@"L."];
    }
    // M RTF_MODIFIED  Modified dynamically (by redirect)
    if ((rtm->rtm_flags) & RTF_MODIFIED) {
        [flags appendString:@"M."];
    }
    // m RTF_MULTICAST The route represents a multicast address
    if ((rtm->rtm_flags) & RTF_MULTICAST) {
        [flags appendString:@"m."];
    }
    // R RTF_REJECT    Host or net unreachable
    if ((rtm->rtm_flags) & RTF_REJECT) {
        [flags appendString:@"R."];
    }
    // S RTF_STATIC    Manually added
    if ((rtm->rtm_flags) & RTF_STATIC) {
        [flags appendString:@"S."];
    }
    // U RTF_UP        Route usable
    if ((rtm->rtm_flags) & RTF_UP) {
        [flags appendString:@"U."];
    }
    // W RTF_WASCLONED Route was generated as a result of cloning
    if ((rtm->rtm_flags) & RTF_WASCLONED) {
        [flags appendString:@"W."];
    }
    // X RTF_XRESOLVE  External daemon translates proto to link address
    if ((rtm->rtm_flags) & RTF_XRESOLVE) {
        [flags appendString:@"X."];
    }
    rt.rt_flags = [NSString stringWithString:flags];
    //NSLog(@"flag:%@", rt.rt_flags);

    [rt.rt_destination retain];
    [rt.rt_family      retain];
    [rt.rt_gateway     retain];
    [rt.rt_netmask     retain];
    //[rt.rt_genmask     retain];
    [rt.rt_ifname      retain];
    //[rt.rt_ifmac       retain];
    //[rt.rt_author      retain];
    //[rt.rt_broadcast   retain];
    [rt.rt_flags       retain];

    if ([rt.rt_family isEqualToString:@"IPv4"])
        [rt4_array addObject:[NSData dataWithBytes:&rt length:sizeof(rt)]];
    if ([rt.rt_family isEqualToString:@"IPv6"])
        [rt6_array addObject:[NSData dataWithBytes:&rt length:sizeof(rt)]];

    [rt_array addObject:[NSData dataWithBytes:&rt length:sizeof(rt)]];

/*
    NSLog(@"flag:%d\n",[rt.rt_flags retainCount]);
    NSLog(@"dest:%d\n",[rt.rt_destination retainCount]);
    NSLog(@"gate:%d\n",[rt.rt_gateway retainCount]);
    NSLog(@"mask:%d\n",[rt.rt_netmask retainCount]);
    NSLog(@"name:%d\n",[rt.rt_ifname retainCount]);
    NSLog(@"\n");
*/

    [flags release];
    [pool drain];
    return;
}
 

- (bool)refresh {
    return [self getRT] & [self getETH];
}

- (bool)getRT {

/*
    NSLog(@"start\n");
    NSLog(@"end\n");
*/

    size_t i;
    for (i=0; i< [rt_array count]; i++) {
        NSData* d;
        struct _rt* rt;
        d = [rt_array objectAtIndex:i];
        rt = (struct _rt*)[d bytes];

        [rt->rt_destination release];
        [rt->rt_family      release];
        [rt->rt_gateway     release];
        [rt->rt_netmask     release];
        //[rt->rt_genmask     release];
        [rt->rt_ifname      release];
        //[rt->rt_ifmac       release];
        //[rt->rt_author      release];
        //[rt->rt_broadcast   release];
        [rt->rt_flags       release];
    }

    [rt_array removeAllObjects];
    [rt4_array removeAllObjects];
    [rt6_array removeAllObjects];

    size_t needed;
    int mib[6];
    u_char *buf;
    u_char *next;
    u_char *limit;
    struct rt_msghdr2 *rtm;     
    
    mib[0] = CTL_NET;
    mib[1] = AF_ROUTE;  
    mib[2] = 0;
    mib[3] = 0;
    mib[4] = NET_RT_DUMP2;  
    mib[5] = 0;

    if (sysctl(mib, 6, NULL, &needed, NULL, 0) < 0) {
        perror("sysctl");
        return false;
    }

    if ((buf = (u_char*)malloc(needed)) == 0) {
        perror("malloc");
        return false;
    }
    memset(buf, 0, sizeof(buf));

    if (sysctl(mib, 6, buf, &needed, NULL, 0) < 0) {
        perror("sysctl");
        free(buf);
        return false;
    }
    limit  = buf + needed;

    for (next = buf; next < limit; next += rtm->rtm_msglen) {
        rtm = (struct rt_msghdr2 *)next;
        [self _rt_process:rtm];
    }

    free(buf);
    return true;
}



// getter ------------------------------------------------------
// ethernet
- (int)eth_icount {
    return [eth_array icount];
}

- (uint32_t)eth_count {
    return [eth_array count];
}

- (NSString*)eth_ifname:(uint32_t)i;
{
    if (i > [eth_array count]) return nil;
    struct _eth* eth;
    eth = (struct _eth*)[[eth_array objectAtIndex:i] bytes];
    return eth->eth_ifname;
}

- (NSString*)eth_flags:(uint32_t)i;
{
    if (i > [eth_array count]) return nil;
    struct _eth* eth;
    eth = (struct _eth*)[[eth_array objectAtIndex:i] bytes];
    return eth->eth_flags;
}

- (NSString*)eth_address:(uint32_t)i;
{
    if (i > [eth_array count]) return nil;
    struct _eth* eth;
    eth = (struct _eth*)[[eth_array objectAtIndex:i] bytes];
    return eth->eth_address;
}


// routing table
- (int)rt_icount {
    return [rt_array icount];
}

- (uint32_t)rt_count {
    return [rt_array count];
}

- (NSString*)rt_ifname:(uint32_t)i;
{
    if (i > [rt_array count]) return nil;
    struct _rt* rt;
    rt = (struct _rt*)[[rt_array objectAtIndex:i] bytes];
    return rt->rt_ifname;
}

- (NSString*)rt_flags:(uint32_t)i;
{
    if (i > [rt_array count]) return nil;
    struct _rt* rt;
    rt = (struct _rt*)[[rt_array objectAtIndex:i] bytes];
    return rt->rt_flags;
}

- (NSString*)rt_family:(uint32_t)i
{
    if (i > [rt_array count]) return nil;
    struct _rt* rt;
    rt = (struct _rt*)[[rt_array objectAtIndex:i] bytes];
    return rt->rt_family;
}

- (NSString*)rt_gateway:(uint32_t)i;
{
    if (i > [rt_array count]) return nil;
    struct _rt* rt;
    rt = (struct _rt*)[[rt_array objectAtIndex:i] bytes];
    return rt->rt_gateway;
}

- (NSString*)rt_netmask:(uint32_t)i;
{
    if (i > [rt_array count]) return nil;
    struct _rt* rt;
    rt = (struct _rt*)[[rt_array objectAtIndex:i] bytes];
    return rt->rt_netmask;
}
- (NSString*)rt_destination:(uint32_t)i;
{
    if (i > [rt_array count]) return nil;
    struct _rt* rt;
    rt = (struct _rt*)[[rt_array objectAtIndex:i] bytes];
    return rt->rt_destination;
}


// routing table ipv4
- (int)rt4_icount {
    return [rt4_array icount];
}

- (uint32_t)rt4_count {
    return [rt4_array count];
}

- (NSString*)rt4_ifname:(uint32_t)i;
{
    if (i > [rt4_array count]) return nil;
    struct _rt* rt;
    rt = (struct _rt*)[[rt4_array objectAtIndex:i] bytes];
    return rt->rt_ifname;
}


- (NSString*)rt4_flags:(uint32_t)i;
{
    if (i > [rt4_array count]) return nil;
    struct _rt* rt;
    rt = (struct _rt*)[[rt4_array objectAtIndex:i] bytes];
    return rt->rt_flags;
}

- (NSString*)rt4_family:(uint32_t)i
{
    if (i > [rt4_array count]) return nil;
    struct _rt* rt;
    rt = (struct _rt*)[[rt4_array objectAtIndex:i] bytes];
    return rt->rt_family;
}

- (NSString*)rt4_gateway:(uint32_t)i;
{
    if (i > [rt4_array count]) return nil;
    struct _rt* rt;
    rt = (struct _rt*)[[rt4_array objectAtIndex:i] bytes];
    return rt->rt_gateway;
}

- (NSString*)rt4_netmask:(uint32_t)i;
{
    if (i > [rt4_array count]) return nil;
    struct _rt* rt;
    rt = (struct _rt*)[[rt4_array objectAtIndex:i] bytes];
    return rt->rt_netmask;
}
- (NSString*)rt4_destination:(uint32_t)i;
{
    if (i > [rt4_array count]) return nil;
    struct _rt* rt;
    rt = (struct _rt*)[[rt4_array objectAtIndex:i] bytes];
    return rt->rt_destination;
}

// routing table ipv6
- (int)rt6_icount {
    return [rt6_array icount];
}

- (uint32_t)rt6_count {
    return [rt6_array count];
}

- (NSString*)rt6_ifname:(uint32_t)i;
{
    if (i > [rt6_array count]) return nil;
    struct _rt* rt;
    rt = (struct _rt*)[[rt6_array objectAtIndex:i] bytes];
    return rt->rt_ifname;
}


- (NSString*)rt6_flags:(uint32_t)i;
{
    if (i > [rt6_array count]) return nil;
    struct _rt* rt;
    rt = (struct _rt*)[[rt6_array objectAtIndex:i] bytes];
    return rt->rt_flags;
}

- (NSString*)rt6_family:(uint32_t)i
{
    if (i > [rt6_array count]) return nil;
    struct _rt* rt;
    rt = (struct _rt*)[[rt6_array objectAtIndex:i] bytes];
    return rt->rt_family;
}

- (NSString*)rt6_gateway:(uint32_t)i;
{
    if (i > [rt6_array count]) return nil;
    struct _rt* rt;
    rt = (struct _rt*)[[rt6_array objectAtIndex:i] bytes];
    return rt->rt_gateway;
}

- (NSString*)rt6_netmask:(uint32_t)i;
{
    if (i > [rt6_array count]) return nil;
    struct _rt* rt;
    rt = (struct _rt*)[[rt6_array objectAtIndex:i] bytes];
    return rt->rt_netmask;
}
- (NSString*)rt6_destination:(uint32_t)i;
{
    if (i > [rt6_array count]) return nil;
    struct _rt* rt;
    rt = (struct _rt*)[[rt6_array objectAtIndex:i] bytes];
    return rt->rt_destination;
}

- (NSString*)defaultRoute4;
{
    struct _rt* rt;
    rt = (struct _rt*)[[rt4_array objectAtIndex:0] bytes];
    if ([rt->rt_netmask isEqualToString:@"0.0.0.0"]) {
        if ([rt->rt_destination isEqualToString:@"0.0.0.0"]) {
            return rt->rt_gateway;
        }
    }
    return nil;
}

- (NSString*)defaultRoute6;
{
    struct _rt* rt;
    rt = (struct _rt*)[[rt6_array objectAtIndex:0] bytes];
    if ([rt->rt_netmask isEqualToString:@"0"]) {
        if ([rt->rt_destination isEqualToString:@"::"]) {
            return rt->rt_gateway;
        }
    }
    return nil;
}

@end //implementation

#endif //__RAPRINS_RT_H_
