#ifndef __RAPRINS_PBUF_H_
#define __RAPRINS_PBUF_H_

// a brief sketch if pbuf
#import <Cocoa/Cocoa.h>

#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "common.h"
#include "utils.h"
#include "category.h"

@interface PacketBuffer : NSObject {

        NSMutableData*  pbufData;
        NSString*       pbufL3proto;
        NSString*       pbufL4proto;
        struct ip*      pbufIPv4;
        struct ip6_hdr* pbufIPv6;
        struct udphdr*  pbufUDP;
        struct tcphdr*  pbufTCP;
        uint8_t*        pbufL3;
        uint8_t*        pbufL4;
        uint8_t*        payload;
        size_t         pbufL3hlen;
        size_t         pbufL4hlen;

        struct in6_addr pbufSrcIPv6;
        struct in6_addr pbufDstIPv6;
        uint32_t        pbufSrcIPv4;
        uint32_t        pbufDstIPv4;
        uint16_t        pbufSrcPort;
        uint16_t        pbufDstPort;

    /*
    @protected
    @private
    @public
    */
}

// public function
- (id)init;
- (void)dealloc;
- (const void*)bytes;
- (size_t)length;
- (bool)sync;
- (bool)withBytes:(unsigned char*)buf :(size_t)len;
- (bool)withData:(NSData*)data;
- (uint8_t*)getL3;
- (uint8_t*)getL4;
- (uint8_t*)getL7;
- (bool)setL7:(unsigned char*)buf :(size_t)len;
- (size_t)getL3len;
- (size_t)getL4len;
- (size_t)getL7len;
- (NSString*)getL3Proto;
- (NSString*)getL4Proto;
- (NSString*)getL3Src;
- (NSString*)getL3Dst;
- (uint16_t)getL4Src;
- (uint16_t)getL4Dst;

// internal function
- (bool)_analyzePBUF:(uint8_t*)buf :(size_t)size;
- (void)_formatPBUF;
- (uint16_t)_checksum:(const uint8_t*)buf
                     :(size_t)size
                     :(uint32_t)adjust;
- (void)_checksum_ipv4;
- (void)_checksum_tcp_ipv4;
- (void)_checksum_udp_ipv4;

// non implementation
//- (void)_checksum_ipv6;
//- (void)_checksum_icmp_ipv4;
//- (void)_checksum_tcp_ipv6;
//- (void)_checksum_udp_ipv6;
//- (void)_checksum_icmp_ipv6;
@end

@implementation PacketBuffer
- (id)init {

    self = [super init];
    if(self != nil) {
        // --------------
        // initial coding
        // --------------
        //pbufData = [[[NSMutableData alloc] init] autorelease];
        pbufData = [[NSMutableData alloc] init];
        [self _formatPBUF];
    }

    return self;
}


- (void)dealloc {

    // --------------
    // release coding
    // --------------
    [pbufData release];
    [super dealloc];

    return;
}


- (uint16_t)_checksum:(const uint8_t*)buf
                     :(size_t)size
                     :(uint32_t)adjust                     
{

    uint32_t sum = 0;
    uint16_t element = 0;

    while (size>0) {
        element = (*buf)<<8;
        buf++;
        size--;
        if (size>0) {
            element |= *buf;
            buf++;
            size--;
        }
        sum += element;
    }
    sum += adjust;

    // --bug--- shift size is less than 31
    while (sum>0xFFFF) {
        sum = (sum>>16) + (sum&0xFFFF);
    }

    return (~sum) & 0xFFFF;
}

- (void)_checksum_ipv4 {
    pbufIPv4->ip_sum = 0x0000;
    uint16_t sum = [self _checksum:(const uint8_t*)pbufIPv4 :pbufL3hlen :0];
    pbufIPv4->ip_sum = htons(sum);
}


/*
 * 0                16               31
 * +----------------+----------------+       ∧
 * |       Source IPv4 Address       |       |
 * +----------------+----------------+       |
 * |     Destination IPv4 Address    | pseudo-header
 * +--------+-------+----------------+       |
 * | dummmy | Proto | TCP/UDP SegLen |       |
 * +--------+-------+----------------+       ∨
 * :                                 :
 */
- (void)_checksum_udp_ipv4 {

    uint32_t pseudoSum = 0;

    // Src Address ipv4
    pseudoSum += (pbufL3[12]<<8) | pbufL3[13];
    pseudoSum += (pbufL3[14]<<8) | pbufL3[15];

    // Dst Address ipv4
    pseudoSum += (pbufL3[16]<<8) | pbufL3[17];
    pseudoSum += (pbufL3[18]<<8) | pbufL3[19];

    // igonre dummpy
    // Protocol Number
    pseudoSum += (uint8_t)IPPROTO_UDP;

    // Segument Length (top of tcp_heder to end of payload)
    size_t size = [pbufData length] - pbufL3hlen;
    pseudoSum += size;

    pbufUDP->uh_sum = 0x0000;
    uint16_t sum = [self _checksum:(const uint8_t*)pbufUDP :size :pseudoSum];
    pbufUDP->uh_sum = htons(sum);

    return;
}

