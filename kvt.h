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
extern NetInfo* ni;
extern bool is_verbose;

@interface keyValueTable : NSObject
{ 
    int cage_sock_fd;
    bool kvt_is_preparation;

    NSTask* kvtTask;
    NSString* kvtTaskPath;
    NSPipe* task_pipe;
    NSFileHandle* task_file;
    NSFileHandle* cage_file;

    NSMutableSet* kvtRequestQueue;

    GPGME* gpg;

    NSMutableDictionary* kvtLocalDB;
}

// public function
- (id)init;
- (id)init_test;
- (void)dealloc;
- (bool)setLocalDB:(NSDictionary*)db;
- (NSString*)ip4key:(NSString*)key;
- (NSString*)port4key:(NSString*)key;
- (NSString*)value4key:(NSString*)key;
//- (void)storeKVT:(NSString*)key :(NSString*)valeu;

- (bool)setTaskPath:(NSString*)path;
- (bool)isCageRun;

// private function
- (void)_debug_dict;
- (bool)_cage_run;

- (void)_gpg_init;

- (void)_cage_init:(NSString*)path;
- (bool)_cage_set_id;
- (bool)_cage_connect;
- (bool)_cage_node_new;
- (bool)_cage_join;

- (void)_cage_console_handler:(NSNotification*)notify;
- (void)_cage_recv_handler:(NSNotification*)notify;

- (NSString*)_commit_message:(NSString*)message;
- (bool)_cage_send_message:(NSString*)message;

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

- (id)init
{
    self = [super init];
    if(self != nil) {
        // --------------
        // initial coding
        // --------------
        kvtLocalDB = [[NSMutableDictionary alloc] init];
        [self _gpg_init];
        [self _cage_init:nil];
    }
    return self;
}

- (id)init:(NSString*)path
{
    self = [super init];
    if(self != nil) {
        // --------------
        // initial coding
        // --------------
        kvtLocalDB = [[NSMutableDictionary alloc] init];
        kvtRequestQueue = [[NSMutableSet alloc] init];
        [self _gpg_init];
        [self _cage_init:path];
    }
    return self;
}

