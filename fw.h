#ifndef __PRISON_FW_H_
#define __PRISON_FW_H_

//#include <sys/param.h>
//#include <sys/types.h>
//#include <sys/queue.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip_fw.h>

#include <stdint.h>
#include <string.h>
#include <unistd.h>

#import <Cocoa/Cocoa.h>
#include "common.h"
#include "utils.h"
#include "category.h"

// using obs.h -> obsFW
struct _fw_param {
    char srcIP[sizeof("255.255.255.255")];
    char dstIP[sizeof("255.255.255.255")];
    uint8_t srcPrefix;
    uint8_t dstPrefix;
    uint16_t srcPort;
    uint16_t dstPort;
    uint8_t protocol;
    uint16_t divertPort;
    uint32_t flags;
    uint32_t addNumber;
};

/*
 * reference of structure
struct ip_fw {
    u_int32_t version;
    void* context;
    u_int64_t fw_pcnt;
    u_int64_t fw_bcnt;
    struct in_addr fw_src;
    struct in_addr fw_dst;
    struct in_addr fw_smsk;
    struct in_addr fw_dmsk;
    u_short fw_number;
    u_int fw_flg;
    #define IP_FW_MAX_PORTS 10
    union {
        u_short fw_pts[IP_FW_MAX_PORTS];
        #define IP_FW_ICMPTYPES_MAX     128
        #define IP_FW_ICMPTYPES_DIM     (IP_FW_ICMPTYPES_MAX / (sizeof(unsigned) * 8))
        unsigned fw_icmptypes[IP_FW_ICMPTYPES_DIM];
    } fw_uar;
    u_int fw_ipflg;
    u_char fw_ipopt;
    u_char fw_ipnopt;
    u_char fw_tcpopt;
    u_char fw_tcpnopt;
    u_char fw_tcpf;
    u_char fw_tcpnf;
    long timestamp;
    union ip_fw_if fw_in_if;
    union ip_fw_if fw_out_if;
    union {
        u_short fu_divert_port;
        u_short fu_pipe_nr;
        u_short fu_skipto_rule;
        u_short fu_reject_code;
        struct sockaddr_in fu_fwd_ip;
    } fw_un;
    u_char fw_prot;
    u_char fw_nports;
    void* pipe_ptr;
    void* next_rule_ptr;
    uid_t fw_uid;
    int fw_logamount;
    u_int64_t fw_loghighest;
};
*/


@interface FWHooker : NSObject {
    @protected
        int fwSock;
        NSMutableSet* fwList;
        struct ip_fw fwRule;
    @private
        //char _fwSrcAddr[sizeof("255.255.255.255")];
        //char _fwDstAddr[sizeof("255.255.255.255")];
        uint16_t _fwSrcPort;
        uint16_t _fwDstPort;
        uint32_t _fwSrcMaskPrefix;
        uint32_t _fwDstMaskPrefix;
    //@public
}

// public function
- (id)init;
- (void)dealloc;
- (void)addRule:(uint16_t)ruleNumber;
- (void)delRule:(uint16_t)ruleNumber;
- (void)delAllRule;
- (void)setSrcRule:(const char*)addr :(uint16_t)port ;
- (void)setDstRule:(const char*)addr :(uint16_t)port ;
- (void)setSrcMaskPrefix:(uint8_t)size;
- (void)setDstMaskPrefix:(uint8_t)size;
- (void)setProtocol:(uint8_t)protocol;
- (void)setFlag:(unsigned int)flag;
- (void)setDivertPort:(uint16_t)port;
- (NSData*)getRuleForNum:(uint16_t)ruleNumber;
- (NSArray*)getNumList;
- (bool)hasNum:(uint16_t)num;
- (uint16_t)getEmptyNum:(bool)waiting;

// private function
- (void)_formatRule;
@end

@implementation FWHooker
// -------------
// publi  method
// -------------

- (id)init
{
    self = [super init];
    if(self != nil) {
        // --------------
        // initial coding
        // --------------
        fwSock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
        if (fwSock == -1) {
            @throw @"[fw init] : Conuld not open rawSocket!\n";
             // you wait for ...
             // @catch (id ex) {} or @catch (NSString ex) {}
        }
        fwList = [[NSMutableSet alloc] init];
        [self _formatRule];
    }
    return self;
}

