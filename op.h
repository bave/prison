#ifndef __PRISON_OP_H_
#define __PRISON_OP_H_

#import <Cocoa/Cocoa.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <resolv.h>
#include <nameser.h>

#include <sys/uio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/kern_event.h>


#include "name.h"
#include "ni.h"
#include "pbuf.h"
#include "common.h"
#include "category.h"
#include "utils.h"
#include "mgmt.h"

#include "obs.h"

#define OP_NAME   0
#define OP_ME2L   1
#define OP_EXT2ME 2
#define OP_ROUTE  3

extern NSLock* extLock;
extern NSLock*  niLock;
extern Manager*   mgmt;
extern NetInfo*     ni; 
extern FWHooker*    fw;
extern Observer*   obs;
extern bool is_verbose;

@interface Operation : NSOperation {
    // if you need set argument
    int socketFD;
    int selector;
}
@property (assign, readwrite) int socketFD;  
@property (assign, readwrite) int selector;

/*
@property (
        {retain, assign, copy},
        {readonly, readwrite},
        getter=[get_name],
        setter=[set_name:]
    ) int number;  
// XXX 数値の引数だったら assign
// XXX ポインターの引数だったらraitain
// XXX コンテナコレクション引数だったらcopy
// XXX ってなつかいわけな気がする．
*/

// public function
- (id)init;
- (void)dealloc;
- (void)main;
- (void)nameOperation;
- (void)me2lOperation;
- (void)ext2meOperation;
- (void)routeOperation;

// private function

@end



@implementation Operation  

@synthesize socketFD;  
@synthesize selector;  

- (id)init {
    self = [super init];
    if(self != nil) {
        // --------------
        // initial coding
        // --------------
    }
    return self;
}

- (void)dealloc
{
    // --------------
    // release coding
    // --------------
    [super dealloc];
    return;
}

- (void)main
{
    id pool = [NSAutoreleasePool new];
    switch(selector) {
        case OP_NAME: {
            [self nameOperation];
            break;
        }
        case OP_ME2L: {
            [self me2lOperation];
            break;
        }
        case OP_EXT2ME: {
            [self ext2meOperation];
            break;
        }
        case OP_ROUTE: {
            [self routeOperation];
            break;
        }
        default: {
            break;
        }
    }
    [pool drain];
    return;
}

