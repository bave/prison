#ifndef __RAPRINS_KVT_H_
#define __RAPRINS_KVT_H_

#import <Cocoa/Cocoa.h>
#include "category.h"
#include "utils.h"
#include "common.h"


@interface keyValueTable : NSObject
{ 
    NSString* kvtPath;
    NSMutableDictionary* kvtDict;
}

// public function
- (id)init;
- (void)dealloc;
- (bool)setPath:(NSString*)path;
- (NSString*)ip4key:(NSString*)key;
- (NSString*)port4key:(NSString*)key;
- (NSString*)value4key:(NSString*)key;
//- (void)storeKVT:(NSString*)key :(NSString*)valeu;


// private function
- (bool)_loadData;
- (void)_debug_dict;
- (void)_debug_path;
@end

@implementation keyValueTable

- (void)_debug_path
{
    NSLog(@"%@",kvtPath);
    return;
}

- (void)_debug_dict
{
    NSLog(@"%@",kvtDict);
    return;
}

- (id)init {
    self = [super init];
    if(self != nil) {
        // --------------
        // initial coding
        // --------------
        kvtPath = nil;
        kvtDict = [NSMutableDictionary new];
    }
    return self;
}

- (bool)setPath:(NSString*)path
{
    [kvtPath release];
    kvtPath = nil;
    [kvtDict removeAllObjects];
    kvtPath = path;
    [kvtPath retain];
    return [self _loadData];
}


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

- (NSString*)ip4key:(NSString*)key
{
    NSString* value = [kvtDict valueForKey:key];
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
    value = [kvtDict valueForKey:key];
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
    return [kvtDict valueForKey:key];
}

- (void)dealloc {
    // --------------
    // release coding
    // --------------
    [kvtPath release];
    [kvtDict release];
    [super dealloc];
    return;
}
@end

#endif



