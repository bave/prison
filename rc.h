#ifndef __PRISON_CONFIG_H_
#define __PRISON_CONFIG_H_

#import <Cocoa/Cocoa.h>


#include "utils.h"
#include "common.h"
#include "category.h"

#include <sys/stat.h>


@interface ResourceConfig : NSObject
{
    NSString* rcPath;
    NSString* rcRunDir;
    NSString* rcPurityPath;
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
- (NSString*)getPurityPath;
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

    //static const mode_t mode_required =
    //               S_ISVTX | S_IRWXU | (S_IRGRP | S_IXGRP) | (S_IROTH | S_IXOTH);
    static const mode_t mode_required = S_IRWXU;
    NSFileManager* manager;
    manager = [NSFileManager defaultManager];

    int ret = false;
    id plist = [NSData dataWithPlist:rcPath];
    if (plist != nil) ret = true;

    if (ret == true) {
        rcRunDir = [plist objectForKey:@"run_directory"];
        if (rcRunDir == nil) {
            ret = false;
        } else {
            BOOL isExist;
            BOOL isDir;
            isExist = [manager fileExistsAtPath:rcRunDir isDirectory:&isDir];

            if (isExist == true && isDir == true) {
                ret = true;
                goto rcRunDir_Leave;
            }
            if (isExist == true && isDir == false) {
                ret = false;
                goto rcRunDir_Leave;
            }
            if (isExist == false) {
                int err;
                err = mkdir([rcRunDir UTF8String], mode_required);
                if (err == -1) {
                    ret = false;
                    NSLog(@"cant make dir\n");
                    goto rcRunDir_Leave;
                } else {
                    ret = true;
                    goto rcRunDir_Leave;
                }
            }

        }

    }
    rcRunDir_Leave:


    if (ret == true) {
        rcPurityPath = [rcRunDir stringByAppendingPathComponent:@"PurityKey"];
        [rcPurityPath retain];

        BOOL isExist;
        BOOL isDir;
        isExist = [manager fileExistsAtPath:rcPurityPath isDirectory:&isDir];
        if (isExist == true && isDir == true) {
            ret = true;
        }
        else if (isExist == true && isDir == false) {
            ret = false;
        }
        else if (isExist == false) {
            int err;
            err = mkdir([rcPurityPath UTF8String], mode_required);
            if (err == -1) {
                ret = false;
            } else {
                ret = true;
            }
        }
    }

    if (ret == true) {
        NSString* internal;
        internal = [plist objectForKey:@"cage_internal_connect"];
        if (internal == nil) {
            ret = false;
        } else {
            rcInternal = [rcRunDir stringByAppendingPathComponent:internal];
            [rcInternal retain];
            ret = true;
        }
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
    [rcPath       release];
    [rcRunDir     release];
    [rcPurityPath release];
    [rcInternal   release];
    [rcExternal   release];
    [rcSeedHost   release];
    [rcSeedPort   release];
    [rcLocalDB    release];
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

- (NSString*)getPurityPath
{
    return rcPurityPath;
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