- (void)nameOperation
{
    id pool = [NSAutoreleasePool new];

    int name_id;
    ns_msg ns_handle;
    //struct sockaddr_in sin_recv;
    struct sockaddr_storage sin_recv;
    socklen_t len;
    ssize_t  size;
    uint8_t  buf[SIZE_RECV_BUFFER];

    for(;;) {
        id loop_pool = [[NSAutoreleasePool alloc] init];

        // recv name packet ----------------------------------------------------
        len = (socklen_t)sizeof(sin_recv);
        memset(&buf, 0, sizeof(buf));
        size = recvfrom(socketFD, buf, sizeof(buf), 0, (SA*)&sin_recv, &len);
        if (size < 0) {
            perror("recvfrom");
            raise(SIGINT);
        }
        // ---------------------------------------------------------------------

        if (is_linking == false) {
            [loop_pool drain];
            continue;
        }

        // set packetbuffer ----------------------------------------------------
        id pbuf = [[[PacketBuffer alloc] init] autorelease];
        [pbuf withBytes:buf :size];
        // ---------------------------------------------------------------------


        // name parser ---------------------------------------------------------
        memset(&ns_handle, 0, sizeof(ns_handle));
        ns_initparse([pbuf getL7],  [pbuf getL7len], &ns_handle);
        name_id = ns_msg_id(ns_handle);
        // ---------------------------------------------------------------------


        // get fqdn ------------------------------------------------------------
        int type;
        NSString* fqdn;
        NSArray* fqdn_array;
        ns_rr rr;
        memset(&rr, 0, sizeof(rr));
        ns_parserr(&ns_handle, ns_s_qd, 0, &rr);
        fqdn = [NSString stringWithUTF8String: ns_rr_name(rr)];
        type = ns_rr_type(rr);
        // ---------------------------------------------------------------------


        // ネーム空間の分割判定
        fqdn_array = array_split(fqdn, @".");
        if (is_verbose) {
            NSLog(@"%@", fqdn_array);
        }


        if ([[fqdn_array lastObject] isEqualToString:@"p2p"]) {
            // address swapping ------------------------------------------------
            struct ip* ip;
            ip = (struct ip*)[pbuf getL3];
            memswap(&ip->ip_src.s_addr, &ip->ip_dst.s_addr, sizeof(uint32_t));
            struct udphdr* udp;
            udp = (struct udphdr*)[pbuf getL4];
            memswap(&udp->uh_sport, &udp->uh_dport, sizeof(uint16_t));
            // -----------------------------------------------------------------

            if (type == 28) {
                //NSLog(@"AAAA");
                // v6 to v4 fallback -> nxdomain?
                // generate name reply message -------------------------------------
                id name=[[NamePacket alloc] init];
                [name n_set_id:name_id];
                [name n_set_flags:QR|RA|RE_Error];
                [name n_create_rr_questionAAAA:fqdn];
                [name n_build_payload];
                [pbuf setL7:[name n_payload] :[name n_payload_size]];
                [name release];
                // -----------------------------------------------------------------
            }


            if (type == 1) {
                // make prison-nameDB ---------------------------------------------
                NSString* lip;
                lip = [mgmt getFQDN2LIP:fqdn];
                if (lip == nil) {
                    if ([mgmt isExistLocalDB:fqdn] == true) {
                        //NSLog(@"existLockDB\n");
                        [mgmt setFQDN:fqdn];
                        lip = [mgmt getFQDN2LIP:fqdn];
                    } else {
                        //NSLog(@"non existLockDB\n");
                    }
                }
                // -----------------------------------------------------------------


                // generate name reply message -------------------------------------
                if (lip != nil) {
                    // reply from localDB
                    id name = [[NamePacket alloc] init];
                    [name n_set_id:name_id];
                    [name n_set_flags:QR|AA|RA];
                    [name n_create_rr_questionA:fqdn];
                    [name n_create_rr_answer:lip];
                    [name n_build_payload];
                    [pbuf setL7:[name n_payload] :[name n_payload_size]];
                    [name release];
                }

                else {
                    // query to cage
                    if ([mgmt isCageJoin]) {
                        [mgmt enqueueRequestA:fqdn 
                                             :pbuf
                                             :socketFD
                                             :(sockaddr*)&sin_recv
                                             :name_id];
                    }
                    // enqueue return following
                    // true : to query cage...
                    // false: already request in request Queue.
                    if ([self isCancelled] == YES) {
                        [loop_pool drain];
                        break;
                    }
                    [loop_pool drain];
                    continue;
                    // replay NSDomain
                    //id name = [[NamePacket alloc] init];
                    //[name n_set_id:name_id];
                    //[name n_set_flags:QR|AA|RE_Error];
                    //[name n_create_rr_questionA:fqdn];
                    //[name n_build_payload];
                    //[pbuf setL7:[name n_payload] :[name n_payload_size]];
                    //[name release];
                }
                // -----------------------------------------------------------------
            }
        }
        
        size = sendto(socketFD, [pbuf bytes], [pbuf length], 0, (SA*)&sin_recv, len);

        if ([self isCancelled] == YES) {
            [loop_pool drain];
            break;
        }

        [loop_pool drain];
    }

    [pool drain];
    return;
}


