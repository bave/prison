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

    NSFileHandle* cage_file;

    NSLock* kvtLock;
    NSMutableDictionary* kvtRequestQueue;
    NSMutableDictionary* kvtCageCache;
    NSMutableArray* kvtReputQueue;
    NSString* kvtOwnID;

    NSMutableDictionary* kvtLocalDB;

    GPGME* gpg;
}

// public function
- (id)init;
- (void)waitLock;
- (void)dealloc;
- (bool)setLocalDB:(NSDictionary*)db;
- (NSString*)ip4key:(NSString*)key;
- (NSString*)port4key:(NSString*)key;
- (NSString*)value4key:(NSString*)key;
- (int)authority4key:(NSString*)key;

- (NSDictionary*)getRequestQueue;
- (NSArray*)dequeueReput;
- (NSArray*)getReputQueue;


- (bool)isCageJoin;
- (bool)sendMessage:(NSString*)message;
- (bool)cage_join;
- (bool)cage_leave;

// private function
- (void)_debug_dict;

- (void)_gpg_init;
- (bool)_gpg_import_db:(NSString*)message;

- (void)_cage_init;
- (bool)_cage_set_id;
- (bool)_cage_connect;
- (bool)_cage_node_new;
- (bool)_cage_join;
- (bool)_cage_put;
- (bool)_cage_get_pubring:(NSString*)message;
- (int)_cage_set_cache:(NSString*)message;

- (void)_cage_recv_handler:(NSNotification*)notify;

- (bool)_dequeueRequest:(NSString*)message;
- (bool)_enqueueRequest:(NSString*)message;
- (bool)_changeAuthority:(NSString*)key :(int)auth;

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
    if (is_verbose)NSLog(@"%@",kvtLocalDB);
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
    /*
    if (ni == nil) {
        return nil;
    }

    if ([ni defaultIP4] == nil) {
        return nil;
    }
    */

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
        kvtOwnID = nil;
        id pool = [NSAutoreleasePool new];
        [self _gpg_init];
        [self _cage_init];
        [pool drain];
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
    close(cage_sock_fd);
    [cage_file closeFile];
    [cage_file release];

    [gpg release];
    [kvtLock release];
    [kvtRequestQueue release];
    [kvtReputQueue release];
    [kvtLocalDB release];
    [kvtCageCache release];
    if (kvtOwnID != nil)[kvtOwnID release];

    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [super dealloc];
    return;
}

- (bool)isCageJoin
{
    return kvt_is_join;
}

