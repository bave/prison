#import <Cocoa/Cocoa.h>
#include <stdio.h>
#include <unistd.h>


@interface myObject : NSObject {
    NSTimer* timer;
    int count;
}
- (id)init;
- (void)dealloc;
- (void)action:(NSTimer*)t;
- (void)setTimer;
@end

@implementation myObject

- (void)setTimer
{
    timer = [NSTimer
                scheduledTimerWithTimeInterval:1
                target:self
                selector:@selector(action:)
                userInfo:nil
                repeats:YES];
    return;
}


- (id)init {
    self = [super init];
    if(self != nil) {
        // --------------
        // initial coding
        // --------------
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
        [NSApp terminate:NSApp];
    }
    return;
}

@end

int main(int argc, const char **argv)
{
    id pool = [NSAutoreleasePool new];
    [NSApplication sharedApplication];
    id m = [myObject new];
    [m setTimer];
    [NSApp run];
    [pool drain];
    return 0;
    //return NSApplicationMain(argc, argv);
}

