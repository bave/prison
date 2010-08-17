#ifndef __RAPRINS_MGMT_H_
#define __RAPRINS_MGMT_H_

#import <Cocoa/Cocoa.h>

#include <arpa/inet.h>

#include "utils.h"
#include "common.h"
#include "category.h"

#include "kvt.h"

#include "fw.h"
extern FWHooker* fw;
extern RaprinsConfig* rc;


@interface Manager : NSObject
{ 
    uint16_t filter_id;
    uint16_t local_id;
    NSLock* mgmtLock;
    keyValueTable* kvt;
    //NSMutableDictionary* mgmtDictFIDCounter;
    NSMutableDictionary* mgmtDictFIDDate;
    NSMutableDictionary* mgmtDictFIDIdle;
    NSMutableDictionary* mgmtDictPPIdle;
    NSMutableDictionary* mgmtDictFQDN2LIP;
    NSMutableDictionary* mgmtDictLIP2FQDN;
    NSMutableDictionary* mgmtDictLIP2FID;
    NSMutableDictionary* mgmtDictFID2LIP;
    NSMutableDictionary* mgmtDictFID2PP;
    NSMutableDictionary* mgmtDictPortPair;
    NSMutableDictionary* mgmtDictPPFlags;
    NSString* defaultIP;
    NSString* defaultRT;
}       

// public function
- (id)init; 
- (void)dealloc;

//- (bool)setPath:(NSString*)path;
- (bool)delFIDINFO:(NSNumber*)fid;

- (bool)setFQDN:(NSString*)fqdn;
- (bool)delFQDN:(NSString*)fqdn;

- (NSString*)getFQDN2LIP:(NSString*)fqdn;
- (NSString*)getFQDN2Value:(NSString*)fqdn;
- (NSString*)getFQDN2GIP:(NSString*)fqdn;
- (NSString*)getFQDN2PORT:(NSString*)fqdn;

- (NSString*)getLIP2FQDN:(NSString*)lip;
- (NSString*)getLIP2Value:(NSString*)lip;
- (NSString*)getLIP2GIP:(NSString*)lip;
- (NSString*)getLIP2PORT:(NSString*)lip;

- (NSString*)getDefaultIP;
- (bool)setDefaultIP:(NSString*)ip;

- (NSString*)getDefaultRT;
- (bool)setDefaultRT:(NSString*)rt;

- (uint16_t)getLIP2FID:(NSString*)lip;
- (NSString*)getFID2LIP:(uint16_t)fid;
- (NSNumber*)adjustFID2LIP:(NSString*)lip;


- (bool)setPortPairWithProtocol:(uint16_t)protocol
                        SrcPort:(uint16_t)srcPort
                        DstPort:(uint16_t)dstPort
                            FID:(NSNumber*)fid;
- (bool)delPortPairWithProtocol:(uint16_t)protocol
                        SrcPort:(uint16_t)srcPort;
- (NSArray*)getFID2PP:(NSNumber*)fid;
- (bool)delFID2PPWithProtocol:(uint16_t)protocol
                      SrcPort:(uint16_t)srcPort
                          FID:(NSNumber*)fid;
//- (uint16_t)getFID2PPofProtocol:(NSNumber*)fid;
//- (uint16_t)getFID2PPofSrcPort:(NSNumber*)fid;
- (uint16_t)getPairPortWithProtocol:(uint16_t)protocol
                            SrcPort:(uint16_t)srcPort;
- (bool)setPPFlagsWithProtocol:(uint16_t)protocol
                       SrcPort:(uint16_t)srcPort
                       PPFlags:(uint8_t)ppFlags;
// unused message event
- (bool)resetPPFlagsWithProtocol:(uint16_t)protocol
                         SrcPort:(uint16_t)srcPort;
- (uint8_t)getPPFlagsWithProtocol:(uint16_t)protocol
                          SrcPort:(uint16_t)srcPort;

- (bool)updateIdleTime:(NSNumber*)fid;
- (bool)updatePPIdleWithProtocol:(uint16_t)protocol
                         SrcPort:(uint16_t)srcPort;
// for main loop -> 後でOPにまわす
- (void)test;
- (NSArray*)idle_timeout;
- (NSArray*)per_timeout;
            
// private function
- (NSString*)_generate_fqdn2ip;
- (uint16_t)_filter_id_resolv;
- (uint16_t)_local_id_resolv;
- (bool)_delPPFlagsWithProtocol:(uint16_t)protocol
                       SrcPort:(uint16_t)srcPort;