- (void)me2lOperation
{
    id pool = [NSAutoreleasePool new];

    struct sockaddr_in sin_recv;
    socklen_t len;
    ssize_t  size;
    uint8_t  buf[SIZE_RECV_BUFFER];

    for (;;) {
        id loop_pool = [[NSAutoreleasePool alloc] init];
        len = (socklen_t)sizeof(sin_recv);
        memset(&buf, 0, sizeof(buf));

        size = recvfrom(socketFD, buf, sizeof(buf), 0, (SA*)&sin_recv, &len);
        if (size < 0) {
            perror("recvfrom");
            raise(SIGINT);
        }

        // set packetbuffer ----------------------------------------------------
        id pbuf = [[[PacketBuffer alloc] init] autorelease];
        [pbuf withBytes:buf :size];
        // ---------------------------------------------------------------------




        // fowarding swap processing -------------------------------------------
        struct ip* ip = (struct ip*)[pbuf getL3];
        NSData* data;
        data = [NSData dataWithBytes:&ip->ip_dst.s_addr length:sizeof(uint32_t)];

        NSString* lip;
        lip = [NSString stringWithUTF8String:ip_ntoa(@"IPv4", data)];
        //NSLog(@"lip%@", lip);
        if (lip == nil) { 
            [loop_pool drain];
            continue;
        }

        NSString* nsSrcIP;
        [niLock lock];
        nsSrcIP = [NSString stringWithString:[ni defaultIP4]];
        [niLock unlock];
        //NSLog(@"srcIP%@", nsSrcIP);
        if (nsSrcIP == nil) {
            [loop_pool drain];
            continue;
        }

        NSString* nsDstIP;
        nsDstIP = [mgmt getLIP2GIP:lip];
        //NSLog(@"dstIP%@", nsDstIP);
        if (nsDstIP == nil) {
            [loop_pool drain];
            continue;
        }

        ip->ip_src.s_addr=inet_addr([nsSrcIP UTF8String]);
        ip->ip_dst.s_addr=inet_addr([nsDstIP UTF8String]);

        uint8_t protocol;
        uint8_t tcpflags;
        uint16_t srcPort;
        uint16_t dstPort;
        uint16_t fromPort;
        if ([[pbuf getL4Proto] isEqualToString:@"TCP"]) {
            struct tcphdr* tcp = (struct tcphdr*)[pbuf getL4];
            dstPort = ntohs(tcp->th_dport);
            srcPort = ntohs(tcp->th_sport);
            protocol = IPPROTO_TCP;
            tcp->th_dport = htons((uint16_t)[[mgmt getLIP2PORT:lip] intValue]);
            fromPort = ntohs(tcp->th_dport);
            tcpflags = tcp->th_flags;
        }
        if ([[pbuf getL4Proto] isEqualToString:@"UDP"]) {
            struct udphdr* udp = (struct udphdr*)[pbuf getL4];
            dstPort = ntohs(udp->uh_dport);
            srcPort = ntohs(udp->uh_sport);
            protocol = IPPROTO_UDP;
            udp->uh_dport = htons((uint16_t)[[mgmt getLIP2PORT:lip] intValue]);
            fromPort = ntohs(udp->uh_dport);
        }
        [pbuf sync];
        // ---------------------------------------------------------------------


        // get FID -------------------------------------------------------------
        uint16_t num;
        num = [mgmt getLIP2FID:lip];
        NSNumber* fid;
        if (num == 0) {
            fid = [mgmt adjustFID2LIP:lip];
            num = [fid unsignedShortValue];
        } else {
            fid = [NSNumber numberWithUnsignedShort:num];
        }

        // ---------------------------------------------------------------------


        // set PairPortNumber --------------------------------------------------
        //NSLog(@"Protocol:%d, srcPort:%d, dstPort:%d", protocol, srcPort, dstPort);
        int ret_PortPair;
        ret_PortPair = [mgmt setPortPairWithProtocol:protocol
                                             SrcPort:srcPort
                                             DstPort:dstPort
                                                 FID:fid];
        if (ret_PortPair) {
            //NSLog(@"true");
        } else {
            //NSLog(@"false");
        }
        // ---------------------------------------------------------------------

        // session control of me2l ---------------------------------------------
        // tcp
        if (protocol == IPPROTO_TCP) {
            if (tcpflags & TH_FIN) {
                uint8_t PPFlags;
                PPFlags = [mgmt getPPFlagsWithProtocol:protocol
                                               SrcPort:srcPort];
                if (PPFlags == PPFLAG_ERROR) {
                    //NSLog(@"false:getPPFlags");
                }
                [mgmt setPPFlagsWithProtocol:protocol
                                     SrcPort:srcPort
                                     PPFlags:(PPFlags|PPFLAG_ME2L_FIN)];
                //NSLog(@"me2l FIN");
            }
            else if (tcpflags & TH_ACK) {
                uint8_t PPFlags;
                PPFlags = [mgmt getPPFlagsWithProtocol:protocol
                                               SrcPort:srcPort];
                if (PPFlags == PPFLAG_ERROR) {
                    //NSLog(@"false:getPPFlags");
                }

                uint8_t condFlag;
                condFlag = PPFLAG_ME2L_FIN | PPFLAG_EXT2ME_FIN;
                if ((PPFlags & condFlag) == condFlag) {
                    [mgmt delPortPairWithProtocol:protocol
                                          SrcPort:srcPort];
                    [mgmt delFID2PPWithProtocol:protocol
                                        SrcPort:srcPort
                                            FID:fid];
                    //NSLog(@"me2l last ack");
                }
            }
            else {}
        }
        // udp
        if (protocol == IPPROTO_UDP) {
        }
        // ---------------------------------------------------------------------


        // make new fw rule-----------------------------------------------------
        //NSLog(@"check : %d", __LINE__);
        if ([fw hasNum:num] == false) {
            //NSLog(@"check : %d", __LINE__);
            struct _fw_param fw_param;
            memset(&fw_param, 0, sizeof(fw_param));
            NSString* gip;
            gip = [mgmt getLIP2GIP:lip];
            //NSLog(@"check : %d", __LINE__);
            memcpy(fw_param.srcIP, [gip UTF8String], [gip length]);
            memcpy(fw_param.dstIP, "0.0.0.0", sizeof("0.0.0.0"));
            fw_param.srcPrefix = 32;
            fw_param.dstPrefix = 0;
            fw_param.srcPort = fromPort;
            fw_param.dstPort = 0;
            fw_param.protocol = protocol;
            fw_param.divertPort = FW_EXT2ME_DIVERT;
            fw_param.flags = (IP_FW_F_DME | IP_FW_F_DIVERT | IP_FW_F_IN | IP_FW_F_OUT);
            fw_param.addNumber = num;
            //NSLog(@"check : %d, num:%d", __LINE__, num);
            [obs setFwParam:&fw_param];
            [[NSNotificationCenter defaultCenter]
                postNotificationName:@"notify://obs.FWHooker"
                object:nil
            ];

        } else {
            // update idle time ------------------------------------------------
            //NSLog(@"check : %d", __LINE__);
            [mgmt updateIdleTime:fid];
            [mgmt updatePPIdleWithProtocol:protocol SrcPort:srcPort];
            // -----------------------------------------------------------------
        }
        // ---------------------------------------------------------------------

        //NSLog(@"check : %d", __LINE__);
        size = sendto(socketFD, [pbuf bytes], [pbuf length], 0, (SA*)&sin_recv, len);

        if ([self isCancelled] == YES) {
            [loop_pool drain];
            break;
        }

        [loop_pool drain];
    }

    [pool drain];
    return;
}

