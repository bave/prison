#import <Cocoa/Cocoa.h>
#include "../utils.h"
#include "../fw.h"
#include "../common.h"

int main(int argc, char** argv)
{
    id pool = [NSAutoreleasePool new];
    id fw = [FWHooker new];

    [fw setSrcRule:"150.65.32.64" :6969];
    [fw setSrcMaskPrefix:32];
    [fw setDstRule:FW_ANY_ADDRESS :0];
    [fw setDstMaskPrefix:0];
    [fw setProtocol:IPPROTO_TCP];
    [fw setDivertPort:FW_EXT2ME_DIVERT];
    [fw setFlag: IP_FW_F_DME | IP_FW_F_DIVERT | IP_FW_F_IN];
    [fw addRule: 1];

    /*
    // dump check
    NSData* data;
    data = [fw getRuleForNum:1001-FW_BIAS_RULE_NUMBER]; 
    memdump((void*)[data bytes], [data length]);
    data = [fw getRuleForNum:1002-FW_BIAS_RULE_NUMBER]; 
    memdump((void*)[data bytes], [data length]);
    data = [fw getRuleForNum:1003-FW_BIAS_RULE_NUMBER]; 
    memdump((void*)[data bytes], [data length]);
    data = [fw getRuleForNum:1004-FW_BIAS_RULE_NUMBER]; 
    memdump((void*)[data bytes], [data length]);
    */

    if ([fw hasNum:1+FW_BIAS_RULE_NUMBER])
        printf("number:1\n");
      
    if ([fw hasNum:2+FW_BIAS_RULE_NUMBER])
        printf("number:2\n");
    [fw delRule:1];

    [pool drain];

    return 0;
}