- (void)_checksum_tcp_ipv4 {

    uint32_t pseudoSum = 0;

    // Src Address ipv4
    pseudoSum += (pbufL3[12]<<8) | pbufL3[13];
    pseudoSum += (pbufL3[14]<<8) | pbufL3[15];

    // Dst Address ipv4
    pseudoSum += (pbufL3[16]<<8) | pbufL3[17];
    pseudoSum += (pbufL3[18]<<8) | pbufL3[19];

    // igonre dummpy
    // Protocol Number
    pseudoSum += (uint8_t)IPPROTO_TCP;

    // Segument Length (top of tcp_heder to end of payload)
    size_t size = [pbufData length] - pbufL3hlen;
    pseudoSum += size;

    pbufTCP->th_sum = 0x0000;
    uint16_t sum = [self _checksum:pbufL4 :size :pseudoSum];
    pbufTCP->th_sum = htons(sum);

    return;
}

- (void) _formatPBUF {

    [pbufData setData:nil];
    pbufL3proto = nil;
    pbufL4proto = nil;
    pbufIPv4 = NULL;
    pbufIPv6 = NULL;
    pbufL3   = NULL;
    pbufL4   = NULL;
    pbufTCP  = NULL;
    pbufUDP  = NULL;
    payload  = NULL;
    pbufL3hlen  = 0;
    pbufL4hlen  = 0;
    pbufSrcIPv4 = 0;
    pbufDstIPv4 = 0;
    pbufSrcPort = 0;
    pbufDstPort = 0;
    memset(&pbufSrcIPv6, 0, sizeof(struct in6_addr));
    memset(&pbufDstIPv6, 0, sizeof(struct in6_addr));

    return;
}


- (bool)sync {
    bool retflag;
    retflag = [self _analyzePBUF:pbufL3 :[pbufData length]];
    if (retflag == true) {
        if ([pbufL3proto isEqualToString:@"IPv4"] & [pbufL4proto isEqualToString:@"TCP"]) {
            [self _checksum_ipv4];
            [self _checksum_tcp_ipv4];
            return true;
        }
        if ([pbufL3proto isEqualToString:@"IPv6"] & [pbufL4proto isEqualToString:@"TCP"]) {
            // non implementation
            return false;
        }
        if ([pbufL3proto isEqualToString:@"IPv4"] & [pbufL4proto isEqualToString:@"UDP"]) {
            [self _checksum_ipv4];
            [self _checksum_udp_ipv4];
            return true;
        }
        if ([pbufL3proto isEqualToString:@"IPv6"] & [pbufL4proto isEqualToString:@"UDP"]) {
            // non implementation
            return false;
        }
    } else {
        return false;
    }
    return false;
}


#define L4CHECK(L3proto) {                              \
    if (L3proto == IPPROTO_TCP) {                       \
        pbufTCP = (struct tcphdr*)(pbufL3 + offset);    \
        pbufSrcPort = ntohs(pbufTCP->th_sport);         \
        pbufDstPort = ntohs(pbufTCP->th_dport);         \
        pbufL4 = (uint8_t*)(pbufL3 + offset);           \
        pbufL4hlen = (size_t)(pbufTCP->th_off * 4);     \
        pbufL4proto = @"TCP";                           \
        offset += pbufL4hlen;                           \
    } else if (L3proto == IPPROTO_UDP) {                \
        pbufUDP = (struct udphdr*)(pbufL3 + offset);    \
        pbufUDP->uh_ulen = htons(size - offset);        \
        pbufSrcPort = ntohs(pbufUDP->uh_sport);         \
        pbufDstPort = ntohs(pbufUDP->uh_dport);         \
        pbufL4 = (uint8_t*)(pbufL3 + offset);           \
        pbufL4hlen = (size_t)SIZE_UDP_HDR;              \
        pbufL4proto = @"UDP";                           \
        offset += pbufL4hlen;                           \
    } else if (L3proto == IPPROTO_ICMP) {               \
        [self _formatPBUF];                             \
        return false;                                   \
    } else {                                            \
        [self _formatPBUF];                             \
        return false;                                   \
    }                                                   \
}


