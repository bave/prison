
#define __PRISON__

#import <Cocoa/Cocoa.h>

#include "../utils.h"
#include "../task.h"

bool is_verbose = true;

int main(int argc, char** argv) {
    id pool = [NSAutoreleasePool new];
    [NSApplication sharedApplication];

    id task = [TASK new];
    NSLog(@"taskPath:%d", [task setTaskPath:@"../bin/cage"]);
    NSLog(@"sockPaht:%d", [task setSockPath:@"/tmp/sock_cage"]);

    NSLog(@"isRun:%d", [task runTask]);

    sleep(1);

    NSLog(@"isRunTask:%d", [task isRunTask]);

    DISPATCH_START(loop)
    {


        sleep(1);

        NSLog(@"terimnate:%d", [task terminateTask]);

        NSLog(@"runSleep3[s]");
        sleep(3);

        NSLog(@"reRun:%d", [task runTask]);

        NSLog(@"exit");
        [task release];
        exit(0);
    }
    DISPATCH_END
    DISPATCH(loop);

    [NSApp run];
    [pool drain];
    return 0;
}