- (void)ext2meOperation
{
    id pool = [NSAutoreleasePool new];
    struct sockaddr_in sin_recv;
    socklen_t len;
    ssize_t  size;
    uint8_t  buf[SIZE_RECV_BUFFER];

    for (;;) {

        id loop_pool = [[NSAutoreleasePool alloc] init];
        len = (socklen_t)sizeof(sin_recv);
        memset(&buf, 0, sizeof(buf));

        size = recvfrom(socketFD, buf, sizeof(buf), 0, (SA*)&sin_recv, &len);
        [extLock lock];
        if (size < 0) {
            perror("recvfrom");
            raise(SIGINT);
        }


        // set packetbuffer ----------------------------------------------------
        id pbuf = [[[PacketBuffer alloc] init] autorelease];
        [pbuf withBytes:buf :size];
        // ---------------------------------------------------------------------

        //NSLog(@"check : %d", __LINE__);

        // filling transport layer information ---------------------------------
        uint8_t protocol = 0;
        uint8_t tcpflags = 0;
        uint16_t dstPort = 0;
        uint16_t srcPort = 0;

        if ([[pbuf getL4Proto] isEqualToString:@"TCP"]) {
            struct tcphdr* tcp = (struct tcphdr*)[pbuf getL4];
            srcPort = ntohs(tcp->th_dport);
            dstPort = htons([mgmt getPairPortWithProtocol:IPPROTO_TCP
                                                  SrcPort:srcPort]);
            //NSLog(@"%d, srcPort:%d", __LINE__, srcPort);
            //NSLog(@"%d, dstPort:%d", __LINE__, ntohs(dstPort));
            if (dstPort != 0) {
                tcp->th_sport = dstPort;
                protocol = IPPROTO_TCP;
                tcpflags = tcp->th_flags;
            }
            else {
            }
        }

        if ([[pbuf getL4Proto] isEqualToString:@"UDP"]) {
            struct udphdr* udp = (struct udphdr*)[pbuf getL4];
            srcPort = ntohs(udp->uh_dport);
            dstPort = htons([mgmt getPairPortWithProtocol:IPPROTO_UDP
                                                  SrcPort:srcPort]);
            if (dstPort != 0) {
                udp->uh_sport = dstPort;
                protocol = IPPROTO_UDP;
            }
            else {
            }
        }
        // ---------------------------------------------------------------------

        //NSLog(@"check : %d", __LINE__);

        // get translation information for ip layer ----------------------------
        uint16_t recvFilterNum;
        recvFilterNum = sin_recv.sin_port-FW_BIAS_RULE_NUMBER;
        NSNumber* fid;
        fid = [NSNumber numberWithUnsignedShort:recvFilterNum];
        [mgmt updateIdleTime:fid];
        [mgmt updatePPIdleWithProtocol:protocol SrcPort:srcPort];

        NSString* lip;
        lip = [mgmt getFID2LIP:recvFilterNum];
        //NSLog(@"%d,LIP     :%@", __LINE__, lip);
        //NSLog(@"%d,Protocol:%d", __LINE__, protocol);
        //NSLog(@"%d,FID     :%d", __LINE__, recvFilterNum);
        // ---------------------------------------------------------------------


        // filling ip layer information ----------------------------------------
        if ((protocol!=0) && (lip!=nil)) {
            struct ip* ip = (struct ip*)[pbuf getL3];
            ip->ip_src.s_addr=inet_addr([lip UTF8String]);
            ip->ip_dst.s_addr=inet_addr("127.0.0.1");
        }
        else {
            //NSLog(@"direct");
        }
        // ---------------------------------------------------------------------


        // session control of ext2me -------------------------------------------
        // tcp
        if (protocol == IPPROTO_TCP) {
            if (tcpflags & TH_FIN) {
                uint8_t PPFlags;
                PPFlags = [mgmt getPPFlagsWithProtocol:protocol
                                               SrcPort:srcPort];
                if (PPFlags == PPFLAG_ERROR) {
                    //NSLog(@"false:getPPFlags");
                }
                [mgmt setPPFlagsWithProtocol:protocol
                                     SrcPort:srcPort
                                     PPFlags:(PPFlags|PPFLAG_EXT2ME_FIN)];
                //NSLog(@"ext2me FIN");
            }
            else if (tcpflags & TH_ACK) {
                uint8_t PPFlags;
                PPFlags = [mgmt getPPFlagsWithProtocol:protocol
                                               SrcPort:srcPort];

                if (PPFlags == PPFLAG_ERROR) {
                    if (is_verbose) {
                        NSLog(@"%d: false:getPPFlags", __LINE__);
                    }
                }

                uint8_t condFlag;
                condFlag = PPFLAG_ME2L_FIN | PPFLAG_EXT2ME_FIN;
                if ((PPFlags & condFlag) == condFlag) {

                    [mgmt delPortPairWithProtocol:protocol
                                          SrcPort:srcPort];

                    [mgmt delFID2PPWithProtocol:protocol
                                        SrcPort:srcPort
                                            FID:fid];

                    //NSLog(@"ext2me last ack");
                }
            }
            else {}

        }
        // udp
        if (protocol == IPPROTO_UDP) {
        }
        // ---------------------------------------------------------------------


        if (protocol != 0) {
            // processing for rerouting ----------------------------------------
            struct ip* ip;
            ip = (struct ip*)[pbuf getL3];
            memset(&sin_recv, 0, sizeof(sin_recv));
            sin_recv.sin_family=AF_INET;
            sin_recv.sin_addr.s_addr=ip->ip_dst.s_addr;
            // -----------------------------------------------------------------
        }


        [pbuf sync];
        //memdump((void*)[pbuf bytes], [pbuf length]);

        /*
        {
            struct ip* ip;
            ip = (struct ip*)[pbuf getL3];
            NSData* data1;
            data1 = [NSData dataWithBytes:&ip->ip_src.s_addr length:sizeof(uint32_t)];
            printf("ip_src:%s\n", ip_ntoa(@"IPv4", data1));
            NSData* data2;
            data2 = [NSData dataWithBytes:&ip->ip_dst.s_addr length:sizeof(uint32_t)];
            printf("ip_dst:%s\n", ip_ntoa(@"IPv4", data2));
            //printf("ip_sum :0x%x\n", ntohs(ip->ip_sum));
            struct tcphdr* tcp;
            tcp = (struct tcphdr*)[pbuf getL4];
            printf("tcp_sport:%d\n", ntohs(tcp->th_sport));
            printf("tcp_dport:%d\n", ntohs(tcp->th_dport));
            printf("pbuf_size:%lu\n", [pbuf length]);
            //printf("tcp_sum:0x%x\n", ntohs(tcp->th_sum));
        }
        */

        size = sendto(socketFD, [pbuf bytes], [pbuf length], 0, (SA*)&sin_recv, len);

        [loop_pool drain];
        if ([self isCancelled] == YES) {
            [extLock unlock];
            break;
        }
        [extLock unlock];
    }

    [pool drain];
    return;
}