- (void)_cage_recv_handler:(NSNotification*)notify
{
    //id pool = [NSAutoreleasePool new];
    if (![[notify userInfo] objectForKey:@"NSFileHandleError"])
    {
        NSData *data;
        const NSStringEncoding* encode = [NSString availableStringEncodings];
        data = [[notify userInfo] objectForKey:NSFileHandleNotificationDataItem];

        NSString* buffer = [[NSString alloc] initWithData:data encoding:*encode];
        [buffer autorelease];

        NSArray* buffer_array;
        buffer_array = [buffer componentsSeparatedByString:@"\n"];
        [buffer_array autorelease];

        unsigned int i;
        for (i=0; i<[buffer_array count]; i++) {
            id element = [buffer_array objectAtIndex:i];
            if ([element length] == 0) { continue; }

            if ([self _dequeueRequest:element]) {

                if (is_verbose) NSLog(@"_cage_recv_handler:\n%@\n", element);
                NSArray* element_array = [element componentsSeparatedByString:@","];

                //SUCCEEDED_PUT = "203"
                if ([[element_array objectAtIndex:0] isEqualToString:@"203"]) { 
                    [kvtLock lock];
                    [kvtReputQueue addObject:[element substringFromIndex:4]];
                    [kvtLock unlock];
                }

                //SUCCEEDED_GET = "204"
                else if ([[element_array objectAtIndex:0] isEqualToString:@"204"]) {
                    int authority;
                    NSString* key = [element_array objectAtIndex:3];
                    if ([key hasSuffix:@".p2p"]) {
                        //TCHK_START(set_cache);
                        authority = [self _cage_set_cache:element];
                        //TCHK_END(set_cache);
                        if (authority == 0) {
                            [self _cage_get_pubring:element];
                        } else {
                            [[NSNotificationCenter defaultCenter]
                               postNotificationName:@"notify://obs.NameReply" object:element];
                        }
                    }
                    else if ([key hasSuffix:@"@prison"]) {
                        //TCHK_START(import);
                            [self _gpg_import_db:element];
                        //TCHK_END(import);
                            NSString* pubring = [element_array objectAtIndex:3];
                            NSString* hostname = [[pubring componentsSeparatedByString:@"@"] objectAtIndex:0];
                            NSString* dummy = [NSString stringWithFormat: @"204,get,prison,%@.p2p,non-value", hostname];

                            [[NSNotificationCenter defaultCenter]
                               postNotificationName:@"notify://obs.NameReply"
                               object:dummy];
                    }
                }

                //ERR_GET_FAILURE = "409"
                else if ([[element_array objectAtIndex:0] isEqualToString:@"409"]) {
                    [[NSNotificationCenter defaultCenter]
                        postNotificationName:@"notify://obs.NameReply" object:element];
                }

                // SUCCEEDED_DELETE = "201"
                else if ([[element_array objectAtIndex:0] isEqualToString:@"201"]) {
                }

                // join process
                else if ([[element_array objectAtIndex:1] isEqualToString:@"join"]) {
                    if ([[element_array objectAtIndex:0] isEqualToString:@"202"]) {
                        kvt_counter_join = 0;
                        [self _cage_put];
                        kvt_is_join = true;
                    }
                    else if ([[element_array objectAtIndex:0] isEqualToString:@"400"]) {
                        kvt_is_join = false;
                    }
                    else if ([[element_array objectAtIndex:0] isEqualToString:@"401"]) {
                        kvt_is_join = false;
                    }
                    else if ([[element_array objectAtIndex:0] isEqualToString:@"406"]) {
                        kvt_is_join = false;
                    }
                    else if ([[element_array objectAtIndex:0] isEqualToString:@"405"]) {
                        kvt_is_join = false;
                    } 

                    if (kvt_is_join == false) {
                        kvt_counter_join++;
                        if (kvt_counter_join <= KVT_CAGE_RETRY_JOIN) {
                            [self cage_join];
                        } else {
                            [self cage_leave];
                        }
                    }
                }
            }
        }
    }
    [cage_file readInBackgroundAndNotify];
    //[pool drain];

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

- (bool)_gpg_import_db:(NSString*)message
{
    NSString* key = nil;
    @try {
    NSArray* message_array = [message componentsSeparatedByString:@","];

    //NSString* code = [message_array objectAtIndex:0];
    //NSString* command = [message_array objectAtIndex:1];
    //NSString* node_name = [message_array objectAtIndex:2];
    NSString* dht_key = [message_array objectAtIndex:3];
    NSString* content = [GPGME appendPublicFrame:[message_array objectAtIndex:4]];

    NSString* hostname = [[dht_key componentsSeparatedByString:@"@"] objectAtIndex:0];
    key = [NSString stringWithFormat:@"%@.p2p", hostname];

    NSDictionary* pubring = [[gpg throw:content] objectAtIndex:0];
    NSString* throw_user = [pubring objectForKey:@"user"];
    NSArray* userlist = [gpg userlist];

    NSEnumerator* user_enum = [userlist objectEnumerator];
    ITERATE(user_element, user_enum) {
        NSString* user = [user_element objectForKey:@"user"];
        if ([user isEqualToString:throw_user]) {
            // already import same name user pubring... in trustdb
            [self _changeAuthority:key :0];
            return false;
        }
    }

    [gpg import:content];
    }
    @catch (id err) {
        NSLog(@"%@", err);
    }

    return [self _changeAuthority:key :1];
}

- (bool)_changeAuthority:(NSString*)key :(int)auth
{
    NSMutableArray* tmp_array = [[NSMutableArray new] autorelease];

    NSString* value;
    value = [kvtCageCache objectForKey:key];
    if (value == nil) return false; 

    NSArray* value_array;
    value_array = [value componentsSeparatedByString:@":"];

    unsigned int i;
    for (i=0; i<([value_array count]-1); i++) {
        [tmp_array addObject:[value_array objectAtIndex:i]];
    }
    [tmp_array addObject:[NSString stringWithFormat:@"%d", auth]];
    value = [tmp_array componentsJoinedByString:@":"];

    [kvtCageCache setObject:value forKey:key];

    return true;
}

- (void)_cage_init
{
    bool is_connect;
    is_connect = [self _cage_connect];
    if (is_connect == false) {
        [self dealloc];
        @throw @"cant connect to internal cage_sock";
    }

    if (is_linking) {
        kvt_counter_join = 0;
        kvt_is_join = [self _cage_join];
    } else {
        [self cage_leave];
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
        [self cage_leave];
    }

    return;
}

- (bool)_cage_get_pubring:(NSString*)message
{
    NSArray* message_array = [message componentsSeparatedByString:@","];
    NSString* key = [message_array objectAtIndex:3];
    NSArray* key_array = [key componentsSeparatedByString:@"."];
    NSString* key_hostname = [key_array objectAtIndex:0];
    NSString* key_request = [NSString stringWithFormat:@"%@@prison", key_hostname];
    return [self sendMessage: [NSString stringWithFormat:@"get,prison,%@\n", key_request]];
}

- (bool)_cage_put
{
    [niLock lock];
    NSString* ip = [ni defaultIP4];
    [niLock unlock];

    if (!is_global(ip)) {
        // XXX
        // if own ip address is private network address....
        return false;
    }

    // いまはまだデータベースからなにかをぷっとしていない
    // なので、いまからここにサービスネームとTransNatName を
    // putting するコードを書く!

    // prison name
    NSString* user = [rc getPrisonName];
    NSString* key = [NSString stringWithFormat:@"%@.p2p", user];
    NSString* value = [NSString stringWithFormat:@"%@:NULL:NULL", ip];
    NSString* sign = [gpg sign:value];
    NSString* value_sign = [GPGME trimContentFromArmor:sign];
    [self sendMessage:[NSString stringWithFormat:
                @"put,prison,%@,%@,%d,unique\n", key, value_sign, KVT_CAGE_TTL]];

    // prison gpg key
    key = [NSString stringWithFormat:@"%@@prison", user];
    value = [GPGME trimContentFromArmor:[gpg exportPubring:key]];
    [self sendMessage:[NSString stringWithFormat:
                @"put,prison,%@,%@,%d,unique\n", key, value, KVT_CAGE_TTL]];

    // prison socket 
    key = [NSString stringWithFormat:@"%@@ps", user];
    [self sendMessage:[NSString stringWithFormat:
                @"put,prison,%@,%@,%d,unique\n", key, kvtOwnID, KVT_CAGE_TTL]];


    return true;
}

- (bool)_cage_connect
{
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
        } else {
            err = -1;
        }
    }

    if (err == 0) {
        err = [self _cage_set_id];
        if (err == true) {
            if (is_verbose) NSLog(@"_cage_set_id: YES");
            err = 0;
        } else {
            if (is_verbose) NSLog(@"_cage_set_id: NO");
            err = -1;
        }
    }

    if (err == 0) {
        return true;
    } else {
        close(cage_sock_fd);
        return false;
    }
}