- (bool)_analyzePBUF:(uint8_t*)buf :(size_t)size {

    size_t  offset = 0;

    if(((struct ip*)pbufL3)->ip_v == IPPROTO_IPV4) {

        pbufIPv4 = (struct ip*)pbufL3;
        pbufL3hlen = pbufIPv4->ip_hl * 4;
        pbufIPv4->ip_len = htons(size);
        //printf("ip_len:%d\n",ntohs(pbufIPv4->ip_len));
        pbufSrcIPv4 = pbufIPv4->ip_src.s_addr;
        pbufDstIPv4 = pbufIPv4->ip_dst.s_addr;
        offset += pbufL3hlen;
        pbufL3proto = @"IPv4";
        L4CHECK(pbufIPv4->ip_p);

    } else if(((struct ip*)pbufL3)->ip_v == IPPROTO_IPV6) {

        pbufIPv6 = (struct ip6_hdr*)pbufL3;
        pbufL3hlen = SIZE_IP6_HDR;
        pbufIPv6->ip6_plen = htons(size-pbufL3hlen);
        memcpy(&pbufSrcIPv6, &(pbufIPv6->ip6_src), sizeof(struct in6_addr));
        memcpy(&pbufDstIPv6, &(pbufIPv6->ip6_dst), sizeof(struct in6_addr));
        offset += pbufL3hlen;
        pbufL3proto = @"IPv6";
        L4CHECK(pbufIPv6->ip6_nxt);

    } else {

        // another L3 Protocols
        // non implimentation
        [self _formatPBUF];
        return false;

    }

    payload = (uint8_t*)(pbufL3 + offset);



    return true;
}
#undef L4CHECK

- (bool)withData:(NSData*)data{

    [pbufData appendBytes:[data bytes] length:[data length]];
    pbufL3 = (uint8_t*)[pbufData bytes];

    return [self _analyzePBUF:pbufL3 :[pbufData length]];
}


- (bool)withBytes:(unsigned char*)buf :(size_t)len {
    
    [pbufData appendBytes:buf length:len];
    pbufL3 = (uint8_t*)[pbufData bytes];
    return [self _analyzePBUF:pbufL3 :[pbufData length]];

    //printf("pbuf-internal->%d:%p\n", __LINE__, pbufL3);
    // icmp==1, tcp==6, udp==17, icmp6==58

    /*
    size_t  offset = 0;
    if(((struct ip*)pbufL3)->ip_v == IPPROTO_IPV4) {
        pbufIPv4 = (struct ip*)pbufL3;
        pbufL3hlen = pbufIPv4->ip_hl * 4;
        pbufSrcIPv4 = pbufIPv4->ip_src.s_addr;
        pbufDstIPv4 = pbufIPv4->ip_dst.s_addr;
        offset += pbufL3hlen;
        pbufL3proto = @"IPv4";
        if (pbufIPv4->ip_p == IPPROTO_TCP) {
            pbufTCP = (struct tcphdr*)(pbufL3 + offset);
            pbufSrcPort = ntohs(pbufTCP->th_sport); 
            pbufDstPort = ntohs(pbufTCP->th_dport);
            pbufL4 = (uint8_t*)(pbufL3 + offset);
            pbufL4hlen = (size_t)(pbufTCP->th_off * 4);
            pbufL4proto = @"TCP";
            offset += pbufL4hlen;
        } else if (pbufIPv4->ip_p == IPPROTO_UDP) {
            pbufUDP = (struct udphdr*)(pbufL3 + offset);
            pbufSrcPort = ntohs(pbufUDP->uh_sport); 
            pbufDstPort = ntohs(pbufUDP->uh_dport);
            pbufL4 = (uint8_t*)(pbufL3 + offset);
            pbufL4hlen = (size_t)SIZE_UDP_HDR;
            pbufL4proto = @"UDP";
            offset += pbufL4hlen;
        } else {
            // another L4 Protocols
            // non implimentation
            [self _formatPBUF];
            return;
        }
    } else if(((struct ip*)pbufL3)->ip_v == IPPROTO_IPV6) {
        pbufIPv6 = (struct ip6_hdr*)pbufL3;
        pbufL3hlen = SIZE_IP6_HDR;
        memcpy(&pbufSrcIPv6, &(pbufIPv6->ip6_src), sizeof(struct in6_addr));
        memcpy(&pbufDstIPv6, &(pbufIPv6->ip6_dst), sizeof(struct in6_addr));
        offset += pbufL3hlen;
        pbufL3proto = @"IPv6";
        if (pbufIPv6->ip6_nxt == IPPROTO_TCP) {
            pbufTCP = (struct tcphdr*)(pbufL3 + offset);
            pbufSrcPort = ntohs(pbufTCP->th_sport); 
            pbufDstPort = ntohs(pbufTCP->th_dport);
            pbufL4 = (uint8_t*)(pbufL3 + offset);
            pbufL4hlen = (size_t)(pbufTCP->th_off * 4);
            pbufL4proto = @"TCP";
            offset += pbufL4hlen;
        } else if (pbufIPv6->ip6_nxt == IPPROTO_UDP) {
            pbufUDP = (struct udphdr*)(pbufL3 + offset);
            pbufSrcPort = ntohs(pbufUDP->uh_sport); 
            pbufDstPort = ntohs(pbufUDP->uh_dport);
            pbufL4 = (uint8_t*)(pbufL3 + offset);
            pbufL4hlen = (size_t)SIZE_UDP_HDR;
            pbufL4proto = @"UDP";
            offset += pbufL4hlen;
        } else {
            // another L4 Protocols
            // non implimentation
            [self _formatPBUF];
            return;
        }
    } else {
        // another L3 Protocols
        // non implimentation
        [self _formatPBUF];
        return;
    }
    payload = (uint8_t*)(pbufL3 + offset);
    */
}



