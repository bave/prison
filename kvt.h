#ifndef __PRISON_KVT_H_
#define __PRISON_KVT_H_

#import <Cocoa/Cocoa.h>

#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <string.h>

#include "category.h"
#include "utils.h"
#include "common.h"

#include "gpg.h"
#include "rc.h"
#include "ni.h"

extern ResourceConfig* rc;
extern NetInfo*     ni;
extern NSLock*  niLock;
extern bool is_verbose;
extern bool is_linking;

@interface keyValueTable : NSObject
{ 
    int cage_sock_fd;
    bool kvt_is_preparation;
    bool kvt_is_join;
    int kvt_counter_join;

    NSTask* kvtTask;
    NSString* kvtTaskPath;
    NSPipe* task_pipe;
    NSFileHandle* task_file;
    NSFileHandle* cage_file;

    NSLock* kvtLock;
    NSMutableDictionary* kvtRequestQueue;
    NSMutableArray* kvtReputQueue;

    GPGME* gpg;
    NSMutableDictionary* kvtLocalDB;
    NSMutableDictionary* kvtCageCache;
}

// public function
- (id)init;
- (id)init:(NSString*)path;
- (void)waitLock;
- (void)dealloc;
- (bool)setLocalDB:(NSDictionary*)db;
- (NSString*)ip4key:(NSString*)key;
- (NSString*)port4key:(NSString*)key;
- (NSString*)value4key:(NSString*)key;

- (NSDictionary*)getRequestQueue;
- (NSArray*)dequeueReput;
- (NSArray*)getReputQueue;
//- (void)storeKVT:(NSString*)key :(NSString*)valeu;

- (bool)setTaskPath:(NSString*)path;

- (bool)isCageRun;
- (bool)sendMessage:(NSString*)message;
- (bool)cage_join;
- (bool)cage_unlink;

// private function
- (void)_debug_dict;

- (void)_gpg_init;

- (bool)_cage_run;
- (void)_cage_init:(NSString*)path;
- (bool)_cage_set_id;
- (bool)_cage_connect;
- (bool)_cage_node_new;
- (bool)_cage_join;
- (bool)_cage_put;
- (void)_cage_set_cache:(NSString*)message;

- (void)_cage_console_handler:(NSNotification*)notify;
- (void)_cage_recv_handler:(NSNotification*)notify;

- (bool)_dequeueRequest:(NSString*)message;
- (bool)_enqueueRequest:(NSString*)message;

- (NSString*)_commit_message:(NSString*)message;

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

- (void)waitLock
{
    [kvtLock lock];
    [kvtLock unlock];
    return;
}

- (NSDictionary*)getRequestQueue
{
    return [NSDictionary dictionaryWithDictionary:kvtRequestQueue];
}

- (NSArray*)dequeueReput
{
    [kvtLock lock];
    NSArray* ret_array = [NSArray arrayWithArray:kvtReputQueue];
    [kvtReputQueue removeAllObjects];
    [kvtLock unlock];
    return ret_array; 
}

- (NSArray*)getReputQueue;
{
    return [NSArray arrayWithArray:kvtReputQueue];
}

- (id)init
{
    if (ni == nil) {
        return nil;
    }

    if ([ni defaultIP4] == nil) {
        return nil;
    }

    self = [super init];
    if(self != nil) {
        // --------------
        // initial coding
        // --------------
        kvtLock = [NSLock new];
        kvtLocalDB = [[NSMutableDictionary alloc] init];
        kvtCageCache = [[NSMutableDictionary alloc] init];
        kvtRequestQueue = [[NSMutableDictionary alloc] init];
        kvtReputQueue = [[NSMutableArray alloc] init];
        [self _gpg_init];
        [self _cage_init:nil];
    }
    return self;
}

