
// system includer
#import <Cocoa/Cocoa.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>

// local includer
#include "rc.h"
#include "fw.h"
#include "op.h"
#include "obs.h"
#include "mgmt.h"
#include "timer.h"

//#include "name.h"
//#include "pbuf.h"

//#include "common.h"
//#include "category.h"
//#include "utils.h"

#define __PRISON__

// global declaration
NSLock*    extLock;
NetInfo*        ni; 
FWHooker*       fw;
ResourceConfig* rc;
Observer*      obs;
Manager*      mgmt;

bool is_verbose;

extern char *optarg;
extern int optind;
extern int opterr;
extern int optopt;

// prototype declaration
void usage(char *cmd);
void sig_action(int sig);
void exit_action(const char* err_name);
void exit_signal(const char* sig_name);
int main(int argc, char** argv);


// implimentation 
int main(int argc, char** argv)
{

    // initialize declaration
    id pool = [[NSAutoreleasePool alloc] init];
    [NSApplication sharedApplication];

    int opt;
    pid_t pid;
    NSString* config_path = nil;
    is_verbose = false;
    bool is_daemon  = false;

    while ((opt = getopt(argc, argv, "vdhf:")) != -1) {
        switch (opt) {
            case 'h':
                usage(argv[0]);
                return 0;
            case 'v':
                // for run_loop of [mgmt test] 
                is_verbose = true;
                break;
            case 'd':
                is_daemon  = true;
                break;
            case 'f':
                config_path = [NSString stringWithUTF8String:optarg];
                break;
        }
    }

    if (config_path == nil) {
        config_path = @"./rc.plist";
    }
    
    rc = [[ResourceConfig alloc] initWithConf:config_path];
    if (rc == nil) {
        usage(argv[0]);
        exit_action("[RaprincConfig init]");
    }

    //XXX change to reload and restart programing.... at SIGHUP
    if (SIG_ERR == signal(SIGHUP, sig_action)) exit_signal("SIGHUP");
    if (SIG_ERR == signal(SIGPIPE, SIG_IGN)) exit_signal("SIGPIPE");
    if (SIG_ERR == signal(SIGINT, sig_action)) exit_signal("SIGINT");
    if (SIG_ERR == signal(SIGTERM, sig_action)) exit_signal("SIGTERM");


    if (is_daemon) {
        if ((pid = fork()) < 0) {
            //NSLog(@"cant fork");
            return -1;
        } else if (pid != 0){
            //NSLog(@"parent process!!");
            exit(0);
        } else {
            //NSLog(@"forked process!!");
            setsid();
            chdir([currentdir() UTF8String]);
            //chdir("/tmp");

            char* arg0 = (char*)argv[0];
            char* arg1 = NULL;
            char* arg2 = NULL;
            char* args[4];
            bool is_break = false;

            int i;
            for (i=1; i<=argc; i++) {
                if (strcmp(argv[i], "-f") == 0) {
                    arg1 = (char*)argv[i];
                    arg2 = (char*)argv[i+1];
                    is_break = true;
                    break;
                }
            }

            if (is_break) {
                args[0] = arg0;
                args[1] = arg1;
                args[2] = arg2;
                args[3] = (char*)NULL;
            } else {
                args[0] = arg0;
                args[1] = (char*)NULL;
                args[2] = (char*)NULL;
                args[3] = (char*)NULL;
            }

            umask(0);
            execvp(arg0, args);
            fprintf(stderr, "child error!!\n");
            return -1;
        }
    }

    // initialize Class -------------------------------------------------------
    @try {
        ni = [NetInfo new];
        mgmt = [Manager new];
        extLock = [NSLock new];
    }
    @catch (id err) {
        NSString* err_str;
        err_str = [NSString stringWithFormat:@"%@%@\n", @"init error: ", err];
        exit_action([err_str UTF8String]);
    }
    // ------------------------------------------------------------------------

    @try {

        fw = [[FWHooker alloc] init];

        // add 0+bias divert 10000 udp from me to any dst-port 53 -------------
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

        // add 0+bias allow ip from me to 127.0.0.1/32 ------------------------
        [fw setSrcRule:FW_ANY_ADDRESS :0];
        [fw setSrcMaskPrefix:0];
        [fw setDstRule:"127.0.0.1" :0];
        [fw setDstMaskPrefix:32];
        [fw setProtocol:IPPROTO_IP];
        [fw setFlag: IP_FW_F_SME | IP_FW_F_ACCEPT | IP_FW_F_IN | IP_FW_F_OUT];
        [fw addRule: 0];
        // --------------------------------------------------------------------

        // add 0+bias divert 10001 ip from 127.0.0.1 to 127/8 -----------------
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
         //NSLog(@"%@", ex);
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
    // ------------------------------------------------------------------------


    // set divert socket for extarnal to me -----------------------------------
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
        perror("bind");
        exit_action("bind");
    }
    // --------------------------------------------------------------------


    NSOperationQueue* queue;
    queue = [[NSOperationQueue alloc] init];

    [queue setMaxConcurrentOperationCount:10];
    [queue setName:@"mainQueue"];

    Operation* opNAME;
    opNAME = [[[Operation alloc] init] autorelease];
    opNAME.socketFD=divertNAME;
    opNAME.selector=OP_NAME;
    [queue addOperation:opNAME];

    Operation* opME2L;
    opME2L = [[[Operation alloc] init] autorelease];
    opME2L.socketFD=divertME2L;
    opME2L.selector=OP_ME2L;
    [queue addOperation:opME2L];

    Operation* opEXT2ME;
    opEXT2ME = [[[Operation alloc] init] autorelease];
    opEXT2ME.socketFD=divertEXT2ME;
    opEXT2ME.selector=OP_EXT2ME;
    [queue addOperation:opEXT2ME];

    Operation* opROUTE;
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
    [[NSNotificationCenter defaultCenter]
        addObserver:obs
        selector:@selector(obsNameReply:)
        name:@"notify://obs.NameReply"
        object:nil
    ];

    // post notificaiton message
    // flushcache
    [[NSNotificationCenter defaultCenter]
        postNotificationName:@"notify://obs.DSflushcache"
        object:nil
    ];



    // - event loop ------------------------------------------
    Timer* t = [Timer new];

    if (is_verbose) {
        [t setVerboseTimer:5];
    }
    [t setIdieTimer:-FW_IDLE_TIMEOUT];
    [t setPerTimer:-PP_IDLE_TIMEOUT];
    [t setReputTimer:KVT_CAGE_TTL-10];

    [NSApp run];
    // -------------------------------------------------------
 
    /*
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

        if (is_verbose) {
            [mgmt test];
        }

        sleep(5);

        [loop_pool drain];
    }
    */

    [fw delAllRule];
    close(divertNAME);
    close(divertME2L);
    close(divertEXT2ME);
    [extLock release];
    [queue release];
    [pool drain];
    return success;
}

void usage(char *cmd)
{
    printf("%s [-v] [-d] [-f config-file-path]\n", cmd);
    printf("    -h: show this help\n");
    printf("    -d: run as daemon\n");
    printf("    -v: varbose output to stdout\n");
    printf("    -f: rc.plist, default value is ./rc.plist\n");
}

void sig_action(int sig) {
    fprintf(stderr, "EXIT: catch signal No.%d\n", sig);
    //close(divertNAME);
    [extLock release];
    [obs     release];
    [ni      release];
    [mgmt    release];
    [fw delAllRule];
    [fw release];
    exit(fail);
}

void exit_action(const char* err_name) {
    fprintf(stderr, "Failed to call: %s\n", err_name);
    //close(divertNAME);
    [extLock release];
    [obs     release];
    [ni      release];
    [mgmt    release];
    [fw delAllRule];
    [fw release];
    exit(fail);
}

void exit_signal(const char* sig_name) {
    fprintf(stderr, "Failed to set signal handler: %s\n", sig_name);
    exit(fail);
}