@end    
    
@implementation Manager

- (void)test
{
    printf("\n");
    printf("-----------------------------------------\n");
    //NSLog(@"FIDCounter%@", mgmtDictFIDCounter);
    NSLog(@"FIDDate%@", mgmtDictFIDDate);
    NSLog(@"FIDIdle%@", mgmtDictFIDIdle);
    NSLog(@"PPIdle%@", mgmtDictPPIdle);
    NSLog(@"FQDN2LIP%@", mgmtDictFQDN2LIP);
    NSLog(@"LIP2FQDN%@", mgmtDictLIP2FQDN);
    NSLog(@"LIP2FID%@", mgmtDictLIP2FID);
    NSLog(@"FID2LIP%@", mgmtDictFID2LIP);
    NSLog(@"FID2PP%@", mgmtDictFID2PP);
    NSLog(@"PortPair%@", mgmtDictPortPair);
    NSLog(@"PPFlags %@", mgmtDictPPFlags);
    printf("-----------------------------------------\n");
    printf("\n");
    return;
}


- (bool)updateIdleTime:(NSNumber*)fid
{
    [mgmtLock lock];
    NSDate* date;
    date = [NSDate date];
    [mgmtDictFIDIdle setObject:date forKey:fid];
    [mgmtLock unlock];
    return true;
}


- (bool)updatePPIdleWithProtocol:(uint16_t)protocol
                         SrcPort:(uint16_t)srcPort
{
    [mgmtLock lock];
    NSDate* date;
    date = [NSDate date];

    if (protocol == 0) {
        [mgmtLock unlock];
        return false;
    }
    if (srcPort == 0) {
        [mgmtLock unlock];
        return false;
    }

    uint32_t ProtoPort;
    ProtoPort = ((uint32_t)protocol<<16) | (uint32_t)(srcPort);

    NSNumber* nsProtoPort;
    nsProtoPort = [NSNumber numberWithUnsignedLong:ProtoPort];

    
    [mgmtDictPPIdle setObject:date forKey:nsProtoPort];
    [mgmtLock unlock];
    return true;
}


- (NSArray*)idle_timeout
{
    NSMutableSet* set;
    set = [NSMutableSet new];
    [set autorelease];

    /*
    // filter がつくられてからのタイムアウト
    // そのうち使うかも
    NSEnumerator* enumerate;
    enumerate = [mgmtDictFIDDate keyEnumerator];
    for (id key in enumerate) {
        if ([[mgmtDictFIDDate objectForKey:key] timeIntervalSinceNow] < INITIALLY_TIMEOUT) {
            [set addObject:key];
        }
    }
    */

    NSEnumerator* enumerate;
    enumerate = [mgmtDictFIDIdle keyEnumerator];
    for (id key in enumerate) {
        if ([[mgmtDictFIDIdle objectForKey:key]
                    timeIntervalSinceNow] < FW_IDLE_TIMEOUT) {
            [set addObject:key];
        }
    }
    return [set allObjects];
}


- (NSArray*)per_timeout {
    NSMutableSet* set;
    set = [NSMutableSet new];
    [set autorelease];

    NSEnumerator* enumerate;
    enumerate = [mgmtDictPPIdle keyEnumerator];
    for (id key in enumerate) {
        if ([[mgmtDictPPIdle objectForKey:key]
                        timeIntervalSinceNow] < PP_IDLE_TIMEOUT) {
            [set addObject:key];
        }
    }

    return [set allObjects];
}


- (bool)delFIDINFO:(NSNumber*)fid
{
    [mgmtLock lock];
    NSString* lip;
    lip = [mgmtDictFID2LIP objectForKey:fid];
    if (lip != nil) {
        [mgmtDictLIP2FID removeObjectForKey:lip];
    }
    [mgmtDictFIDDate removeObjectForKey:fid];
    [mgmtDictFIDIdle removeObjectForKey:fid];
    [mgmtDictFID2PP  removeObjectForKey:fid];
    [mgmtDictFID2LIP removeObjectForKey:fid];
    [mgmtLock unlock];
    return true;
}
    

