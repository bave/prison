#import <Cocoa/Cocoa.h>

#include "../mgmt.h"


int main()
{
    id pool = [NSAutoreleasePool new];
    bool ret;

    Manager* mgmt;
    NSLog(@"%d", __LINE__);
    mgmt = [Manager new];
    ret = [mgmt setPath:@"../data.conf"];
    ret = [mgmt setFQDN:@"torrent.razgriz.p2p"];
    NSLog(@"torrent.razgriz.p2p:%@",[mgmt getFQDN2LIP:@"torrent.razgriz.p2p"]);
    NSLog(@"torrent.razgriz.p2p:%@",[mgmt getFQDN2GIP:@"torrent.razgriz.p2p"]);
    NSLog(@"torrent.razgriz.p2p:%@",[mgmt getFQDN2PORT:@"torrent.razgriz.p2p"]);
    NSLog(@"127.0.0.2:%@", [mgmt getLIP2FQDN:@"127.0.0.2"]);
    NSLog(@"127.0.0.2:%@", [mgmt getLIP2GIP:@"127.0.0.2"]);
    NSLog(@"127.0.0.2:%@", [mgmt getLIP2PORT:@"127.0.0.2"]);
    ret = [mgmt delFQDN:@"torrent.razgriz.p2p"];
    NSLog(@"torrent.razgriz.p2p:%@",[mgmt getFQDN2LIP:@"torrent.razgriz.p2p"]);
    NSLog(@"torrent.razgriz.p2p:%@",[mgmt getFQDN2GIP:@"torrent.razgriz.p2p"]);
    NSLog(@"torrent.razgriz.p2p:%@",[mgmt getFQDN2PORT:@"torrent.razgriz.p2p"]);
    NSLog(@"127.0.0.2:%@", [mgmt getLIP2FQDN:@"127.0.0.2"]);
    NSLog(@"127.0.0.2:%@", [mgmt getLIP2GIP:@"127.0.0.2"]);
    NSLog(@"127.0.0.2:%@", [mgmt getLIP2PORT:@"127.0.0.2"]);


    NSLog(@"PortPair_test");
    bool re1;
    re1 = [mgmt setPortPairWithProtocol:IPPROTO_TCP
        SrcPort:50000
        DstPort:6969];
    NSLog(@"ret:%d", ret);
    re1 = [mgmt setPortPairWithProtocol:IPPROTO_UDP
        SrcPort:50001
        DstPort:4949];
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

    /*
       int i=0;
       for (;;i++) {
       id loop_pool = [NSAutoreleasePool new];
       [loop_pool drain];
       sleep(1);
       }
       */



    [mgmt release];
    [pool drain];
    return 0;
}
