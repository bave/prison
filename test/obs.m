#import <Cocoa/Cocoa.h>
#include <stdio.h>
@interface OBS : NSObject
- (void)test:(NSNotification*)notif;
- (void)test2:(NSNotification*)notif;
- (id)init;
- (void)dealloc;
@end
@implementation OBS
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
    [super dealloc];
    return;
}

- (void)test:(NSNotification*)notif
{
    [[NSNotificationCenter defaultCenter]
        postNotificationName:@"notify://obs.test2"
        object:nil
    ];
    NSLog(@"%@", [notif object]);
    return;
}

- (void)test2:(NSNotification*)notif
{
    NSLog(@"END");
    return;
}

@end



int main(int argc, char** argv) {
    id pool = [[NSAutoreleasePool alloc] init];
    id obs = [OBS new];
    [[NSNotificationCenter defaultCenter]
        addObserver:obs
        selector:@selector(test:)
        name:@"notify://obs.test"
        object:nil
    ];
    [[NSNotificationCenter defaultCenter]
        addObserver:obs
        selector:@selector(test2:)
        name:@"notify://obs.test2"
        object:nil
    ];
    [[NSNotificationCenter defaultCenter]
        postNotificationName:@"notify://obs.test"
        object:@"test"
    ];
    [pool drain];
    return 0;
}