- (id)init
{
    self = [super init];
    if (self != nil) {
        // --------------
        // initial coding
        // --------------
        filter_id = 0;
        local_id  = 1;
        kvt = [keyValueTable new];
        //[kvt setPath:@"./data.conf"];
        [kvt setLocalDB:[rc getLocalDB]];
        mgmtLock = [NSLock new];
        mgmtDictFIDDate    = [NSMutableDictionary new];
        mgmtDictFIDIdle    = [NSMutableDictionary new];
        mgmtDictPPIdle     = [NSMutableDictionary new];
        //mgmtDictFIDCounter = [NSMutableDictionary new];
        mgmtDictLIP2FID    = [NSMutableDictionary new];
        mgmtDictFID2LIP    = [NSMutableDictionary new];
        mgmtDictFQDN2LIP   = [NSMutableDictionary new];
        mgmtDictLIP2FQDN   = [NSMutableDictionary new];
        mgmtDictFID2PP     = [NSMutableDictionary new];
        mgmtDictPortPair   = [NSMutableDictionary new];
        mgmtDictPPFlags    = [NSMutableDictionary new];
        defaultIP = nil;
        defaultRT = nil;
    }   
    return self;
}       


/*
- (bool)setPath:(NSString*)path
{
    return [kvt setPath:path];
}
*/


- (NSString*)_generate_fqdn2ip
{
    uint16_t tmp_id;
    local_id  = [self _local_id_resolv];
    tmp_id = htons(local_id);
    uint8_t low_segment[2];
    memcpy(low_segment , &tmp_id, sizeof(uint16_t));

    NSString* seg3;
    NSString* seg4;
    seg3 = [NSString stringWithFormat:@"%d.", low_segment[0]];
    seg4 = [NSString stringWithFormat:@"%d" , low_segment[1]];

    NSMutableString* ip;
    ip = [[NSMutableString new] autorelease];
    [ip appendString:@"127.0."];
    [ip appendString:seg3];
    [ip appendString:seg4];

    NSString* lip;
    lip = [NSString stringWithString:ip];
    //NSLog(@"lip:%@", lip);

    NSNumber* fid;
    filter_id = [self _filter_id_resolv];
    fid = [NSNumber numberWithUnsignedShort:filter_id];
    [mgmtDictFID2LIP setObject:lip forKey:fid];
    [mgmtDictLIP2FID setObject:fid forKey:lip];

    /*
    NSNumber* counter;
    counter = [NSNumber numberWithUnsignedShort:0];
    [mgmtDictFIDCounter setObject:counter forKey:fid];
    */

    NSDate* date;
    date = [NSDate date];
    [mgmtDictFIDDate setObject:date forKey:fid];

    return lip;
}


- (NSNumber*)adjustFID2LIP:(NSString*)lip
{
    NSNumber* fid;
    filter_id = [self _filter_id_resolv];
    fid = [NSNumber numberWithUnsignedShort:filter_id];
    [mgmtDictFID2LIP setObject:lip forKey:fid];
    [mgmtDictLIP2FID setObject:fid forKey:lip];

    /*
    NSNumber* counter;
    counter = [mgmtDictFIDCounter objectForKey:fid];
    if (counter == nil) {
        [mgmtDictFIDCounter
                setObject:[NSNumber numberWithUnsignedShort:0] forKey:fid];
    } else {
        // XXX この条件ブロックいらないかもしれない．．
        // XXX adjusting するときは初めて使われるため．．．
        uint16_t tcounter;
        tcounter = [counter unsignedShortValue];
        tcounter++;
        [mgmtDictFIDCounter
                setObject:[NSNumber numberWithUnsignedShort:tcounter] forKey:fid];
    }
    */

    NSDate* date;
    date = [mgmtDictFIDDate objectForKey:fid];
    if (date == nil) {
        [mgmtDictFIDDate setObject:[NSDate date] forKey:fid];
    }

    //NSLog(@"adjust:%@", fid);
    return fid;
}

- (uint16_t)_local_id_resolv
{
    int tmp_id;
    tmp_id = local_id;
    tmp_id++;
    return tmp_id;
}

- (uint16_t)_filter_id_resolv
{
    /*
    int tmp_id;
    tmp_id = filter_id;
    tmp_id++;
    return tmp_id;
    */
    return [fw getEmptyNum:true];
}

