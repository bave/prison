#ifndef __PRISON_TIMER_H_
#define __PRISON_TIMER_H_

#import <Cocoa/Cocoa.h>

#include "mgmt.h"

#include "utils.h"
#include "common.h"
#include "category.h"

extern Manager* mgmt;

@interface Timer : NSObject
{
    NSTimer* idle_timer;
    NSTimer* per_timer;
    NSTimer* verbose_timer;
    NSTimer* reput_timer;
}

- (void)setIdieTimer:(NSTimeInterval)interval;
- (void)setPerTimer:(NSTimeInterval)interval;
- (void)setVerboseTimer:(NSTimeInterval)interval;
- (void)setReputTimer:(NSTimeInterval)interval;

- (void)idle_action:(NSTimer*)timer;
- (void)per_action:(NSTimer*)timer;
- (void)verbose_action:(NSTimer*)timer;
- (void)reput_action:(NSTimeInterval)interval;

- (void)stopIdieTimer;
- (void)stopPerTimer;
- (void)stopVerboseTimer;
- (void)stopReputTimer;

- (id)init;
- (void)dealloc;

@end

@implementation Timer

- (void)setIdieTimer:(NSTimeInterval)interval
{
    idle_timer = [NSTimer
        scheduledTimerWithTimeInterval:interval
        target:self
        selector:@selector(idle_action:)
        userInfo:nil
        repeats:YES];
    return;
}

- (void)idle_action:(NSTimer*)timer
{
    id pool = [NSAutoreleasePool new];
    NSArray* array;
    array = [mgmt idle_timeout];
    if ([array count] != 0) {
        [[NSNotificationCenter defaultCenter]
            postNotificationName:@"notify://obs.FWTimeout"
            object:array];
    }
    [pool drain];
}

- (void)stopIdieTimer
{
    [idle_timer invalidate];
    [idle_timer release];
    return;
}

- (void)setPerTimer:(NSTimeInterval)interval
{
    idle_timer = [NSTimer
        scheduledTimerWithTimeInterval:interval
        target:self
        selector:@selector(per_action:)
        userInfo:nil
        repeats:YES];
    return;
}

- (void)per_action:(NSTimer*)timer
{
    id pool = [NSAutoreleasePool new];
    NSArray* array;
    array = [mgmt per_timeout];
    if ([array count] != 0) {
        [[NSNotificationCenter defaultCenter]
            postNotificationName:@"notify://obs.PPTimeout"
            object:array];
    }
    [pool drain];
}

- (void)stopPerTimer
{
    [per_timer invalidate];
    [per_timer release];
    return;
}

- (void)setVerboseTimer:(NSTimeInterval)interval
{
    verbose_timer = [NSTimer
        scheduledTimerWithTimeInterval:interval
        target:self
        selector:@selector(verbose_action:)
        userInfo:nil
        repeats:YES];
    return;
}

- (void)verbose_action:(NSTimer*)timer
{
    id pool = [NSAutoreleasePool new];
    [mgmt test];
    [pool drain];
    return;
}

- (void)stopVerboseTimer
{
    [verbose_timer invalidate];
    [verbose_timer release];
    return;
}

- (void)setReputTimer:(NSTimeInterval)interval
{
    reput_timer = [NSTimer
        scheduledTimerWithTimeInterval:interval
        target:self
        selector:@selector(reput_action:)
        userInfo:nil
        repeats:YES];
    return;
}

- (void)reput_action:(NSTimeInterval)interval
{ 

    id pool = [NSAutoreleasePool new];
    NSArray* reput_array = [mgmt dequeuePutList];
    NSEnumerator* reput_enum = [reput_array objectEnumerator];
    ITERATE (reput_element, reput_enum) {
        [mgmt putCage:reput_element];
    }
    [pool drain];
    return;
}

- (void)stopReputTimer
{
    [reput_timer invalidate];
    [reput_timer release];
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
    if (idle_timer == nil) [idle_timer release];
    if (per_timer  == nil) [per_timer release];
    if (per_timer  == nil) [verbose_timer release];
    [super dealloc];
    return;
}


@end

#endif