- (id)init_test
{
    self = [super init];
    if(self != nil) {
        // --------------
        // initial coding
        // --------------
        fwSock = socket(AF_INET, SOCK_DGRAM, 0);
        if (fwSock == -1) {
            @throw @"[fw init] : Conuld not open rawSocket!\n";
             // you wait for ...
             // @catch (id ex) {} or @catch (NSString ex) {}
        }
        fwList = [[NSMutableSet alloc] init];
        [self _formatRule];
    }
    return self;
}

- (void)dealloc
{
    // --------------
    // release coding
    // --------------
    close(fwSock);
    [fwList release];
    [super dealloc];
    return;
}

- (NSArray*)getNumList
{
    return [fwList allObjects];
}


- (uint16_t)getEmptyNum:(bool)waiting
{
    uint16_t i;
    while (waiting) {
        for (i=1; i<=FW_UPPER_LIMIT; i++) {
            NSNumber* ns_num;
            ns_num = [NSNumber numberWithUnsignedShort:i+FW_BIAS_RULE_NUMBER];
            if ([fwList containsObject:ns_num] == false) {
                return i;
            }
        }
        sleep(1);
    }
    // unused return;
    return 0;
}


- (bool)hasNum:(uint16_t)num{
    /*
    NSNumber* ns_num;
    NSEnumerator* enumerate = [fwList objectEnumerator];
    while ((ns_num = [enumerate nextObject])){
        if([ns_num unsignedShortValue] == num+FW_BIAS_RULE_NUMBER) {
            return true;
        }
    } 
    return false;
    */
    NSNumber* ns_num;
    ns_num = [NSNumber numberWithUnsignedShort:num+FW_BIAS_RULE_NUMBER];
    return [fwList containsObject:ns_num];
}

- (NSData*)getRuleForNum:(uint16_t)ruleNumber
{

    // start of get ipfw rule list ---------------------------------------------------------
    struct ip_fw* rules;
    int indicateDataSize;
    int actualDataSize;
    int numRules;
    int ret;

    rules = NULL;
    indicateDataSize = 0;
    actualDataSize = 0;
    numRules = 1;

    do {

        indicateDataSize = sizeof(struct ip_fw) * numRules;
        rules = (struct ip_fw*)realloc(rules, indicateDataSize);
        memset(rules, 0, sizeof(indicateDataSize));

        rules->version = IP_FW_CURRENT_API_VERSION;
        actualDataSize = indicateDataSize;

        ret = getsockopt(fwSock, IPPROTO_IP, IP_FW_GET, rules, (socklen_t*)&actualDataSize);
        if( ret != 0 )
        {
            printf("%d\n", ret); 
            free(rules);
            @throw @"[getRule] : getsockopt!\n";
        }

        numRules++;

    } while( indicateDataSize == actualDataSize );
    // end of get ipfw rule list ----------------------------------------------------------

    int i;
    int matchingNumber;
    NSMutableData* retData;
    retData = [[NSMutableData alloc] autorelease];
    matchingNumber = FW_BIAS_RULE_NUMBER + ruleNumber;
    for( i = 0; i < numRules; i++ ) {
        if(matchingNumber == rules[i].fw_number) {
            [retData appendBytes:&rules[i] length:sizeof(struct ip_fw)];
        }
    }
    free(rules);
    if ([retData length] != 0) {
        return [NSData dataWithBytes:[retData bytes] length:[retData length]];
    } else {
        return nil;
    }
}


- (void)setFlag:(unsigned int)flag
{
    fwRule.fw_flg |= flag;
    //fwRule.fw_flg |= IP_FW_F_DENY | IP_FW_F_IN | IP_FW_F_OUT;
    // see also /usr/include/netinet/ip_fw.h of IP_FW_F_*
    return;
}


- (void)setDivertPort:(uint16_t)port
{
    fwRule.fw_un.fu_divert_port=port;
    return;
}


- (void)setProtocol:(uint8_t)protocol
{
    fwRule.fw_prot = protocol;
    //IPPROTO_TCP;
    return;
}


- (void)setSrcMaskPrefix:(uint8_t)size
{
    if (size == 32) {
        _fwSrcMaskPrefix = 0xFFFFFFFF;
    } else if (size < 32 && size > 0)  {
        _fwSrcMaskPrefix = FW_PREFIX(size);
    } else {
        _fwSrcMaskPrefix = 0x00000000;
    }   
    return;
}


- (void)setDstMaskPrefix:(uint8_t)size
{
    if (size == 32) {
        _fwDstMaskPrefix = 0xFFFFFFFF;
    } else if (size < 32 && size > 0)  {
        _fwDstMaskPrefix = FW_PREFIX(size);
    } else {
        _fwDstMaskPrefix = 0x00000000;
    }   
    return;
}