- (bool)setFQDN:(NSString*)fqdn 
{
    [mgmtLock lock];

    NSString* lip;
    NSString* kvtValue;

    if (fqdn == nil) {
        [mgmtLock unlock];
        return false;
    }

    // query for DHT --------------------------------
    // 未実装 non implementation
    // もしみつからなかったらNXDomain をかえす
    // ように実装を変える
    // ----------------------------------------------

    kvtValue = [kvt value4key:fqdn];
    if (kvtValue==nil) {
        [mgmtLock unlock];
        return false;
    }

    lip = [mgmtDictFQDN2LIP valueForKey:fqdn];
    if (lip != nil) {
        [mgmtLock unlock];
        return true;
    }

    if ([kvt port4key:fqdn] == nil) {
        lip = [kvt ip4key:fqdn];
        //NSLog(@"%d:%@", __LINE__, lip);
    } else {
        lip = [self _generate_fqdn2ip];
        if (lip == nil) {
            [mgmtLock unlock];
            return false;
        }
        //NSLog(@"%d:%@", __LINE__, lip);
    }
       
    //NSLog(@"%d:%@", __LINE__, fqdn);
    //NSLog(@"%d:%@", __LINE__, lip);
    [mgmtDictFQDN2LIP setObject:lip forKey:fqdn];
    [mgmtDictLIP2FQDN setObject:fqdn forKey:lip];
    [mgmtLock unlock];

    return true;
}

- (bool)delFQDN:(NSString*)fqdn 
{
    [mgmtLock lock];


    if (fqdn == nil) {
        [mgmtLock unlock];
        return false;
    }
        
    NSString* lip;
    lip = [mgmtDictFQDN2LIP valueForKey:fqdn];
    if (lip == nil) {
        [mgmtLock unlock];
        return false;
    }
    NSNumber* fid;
    fid = [mgmtDictLIP2FID valueForKey:lip];
    if (fid == nil) {
        [mgmtLock unlock];
        return false;
    }

    [mgmtDictFQDN2LIP removeObjectForKey:fqdn];
    [mgmtDictLIP2FQDN removeObjectForKey:lip];
    [mgmtDictFID2LIP removeObjectForKey:fid];
    [mgmtDictLIP2FID removeObjectForKey:lip];

    [mgmtLock unlock];
    return true;
}

- (NSString*)getFQDN2LIP:(NSString*)fqdn 
{
    return [mgmtDictFQDN2LIP valueForKey:fqdn];
}


- (NSString*)getFQDN2Value:(NSString*)fqdn
{
    NSString* lip;

    lip = [mgmtDictFQDN2LIP valueForKey:fqdn];
    if(lip == nil) {
        return nil;
    }

    return [kvt value4key:fqdn];
}

- (NSString*)getFQDN2GIP:(NSString*)fqdn
{
    NSString* lip;

    lip = [mgmtDictFQDN2LIP valueForKey:fqdn];
    if(lip == nil) {
        return nil;
    }

    return [kvt ip4key:fqdn];
}

- (NSString*)getFQDN2PORT:(NSString*)fqdn
{
    NSString* lip;

    lip = [mgmtDictFQDN2LIP valueForKey:fqdn];
    if(lip == nil) {
        return nil;
    }

    return [kvt port4key:fqdn];
}

- (NSString*)getLIP2FQDN:(NSString*)lip
{
    return [mgmtDictLIP2FQDN valueForKey:lip];
}

- (NSString*)getLIP2Value:(NSString*)lip
{
    NSString* fqdn;

    fqdn =  [mgmtDictLIP2FQDN valueForKey:lip];
    if (fqdn == nil) {
        return nil;
    }

    return [kvt value4key:fqdn];
}

- (NSString*)getLIP2GIP:(NSString*)lip
{
    NSString* fqdn;
    //NSLog(@"%d:%@", __LINE__, lip);
    fqdn =  [mgmtDictLIP2FQDN valueForKey:lip];
    //NSLog(@"%d:%@", __LINE__, fqdn);
    if (fqdn == nil) {
        return nil;
    }

    return [kvt ip4key:fqdn];
}

- (NSString*)getLIP2PORT:(NSString*)lip
{
    NSString* fqdn;

    fqdn =  [mgmtDictLIP2FQDN valueForKey:lip];
    if (fqdn == nil) {
        return nil;
    }

    return [kvt port4key:fqdn];
}

- (NSString*)getDefaultIP;
{
    //NSLog(@"%@", defaultIP);
    return defaultIP;
}
- (bool)setDefaultIP:(NSString*)ip;
{
    //if (ip == nil) return false;
    [mgmtLock lock];
    [defaultIP release];
    defaultIP = ip;
    [defaultIP retain];
    [mgmtLock unlock];
    return true;
}

