#ifndef __PRISON_TIMER_H_
#define __PRISON_TIMER_H_

#import <Cocoa/Cocoa.h>

#include "mgmt.h"

#include "utils.h"
#include "common.h"
#include "category.h"

extern Manager*   mgmt;

@interface Timer : NSObject
{
    NSTimer* idle_timer;
    NSTimer* per_timer;
    NSTimer* verbose_timer;
}

- (void)setIdieTimer:(NSTimeInterval)interval;
- (void)setPerTimer:(NSTimeInterval)interval;
- (void)setVerboseTimer:(NSTimeInterval)interval;

- (void)idle_action:(NSTimer*)timer;
- (void)per_action:(NSTimer*)timer;
- (void)verbose_action:(NSTimer*)timer;

- (void)stopIdieTimer;
- (void)stopPerTimer;
- (void)stopVerboseTimer;

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
    [mgmt test];
    return;
}

- (void)stopVerboseTimer
{
    [verbose_timer invalidate];
    [verbose_timer release];
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