- (bool)cage_join
{
    if (is_verbose) NSLog(@"cage_join");
    NSString* ip = [rc getSeedHost];
    NSString* port = [rc getSeedPort];
    NSString* message;
    message = [NSString stringWithFormat:@"join,prison,%@,%@\n", ip, port];

    int ret;
    ret = [self sendMessage:message];

    return ret;
}

- (bool)cage_leave
{
    if (is_verbose) NSLog(@"cage_leave");
    [kvtLock lock];
    kvt_is_join = false;
    [kvtRequestQueue removeAllObjects];
    [kvtCageCache removeAllObjects];
    [kvtReputQueue removeAllObjects];
    [kvtLock unlock];
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

    // set_own_id
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

    // get_own_id
    if (ret == true) {
        buf_string = [self _commit_message:@"get_id,prison\n"];
        if (buf_string == nil) {
            ret = false;
        } else {
            ret = true;
        }
    }

    if (ret == true) {
        NSArray* buf_array = [buf_string componentsSeparatedByString:@","];

        //SUCCEEDED_GET_ID = "208";
        if ([[buf_array objectAtIndex:0] isEqualToString:@"208"]) {
            kvtOwnID = [[[buf_array objectAtIndex:3] trim]copy];
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

- (int)_cage_set_cache:(NSString*)message
{
    int authority = 0;

    // unique
    NSArray* message_array = [message componentsSeparatedByString:@","];

    //NSString* code = [message_array objectAtIndex:0];
    //NSString* command = [message_array objectAtIndex:1];
    //NSString* node_name = [message_array objectAtIndex:2];
    NSString* key = [message_array objectAtIndex:3];
    NSString* content = [GPGME appendMessageFrame:[message_array objectAtIndex:4]];

    [kvtLock lock];
    NSString* message_verify = [gpg verify:content];
    int valid = [gpg getValid];
    int trust = [gpg getTrust];
    authority = valid + trust;
    NSString* value = [NSString stringWithFormat:@"%@:%d", message_verify, authority];
    [kvtCageCache setObject:value forKey:key];
    if (is_verbose) {
        NSLog(@"kvt_func:_cage_set_cache\n");
        NSLog(@"key:%@\n", key);
        NSLog(@"value:%@\n", value);
        NSLog(@"authority:%d\n", authority);
    }
    [kvtLock unlock];
    
    return authority;
}

- (int)authority4key:(NSString*)key
{
    NSString* value;

    value = [kvtLocalDB valueForKey:key];
    if (value != nil) {
        return -1;
    }

    value = [kvtCageCache objectForKey:key];
    NSArray* value_array = [value componentsSeparatedByString:@":"];
    NSString* authority_string = [value_array lastObject];
    int authority = [authority_string intValue];

    return authority;
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

        if (s_retval <= 0) {
            if (s_retval == -1) perror("select");
            if (s_retval == 0) fprintf(stderr, "timeout! fd : %d \n", cage_sock_fd);
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
    NSString* string;

    if ([command isEqualToString:@"put"]) { 
        NSString* key = [mesg_array objectAtIndex:3];
        string = [NSString stringWithFormat:@"%@,%@", command, key];
    }
    else if ([command isEqualToString:@"get"]) {
        NSString* key = [mesg_array objectAtIndex:3];
        string = [NSString stringWithFormat:@"%@,%@", command, key];
    }
    else if ([command isEqualToString:@"join"]) {
        string = [NSString stringWithString:@"join"];
    }
    else {
        return false;
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

    NSString* string = nil;
    NSString* command = [mesg_array objectAtIndex:0];
    if ([command isEqualToString:@"put"]) {
        NSString* key = [mesg_array objectAtIndex:2];
        string = [NSString stringWithFormat:@"%@,%@", command, key];
    }
    else if ([command isEqualToString:@"get"]) {
        NSString* key = [mesg_array objectAtIndex:2];
        string = [NSString stringWithFormat:@"%@,%@", command, key];
    }
    else if ([command isEqualToString:@"join"]) {
        string = [NSString stringWithString:@"join"];
    }

    [kvtRequestQueue setObject:message forKey:string];
    return true;
}


- (bool)sendMessage:(NSString*)message
{
    int ret;
    NSString* queue_buffer = nil;
    NSString* send_buffer = nil;

    if ([message hasSuffix:@"\n"]) {
        NSRange range = NSMakeRange(0 ,[message length]-1);
        queue_buffer = [NSString stringWithString:[message substringWithRange:range]];
        send_buffer = message;
    } else {
        queue_buffer = message;
        send_buffer = [NSString stringWithFormat:@"%@\n", message];
    }

    ret = [self _enqueueRequest:queue_buffer];

    if (is_verbose) NSLog(@"sendMessage:\n%@\n", queue_buffer);

    if (ret == true) {
        ret = send(cage_sock_fd, [send_buffer UTF8String], [send_buffer length], 0);
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