- (NSString*)getL3Src {

    if ([pbufL3proto isEqualToString:@"IPv4"]) {

        NSData* data = [NSData dataWithBytes:&pbufSrcIPv4 length:sizeof(pbufSrcIPv4)];
        NSString* str = [NSString stringWithUTF8String: ip_ntoa(pbufL3proto, data)];
        return str;

    } else if ([pbufL3proto isEqualToString:@"IPv6"]) {

        NSData* data = [NSData dataWithBytes:&pbufSrcIPv6 length:sizeof(pbufSrcIPv6)];
        NSString* str = [NSString stringWithUTF8String: ip_ntoa(pbufL3proto, data)];
        return str;

    } else {
        return nil;
    }
    return nil;
}

- (NSString*)getL3Dst {

    if ([pbufL3proto isEqualToString:@"IPv4"]) {

        NSData* data = [NSData dataWithBytes:&pbufDstIPv4 length:sizeof(pbufDstIPv4)];
        NSString* str = [NSString stringWithUTF8String: ip_ntoa(pbufL3proto, data)];
        return str;

    } else if ([pbufL3proto isEqualToString:@"IPv6"]) {

        NSData* data = [NSData dataWithBytes:&pbufDstIPv6 length:sizeof(pbufDstIPv6)];
        NSString* str = [NSString stringWithUTF8String: ip_ntoa(pbufL3proto, data)];
        return str;

    } else {
        return nil;
    }
    return nil;
}


- (uint16_t)getL4Src {
    return pbufSrcPort;
}


- (uint16_t)getL4Dst {
    return pbufDstPort;
}


- (const void*)bytes {
    return [pbufData bytes];
}


- (size_t)length {
    return [pbufData length];
}


- (uint8_t*)getL3 {
    //printf("getL3->%d:%p\n", __LINE__, pbufL3);
    return pbufL3;
}


- (uint8_t*)getL4 {
    //printf("getL4->%d:%p\n", __LINE__, pbufL4);
    return pbufL4;
}


- (uint8_t*)getL7 {
    //printf("getL7->%d:%p\n", __LINE__, payload);
    return payload;
}


- (bool)setL7:(unsigned char*)buf :(size_t)len {

    size_t header_size;
    unsigned char* header;
    header_size = [self getL3len] - [self getL7len];

    if (header_size <= 0) {
        return false;
    }

    header = (unsigned char*)malloc(header_size);
    if (header == NULL) {
        return false;
    }

    memcpy(header, pbufL3, header_size);
    [self _formatPBUF];
    [pbufData appendBytes:header length:header_size];
    [pbufData appendBytes:buf length:len];
    free(header);
    pbufL3 = (uint8_t*)[pbufData bytes];

    //return [self _analyzePBUF:pbufL3 :[pbufData length]];
    return [self sync];
}


- (size_t)getL3len {
    return ([pbufData length]);
}


- (size_t)getL4len {
    return ([pbufData length]-pbufL3hlen);
}


- (size_t)getL7len {
    return ([pbufData length]-pbufL3hlen-pbufL4hlen);
}


- (NSString*)getL3Proto {
    return pbufL3proto;
}


- (NSString*)getL4Proto {
    return pbufL4proto;
}

@end
/*
// ---------------------
// utilize of this class
// ---------------------
int main(int argc, char** argv)
{
    id pool = [[NSAutoreleasePool alloc] init];
@try {
    id pbuf = [[PacketBuffer alloc] init];
}
@catch(id ex) {
    NSLog(@"%@", ex);
}
@finally {
}
    [pool release];
    return 0;
}
*/
#endif