- (void)addRule:(uint16_t)ruleNumber {
    int ret = 0;
    int portlist = 0;

    // fill fwRule -----------------------
    fwRule.fw_number = FW_BIAS_RULE_NUMBER + ruleNumber;

    //fwRule.fw_src.s_addr  = inet_addr(_fwSrcAddr);
    fwRule.fw_smsk.s_addr = _fwSrcMaskPrefix;

    //fwRule.fw_dst.s_addr  = inet_addr(_fwDstAddr);
    fwRule.fw_dmsk.s_addr = _fwDstMaskPrefix;

    if (_fwSrcPort != 0) {
        fwRule.fw_uar.fw_pts[portlist] = _fwSrcPort;
        portlist++;
        IP_FW_SETNSRCP(&fwRule, 1);
    }

    if (_fwDstPort != 0) {
        fwRule.fw_uar.fw_pts[portlist] = _fwDstPort;
        IP_FW_SETNDSTP(&fwRule, 1);
    }
    // -----------------------------------

    ret = setsockopt(fwSock, IPPROTO_IP, IP_FW_ADD, &fwRule, sizeof(fwRule));
    if (ret != 0) {
        //printf("setsockopt:%d, error:%d\n", ret, errno);
        @throw @"[addRule] : Conuld not set fwRule!\n";
    }
    [fwList addObject:[NSNumber numberWithInt:fwRule.fw_number]];
    [self _formatRule];
    return;
}

- (void)delRule:(uint16_t)ruleNumber {
    int ret = 0;
    NSNumber* num;
    fwRule.fw_number = FW_BIAS_RULE_NUMBER + ruleNumber;
    num = [fwList member:[NSNumber numberWithInt:fwRule.fw_number]];
    if ( num != nil) { 
        ret = setsockopt(fwSock, IPPROTO_IP, IP_FW_DEL, &fwRule, sizeof(fwRule));
        if (ret != 0) {
            @throw @"[delRule] : Conuld not set fwRule!\n";
        }
        [fwList removeObject:[NSNumber numberWithInt:fwRule.fw_number]];
    } else {
        @throw @"[delRule] : Didn't ipfw have argument Number Rule!\n";
    }
    [self _formatRule];
    return;
}

- (void)delAllRule {
    int i;
    uint16_t num;
    NSArray* array;
    array = [fwList allObjects];
    for (i=0; i<[array icount]; i++) {
        num = [[array objectAtIndex:i] unsignedShortValue]-FW_BIAS_RULE_NUMBER;
        [self delRule:num];
    }
    [fwList removeAllObjects];
    return;
}

- (void)setSrcRule:(const char*)addr:(uint16_t)port {
    //memcpy(_fwSrcAddr, addr, sizeof(addr));
    fwRule.fw_src.s_addr = inet_addr(addr);
    _fwSrcPort = port;
    /*
    fwRule.fw_uar.fw_pts[0] = port;
    if (port != 0) {
        IP_FW_SETNSRCP(&fwRule, 1);
    }
    fwRule.fw_smsk.s_addr = htonl(~0);
    */
    return;
}

- (void)setDstRule:(const char*)addr:(uint16_t)port {
    //memcpy(_fwDstAddr, addr, sizeof(addr));
    fwRule.fw_dst.s_addr = inet_addr(addr);
    _fwDstPort = port;
    /*
    fwRule.fw_uar.fw_pts[1] = port;
    if (port != 0) {
        IP_FW_SETNDSTP(&fwRule, 1);
    }
    fwRule.fw_dmsk.s_addr = htonl(~0);
    */
    return;
}

// ---------------
// internel method
// ---------------
- (void)_formatRule {

    // fill ZERO of struct ip_fw
    memset(&fwRule, 0, sizeof(struct ip_fw));

    // set CurrentVersion (in /netinet/ip_fw.h)
    fwRule.version = IP_FW_CURRENT_API_VERSION;

    // use program context
    //fwRule.context = (void*)FW_PROGRAM_NAME;

    // all Network interface checking flag
    fwRule.fw_in_if.fu_via_if.unit  = -1;
    fwRule.fw_out_if.fu_via_if.unit = -1;

    
    //memset(_fwSrcAddr, 0, sizeof(_fwSrcAddr));
    //memset(_fwDstAddr, 0, sizeof(_fwDstAddr));
    _fwSrcPort = 0;
    _fwDstPort = 0;
    _fwSrcMaskPrefix = 0;
    _fwDstMaskPrefix = 0;

    return;
}

@end

#endif
