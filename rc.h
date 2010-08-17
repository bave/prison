#ifndef __PRISON_CONFIG_H_
#define __PRISON_CONFIG_H_

#import <Cocoa/Cocoa.h>

#include "utils.h"
#include "common.h"
#include "category.h"

@interface ResourceConfig : NSObject
{
    NSString* rcPath;
    NSString* rcRunDir;
    NSString* rcInternal;
    NSString* rcExternal;
    NSString* rcSeedHost;
    NSString* rcSeedPort;
    NSDictionary* rcLocalDB;
}

- (id)init;
- (id)initWithConf:(NSString*)path;
- (void)dealloc;

- (NSString*)getPath;
- (NSString*)getRunDir;
- (NSString*)getInternal;
- (NSString*)getExternal;
- (NSString*)getSeedHost;
- (NSString*)getSeedPort;
- (NSDictionary*)getLocalDB;

- (bool)_loadConfig;

@end


@implementation ResourceConfig

- (id)init
{
    self = [super init];
    if(self != nil) {
        // --------------
        // initial coding
        // --------------
        rcPath = @"rc.plist";
        if ([self _loadConfig] == false) {
            [self dealloc];
            return nil;
        }
    }
    return self;
}

- (id)initWithConf:(NSString*)path;
{
    self = [super init];
    if(self != nil) {
        if (path == nil) {
            rcPath = @"rc.plist";
            if ([self _loadConfig] == false) {
                [self dealloc];
                return nil;
            }
        } else {
            rcPath = path;
            if ([self _loadConfig] == false) {
                [self dealloc];
                return nil;
            }
        }
    }
    return self;
}

- (bool)_loadConfig
{
    int ret = false;
    id plist = [NSData dataWithPlist:rcPath];
    if (plist != nil) ret = true;

    if (ret == true) {
        rcRunDir = [plist objectForKey:@"run_directory"];
        if (rcRunDir == nil) ret = false;
    }
    
    if (ret == true) {
        rcInternal = [plist objectForKey:@"cage_internal_connect"];
        if (rcInternal == nil) ret = false;
    }

    if (ret == true) {
        rcExternal = [plist objectForKey:@"cage_external_port"];
        if (rcExternal == nil) ret = false;
    }

    if (ret == true) {
        rcSeedHost = [plist objectForKey:@"cage_seed_host"];
        if (rcSeedHost == nil) ret = false;
    }

    if (ret == true) {
        rcSeedPort = [plist objectForKey:@"cage_seed_port"];
        if (rcSeedPort == nil) ret = false;
    }

    if (ret == true) {
        rcLocalDB  = [plist objectForKey:@"local_db"];
        if (rcLocalDB == nil) ret = false;
    }

    return ret;
}

- (void)dealloc {
    // --------------
    // release coding
    // --------------
    [rcPath     release];
    [rcRunDir   release];
    [rcInternal release];
    [rcExternal release];
    [rcSeedHost release];
    [rcSeedPort release];
    [rcLocalDB  release];
    [super dealloc];
    return;
}

- (NSString*)getPath
{
    return rcPath;
}

- (NSString*)getRunDir;
{
    return rcRunDir;
}

- (NSString*)getInternal;
{
    return rcInternal;
}

- (NSString*)getExternal;
{
    return rcExternal;
}

- (NSString*)getSeedHost;
{
    return rcSeedHost;
}

- (NSString*)getSeedPort;
{
    return rcSeedPort;
}

- (NSDictionary*)getLocalDB;
{
    return rcLocalDB;
}

@end

#endif //__PRISON_CONFIG_H_