- (id)init:(NSString*)path
{
    if (ni == nil) {
        return nil;
    }

    if ([ni defaultIP4] == nil) {
        return nil;
    }

    self = [super init];
    if(self != nil) {
        // --------------
        // initial coding
        // --------------
        kvtLock = [NSLock new];
        kvtLocalDB = [[NSMutableDictionary alloc] init];
        kvtCageCache = [[NSMutableDictionary alloc] init];
        kvtRequestQueue = [[NSMutableDictionary alloc] init];
        kvtReputQueue = [[NSMutableArray alloc] init];
        [self _gpg_init];
        [self _cage_init:path];
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

    value = [kvtLocalDB valueForKey:key];

    if (value == nil) {
        value = [kvtCageCache valueForKey:key];
    }

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

    value = [kvtLocalDB valueForKey:key];

    if (value == nil) {
        value = [kvtCageCache valueForKey:key];
    }

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
    NSString* ret_string = [kvtLocalDB valueForKey:key];
    if (ret_string == nil) {
        ret_string = [kvtCageCache valueForKey:key];
    }
    return ret_string;
}

- (void)dealloc {
    // --------------
    // release coding
    // --------------

    if ([kvtTask isRunning]) {
        [kvtTask terminate];
    }

    [task_pipe release];
    [task_file closeFile];
    [task_file release];

    close(cage_sock_fd);
    [cage_file closeFile];
    [cage_file release];

    [kvtTaskPath release];
    [kvtTask release];
    [gpg release];
    [kvtLock release];
    [kvtRequestQueue release];
    [kvtReputQueue release];
    [kvtLocalDB release];
    [kvtCageCache release];

    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [super dealloc];
    return;
}

- (bool)_cage_run;
{
    if([kvtTask isRunning]) {
        //NSLog(@"isRunning\n");
        return false;
    }

    id pool = [NSAutoreleasePool new];
    id saved_err = nil;

    task_pipe = [[NSPipe alloc] init];
    task_file = [task_pipe fileHandleForReading];

    @try{

        NSMutableArray* args;
        args = [NSMutableArray array];
        [args addObject:@"-f"];
        [args addObject:[rc getInternal]];

        // kvtTask
        [kvtTask setLaunchPath:kvtTaskPath];
        //[task setStandardInput:in_pipe];
        //[task setStandardOutput:out_pipe];
        [kvtTask setStandardError:task_pipe];
        [kvtTask setArguments:args];
        [kvtTask launch];


        [[NSNotificationCenter defaultCenter]
            addObserver:self
            selector:@selector(_cage_console_handler:)
            name:NSFileHandleReadCompletionNotification
            object:task_file
        ];
    }

    @catch (NSString* err) {
        NSString* err_string = [NSString stringWithFormat:
                                @"error: [rc _cage_run] %@", err];
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

    if ([kvtTask isRunning]) {
        return true;
    } else {
        return false;
    }
}

- (bool)setTaskPath:(NSString*)path
{
    id pool = [NSAutoreleasePool new];
    bool ret;
    BOOL isExist;
    BOOL isDir;
    NSFileManager* manager;
    manager = [NSFileManager defaultManager];

    if ([path characterAtIndex:0] == '/' &&
        [[path lastPathComponent] isEqualToString:@"cage"])
    {
        isExist = [manager fileExistsAtPath:path isDirectory:&isDir];
        if (isExist==true && isDir == false) {
            [kvtTaskPath release];
            kvtTaskPath = [[NSString alloc] initWithString:path];
            ret = true;
        } else {
            kvtTaskPath = nil;
            ret = false;
        }
    } else {
        NSString* tmp_path;
        tmp_path = [NSString stringWithFormat:@"%@%@", currentdir(), path];
        isExist = [manager fileExistsAtPath:tmp_path isDirectory:&isDir];
        if (isExist == true && isDir == false) {
            [kvtTaskPath release];
            kvtTaskPath = [[NSString alloc] initWithString:tmp_path];
            ret = true;
        } else {
            ret = false;
        }
    }
    [pool drain];
    return ret;
}

- (bool)isCageRun
{
    return [kvtTask isRunning];
}

- (void)_cage_console_handler:(NSNotification*)notify
{
    id pool = [NSAutoreleasePool new];
    //NSLog(@"\n%@\n", notify);
    //static kvt_is_preparation = false;
            
    if (!kvt_is_preparation)
    {
        NSData* err_data = nil;
        NSString* err_string = nil;
        const NSStringEncoding* encode;
        encode = [NSString availableStringEncodings];
        err_data = [task_file availableData];
        err_string = [[NSString alloc] initWithData:err_data encoding:*encode];
        [err_string autorelease];

        if (is_verbose) {
            NSLog(@"_cage_console_handler(!preparation):\n%@\n", err_string);
        }

        if ([err_string isEqualToString:@"cage: preparations for listen"]) {
            //NSLog(@"cage: preparation\n");
            kvt_is_preparation = true;
        }

        //NSLog(@"preparetion:%d\n", kvt_is_preparation);
        if ([err_string isEqualToString: @"start_listen: No such file or directory"])
        {
            if (is_verbose) {
                NSLog(@"_cage_console_handler: no such file internal sock\n");
            }
            [pool drain];
            @throw @"cant make internal connect socket";
        }
    }

    else if (![[notify userInfo] objectForKey:@"NSFileHandleError"]
             && kvt_is_preparation)
    {
        NSData *data;
        NSString *string;
        const NSStringEncoding* encode;
        encode = [NSString availableStringEncodings];
        data = [[notify userInfo] objectForKey:NSFileHandleNotificationDataItem];
        string = [[NSString alloc] initWithData:data encoding:*encode];
        [string autorelease];

        if (is_verbose) {
            NSLog(@"_cage_console_handler(preparation):\n%@\n", string);
        }

        // rise exception
        if ([string isEqualToString:@"start_listen: No such file or directory"]) {
            [pool drain];
            @throw @"cant make internal connect socket";
        }
    }

    if (kvt_is_preparation) {
        NSLog(@"%@\n", task_file);
        [task_file readInBackgroundAndNotify];
    }
    [pool drain];
    return;
}

- (void)_cage_recv_handler:(NSNotification*)notify
{
    id pool = [NSAutoreleasePool new];
    if (![[notify userInfo] objectForKey:@"NSFileHandleError"])
    {
        NSData *data;
        NSString *buf_string;
        const NSStringEncoding* encode;
        encode = [NSString availableStringEncodings];
        data = [[notify userInfo] objectForKey:NSFileHandleNotificationDataItem];
        buf_string = [[NSString alloc] initWithData:data encoding:*encode];
        [buf_string autorelease];
        if (is_verbose) NSLog(@"_cage_recv_handler:\n%@\n", buf_string);

        if ([self _dequeueRequest:buf_string]) {

            NSString* m;
            if ([buf_string hasSuffix:@"\n"]) {
                NSRange range = NSMakeRange(0 ,[buf_string length]-1);
                m = [NSString stringWithString:[buf_string substringWithRange:range]];
            } else {
                m = buf_string;
            }

            NSArray* m_array = [m componentsSeparatedByString:@","];
            //SUCCEEDED_PUT = "203"
            if ([[m_array objectAtIndex:0] isEqualToString:@"203"]) { 
                [kvtLock lock];
                [kvtReputQueue addObject:[buf_string substringFromIndex:4]];
                [kvtLock unlock];
            }

            //SUCCEEDED_GET = "204"
            else if ([[m_array objectAtIndex:0] isEqualToString:@"204"]) {
                [self _cage_set_cache:m];
                [[NSNotificationCenter defaultCenter]
                    postNotificationName:@"notify://obs.NameReply"
                                  object:m];
            }

            //ERR_GET_FAILURE = "409"
            else if ([[m_array objectAtIndex:0] isEqualToString:@"409"]) {
                [[NSNotificationCenter defaultCenter]
                    postNotificationName:@"notify://obs.NameReply"
                                  object:m];
            }

            // SUCCEEDED_DELETE = "201"
            else if ([[m_array objectAtIndex:0] isEqualToString:@"201"]) {
            }

            // join process
            else if ([[m_array objectAtIndex:1] isEqualToString:@"join"]) {
                if ([[m_array objectAtIndex:0] isEqualToString:@"202"]) {
                    kvt_counter_join = 0;
                    kvt_is_join = true;
                } else if ([[m_array objectAtIndex:0] isEqualToString:@"400"]) {
                    kvt_is_join = false;
                } else if ([[m_array objectAtIndex:0] isEqualToString:@"401"]) {
                    kvt_is_join = false;
                } else if ([[m_array objectAtIndex:0] isEqualToString:@"406"]) {
                    kvt_is_join = false;
                } else if ([[m_array objectAtIndex:0] isEqualToString:@"405"]) {
                    kvt_is_join = false;
                } 

                if (kvt_is_join == false) {
                    kvt_counter_join++;
                    if (kvt_counter_join <= KVT_CAGE_RETRY_JOIN) {
                        [self cage_join];
                    } else {
                        [self cage_unlink];
                    }
                }
            }

        }
    }
    [cage_file readInBackgroundAndNotify];
    [pool drain];

    return;
}

- (void)_gpg_init
{
    gpg = [[GPGME alloc] initWithDir:[rc getPurityPath]];
    if (gpg == nil) {
        [self dealloc];
        @throw @"cant init gpg";
    } else {
        [gpg setPasswd:[rc getPasswd]];
        if ([gpg hasSecring] == false && [gpg hasPubring] == false) {
            NSString* user  = [rc getPrisonName];
            NSString* uid = [NSString stringWithFormat:@"%@@prison", user];
            if (user == nil || uid == nil) {
                [self dealloc];
                @throw @"cant make PurityKeys";
            } else {
                [gpg mkKeyParams:@"RSA" :@"1024" :@"RSA" :@"1024" :user :uid];
                bool is_genkey = true;
                is_genkey = [gpg genkey];
                if (is_genkey == false) {
                    [self dealloc];
                    @throw @"cant make PurityKeys";
                }
            }
        }
    }
    return;
}

- (void)_cage_init:(NSString*)path
{
    id pool = [NSAutoreleasePool new];
    kvtTask = [[NSTask alloc] init];
    if (path == nil) {
        kvtTaskPath = [[NSString alloc] initWithString:@"./cage"];
    } else {
        if ([self setTaskPath:path] == false) {
            [self dealloc];
            [pool drain];
            @throw @"non exist cage";
        }
    }
    [self _cage_run];
    if ([self isCageRun] == false) {
        [self dealloc];
        [pool drain];
        @throw @"cant run cage";
    }

    kvt_is_preparation = false;
    [[NSNotificationCenter defaultCenter]
        postNotificationName:NSFileHandleReadCompletionNotification
        object:task_file
    ]; 

    bool is_connect;
    is_connect = [self _cage_connect];
    if (is_connect == false) {
        [self dealloc];
        [pool drain];
        @throw @"cant connect to internal cage_sock";
    }

    if (is_linking) {
        kvt_counter_join = 0;
        kvt_is_join = [self _cage_join];
    } else {
        kvt_is_join = false;
    }

    cage_file = [[NSFileHandle alloc] initWithFileDescriptor:cage_sock_fd];
    [[NSNotificationCenter defaultCenter]
        addObserver:self
        selector:@selector(_cage_recv_handler:)
        name:NSFileHandleReadCompletionNotification
        object:cage_file
    ];
    [cage_file readInBackgroundAndNotify];

    if (kvt_is_join) {
        [self _cage_put];
    } else {
        [self cage_unlink];
    }

    [pool drain];
    return;
}

- (bool)_cage_put
{
    [kvtLock lock];
    [kvtReputQueue removeAllObjects];
    [kvtLock unlock];

    NSString* hostname = [rc getPrisonName];
    NSString* key = [NSString stringWithFormat:@"%@.p2p", hostname];

    [niLock lock];
    NSString* ip = [ni defaultIP4];
    [niLock unlock];

    NSString* value = [NSString stringWithFormat:@"%@:NULL:NULL", ip];

    NSString* sign = [gpg sign:value];
    NSString* value_sign = [GPGME trimContentFromArmor:sign];

    return [self sendMessage:[NSString stringWithFormat:
                @"put,prison,%@,%@,%d,unique\n", key, value_sign, KVT_CAGE_TTL]];
}

- (bool)_cage_connect
{
    id pool = [NSAutoreleasePool new];
    int err = 0;
    struct sockaddr_un conn_request;
    memset(&conn_request, 0, sizeof(conn_request));

    cage_sock_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (cage_sock_fd == -1) {
        err = errno;
        perror("socket");
    }

    if (err == 0) {
        #ifndef __linux__
        conn_request.sun_len = sizeof(conn_request);
        #endif
        // cis : cage internal sock
        NSString* cis = [rc getInternal];
        memcpy(conn_request.sun_path, [cis UTF8String], [cis length]);
        conn_request.sun_family = AF_LOCAL;
    }

    if (err == 0) {
        err = connect(cage_sock_fd, (SA*)&conn_request, SUN_LEN(&conn_request));
        if (err != 0) {
            perror("connect");
            err = errno;
        }
    }

    if (err == 0) {
        err = [self _cage_node_new];
        if (err == true) {
            err = 0;
            //if (is_verbose) NSLog(@"_cage_node_new: YES");
        } else {
            //if (is_verbose) NSLog(@"_cage_node_new: NO");
            err = -1;
        }
    }

    if (err == 0) {
        err = [self _cage_set_id];
        if (err == true) {
            //if (is_verbose) NSLog(@"_cage_set_id: YES");
            err = 0;
        } else {
            //if (is_verbose) NSLog(@"_cage_set_id: NO");
            err = -1;
        }
    }


    [pool drain];
    if (err == 0) {
        return true;
    } else {
        close(cage_sock_fd);
        return false;
    }
}

- (bool)cage_join
{
    id pool = [NSAutoreleasePool new];
    NSString* ip = [rc getSeedHost];
    NSString* port = [rc getSeedPort];
    NSString* message;
    message = [NSString stringWithFormat:@"join,prison,%@,%@\n", ip, port];

    int ret;
    ret = [self sendMessage:message];

    [pool drain];
    return ret;
}

- (bool)cage_unlink
{
    //kvt_cage_join
    return true;
}

- (bool)_cage_join
{
    NSString* ip = [rc getSeedHost];
    NSString* port = [rc getSeedPort];
    NSString* message;
    message = [NSString stringWithFormat:@"join,prison,%@,%@\n", ip, port];

    retry:
    int ret;
    NSString* buf_string = [self _commit_message:message];
    if (buf_string == nil) {
        ret = false;
    } else {
        ret = true;
    }

    if (ret == true) {
        NSArray* buf_array = [buf_string componentsSeparatedByString:@","];
        //SUCCEEDED_JOIN = "202"
        if ([[buf_array objectAtIndex:0] isEqualToString:@"202"]) {
            kvt_counter_join = 0;
            ret = true;
        } else {
            kvt_counter_join++;
            if (kvt_counter_join <= KVT_CAGE_RETRY_JOIN) {
                goto retry;
            } else {
                ret = false;
            }
        }
    }
    return ret;
}

- (bool)_cage_set_id
{
    if (cage_sock_fd <= 0) {
        return false;
    }

    const NSStringEncoding* encode;
    encode = [NSString availableStringEncodings];
    NSString* user  = [rc getPrisonName];
    NSString* uid = [NSString stringWithFormat:@"%@@prison", user];
    NSString* pubring = [GPGME trimContentFromArmor:[gpg exportPubring:uid]];
    NSString* secring = [GPGME trimContentFromArmor:[gpg exportSecring:nil]];
    NSString* id_str = [NSString stringWithFormat:@"%@%@", pubring, secring];
    NSString* id_hash = [[[id_str dataUsingEncoding:*encode] sha1Digest] hexString];

    NSString* message = [NSString stringWithFormat:@"set_id,prison,%@\n", id_hash];

    int ret;
    NSString* buf_string = [self _commit_message:message];
    if (buf_string == nil) {
        ret = false;
    } else {
        ret = true;
    }

    if (ret == true) {
        NSArray* buf_array = [buf_string componentsSeparatedByString:@","];

        //SUCCEEDED_SET_ID = "205";
        if ([[buf_array objectAtIndex:0] isEqualToString:@"205"]) {
            ret = true;
        } else {
            ret = false;
        }
    }
    return ret;
}

- (bool)_cage_node_new
{
    if (cage_sock_fd <= 0) {
        return false;
    }

    NSString* init_message;
    [niLock lock];
    if (is_global([ni defaultIP4])) {
        init_message = [NSString 
                    stringWithFormat:@"new,prison,%@,global\n", [rc getExternal]];
    } else {
        init_message = [NSString
                    stringWithFormat:@"new,prison,%@\n", [rc getExternal]];
    }
    [niLock unlock];

    int ret;
    NSString* buf_string = [self _commit_message:init_message];
    if (buf_string == nil) {
        ret = false;
    } else {
        ret = true;
    }

    if (ret == true) {
        NSArray* buf_array = [buf_string componentsSeparatedByString:@","];

        //SUCCEEDED_NEW = "200";
        if ([[buf_array objectAtIndex:0] isEqualToString:@"200"]) {
            ret = true;
        } else {
            ret = false;
        }
    }
    return ret;
}

- (void)_cage_set_cache:(NSString*)message
{
    [kvtLock lock];
    // unique
    NSArray* mesg_array = [message componentsSeparatedByString:@","];

    //NSString* code = [mesg_array objectAtIndex:0];
    //NSString* command = [mesg_array objectAtIndex:1];
    //NSString* node_name = [mesg_array objectAtIndex:2];
    NSString* key = [mesg_array objectAtIndex:3];
    NSString* content = [GPGME appendMessageFrame:[mesg_array objectAtIndex:4]];
    NSString* value = [gpg verify:content];
    BOOL valid = [gpg getValid];
    BOOL trust = [gpg getTrust];
    if (valid == YES && trust == YES) {
        [kvtCageCache setObject:value forKey:key];
        if (is_verbose) {
            NSLog(@"kvt_func:_cage_set_cache\n");
            NSLog(@"key:%@\n", key);
            NSLog(@"value:%@\n", value);
        }
    }
    [kvtLock unlock];
    return;
}

- (NSString*)_commit_message:(NSString*)message
{
    char buffer[65535];
    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer, [message UTF8String], [message length]);
    NSString* ret_string = nil;

    int ret = true;
    ret = send(cage_sock_fd, buffer, (socklen_t)sizeof(buffer), 0);
    if (ret == -1) {
        ret = false;
    } else {
        ret = true;
    }

    if (ret == true) {
        int s_retval;
        struct timeval t_val;
        fd_set s_fd;
        FD_ZERO(&s_fd);
        FD_SET(cage_sock_fd, &s_fd);
        memset(&t_val, 0, sizeof(t_val));
        t_val.tv_sec = KVT_CAGE_TIMEOUT;

        s_retval = select((cage_sock_fd+1), &s_fd, NULL, NULL, &t_val);

        if(s_retval <= 0) {
            if(s_retval == -1) perror("select");
            if(s_retval == 0) fprintf(stderr, "timeout! fd : %d \n", cage_sock_fd);
            ret = false;
        }

        else {
            memset(buffer, 0, sizeof(buffer));
            ret = recv(cage_sock_fd, buffer, sizeof(buffer), 0);

            if (ret == -1) {
                ret = false;
            } else {
                ret_string = [NSString stringWithUTF8String:buffer];
                if (is_verbose) {
                    NSLog(@"_commit_message:recvMessageFromCage\n%@\n", ret_string);
                }
                ret = true;
            }
        }
    }

    if (ret == true) {
        return ret_string;
    } else {
        return nil;
    }
}

- (bool)_dequeueRequest:(NSString*)message
{
    //NSLog(@"message:%@\n", message);
    if (message == nil) {
        return false;
    }

    if ([message length] == 0) {
        return false;
    }

    NSArray* mesg_array = [message componentsSeparatedByString:@","];

    if ([mesg_array count] <= 3) {
        return false;
    }

    NSString* command = [mesg_array objectAtIndex:1];
    NSString* key = [mesg_array objectAtIndex:3];
    NSString* string;
    if ([command isEqualToString:@"put"] || [command isEqualToString:@"get"]) {
        if ([key hasSuffix:@"\n"]) {
            string = [NSString stringWithFormat:@"%@,%@", command,
                            [key substringWithRange:NSMakeRange(0,[key length]-1)]];
        } else {
            string = [NSString stringWithFormat:@"%@,%@", command, key];
        }
    }
    if ([command isEqualToString:@"join"]) {
            string = [NSString stringWithString:@"join"];
    }

    //NSLog(@"string:%@\n", string);

    if ([kvtRequestQueue objectForKey:string] != nil) {
        [kvtRequestQueue removeObjectForKey:string];
        return true;
    }
    return false;
}

- (bool)_enqueueRequest:(NSString*)message
{
    if (message == nil) {
        return false;
    }

    if ([message length] == 0) {
        return false;
    }

    NSArray* mesg_array = [message componentsSeparatedByString:@","];

    NSString* string;
    NSString* command = [mesg_array objectAtIndex:0];
    if ([command isEqualToString:@"put"] || [command isEqualToString:@"get"]) {
        NSString* key = [mesg_array objectAtIndex:2];
        if ([key hasSuffix:@"\n"]) {
            string = [NSString stringWithFormat:@"%@,%@", command,
                            [key substringWithRange:NSMakeRange(0,[key length]-1)]];
        } else {
            string = [NSString stringWithFormat:@"%@,%@", command, key];
        }
    }
    if ([command isEqualToString:@"join"]) {
        string = [NSString stringWithString:@"join"];
    }

    [kvtRequestQueue setObject:message forKey:string];
    return true;
}


- (bool)sendMessage:(NSString*)message
{
    int ret = [self _enqueueRequest:message];

    if (is_verbose) NSLog(@"sendMessage:\n%@\n", message);
    if (ret == true) {
        ret = send(cage_sock_fd, [message UTF8String], [message length], 0);
        if (ret == -1) {
            ret = false;
        } else {
            ret = true;
        }
    }

    return ret;
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



