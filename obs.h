#ifndef __PRISON_OBS_H_
#define __PRISON_OBS_H_

#import <Cocoa/Cocoa.h>
#import <servers/bootstrap.h>

#include "op.h"
#include "ds.h"
#include "fw.h"
#include "mgmt.h"
#include "name.h"
#include "pbuf.h"


#include "utils.h"
#include "common.h"
#include "category.h"


extern FWHooker*    fw;
extern Manager*   mgmt;
extern NSLock* extLock;


@interface Observer : NSObject
{
    NSLock* fwLock;
    struct _fw_param fw_param;
}

- (id)init;
- (void)dealloc;
- (void)test:(NSNotification*)notify;

// make ipfw divert filter
- (bool)obsFW:(NSNotification*)notify;
- (bool)setFwParam:(struct _fw_param*)param;

// make operation thread - not implementation
- (void)obsOP:(NSNotification*)notify;
- (void)setOpParam:(struct _op_param*)param;

// call flushcachce...
- (void)obsDS:(NSNotification*)notify;

// timer event
- (void)obsFWT:(NSNotification*)notify;
- (void)obsPPT:(NSNotification*)notify;

// name event
- (void)obsNameReply:(NSNotification*)notify;

@end

@implementation Observer

- (id)init {
    self = [super init];
    if(self != nil) {
        // --------------
        // initial coding
        // --------------
        fwLock = [NSLock new];
    }
    return self;
}

- (void)dealloc {
    // --------------
    // release coding
    // --------------
    [fwLock release];
    [super dealloc];
    return;
}


- (void)test:(NSNotification*)notify
{
    printf("sel1 was called by main loop\n");
    return;
}

- (bool)setFwParam:(struct _fw_param*)param {
    [fwLock lock];
    if(param == NULL)  {
        [fwLock unlock];
        return false;
    }
    memset(&fw_param, 0, sizeof(struct _fw_param));
    memcpy(&fw_param, param, sizeof(struct _fw_param));
    [fwLock unlock];
    return true;
}

- (bool)obsFW:(NSNotification*)notify
{
    [fwLock lock];
    @try {
        [fw setSrcRule:fw_param.srcIP :fw_param.srcPort];
        [fw setSrcMaskPrefix:fw_param.srcPrefix];
        [fw setDstRule:fw_param.dstIP :fw_param.dstPort];
        [fw setDstMaskPrefix:fw_param.dstPrefix];
        [fw setProtocol:fw_param.protocol];
        [fw setDivertPort:fw_param.divertPort];
        [fw setFlag:fw_param.flags];
        [fw addRule: fw_param.addNumber];
    }
    @catch(id ex) {
        NSLog(@"%@", ex);
        raise(SIGINT);
    }
    @finally { }
    [fwLock unlock];
    return true;
}

- (void)setOpParam:(struct _op_param*)param {
    return;
}

- (void)obsOP:(NSNotification*)notify
{
    return;
}

- (void)obsDS:(NSNotification*)notify
{
    flushcache();
    return;
}

// FWT : fire wall table
- (void)obsFWT:(NSNotification*)notify
{
    [extLock lock];
    uint16_t port;
    uint16_t protocol;
    NSArray* array_i;
    // notify object is timeout filter number array list
    array_i = [notify object];
    NSEnumerator* enumerate_i;
    enumerate_i = [array_i objectEnumerator];
    for (id obj in enumerate_i) {
        NSArray* array_j;
        array_j = [mgmt getFID2PP:obj];
        NSEnumerator* enumerate_j;
        enumerate_j = [array_j objectEnumerator];
        for (id obj2 in enumerate_j) {
            uint32_t ProtoPort;
            ProtoPort = [obj2 unsignedIntValue];
            protocol = ((ProtoPort>>16)&0x0000FFFF);
            port = (ProtoPort&0x0000FFFF);
            [mgmt delPortPairWithProtocol:protocol SrcPort:port];
        }
        [mgmt delFIDINFO:obj];
        uint16_t rule_num;
        rule_num = [obj unsignedShortValue];
        if (rule_num == 0) continue;
        [fwLock lock];
        [fw delRule:rule_num];
        [fwLock unlock];
    }
    [extLock unlock];
    return;
}

