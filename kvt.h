#ifndef __RAPRINS_KVT_H_
#define __RAPRINS_KVT_H_

#import <Cocoa/Cocoa.h>
#include "category.h"
#include "utils.h"
#include "common.h"

#include "gpg.h"

extern RaprinsConfig* rc;

@interface keyValueTable : NSObject
{ 
    GPGME* gpg;
    NSString* kvtPath;
    NSMutableDictionary* kvtDict;
    NSMutableDictionary* kvtLocalDB;
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
        kvtPath = nil;
        kvtDict    = [NSMutableDictionary new];
        kvtLocalDB = [[NSMutableDictionary alloc] init];
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
    NSLog(@"\n%@\n", kvtLocalDB);

    return true;
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
    [kvtPath release];
    [kvtDict release];
    [kvtLocalDB release];
    [super dealloc];
    return;
}
@end

#endif