- (id)init_test
{
    self = [super init];
    if(self != nil) {
        kvtLocalDB = [[NSMutableDictionary alloc] init];
        // --------------
        // initial coding
        // --------------
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
    if ([kvtTask isRunning]) {
        [kvtTask terminate];
    }
    [task_file release];
    [cage_file release];
    [task_pipe release];
    [kvtTaskPath release];
    [kvtTask release];
    [gpg release];
    [kvtRequestQueue release];
    [kvtLocalDB release];
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
        //[args addObject:sock_cage_path];
        //NSLog(@"%@\n", sock_cage_path);
        //[args addObject:sock_cage_path];

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

        //[task_file readInBackgroundAndNotify];

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
        //bool ret;
        //ret = [kvtTask terminationStatus];

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
    //NSLog(@"\n%@\n", notify);
    kvt_is_preparation = false;
            
    if (!kvt_is_preparation)
    {
        NSData* err_data = nil;
        NSString* err_string = nil;
        const NSStringEncoding* encode;
        encode = [NSString availableStringEncodings];
        err_data = [task_file availableData];
        err_string = [[NSString alloc] initWithData:err_data encoding:*encode];

        //NSLog(@"err_string:%@\n", err_string);
        if ([err_string isEqualToString:@"cage: preparations for listen"]) {
            //NSLog(@"cage: preparation\n");
            kvt_is_preparation = true;
        }

        //NSLog(@"preparetion:%d\n", kvt_is_preparation);
        if ([err_string isEqualToString:@"start_listen: No such file or directory"]) {
            if (is_verbose) NSLog(@"_cage_console_handler: no such file internal sock\n");
            @throw @"cant make internal connect socket";
        }
    }
    else if ( ![[notify userInfo] objectForKey:@"NSFileHandleError"]
              && kvt_is_preparation)
    {
        NSData *data;
        NSString *string;
        const NSStringEncoding* encode;
        encode = [NSString availableStringEncodings];
        data = [[notify userInfo] objectForKey:NSFileHandleNotificationDataItem];
        string = [[NSString alloc] initWithData: data encoding:*encode];

        if (is_verbose) NSLog(@"_cage_console_handler:\n%@\n", string);
        if ([string isEqualToString:@"start_listen: No such file or directory"]) {
            @throw @"cant make internal connect socket";
        }
    }

    if (kvt_is_preparation) {
        //NSLog(@"event check\n");
        [task_file readInBackgroundAndNotify];
    }
    return;
}

- (void)_cage_recv_handler:(NSNotification*)notify
{
    if (![[notify userInfo] objectForKey:@"NSFileHandleError"]) {
        NSData *data;
        NSString *buf_string;
        const NSStringEncoding* encode;
        encode = [NSString availableStringEncodings];
        data = [[notify userInfo] objectForKey:NSFileHandleNotificationDataItem];
        buf_string = [[NSString alloc] initWithData:data encoding:*encode];
        if (is_verbose) NSLog(@"_cage_recv_handler:\n%@\n", buf_string);

        if (buf_string != nil) {
            NSArray* buf_array = [buf_string componentsSeparatedByString:@","];

            //SUCCEEDED_PUT = "203"
            if ([[buf_array objectAtIndex:0] isEqualToString:@"202"]) { 
            }

            //SUCCEEDED_GET = "204"
            else if ([[buf_array objectAtIndex:0] isEqualToString:@"204"]) {
            }

            //ERR_GET_FAILURE = "409"
            else if ([[buf_array objectAtIndex:0] isEqualToString:@"409"]) {
            }
        }

    }

    [cage_file readInBackgroundAndNotify];

    return;
}

- (void)_gpg_init
{
    gpg = [[GPGME alloc] initWithDir:[rc getPurityPath]];
    if (gpg == nil) {
        [self dealloc];
        @throw @"cant init gpg";
    } else {
        NSLog(@"setPasswd\n");
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
    kvtTask = [[NSTask alloc] init];
    if (path == nil) {
        kvtTaskPath = [[NSString alloc] initWithString:@"./cage"];
    } else {
        if ([self setTaskPath:path] == false) {
            [self dealloc];
            @throw @"non exist cage";
        }
    }
    [self _cage_run];
    if ([self isCageRun] == false) {
        [self dealloc];
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
        @throw @"cant connect to internal cage_sock";
    }

    bool is_join;
    is_join = [self _cage_join];
    if (is_join == false) {
        [self dealloc];
        @throw @"cant join to seed host";
    }

    NSLog(@"_cage_join\n");
    cage_file = [[NSFileHandle alloc] initWithFileDescriptor:cage_sock_fd];
    NSLog(@"_cage_mk_handler\n");
    [[NSNotificationCenter defaultCenter]
        addObserver:self
        selector:@selector(_cage_recv_handler:)
        name:NSFileHandleReadCompletionNotification
        object:cage_file
    ];
    NSLog(@"_cage_set_observer\n");

    // put on default_hostname for cage.
    NSString* key = [NSString stringWithFormat:@"%@.p2p", [rc getPrisonName]];
    NSLog(@"_cage_mk_key_message\n");
    NSString* value = [NSString stringWithFormat:@"%@.p2p:NULL:NULL", [ni defaultIP4]];
    NSLog(@"_cage_mk_value_message\n");
    //[gpg setPasswd:@"passhprase"];
    NSLog(@"%@\n", [gpg getPass]);
    NSString* sign = [gpg sign:value];
    NSString* value_sign = [GPGME trimContentFromArmor:sign];
    NSLog(@"_cage_mk_value_sign_message\n");
    [self _cage_send_message: [NSString stringWithFormat:
                        @"put,prison,%@,%@,%d,unique", key, value_sign, KVT_CAGE_TTL]];
    NSLog(@"_cage_pute\n");


    return;
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
        //perror("socket");
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
            //perror("connect");
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

- (bool)_cage_join
{
    id pool = [NSAutoreleasePool new];
    if (cage_sock_fd <= 0) {
        return false;
    }

    NSString* ip = [rc getSeedHost];
    NSString* port = [rc getSeedPort];
    NSString* message;
    message = [NSString stringWithFormat:@"join,prison,%@,%@", ip, port];

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
            ret = true;
        } else {
            ret = false;
        }
    }
    [pool drain];
    return ret;
}

- (bool)_cage_set_id
{
    id pool = [NSAutoreleasePool new];
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

    NSString* message = [NSString stringWithFormat:@"set_id,prison,%@", id_hash];

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
    [pool drain];
    return ret;
}

- (bool)_cage_node_new
{
    if (cage_sock_fd <= 0) {
        return false;
    }

    NSString* init_message;
    if (is_global([ni defaultIP4])) {
        init_message = [NSString 
                    stringWithFormat:@"new,prison,%@,global", [rc getExternal]];
    } else {
        init_message = [NSString
                    stringWithFormat:@"new,prison,%@", [rc getExternal]];
    }

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

- (bool)_cage_send_message:(NSString*)message
{
    NSLog(@"_cage_send_message\n");
    char buffer[65535];
    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer, [message UTF8String], [message length]);

    [kvtRequestQueue addObject:message];
    NSLog(@"_cage_send_message\n");

    int ret = true;
    ret = send(cage_sock_fd, buffer, (socklen_t)sizeof(buffer), 0);
    NSLog(@"_cage_raw_send\n");
    if (ret == -1) {
        ret = false;
    } else {
        ret = true;
    }

    return ret;
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