- (void)routeOperation
{
    id pool = [NSAutoreleasePool new];

    int kq;
    kq = kqueue();

    int fd;
    fd = socket(AF_SYSTEM, SOCK_RAW, SYSPROTO_EVENT);

    struct kev_request kev_req;
    memset(&kev_req, 0, sizeof(kev_req));

    struct kevent kev;
    memset(&kev, 0, sizeof(kev));

    kev_req.vendor_code = KEV_VENDOR_APPLE;
    kev_req.kev_class = KEV_NETWORK_CLASS;
    ioctl(fd, SIOCSKEVFILT, &kev_req);

    EV_SET(&kev, fd, EVFILT_READ, EV_ADD|EV_RECEIPT, 0, 0, NULL);

    int ret = 0;
    ret = kevent(kq, &kev, 1, NULL, 0, NULL);
    if (ret) {
        perror("kevent-registor");
        raise(SIGINT);
    }


   //[mgmt setDefaultRT:[ni defaultRoute4]];
   //[mgmt setDefaultIP:[ni defaultIP4]];

    
    bool is_first = true;
    for (;;) {

        id loop_pool = [NSAutoreleasePool new];

        NSString* ip;
        NSString* rt;

        [niLock lock];
        [ni refresh];
        rt = [ni defaultRoute4];
        ip = [ni defaultIP4];
        [niLock unlock];

        if (ip == nil && rt == nil) {
            is_linking = false;
        } else {
            is_linking = true;
        }

        int flag;
        flag = false;

        if (!ip4comp(rt, [mgmt getDefaultRT])) {
            //NSLog(@"%d", __LINE__);
            [mgmt setDefaultRT:rt];
            flag = true;
        }

        if (!ip4comp(ip, [mgmt getDefaultIP])) {
            //NSLog(@"%d", __LINE__);
            [mgmt setDefaultIP:ip];
            flag = true;
        }

        if (flag) {
            if (is_verbose) {
                NSLog(@"change-log");
                NSLog(@"default-Router:%@\n", rt);
                NSLog(@"default-IPaddr:%@\n", ip);
            }

            if (!is_first) {
                [fw delExtraRule];
                [mgmt recage];
            }

        }

        is_first = false;

#define KEV_BUF_SIZE 128
        char buf[KEV_BUF_SIZE];
        memset(buf, 0, sizeof(buf));

        struct timespec wait;
        wait.tv_sec = 2;
        wait.tv_nsec = 0;

        size_t read_size;
        read_size = 0;

        // block wait
        ret = kevent(kq, NULL, 0, &kev, 1, NULL);
        read_size = read((int)kev.ident, buf, sizeof(buf));
        // throw away noise message without catching
        for (;;) {
            ret = kevent(kq, NULL, 0, &kev, 1, &wait);
            if (kev.flags & EV_ERROR) {
                perror("kevent-block");
                raise(SIGINT);
            }
            if (ret == 0) {
                // TimeOver
                break;
            }
            read_size  = read((int)kev.ident, buf, sizeof(buf));
            //printf("event -> ret:%d -> buf:%s\n", ret, buf);
        }

        //memdump(buf, KEV_BUF_SIZE);
#undef KEV_BUF_SIZE

        if ([self isCancelled] == YES) {
            break;
        }

        [loop_pool drain];
    }

    [pool drain];
    return;
}

@end

/*
int main(int argc, char** argv) {
    id pool = [[NSAutoreleasePool alloc] init];
    NSOperationQueue* queue;
    queue = [[NSOperationQueue alloc] init];
    [queue setMaxConcurrentOperationCount:10];
    [queue setName:@"testQueue"];

    Operation* op = [[[Operation alloc] init] autorelease];
    op.socketFD=1;
    op.selector=OP_NAME;
    [queue addOperation:op];
    sleep(10);
    [pool drain];
    return 0;
}
*/
#endif //__PRISON_OP_H_
