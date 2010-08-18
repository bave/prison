#ifndef __PRISON_KVT_H_
#define __PRISON_KVT_H_

#import <Cocoa/Cocoa.h>
#include "category.h"
#include "utils.h"
#include "common.h"

#include "gpg.h"

extern ResourceConfig* rc;

@interface keyValueTable : NSObject
{ 
    NSTask* kvtTask;
    NSPipe* task_pipe;
    NSFileHandle* task_file;

    GPGME* gpg;
    //NSString* kvtPath;
    //NSMutableDictionary* kvtDict;
    NSMutableDictionary* kvtLocalDB;
}

// public function
- (id)init;
- (void)dealloc;
- (bool)setLocalDB:(NSDictionary*)db;
- (NSString*)ip4key:(NSString*)key;
- (NSString*)port4key:(NSString*)key;
- (NSString*)value4key:(NSString*)key;
//- (void)storeKVT:(NSString*)key :(NSString*)valeu;

- (BOOL)isCageRun;

// private function
- (void)_debug_dict;
- (BOOL)_run_cage;
- (void)_cage_recv_handler:(NSNotification*)notify;

// trush function
//- (bool)setPath:(NSString*)path;
//- (bool)_loadData;
//- (void)_debug_path;



@end

@implementation keyValueTable


- (void)_debug_dict
{
    //NSLog(@"%@",kvtDict);
    NSLog(@"%@",kvtLocalDB);
    return;
}

- (id)init {
    self = [super init];
    if(self != nil) {
        // --------------
        // initial coding
        // --------------
        //gpg = [GMEME initWithDir:[rc gpgmePath]]
        //kvtPath = nil;
        //kvtDict    = [NSMutableDictionary new];
        kvtLocalDB = [[NSMutableDictionary alloc] init];
        kvtTask = [[NSTask alloc] init];
        task_pipe = [[NSPipe alloc] init];
        task_file = [task_pipe fileHandleForReading];
        int ret;
        ret = [self _run_cage];
        if (ret == false) {
            [self dealloc];
            @throw @"cant run cage";
        }
    }
    return self;
}

- (bool)setLocalDB:(NSDictionary*)db
{
    if (db == nil) return false;

    [kvtLocalDB removeAllObjects];
    NSEnumerator* key_enum;
    key_enum = [db keyEnumerator];
    ITERATE(key_element, key_enum) {
        [kvtLocalDB setObject:[db objectForKey:key_element] forKey:key_element]; 
    }
    //NSLog(@"\n%@\n", kvtLocalDB);

    return true;
}

- (NSString*)ip4key:(NSString*)key
{
    NSString* value;
    //value = [kvtDict valueForKey:key];
    value = [kvtLocalDB valueForKey:key];
    if (value == nil) {
        return nil;
    }
    NSArray* array;
    array = array_split(value, @":");
    return [array objectAtIndex:0];
}

- (NSString*)port4key:(NSString*)key
{
    NSString* port;
    NSString* value;
    //value = [kvtDict valueForKey:key];
    value = [kvtLocalDB valueForKey:key];
    if (value == nil) {
        return nil;
    }
    NSArray* array;
    array = array_split(value, @":");
    port = [array objectAtIndex:1];
    if ([port isEqualToString:@"NULL"]) return nil;
    return port;
}

- (NSString*)value4key:(NSString*)key
{
    //return [kvtDict valueForKey:key];
    return [kvtLocalDB valueForKey:key];
}

- (void)dealloc {
    // --------------
    // release coding
    // --------------
    //[kvtPath release];
    //[kvtDict release];
    if ([self isCageRun]) {
        [kvtTask terminate];
    }
    [task_file release];
    [task_pipe release];
    [kvtTask release];
    [gpg release];
    [kvtLocalDB release];
    [super dealloc];
    return;
}

