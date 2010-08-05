#ifndef __RAPRINS_OBS_H_
#define __RAPRINS_OBS_H_

#import <Cocoa/Cocoa.h>
#import <servers/bootstrap.h>

#include "op.h"
#include "ds.h"
#include "fw.h"
#include "mgmt.h"


#include "utils.h"
#include "common.h"
#include "category.h"


extern FWHooker   * fw;
extern Manager*   mgmt;
//extern NSLock*  meLock;
extern NSLock* extLock;


@interface Observer : NSObject
{
    NSLock* fwLock;
    struct _fw_param fw_param;
}

- (id)init;
- (void)dealloc;
- (void)test:(NSNotification*)notif;

// make ipfw divert filter
- (bool)obsFW:(NSNotification*)notif;
- (bool)setFwParam:(struct _fw_param*)param;

// make operation thread
- (void)obsOP:(NSNotification*)notif;
- (void)setOpParam:(struct _op_param*)param;

- (void)obsDS:(NSNotification*)notif;

// close session
- (void)obsFWT:(NSNotification*)notif;
- (void)obsPPT:(NSNotification*)notif;

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



- (void)test:(NSNotification*)notif
{
    //NSLog(@"%d\n", __LINE__); 
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

- (bool)obsFW:(NSNotification*)notif
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

- (void)obsOP:(NSNotification*)notif
{
    return;
}

- (void)obsDS:(NSNotification*)notif
{
    flushcache();
    return;
}


- (void)obsFWT:(NSNotification*)notif
{
    [extLock lock];
    //NSLog(@"chekc : %d", __LINE__);
    uint16_t port;
    uint16_t protocol;
    NSArray* array_i;
    // notif object is timeout filter number array list
    array_i = [notif object];
    NSEnumerator *enumerate_i;
    enumerate_i = [array_i objectEnumerator];
    for (id obj in enumerate_i) {
        NSArray* array_j;
        array_j = [mgmt getFID2PP:obj];
        NSEnumerator *enumerate_j;
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


- (void)obsPPT:(NSNotification*)notif
{
    [extLock lock];
    uint16_t port;
    uint16_t protocol;
    NSArray* array;
    // notif object is timeout filter number array list
    array = [notif object];
    NSEnumerator *enumerate;
    enumerate = [array objectEnumerator];
    for (id obj in enumerate) {
        uint32_t ProtoPort;
        ProtoPort = [obj unsignedIntValue];
        protocol = ((ProtoPort>>16)&0x0000FFFF);
        port = (ProtoPort&0x0000FFFF);
        [mgmt delPortPairWithProtocol:protocol SrcPort:port];
        [mgmt delFID2PPWithProtocol:protocol SrcPort:port FID:nil];
    }
    [extLock unlock];
    return;
}

@end

#endif