- (NSString*)getDefaultRT;
{
    return defaultRT;
}
- (bool)setDefaultRT:(NSString*)rt
{
    // written by opROUTE
    //if (rt == nil) return false;
    [mgmtLock lock];
    [defaultRT release];
    defaultRT = rt;
    [defaultRT retain];
    [mgmtLock unlock];
    return true;
}

- (uint16_t)getLIP2FID:(NSString*)lip
{
    NSNumber* nsfid;
    nsfid = [mgmtDictLIP2FID objectForKey:lip];
    if (nsfid == nil) {
        return 0;
    } else {
        return [nsfid unsignedShortValue];
    }
}

- (NSString*)getFID2LIP:(uint16_t)fid
{
    NSNumber* nsfid;
    //NSLog(@"fid:%d",fid);
    nsfid = [NSNumber numberWithUnsignedShort:fid];
    //NSLog(@"nsfid:%@",nsfid);
    NSString* nslip;
    nslip = [mgmtDictFID2LIP objectForKey:nsfid];
    //NSLog(@"nslip:%@",nslip);
    if (nslip == nil) {
        return 0;
    } else {
        return nslip;
    }
}

- (bool)setPortPairWithProtocol:(uint16_t)protocol
                        SrcPort:(uint16_t)srcPort
                        DstPort:(uint16_t)dstPort
                            FID:(NSNumber*)fid
{
    /*
     * ------------------------   -------------
     * |   proto  |  srcPort  | : |  dstPort  |
     * ------------------------   -------------
    */
    [mgmtLock lock];
    if (protocol == 0) {
        [mgmtLock unlock];
        return false;
    }
    if (srcPort == 0) {
        [mgmtLock unlock];
        return false;
    }
    if (dstPort == 0) {
        [mgmtLock unlock];
        return false;
    }

    uint32_t ProtoPort;
    ProtoPort = ((uint32_t)protocol<<16) | (uint32_t)(srcPort);

    NSNumber* nsProtoPort;
    nsProtoPort = [NSNumber numberWithUnsignedLong:ProtoPort];

    NSNumber* nsDstPort;
    nsDstPort = [NSNumber numberWithUnsignedShort:dstPort];

    if ([mgmtDictPortPair objectForKey:nsProtoPort] != nil) {
        [mgmtLock unlock];
        return false;
    }

    NSNumber* nsPPFlags;
    nsPPFlags = [NSNumber numberWithUnsignedShort:0x00];
    [mgmtDictPPFlags  setObject:nsPPFlags forKey:nsProtoPort];
    [mgmtDictPortPair setObject:nsDstPort forKey:nsProtoPort];

    NSMutableArray* array_fid2pp;
    array_fid2pp = [mgmtDictFID2PP objectForKey:fid];
    if (array_fid2pp == nil) {
        array_fid2pp = [NSMutableArray new];
        [array_fid2pp addObject:nsProtoPort];
        [mgmtDictFID2PP setObject:array_fid2pp forKey:fid];
    } else {
        [array_fid2pp addObject:nsProtoPort];
        [mgmtDictFID2PP setObject:array_fid2pp forKey:fid];
    }

    [mgmtLock unlock];
    return true;
}

- (bool)delPortPairWithProtocol:(uint16_t)protocol
                        SrcPort:(uint16_t)srcPort;
{
    [mgmtLock lock];
    if (protocol == 0) {
        [mgmtLock unlock];
        return false;
    }
    if (srcPort == 0) {
        [mgmtLock unlock];
        return false;
    }

    uint32_t ProtoPort;
    ProtoPort = ((uint32_t)protocol<<16) | (uint32_t)(srcPort);

    NSNumber* nsProtoPort;
    nsProtoPort = [NSNumber numberWithUnsignedLong:ProtoPort];

    if ([mgmtDictPortPair objectForKey:nsProtoPort] != nil || 
        [mgmtDictPPIdle   objectForKey:nsProtoPort] != nil)
    {
        //NSLog(@"check : %d", __LINE__);
        [mgmtDictPortPair removeObjectForKey:nsProtoPort];
        [mgmtDictPPIdle   removeObjectForKey:nsProtoPort];


        /*
        unsigned int pp = [mgmtDictPortPair count];
        unsigned int ppidle = [mgmtDictPPIdle count];
        if (pp != ppidle) {
            NSEnumerator* key_enum;
            if (pp > ppidle) {
                key_enum = [mgmtDictPortPair keyEnumerator];
                ITERATE(element_pp, key_enum) {
                    id content;
                    content = [mgmtDictPPIdle objectForKey:element_pp];
                    if (content == nil) {
                        [mgmtDictPortPair removeObjectForKey:element_pp];
                    }
                }
            }
            if (ppidle > pp) {
                key_enum = [mgmtDictPPIdle keyEnumerator];
                ITERATE(element_ppidle, key_enum) {
                    id content;
                    content = [mgmtDictPortPair objectForKey:element_ppidle];
                    if (content == nil) {
                        [mgmtDictPPIdle removeObjectForKey:element_ppidle];
                    }
                }
            }
        }
        */

        [mgmtLock unlock];
        [self _delPPFlagsWithProtocol:protocol SrcPort:srcPort];
        return true;
    }

    [mgmtLock unlock];


    return false;
}


