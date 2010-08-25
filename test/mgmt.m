#import <Cocoa/Cocoa.h>

#include "../ni.h"
#include "../rc.h"
#include "../mgmt.h"

FWHooker* fw;
ResourceConfig* rc;
NetInfo* ni;
bool is_verbose;


int main()
{
    id pool = [NSAutoreleasePool new];
    bool ret;
    is_verbose = true;
    Manager* mgmt;
    @try {
        rc = [[ResourceConfig alloc] initWithConf:@"../rc.plist"];
        ni = [NetInfo new];
        fw = [[FWHooker alloc] init_test];
        mgmt = [[Manager alloc] init_test];
        //mgmt = [Manager new];
    } @catch (id err) {
        NSLog(@"%@\n", err);
    }


    ret = [mgmt setFQDN:@"test0.aris.p2p"];
    NSLog(@"test0.aris.p2p:%@",[mgmt getFQDN2LIP:@"test0.aris.p2p"]);
    NSLog(@"test0.aris.p2p:%@",[mgmt getFQDN2GIP:@"test0.aris.p2p"]);
    NSLog(@"test0.aris.p2p:%@",[mgmt getFQDN2PORT:@"test0.aris.p2p"]);
    NSLog(@"127.0.0.2:%@", [mgmt getLIP2FQDN:@"127.0.0.2"]);
    NSLog(@"127.0.0.2:%@", [mgmt getLIP2GIP:@"127.0.0.2"]);
    NSLog(@"127.0.0.2:%@", [mgmt getLIP2PORT:@"127.0.0.2"]);
    ret = [mgmt delFQDN:@"test0.aris.p2p"];
    NSLog(@"test0.aris.p2p:%@",[mgmt getFQDN2LIP:@"test0.aris.p2p"]);
    NSLog(@"test0.aris.p2p:%@",[mgmt getFQDN2GIP:@"test0.aris.p2p"]);
    NSLog(@"test0.aris.p2p:%@",[mgmt getFQDN2PORT:@"test0.aris.p2p"]);
    NSLog(@"127.0.0.2:%@", [mgmt getLIP2FQDN:@"127.0.0.2"]);
    NSLog(@"127.0.0.2:%@", [mgmt getLIP2GIP:@"127.0.0.2"]);
    NSLog(@"127.0.0.2:%@", [mgmt getLIP2PORT:@"127.0.0.2"]);


    NSLog(@"PortPair_test");
    bool re1;
    NSNumber* n = [NSNumber numberWithInt:1];
    re1 = [mgmt setPortPairWithProtocol:IPPROTO_TCP SrcPort:50000 DstPort:6969 FID:n];
    NSLog(@"ret:%d", ret);
    re1 = [mgmt setPortPairWithProtocol:IPPROTO_UDP SrcPort:50001 DstPort:4949 FID:n];
    NSLog(@"ret:%d", ret);
    uint16_t dstPort;
    dstPort = [mgmt getPairPortWithProtocol:IPPROTO_TCP
        SrcPort:50000];
    NSLog(@"PortPair_check:%d", dstPort);
    NSLog(@"true :%d", true);
    NSLog(@"false:%d", false);

    int ppflags;
    [mgmt setPPFlagsWithProtocol:6 SrcPort:6 PPFlags:PPFLAG_ME2L_FIN];
    ppflags = [mgmt getPPFlagsWithProtocol:6 SrcPort:6];
    NSLog(@"%d", ppflags);
    [mgmt resetPPFlagsWithProtocol:6 SrcPort:6];
    ppflags = [mgmt getPPFlagsWithProtocol:6 SrcPort:6];
    NSLog(@"%d", ppflags);


    [mgmt release];
    [pool drain];
    return 0;
}
