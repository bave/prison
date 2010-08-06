
// system includer
#import <Cocoa/Cocoa.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>

// local includer
#include "fw.h"
#include "op.h"
#include "obs.h"
#include "mgmt.h"

//#include "JSON/JSON.h"
//#include "name.h"
//#include "pbuf.h"

//#include "common.h"
//#include "category.h"
//#include "utils.h"

// global declaration
NSLock* extLock;
FWHooker*  fw;
Observer* obs;
Manager* mgmt;

// prototype declaration
void sig_action(int sig);
void exit_action(const char* err_name);
void exit_signal(const char* sig_name);
int main(int argc, char** argv);

// implimentation 
int main(int argc, char** argv)
{

    // initialize declaration
    id pool = [[NSAutoreleasePool alloc] init];

    if (SIG_ERR == signal(SIGPIPE, SIG_IGN)) exit_signal("SIGHUP");
    if (SIG_ERR == signal(SIGHUP, sig_action)) exit_signal("SIGHUP");
    if (SIG_ERR == signal(SIGINT, sig_action)) exit_signal("SIGINT");
    if (SIG_ERR == signal(SIGTERM, sig_action)) exit_signal("SIGTERM");

    // initialize Class -------------------------------------------------------
    mgmt = [Manager new];
    extLock = [NSLock new];
    // ------------------------------------------------------------------------

    @try {

        fw = [[FWHooker alloc] init];

        // add 0+bias divert 10000 udp from me to any dst-port 53 ------------------------
        [fw setSrcRule:FW_ANY_ADDRESS :0];
        [fw setSrcMaskPrefix:0];
        [fw setDstRule:FW_ANY_ADDRESS :FW_RESOLVER_PORT];
        [fw setDstMaskPrefix:0];
        [fw setProtocol:IPPROTO_UDP];
        [fw setDivertPort:FW_NAME_DIVERT];
        //[fw setFlag: IP_FW_F_SME | IP_FW_F_DIVERT | IP_FW_F_IN | IP_FW_F_OUT];
        [fw setFlag: IP_FW_F_DIVERT | IP_FW_F_IN | IP_FW_F_OUT];
        [fw addRule: 0];
        // --------------------------------------------------------------------

        // add 0+bias allow ip from me to 127.0.0.1/32 --------------------------------
        [fw setSrcRule:FW_ANY_ADDRESS :0];
        [fw setSrcMaskPrefix:0];
        [fw setDstRule:"127.0.0.1" :0];
        [fw setDstMaskPrefix:32];
        [fw setProtocol:IPPROTO_IP];
        [fw setFlag: IP_FW_F_SME | IP_FW_F_ACCEPT | IP_FW_F_IN | IP_FW_F_OUT];
        [fw addRule: 0];
        // --------------------------------------------------------------------

        // add 0+bias divert 10001 ip from 127.0.0.1 to 127/8 ---------------------
        [fw setSrcRule:"127.0.0.1" :0];
        [fw setSrcMaskPrefix:32];
        [fw setDstRule:"127.0.0.0" :0];
        [fw setDstMaskPrefix:8];
        [fw setProtocol:IPPROTO_IP];
        [fw setDivertPort:FW_ME2L_DIVERT];
        [fw setFlag: IP_FW_F_DIVERT | IP_FW_F_IN | IP_FW_F_OUT];
        [fw addRule: 0];
        // --------------------------------------------------------------------

        // add 52 divert 10002 tcp from 150.65.32.64 6969 to me ---------------
        /*
        [fw setSrcRule:"150.65.32.64" :6969];
        [fw setSrcMaskPrefix:32];
        [fw setDstRule:FW_ANY_ADDRESS :0];
        [fw setDstMaskPrefix:0];
        [fw setProtocol:IPPROTO_TCP];
        [fw setDivertPort:FW_EXT2ME_DIVERT];
        [fw setFlag: IP_FW_F_DME | IP_FW_F_DIVERT | IP_FW_F_IN | IP_FW_F_OUT];
        [fw addRule: 2];
        */
        // --------------------------------------------------------------------
    }
    @catch(id ex) {
         NSLog(@"%@", ex);
         exit_action("error->exit : fw routing");
    } 
    @finally { }


    // set divert socket for name ---------------------------------------------
    struct sockaddr_in sin_divert;

    memset(&sin_divert, 0,sizeof(sin_divert));
    sin_divert.sin_family = AF_INET;
    sin_divert.sin_port   = htons(FW_NAME_DIVERT);

    int divertNAME;
    divertNAME = socket(AF_INET, SOCK_RAW, IPPROTO_DIVERT);
    if (divertNAME < 0) {
        perror("socket");
        exit_action("socket");
    }
    if (bind(divertNAME, (SA*)&sin_divert, sizeof(sin_divert)) < 0) {
        printf("%d\n", __LINE__);
        perror("bind");
        exit_action("bind");
    }
    // ----------------------------------------------------------------------


    // set divert socket for me to local ------------------------------------
    memset(&sin_divert, 0,sizeof(sin_divert));
    sin_divert.sin_family = AF_INET;
    sin_divert.sin_port   = htons(FW_ME2L_DIVERT);

    int divertME2L;
    divertME2L = socket(AF_INET, SOCK_RAW, IPPROTO_DIVERT);
    if (divertME2L < 0) {
        perror("socket");
        exit_action("socket");
    }
    if (bind(divertME2L, (SA*)&sin_divert, sizeof(sin_divert)) < 0) {
        printf("%d\n", __LINE__);
        perror("bind");
        exit_action("bind");
    }
    // --------------------------------------------------------------------


    // set divert socket for extarnal to me -------------------------------
    memset(&sin_divert, 0,sizeof(sin_divert));
    sin_divert.sin_family = AF_INET;
    sin_divert.sin_port   = htons(FW_EXT2ME_DIVERT);

    int divertEXT2ME;
    divertEXT2ME = socket(AF_INET, SOCK_RAW, IPPROTO_DIVERT);
    if (divertEXT2ME < 0) {
        perror("socket");
        exit_action("socket");
    }
    if (bind(divertEXT2ME, (SA*)&sin_divert, sizeof(sin_divert)) < 0) {
        printf("%d\n", __LINE__);
        perror("bind");
        exit_action("bind");
    }
    // --------------------------------------------------------------------


    NSOperationQueue *queue;
    queue = [[NSOperationQueue alloc] init];

    [queue setMaxConcurrentOperationCount:10];
    [queue setName:@"mainQueue"];

    Operation *opNAME;
    opNAME = [[[Operation alloc] init] autorelease];
    opNAME.socketFD=divertNAME;
    opNAME.selector=OP_NAME;
    [queue addOperation:opNAME];

    Operation *opME2L;
    opME2L = [[[Operation alloc] init] autorelease];
    opME2L.socketFD=divertME2L;
    opME2L.selector=OP_ME2L;
    [queue addOperation:opME2L];

    Operation *opEXT2ME;
    opEXT2ME = [[[Operation alloc] init] autorelease];
    opEXT2ME.socketFD=divertEXT2ME;
    opEXT2ME.selector=OP_EXT2ME;
    [queue addOperation:opEXT2ME];

    Operation *opROUTE;
    opROUTE = [[[Operation alloc] init] autorelease];
    opROUTE.socketFD=0;
    opROUTE.selector=OP_ROUTE;
    [queue addOperation:opROUTE];


    // set observer instance
    obs = [Observer new];
    //obs.queue = queue;

    // add observer Object
    [[NSNotificationCenter defaultCenter]
        addObserver:obs
        selector:@selector(obsFW:)
        name:@"notify://obs.FWHooker"
        object:nil
    ];
    [[NSNotificationCenter defaultCenter]
        addObserver:obs
        selector:@selector(obsDS:)
        name:@"notify://obs.DSflushcache"
        object:nil
    ];
    [[NSNotificationCenter defaultCenter]
        addObserver:obs
        selector:@selector(obsFWT:)
        name:@"notify://obs.FWTimeout"
        object:nil
    ];
    [[NSNotificationCenter defaultCenter]
        addObserver:obs
        selector:@selector(obsPPT:)
        name:@"notify://obs.PPTimeout"
        object:nil
    ];


    // post notificaiton message
    // flushcache
    [[NSNotificationCenter defaultCenter]
        postNotificationName:@"notify://obs.DSflushcache"
        object:nil
    ];


    for (;;) {
        id loop_pool = [NSAutoreleasePool new];

        // timer function
        NSArray* array;
        array = [mgmt idle_timeout];
        if ([array count] != 0) {
            [[NSNotificationCenter defaultCenter]
                postNotificationName:@"notify://obs.FWTimeout"
                object:array
            ];
        }

        array = [mgmt per_timeout];
        if ([array count] != 0) {
            [[NSNotificationCenter defaultCenter]
                postNotificationName:@"notify://obs.PPTimeout"
                object:array
            ];
        }

        [mgmt test];
        sleep(5);

        [loop_pool drain];
    }

    [fw delAllRule];
    close(divertNAME);
    close(divertME2L);
    close(divertEXT2ME);
    [extLock release];
    [queue release];
    [pool drain];
    return success;
}

void sig_action(int sig) {
    printf("EXIT: catch signal No.%d\n", sig);
    //close(divertNAME);
    [fw delAllRule];
    exit(fail);
}

void exit_signal(const char* sig_name) {
    printf("Failed to set signal handler: %s\n", sig_name);
    exit(fail);
}

void exit_action(const char* err_name) {
    printf("Failed to call: %s\n", err_name);
    //close(divertNAME);
    [fw delAllRule];
    exit(fail);
}