- (NSArray*)getFID2PP:(NSNumber*)fid
{
    return [mgmtDictFID2PP objectForKey:fid];
}


/*
- (uint16_t)getFID2PPofProtocol:(NSNumber*)fid
{
    NSNumber* nsProtoPort;
    nsProtoPort = [mgmtDictFID2PP objectForKey:fid];
    uint32_t ProtoPort;
    ProtoPort = [nsProtoPort unsignedIntValue];
    return ((ProtoPort>>16)&0x0000FFFF);
    return 0;
}


- (uint16_t)getFID2PPofSrcPort:(NSNumber*)fid
{
    NSNumber* nsProtoPort;
    nsProtoPort = [mgmtDictFID2PP objectForKey:fid];
    uint32_t ProtoPort;
    ProtoPort = [nsProtoPort unsignedIntValue];
    return (ProtoPort&0x0000FFFF);
    return 0;
}
*/


- (uint16_t)getPairPortWithProtocol:(uint16_t)protocol
                            SrcPort:(uint16_t)srcPort;
{
    if (protocol == 0) {
        return 0;
    }
    if (srcPort == 0) {
        return 0;
    }

    uint32_t ProtoPort;
    ProtoPort = ((uint32_t)protocol<<16) | (uint32_t)(srcPort);
    NSNumber* nsProtoPort;
    nsProtoPort = [NSNumber numberWithUnsignedLong:ProtoPort];
    NSNumber* nsDstPort;
    nsDstPort =[mgmtDictPortPair objectForKey:nsProtoPort];
    if(nsDstPort == nil) {
        return 0;
    } else {
        return [nsDstPort unsignedShortValue];
    }
}


- (bool)delFID2PPWithProtocol:(uint16_t)protocol
                      SrcPort:(uint16_t)srcPort
                          FID:(NSNumber*)fid
{
    [mgmtLock lock];

    if (protocol == 0) {
        [mgmtLock unlock];
        return false;
    }
    if (srcPort == 0) {
        [mgmtLock unlock];
        return false;
    }

    uint32_t ProtoPort;
    ProtoPort = ((uint32_t)protocol<<16) | (uint32_t)(srcPort);
    NSNumber* nsProtoPort;
    nsProtoPort = [NSNumber numberWithUnsignedLong:ProtoPort];

    // XXX とりあえず、あとで PP2FID をつくること．．
    NSNumber* tmp_fid;
    tmp_fid = fid;
    if (tmp_fid == nil) {
        NSEnumerator* enumerate_i;
        enumerate_i = [mgmtDictFID2PP keyEnumerator];
        for (id obj_i in enumerate_i) {
            // XXX array_pp を NSMutabelSet にしてContainsObject に書き直す．
            NSEnumerator* enumerate_j;
            enumerate_j = [[mgmtDictFID2PP objectForKey:obj_i] objectEnumerator];
            for (id obj_j in enumerate_j) {
                if ([nsProtoPort isEqualToNumber:obj_j]) {
                    tmp_fid = obj_i; 
                    break;
                }
            }
            if (tmp_fid != nil) break;
        }
    }

    NSMutableArray* array;
    array = [mgmtDictFID2PP objectForKey:tmp_fid];
    [array removeObject:nsProtoPort];
    [mgmtLock unlock];
    return true;
}


