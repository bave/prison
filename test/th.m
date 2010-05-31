#import <Cocoa/Cocoa.h>
#include <unistd.h>

#define TH_ACT0 0
#define TH_ACT1 1


@interface th : NSObject
{
    BOOL thFlag;
}
- (id)init;
- (void)dealloc;
- (void)action0:(id)arg;
- (void)action1:(id)arg;
- (void)startThread;
- (void)stopThread;
- (void)runThread:(int)actionIndex :(id)arg;
@end //interface th

@implementation th
- (id)init {
    self = [super init];
    if(self != nil) {
        // --------------
        // initial coding
        // --------------
        thFlag = true;
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

-(void)startThread {
    thFlag = true; 
}

-(void)stopThread {
    thFlag = false; 
}

-(void)runThread:(int)actionIndex :(id)arg {
    switch(actionIndex) {
        case TH_ACT0: {
            [NSThread detachNewThreadSelector:@selector(action0:) toTarget:self withObject:nil];
            break;
        }
        case TH_ACT1: {
            [NSThread detachNewThreadSelector:@selector(action1:) toTarget:self withObject:arg];
            break;
        }
        default: {
            break;
        }
    }
    return;
}


/*
-(void)thMain {
    NSAutoreleasePool* pool;
    NSAutoreleasePool* internalPool; 

    pool = [[NSAutoreleasePool alloc]init];
    while(mugenFlag)
    {
        internalPool = [[NSAutoreleasePool alloc]init];
        [internalPool release]; 
    }
    [pool release];
    [NSThread exit];
} 
*/

-(void)action0:(id)arg {
    id pool = [[NSAutoreleasePool alloc]init];
    // thread proccessing
    printf("th_start:%d\n", __LINE__);
    sleep(2);
    printf("th_stop:%d\n", __LINE__);
    [pool drain];
    [NSThread exit];
    return;
}

-(void)action1:(id)arg
{
    id pool = [[NSAutoreleasePool alloc]init];
    // thread proccessing
    printf("th_start:%d\n", __LINE__);
    int i;
    for (i=0; i<5; i++) {
        NSLog(@"%@:%d\n", arg, i);
    }
    sleep(2);
    printf("th_stop:%d\n", __LINE__);
    [pool drain];
    [NSThread exit]; 
    return;
}

@end // implementation th

int main(int argc, char** argv) {
    id pool = [[NSAutoreleasePool alloc]init];
    id proc1 = [[[th alloc] init] autorelease];
    id proc2 = [[[th alloc] init] autorelease];
    id proc3 = [[[th alloc] init] autorelease];
    id proc4 = [[[th alloc] init] autorelease];
    //[proc runThread:TH_ACT1 :[NSNumber numberWithInt:1]];
    [proc1 runThread:TH_ACT1 :[NSNumber numberWithInt:1]];
    [proc2 runThread:TH_ACT1 :[NSNumber numberWithInt:2]];
    [proc3 runThread:TH_ACT1 :[NSNumber numberWithInt:3]];
    [proc4 runThread:TH_ACT1 :[NSNumber numberWithInt:4]];
    sleep(5);
    [pool drain];
    return 0;
}
