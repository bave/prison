#import <Cocoa/Cocoa.h>
#include <stdio.h>
#include <unistd.h>


@interface myObject : NSObject {
    id mainThread;
    NSTimer* timer;
    int count;
}
- (id)init;
- (void)dealloc;
- (void)action:(NSTimer*)t;
- (void)setMainThread:(id)sender;
@end

@implementation myObject



- (id)init {
    self = [super init];
    if(self != nil) {
        // --------------
        // initial coding
        // --------------
        timer = [[NSTimer
                    scheduledTimerWithTimeInterval:1
                    target:self
                    selector:@selector(action:)
                    userInfo:nil
                    repeats:YES]
                  retain
                ];

    }
    return self;
}


- (void)dealloc {
    // --------------
    // release coding
    // --------------
    [timer invalidate];
    [timer release];
    [super dealloc];
    return;
}

- (void)action:(NSTimer*)t {
    count++;
    NSLog(@"%d\n", count);
    if (count==5) {
        [NSApp terminate:mainThread];
    }
    return;
}

- (void)setMainThread:(id)sender {
    mainThread = sender; 
    return;
}

@end

int main(int argc, const char **argv)
{
    id pool = [NSAutoreleasePool new];
    [NSApplication sharedApplication];

    if([NSBundle loadNibNamed:nil owner:NSApp]) {
        id m = [myObject new];
        [m setMainThread:NSApp];
        //[NSApp setDelegate:];
        [NSApp run];
    }

    [pool drain];
    return 0;
    //return NSApplicationMain(argc, argv);
}