- (bool)setPPFlagsWithProtocol:(uint16_t)protocol
                       SrcPort:(uint16_t)srcPort
                       PPFlags:(uint8_t)ppFlags
{
    /*
     * ------------------------   -------------
     * |   proto  |  srcPort  | : |  PPFlags  |
     * ------------------------   -------------
    */

    [mgmtLock lock];
    if (protocol == 0) {
        [mgmtLock unlock];
        return false;
    }
    if (srcPort == 0) {
        [mgmtLock unlock];
        return false;
    }
    if (ppFlags == 0) {
         [mgmtLock unlock];
        return false;
    }

    uint32_t ProtoPort;
    ProtoPort = ((uint32_t)protocol<<16) | (uint32_t)(srcPort);

    NSNumber* nsProtoPort;
    nsProtoPort = [NSNumber numberWithUnsignedLong:ProtoPort];

    NSNumber* nsPPFlags;
    nsPPFlags = [NSNumber numberWithUnsignedChar:ppFlags];
    [mgmtDictPPFlags  setObject:nsPPFlags forKey:nsProtoPort];

    [mgmtLock unlock];
    return true;
}

- (bool)_delPPFlagsWithProtocol:(uint16_t)protocol
                        SrcPort:(uint16_t)srcPort;
{
    [mgmtLock lock];
    if (protocol == 0) {
        [mgmtLock unlock];
        return false;
    }
    if (srcPort == 0) {
        [mgmtLock unlock];
        return false;
    }

    uint32_t ProtoPort;
    ProtoPort = ((uint32_t)protocol<<16) | (uint32_t)(srcPort);

    NSNumber* nsProtoPort;
    nsProtoPort = [NSNumber numberWithUnsignedLong:ProtoPort];

    if ([mgmtDictPPFlags objectForKey:nsProtoPort] != nil) {
        [mgmtDictPPFlags removeObjectForKey:nsProtoPort];
        [mgmtLock unlock];
        return true;
    }

    [mgmtLock unlock];
    return false;
}

// unused message event
- (bool)resetPPFlagsWithProtocol:(uint16_t)protocol
                         SrcPort:(uint16_t)srcPort;
{
    [mgmtLock lock];
    if (protocol == 0) {
        [mgmtLock unlock];
        return false;
    }
    if (srcPort == 0) {
        [mgmtLock unlock];
        return false;
    }

    uint32_t ProtoPort;
    ProtoPort = ((uint32_t)protocol<<16) | (uint32_t)(srcPort);

    NSNumber* nsProtoPort;
    nsProtoPort = [NSNumber numberWithUnsignedLong:ProtoPort];

    if ([mgmtDictPPFlags objectForKey:nsProtoPort] != nil) {
        NSNumber* nsPPFlags;
        nsPPFlags = [NSNumber numberWithUnsignedChar:0x00];
        [mgmtDictPPFlags setObject:nsPPFlags forKey:nsProtoPort];
        [mgmtLock unlock];
        return true;
    }

    [mgmtLock unlock];
    return false;
}


- (uint8_t)getPPFlagsWithProtocol:(uint16_t)protocol
                          SrcPort:(uint16_t)srcPort;
{
    if (protocol == 0) {
        return PPFLAG_ERROR;
    }
    if (srcPort == 0) {
        return PPFLAG_ERROR;
    }

    uint32_t ProtoPort;
    ProtoPort = ((uint32_t)protocol<<16) | (uint32_t)(srcPort);
    NSNumber* nsProtoPort;
    nsProtoPort = [NSNumber numberWithUnsignedLong:ProtoPort];
    NSNumber* nsPPFlags;
    nsPPFlags =[mgmtDictPPFlags objectForKey:nsProtoPort];
    if(nsPPFlags == nil) {
        return PPFLAG_ZERO;
    } else {
        return [nsPPFlags unsignedCharValue];
    }
}

        
- (void)dealloc
{
    // --------------
    // release coding
    // --------------
    [kvt release];
    [defaultIP release];
    [defaultRT release];
    [mgmtDictPortPair release];
    [mgmtDictPPFlags  release];
    [mgmtDictFQDN2LIP release];
    [mgmtDictLIP2FQDN release];
    [mgmtDictLIP2FID  release];
    [mgmtDictFID2LIP  release];
    [mgmtDictFIDDate  release];
    [mgmtDictFIDIdle  release];
    [mgmtDictPPIdle   release];
    //[mgmtDictFIDCounter release];
    [mgmtLock release];
    [super dealloc];
    return;
}
@end
#endif