- (void)obsPPT:(NSNotification*)notify
{
    [extLock lock];
    uint16_t port;
    uint16_t protocol;
    NSArray* array;
    // notif object is timeout filter number array list
    array = [notify object];
    NSEnumerator* enumerate;
    enumerate = [array objectEnumerator];
    for (id obj in enumerate) {

        uint32_t ProtoPort;
        ProtoPort = [obj unsignedIntValue];
        //NSLog(@"ProtoPort:%u\n", ProtoPort);

        protocol = ((ProtoPort>>16)&0x0000FFFF);
        port = (ProtoPort&0x0000FFFF);

        //NSLog(@"protocol:%u\n", protocol);
        //NSLog(@"port:%u\n", port);
        //NSLog(@"and:%u\n", (protocol<<16)|port);

        [mgmt delPortPairWithProtocol:protocol SrcPort:port];
        [mgmt delFID2PPWithProtocol:protocol SrcPort:port FID:nil];

    }
    [extLock unlock];
    return;
}

- (void)obsNameReply:(NSNotification*)notify
{
    //id name = [notify name];
    //NSLog(@"objectNotifyName:%@\n", name);

    NSString* message = [notify object];
    //NSLog(@"object:%@\n", object);

    //id user = [notify userInfo];
    //NSLog(@"userInfo:%@\n", user);

    //204,get,NodeName,Key,Value
    NSArray* message_array = [message componentsSeparatedByString:@","];
    NSString* code = [message_array objectAtIndex:0];
    //NSString* command = [message_array objectAtIndex:1];
    //NSString* node_name = [message_array objectAtIndex:2];
    NSString* key = [message_array objectAtIndex:3];
    //NSString* value = [message_array objectAtIndex:4];

    NSArray* request_array = [mgmt dequeueRequestA:key];
    //0: fqdn
    //1: socket descriptor
    //2: sa_data - sockaddr_in
    //3: request divert socket payload
    //4: name_id

    NSString* fqdn = [request_array objectAtIndex:0];
    int sock = [[request_array objectAtIndex:1] intValue];
    SAIN* sin = (SAIN*)[[request_array objectAtIndex:2] bytes];
    PacketBuffer* pbuf = [[[PacketBuffer alloc] init] autorelease];
    [pbuf withData:[request_array objectAtIndex:3]];
    int name_id = [[request_array objectAtIndex:4] intValue];

    NSString* lip = nil;
    if ([code isEqualToString:@"204"] == YES) {
        [mgmt setFQDN:fqdn];
        lip = [mgmt getFQDN2LIP:fqdn];
    }
    else if ([code isEqualToString:@"409"] == YES) {
        if (is_verbose) NSLog(@"%@\n", code);
    }

    int auth = [mgmt getFQDN2AUTH:fqdn];
    if (lip != nil && auth != 0) {
        // reply from localDB
        id name = [[NamePacket alloc] init];
        [name n_set_id:name_id];
        if (auth >= 1 || auth == 1) {
            [name n_set_flags:QR|AA|RA];
        } else if (auth == 1) {
            [name n_set_flags:QR|RA];
        }
        [name n_create_rr_questionA:fqdn];
        [name n_create_rr_answer:lip];
        [name n_build_payload];
        [pbuf setL7:[name n_payload] :[name n_payload_size]];
        [name release];
    }

    else {
        // replay NXDomain
        if (is_verbose) {
            NSLog(@"NXDOMAIN\n", code);
            NSLog(@"name_id:%d\n", name_id);
            NSLog(@"fqdn:%@\n", fqdn);
        }
        id name = [[NamePacket alloc] init];
        [name n_set_id:name_id];
        [name n_set_flags:QR|AA|RA|RE_Error];
        [name n_create_rr_questionA:fqdn];
        [name n_build_payload];
        [pbuf setL7:[name n_payload] :[name n_payload_size]];
        [name release];
    }

    socklen_t len;
    len = (socklen_t)sizeof(SAST);
    size_t size;
    size = sendto(sock, [pbuf bytes], [pbuf length], 0, (SA*)sin, len);
    if ((int)size == -1) {
        perror("sendto");
    }

    return;
}

@end

#endif



