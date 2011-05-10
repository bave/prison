#ifndef __PRISON_TASK_H_
#define __PRISON_TASK_H_

#import <Cocoa/Cocoa.h>

#include "category.h"
#include "utils.h"
#include "common.h"

#include "rc.h"
#include "ni.h"

// todo
// *** Terminating app due to uncaught exception 'NSInvalidArgumentException', reason: 'task already launched'

extern ResourceConfig* rc;
extern bool is_verbose;

@interface TASK : NSObject
{ 
    BOOL is_once_run;
    NSTask* task;
    NSString* task_path;
    NSString* sock_path;
    NSPipe* task_pipe;
    NSFileHandle* task_handle;
    NSFileHandle* devNull;
}

// public function
- (id)init;
- (void)dealloc;
- (bool)setTaskPath:(NSString*)path;
- (bool)setSockPath:(NSString*)path;
- (bool)runTask;
- (bool)terminateTask;
- (bool)isRunTask;
- (void)_task_handler:(NSNotification*)notify;

@end

@implementation TASK

- (void)_task_handler:(NSNotification*)notify
{
    //id pool = [NSAutoreleasePool new];
    if (![[notify userInfo] objectForKey:@"NSFileHandleError"])
    {
        NSData *data;
        const NSStringEncoding* encode = [NSString availableStringEncodings];
        data = [[notify userInfo] objectForKey:NSFileHandleNotificationDataItem];

        NSString* buffer = [[NSString alloc] initWithData:data encoding:*encode];
        [buffer autorelease];

        //NSLog(@"pipe_handler_message: %@", buffer);

        NSArray* buffer_array;
        buffer_array = [buffer componentsSeparatedByString:@"\n"];
        [buffer_array autorelease];

        unsigned int i;
        for (i=0; i<[buffer_array count]; i++)
        {
            id element = [buffer_array objectAtIndex:i];
            if ([element length] == 0) { continue; }

            if (is_verbose) {
                /*
                if ([element isEqualToString:@"cage: preparations for listen"]) {;}
                */
                NSLog(@"task: %@", element);
            }
        }
    }
    if ([task isRunning]) {
        [task_handle readInBackgroundAndNotify];
    }
    //[pool drain];
    return;
}

- (id)init
{
    self = [super init];
    if(self != nil) {
        // --------------
        // initial coding
        // --------------
        is_once_run = NO;
        task = [[NSTask alloc] init];
        task_pipe = nil;
        task_handle = nil;
        sock_path = nil;
        task_path = nil;
        devNull = nil;
    }

    return self;
}

- (void)dealloc {
    // --------------
    // release coding
    // --------------
    if (task != nil) {
        [self terminateTask];
    }
    [sock_path release];
    [task_path release];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [super dealloc];
    return;
}

- (bool)terminateTask
{
    if ([task isRunning] == YES) {
        [task terminate];
        [task_pipe release];
        [task_handle closeFile];
        [task_handle release];
        [devNull closeFile]; 
        [devNull release]; 
        [task release];
        task = nil;
        task_pipe = nil;
        task_handle = nil;
        return true;
    }
    return false;
}

- (bool)runTask
{
    if (is_once_run == YES) {
        return false;
    }

    if ([task isRunning] == YES) {
        return false;
    }

    if (task_path == nil) {
        task_path = [[NSString alloc] initWithString:@"./cage"];
    }

    if (sock_path == nil) {
        sock_path = [[NSString alloc] initWithString:@"/tmp/sock_cage"];
    }


    id saved_err = nil;
    @try{

        if (is_verbose) {
            task_pipe = [[NSPipe alloc] init];
            task_handle = [task_pipe fileHandleForReading];
        }
        devNull = [NSFileHandle fileHandleForUpdatingAtPath:@"/dev/null"];

        NSMutableArray* args;
        args = [NSMutableArray array];
        [args addObject:@"-n"];
        [args addObject:sock_path];

        // task
        [task setLaunchPath:task_path];
        [task setArguments:args];

        if (is_verbose) {
            [task setStandardInput:devNull];
            [task setStandardOutput:devNull];
            [task setStandardError:task_pipe];

            [[NSNotificationCenter defaultCenter]
                addObserver:self
                selector:@selector(_task_handler:)
                name:NSFileHandleReadCompletionNotification
                object:task_handle
            ];
            [task_handle readInBackgroundAndNotify];
        } else {
            [task setStandardInput:devNull];
            [task setStandardOutput:devNull];
            [task setStandardError:devNull];
        }

        [task launch];
    }

    @catch (NSString* err) {
        NSString* err_string = [NSString stringWithFormat:
                                @"error: task cage_run : %@", err];
        saved_err = [err retain];
        @throw err_string;
    }

    @catch (id err) {
        saved_err = [err retain];
        @throw err;
    }
    @finally {
        [saved_err autorelease];
    }

    if ([task isRunning] == YES) {
        is_once_run = YES;
        return true;
    } else {
        return false;
    }
}

- (bool)isRunTask
{
    if (is_once_run == YES && [task isRunning]) {
        return true;
    }
    else {
        return false;
    }
}

- (bool)setSockPath:(NSString*)path
{
    bool ret;
    BOOL isExist;
    BOOL isDir;
    NSFileManager* manager;
    manager = [NSFileManager defaultManager];

    if ([path characterAtIndex:0] == '/') {
        [sock_path release];
        sock_path = [[NSString alloc] initWithString:path];
        ret = true;
    } else {
        NSString* tmp_path;
        tmp_path = [NSString stringWithFormat:@"%@%@", currentdir(), path];
        isExist = [manager fileExistsAtPath:tmp_path isDirectory:&isDir];
        if (isExist == true && isDir == false) {
            [sock_path release];
            sock_path = [[NSString alloc] initWithString:tmp_path];
            ret = true;
        } else {
            ret = false;
        }
    }
    return ret;
}

- (bool)setTaskPath:(NSString*)path
{
    bool ret;
    BOOL isExist;
    BOOL isDir;
    NSFileManager* manager;
    manager = [NSFileManager defaultManager];

    if ([path characterAtIndex:0] == '/' &&
        [[path lastPathComponent] isEqualToString:@"cage"])
    {
        isExist = [manager fileExistsAtPath:path isDirectory:&isDir];
        if (isExist==false && isDir == true) {
            [task_path release];
            task_path = [[NSString alloc] initWithString:path];
            ret = true;
        } else {
            task_path = nil;
            ret = false;
        }
    } else {
        NSString* tmp_path;
        tmp_path = [NSString stringWithFormat:@"%@%@", currentdir(), path];
        isExist = [manager fileExistsAtPath:tmp_path isDirectory:&isDir];
        if (isExist == true && isDir == false) {
            [task_path release];
            task_path = [[NSString alloc] initWithString:tmp_path];
            ret = true;
        } else {
            ret = false;
        }
    }
    return ret;
}

@end
#endif