- (BOOL)_run_cage;
{
    if([kvtTask isRunning]) {
        //NSLog(@"isRunning\n");
        return false;
    }

    id pool = [NSAutoreleasePool new];
    id saved_err = nil;

    NSString* sock_cage_path;

    BOOL ret = true;

    @try{

        sock_cage_path = [[NSString stringWithString:[rc getRunDir]]
                             stringByAppendingPathComponent:[rc getInternal]];

        if (sock_cage_path == nil) {
            return false;
        }

        if ([sock_cage_path length] == 0) {
            return false;
        }

        NSMutableArray* args;
        args = [NSMutableArray array];
        [args addObject:@"-f"];
        [args addObject:@"/tmp/sock_cage"];
        //[args addObject:sock_cage_path];
        //NSLog(@"%@\n", sock_cage_path);
        //[args addObject:sock_cage_path];

        // kvtTask
        [kvtTask setLaunchPath:@"./cage"];
        //[task setStandardInput:in_pipe];
        //[task setStandardOutput:out_pipe];
        [kvtTask setStandardError:task_pipe];
        [kvtTask setArguments:args];
        [kvtTask launch];

        if([kvtTask isRunning]) {
            //@throw @"cant launch cage. maybe mistake path??";
        } else {
            NSLog(@"mistake!!\n");
            @throw @"cant launch cage. maybe mistake path??";
        }



        [[NSNotificationCenter defaultCenter]
            addObserver:self
            selector:@selector(_cage_recv_handler:)
            name:NSFileHandleReadCompletionNotification
            object:task_file
        ];

        [task_file readInBackgroundAndNotify];

        /*
        NSData* err_data = nil;
        NSString* err_string = nil;
        const NSStringEncoding* encode;
        encode = [NSString availableStringEncodings];
        err_data = [task_file readDataToEndOfFile];
        err_string = [[NSString alloc] initWithData:err_data encoding:*encode];
        [err_string autorelease];
        NSLog(@"err:\n%@\n", err_string);
        */

        //[kvtTask waitUntilExit];
        //ret = [kvtTask terminationStatus];

    }

    @catch (NSString* err) {
        NSString* err_string = [NSString stringWithFormat:
                                @"error: [rc _run_cage] %@", err];
        saved_err = [err retain];
        @throw err_string;
    }

    @catch (id err) {
        saved_err = [err retain];
        @throw err;
    }
    @finally {
        //[stat_file closeFile];
        //[out_file closeFile];
        //[err_file closeFile];
        [pool drain];
        [saved_err autorelease];
    }

    return ret;
}

- (BOOL)isCageRun
{
    return [kvtTask isRunning];
}

- (void)_cage_recv_handler:(NSNotification*)notify
{
    //NSLog(@"\n%@\n", notify);
    if(![[notify userInfo] objectForKey:@"NSFileHandleError"]) {
        NSData *data;
        NSString *string;
        data = [[notify userInfo] objectForKey:NSFileHandleNotificationDataItem];
        string = [[NSString alloc] initWithData: data encoding: NSASCIIStringEncoding];
        NSLog(@"\n%@\n", string);
        if (![string isEqualToString:@"start_listen: No such file or directory"]) {
            @throw @"cant make internal connect socket";
        }
    }
    else{
        //NSLog(@"\nerror\n");
    }
    //[task_file readInBackgroundAndNotify];
    return;
}

/*
- (void)_debug_path
{
    NSLog(@"%@",kvtPath);
    return;
}
*/

/*
- (bool)setPath:(NSString*)path
{
    [kvtPath release];
    kvtPath = nil;
    [kvtDict removeAllObjects];
    kvtPath = path;
    [kvtPath retain];
    return [self _loadData];
}
*/


/*
- (bool)_loadData
{
    if (kvtPath == nil) return false;
    NSString* file;
    file = [[NSString stringWithFile:kvtPath] retain];
    if (file == nil) return false;
    NSArray* array;
    array = array_split(file, @"\n");
    int i;
    for (i=0; i<[array icount]; i++) {
        //NSLog(@"%@", [array objectAtIndex:i]);
        NSArray* line;
        line = array_split([array objectAtIndex:i], @" ");
        NSString* key   = [line objectAtIndex:0];
        NSString* value = [line objectAtIndex:1];
        [kvtDict setValue:value forKey:key]; 
    }
    return true;
}
*/

@end
#endif



