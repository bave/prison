#import <Cocoa/Cocoa.h>

#include "utils.h"
#include "common.h"
#include "category.h"

@interface RaprinsConfig : NSObject
{
    NSString* rcPath;
    NSString* rcRun;
    NSString* rcSock;
    NSString* rcCagePID;
    NSString* rcRaprinsPID;
    NSString* rcLocalDB;
}

- (id)init;
- (id)initWithConf:(NSString*)path;
- (void)dealloc;

- (bool)_loadConfig;

@end


@implementation RaprinsConfig

- (id)init
{
    self = [super init];
    if(self != nil) {
        // --------------
        // initial coding
        // --------------
        rcPath = @"raprins.conf";
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
            rcPath = @"raprins.conf";
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
    NSString* file = nil;
    file = [[NSString stringWithFile:rcPath] retain];
    if (file == nil) {
        return false;
    }

    NSArray* line_array = nil;
    line_array = [file componentsSeparatedByString:@"\n"];

    NSEnumerator* line_enum = nil;
    line_enum = [line_array objectEnumerator];

    ITERATE(line_element, line_enum) {
        if ([line_element characterAtIndex:0] == '#') {
            continue;
        }

        NSComparisonResult compResult;
        compResult = [line_element compare:@"run_directory"
                                   options:NSCaseInsensitiveSearch
                                     range:NSMakeRange(0,[@"run_directory" length])];

        compResult = [line_element compare:@"local_connect"
                                   options:NSCaseInsensitiveSearch
                                     range:NSMakeRange(0,[@"local_connect" length])];

        NSLog(@"%@\n", line_element);
    }
    

    return true;
}

- (void)dealloc {
    // --------------
    // release coding
    // --------------
    [super dealloc];
    return;
}

@end
