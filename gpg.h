#ifndef __RAPRINS_GPGME_H_
#define __RAPRINS_GPGME_H_

/*
 * todo
 * - mk sign-key
 * - mk delkey
 * - mk revoke
 * - mk templary import
 * - remake import return only ture...
 * - this class is have to reconstruct signleton.
 * - all method reconstruct mutex lock
 */

#import <Cocoa/Cocoa.h>

#include <string.h>
#include <locale.h>

#include <gpgme.h>
#include <gpg-error.h>

#ifdef __linux__
#ifndef true
#define true  1
#endif
#ifndef false
#define false 0
#endif
#include <sys/stat.h>
#endif

#ifdef __MACH__
#endif

// debug line
//NSLog(@"debug_%d\n", __LINE__);
//NSLog(@"%s\n", gpg_strerror(gpgErr));
//NSLog(@"%d:%s\n", __LINE__, gpg_strerror(gpgErr));


@interface GPGME : NSObject
{
    int gpgValid;
    int gpgTrust;
    NSString* gpgDir;
    NSString* gpgExe;
    NSString* gpgVer;
    NSString* gpgPasswd;

    NSMutableDictionary* gpgDict;

    gpgme_engine_info_t gpgInfo;
    gpgme_error_t gpgErr;

    NSLock* gpgLock;
}


// public function

- (id)init;
- (id)initWithDir:(NSString*)dir;
- (void)dealloc;

- (int)getValid;
- (int)getTrust;

- (NSString*)getDir;
- (NSString*)getExe;
- (NSString*)getVer;
- (NSString*)getPass;

- (BOOL)hasSecring;
- (BOOL)hasPubring;

- (void)setPasswd:(NSString*)passwd;
- (void)mkKeyParams:(NSString*)key
                   :(NSString*)keylen
                   :(NSString*)sub
                   :(NSString*)sublen
                   :(NSString*)user
                   :(NSString*)mail;

- (int)genkey;
- (int)delkey:(NSString*)user;

- (int)import:(NSString*)key;
- (NSString*)export:(NSString*)uid;
- (NSArray*)throw:(NSString*)key;
- (NSArray*)userlist;
- (NSArray*)ownerlist;

- (NSString*)sign:(NSString*)txt;
- (NSString*)verify:(NSString*)sig;

- (NSString*)decrypt:(NSString*)sig;
- (NSString*)encrypt:(NSString*)txt :(NSString*)uid;
- (NSString*)encryptForce:(NSString*)txt :(NSString*)uid;

// not implementation
- (int)genrkey;
- (int)_mk_directory:(NSString*)dir;
- (int)_rm_directory:(NSString*)dir;


// private function
- (void)_print_sig_summary:(gpgme_sigsum_t)summary;
- (void)_pinrt_sig_status:(gpgme_error_t)status;
- (void)_print_data:(gpgme_data_t)data;

- (int)_is_trust:(gpgme_verify_result_t)result;
- (int)_is_valid:(gpgme_verify_result_t)result;

- (NSData*)_data_to_nsdata:(gpgme_data_t)data;


- (NSArray*)_parse_list_pub:(NSString*)pub;
- (NSArray*)_parse_list_sec:(NSString*)sec;
- (NSArray*)_parse_list_throw:(NSString*)key;

@end


@implementation GPGME

// gpgme callback function ------------------------------------------------------
static gpgme_error_t _passwd_cb(void* object,
                                const char* uid_hint,
                                const char* passphrase_info,
                                int prev_was_bad,
                                int fd)
{
     id pool = [NSAutoreleasePool new];
    GPGME* gpg = (GPGME*)object;

    //NSLog(@"passphrase:%@\n", [(GPGME*)object getPass]);
    NSString* passwd = [NSString stringWithFormat:@"%@\n", [gpg getPass]];

    if (passwd == nil) return GPG_ERR_BAD_PASSPHRASE;

    write (fd, [passwd UTF8String], [passwd length]);

    [pool drain];

    return GPG_ERR_NO_ERROR;
}
// ------------------------------------------------------------------------------

// public function

- (int)getValid
{
    return gpgValid;
}

- (int)getTrust
{
    return gpgTrust;
}

- (NSString*)getDir
{
    return gpgDir;
}

- (NSString*)getExe
{
    return gpgExe;
}

- (NSString*)getVer
{
    return gpgVer;
}

- (NSString*)getPass
{
    return gpgPasswd;
}

- (void)setPasswd:(NSString*)passwd
{
    if (gpgPasswd != nil) [gpgPasswd release];
    gpgPasswd = [[NSString alloc] initWithString:passwd];
}

- (NSArray*)ownerlist
{
    id pool = [NSAutoreleasePool new];
    id saved_err = nil;

    //NSPipe* in_pipe = nil;;
    //NSFileHandle* input = nil;
    NSPipe* out_pipe = nil;
    NSFileHandle* output = nil;
    NSFileHandle* devNull = nil;

    NSTask* task = nil;
    NSData* out_data = nil;
    NSString* out_string = nil;

    int ret;
    const NSStringEncoding* encode;

    @try{

        NSMutableArray* args;
        args = [NSMutableArray array];
        [args addObject:@"--homedir"];
        [args addObject:gpgDir];
        [args addObject:@"--list-secret-keys"];

        /*
        // input pipe
        in_pipe = [NSPipe pipe];
        input = [in_pipe fileHandleForWriting];
        */

        // output pipe
        out_pipe = [NSPipe pipe];
        output = [out_pipe fileHandleForReading];

        // error pipe
        devNull = [NSFileHandle fileHandleForWritingAtPath:@"/dev/null"];

        task = [[NSTask alloc] init];
        [task setLaunchPath:gpgExe];
        //[task setStandardInput:in_pipe];
        [task setStandardOutput:out_pipe];
        [task setStandardError:devNull];
        [task setArguments:args];
        [task launch];

        [task waitUntilExit];
        ret = [task terminationStatus];

        if (ret != 0) {
            @throw @"violation error";
        }

        // stdout
        encode = [NSString availableStringEncodings];
        out_data = [output readDataToEndOfFile];
        out_string = [[NSString alloc] initWithData:out_data encoding:*encode];

        if ([out_string length] == 0) {
            @throw @"no data";
        }
    }

    @catch (NSString* err) {
        NSString* err_string = [NSString stringWithFormat:
                                @"error: [gpg ownerlist] %@", err];
        saved_err = [err retain];
        @throw err_string;
    }

    @catch (id err) {
        saved_err = [err retain];
        @throw err;
    }
    @finally {
        [output closeFile];
        [devNull closeFile];
        [task release];
        [pool drain];
        [saved_err autorelease];
    }

    [out_string autorelease];
    //NSLog(@"%@\n", out_string);

    return [self _parse_list_sec:out_string];
}

- (NSArray*)userlist
{
    id pool = [NSAutoreleasePool new];
    id saved_err = nil;

    //NSPipe* in_pipe = nil;;
    //NSFileHandle* input = nil;
    NSPipe* out_pipe = nil;
    NSFileHandle* output = nil;
    NSFileHandle* devNull = nil;

    NSTask* task = nil;
    NSData* out_data = nil;
    NSString* out_string = nil;

    int ret;
    const NSStringEncoding* encode;

    @try{

        NSMutableArray* args;
        args = [NSMutableArray array];
        [args addObject:@"--homedir"];
        [args addObject:gpgDir];
        [args addObject:@"--list-public-key"];

        /*
        // input pipe
        in_pipe = [NSPipe pipe];
        input = [in_pipe fileHandleForWriting];
        */

        // output pipe
        out_pipe = [NSPipe pipe];
        output = [out_pipe fileHandleForReading];

        // error pipe
        devNull = [NSFileHandle fileHandleForWritingAtPath:@"/dev/null"];

        task = [[NSTask alloc] init];
        [task setLaunchPath:gpgExe];
        //[task setStandardInput:in_pipe];
        [task setStandardOutput:out_pipe];
        [task setStandardError:devNull];
        [task setArguments:args];
        [task launch];

        [task waitUntilExit];
        ret = [task terminationStatus];

        if (ret != 0) {
            @throw @"violation error";
        }

        // stdout
        encode = [NSString availableStringEncodings];
        out_data = [output readDataToEndOfFile];
        out_string = [[NSString alloc] initWithData:out_data encoding:*encode];

        if ([out_string length] == 0) {
            @throw @"no data";
        }
    }

    @catch (NSString* err) {
        NSString* err_string = [NSString stringWithFormat:
                                @"error: [gpg userlist] %@", err];
        saved_err = [err retain];
        @throw err_string;
    }

    @catch (id err) {
        saved_err = [err retain];
        @throw err;
    }
    @finally {
        [output closeFile];
        [devNull closeFile];
        [task release];
        [pool drain];
        [saved_err autorelease];
    }

    [out_string autorelease];
    //NSLog(@"%@\n", out_string);

    return [self _parse_list_pub:out_string];
}

- (NSString*)encrypt:(NSString*)txt :(NSString*)uid
{
    if (txt == nil) {
        @throw @"error:[gpg encrypt] nonexistent txt";
    }

    if ([txt length] == 0) {
        @throw @"error:[gpg encrypt] nonexistent txt";
    }

    if (uid == nil) {
        @throw @"error:[gpg encrypt] nonexistent uid";
    }

    if ([uid length] == 0) {
        @throw @"error:[gpg encrypt] nonexistent uid";
    }

    // ----------------------------------------
    // if have a uid list of user trustdb....
    // ----------------------------------------

    id pool = [NSAutoreleasePool new];

    NSMutableArray* args;
    args = [ NSMutableArray array];
    [args addObject:@"--homedir"];
    [args addObject:gpgDir];
    [args addObject:@"--batch"];
    [args addObject:@"--armor"];
    [args addObject:@"-r"];
    [args addObject:uid];
    [args addObject:@"--encrypt"];

    // input pipe
    NSPipe* in_pipe;
    in_pipe = [NSPipe pipe];
    NSFileHandle* input;
    input = [in_pipe fileHandleForWriting];

    // output pipe
    NSPipe* out_pipe;
    out_pipe = [NSPipe pipe];
    NSFileHandle* output;
    output = [out_pipe fileHandleForReading];

    // error pipe
    NSFileHandle* devNull;
    devNull = [NSFileHandle fileHandleForWritingAtPath:@"/dev/null"];

    // task
    NSTask* task;
    task = [[NSTask alloc] init];
    [task setLaunchPath:gpgExe];
    [task setStandardInput:in_pipe];
    [task setStandardOutput:out_pipe];
    [task setStandardError:devNull];
    [task setArguments:args];
    [task launch];

    // encoding environment value
    const NSStringEncoding* encode;
    encode = [NSString availableStringEncodings];

    // stdin processing
    NSString* in_string = [NSString stringWithString:txt];
    NSData* in_data;
    in_data = [in_string dataUsingEncoding:*encode];
    [input writeData:in_data];
    [input closeFile];

    // stdout processing
    NSData* out_data;
    out_data = [output readDataToEndOfFile];
    NSString* out_string;
    out_string = [[NSString alloc] initWithData:out_data encoding:*encode];

    int ret;
    [task waitUntilExit];

    // closefile
    [output closeFile];
    [devNull closeFile];

    ret = [task terminationStatus];
    //NSLog(@"ret:%d", ret);

    if (ret != 0) {
        [task release];
        [pool drain];
        @throw @"error:[gpg encrypt] violation error";
    }


    [task release];
    [pool drain];

    [out_string autorelease];

    return out_string;
}

- (NSString*)encryptForce:(NSString*)txt :(NSString*)uid
{
    if (txt == nil) {
        @throw @"error:[gpg encrypt] nonexistent txt";
    }

    if ([txt length] == 0) {
        @throw @"error:[gpg encrypt] nonexistent txt";
    }

    if (uid == nil) {
        @throw @"error:[gpg encrypt] nonexistent uid";
    }

    if ([uid length] == 0) {
        @throw @"error:[gpg encrypt] nonexistent uid";
    }

    // ----------------------------------------
    // if have a uid list of user trustdb....
    // ----------------------------------------

    id pool = [NSAutoreleasePool new];

    NSMutableArray* args;
    args = [ NSMutableArray array];
    [args addObject:@"--homedir"];
    [args addObject:gpgDir];
    [args addObject:@"--batch"];
    [args addObject:@"--always-trust"];
    [args addObject:@"--armor"];
    [args addObject:@"-r"];
    [args addObject:uid];
    [args addObject:@"--encrypt"];

    // input pipe
    NSPipe* in_pipe;
    in_pipe = [NSPipe pipe];
    NSFileHandle* input;
    input = [in_pipe fileHandleForWriting];

    // output pipe
    NSPipe* out_pipe;
    out_pipe = [NSPipe pipe];
    NSFileHandle* output;
    output = [out_pipe fileHandleForReading];

    // error pipe
    NSFileHandle* devNull;
    devNull = [NSFileHandle fileHandleForWritingAtPath:@"/dev/null"];

    // task
    NSTask* task;
    task = [[NSTask alloc] init];
    [task setLaunchPath:gpgExe];
    [task setStandardInput:in_pipe];
    [task setStandardOutput:out_pipe];
    [task setStandardError:devNull];
    [task setArguments:args];
    [task launch];

    // encoding environment value
    const NSStringEncoding* encode;
    encode = [NSString availableStringEncodings];

    // stdin processing
    NSString* in_string = [NSString stringWithString:txt];
    NSData* in_data;
    in_data = [in_string dataUsingEncoding:*encode];
    [input writeData:in_data];
    [input closeFile];

    // stdout processing
    NSData* out_data;
    out_data = [output readDataToEndOfFile];
    NSString* out_string;
    out_string = [[NSString alloc] initWithData:out_data encoding:*encode];

    int ret;
    [task waitUntilExit];

    // closefile
    [output closeFile];
    [devNull closeFile];

    ret = [task terminationStatus];
    //NSLog(@"ret:%d", ret);

    if (ret != 0) {
        [task release];
        [pool drain];
        @throw @"error:[gpg encrypt] violation error";
    }


    [task release];
    [pool drain];

    [out_string autorelease];

    return out_string;
}

- (NSString*)decrypt:(NSString*)sig
{
    id pool = [NSAutoreleasePool new];

    gpgme_ctx_t ctx;
    gpgErr = gpgme_new (&ctx);
    gpgErr = gpgme_set_protocol(ctx, GPGME_PROTOCOL_OpenPGP);
    gpgme_set_passphrase_cb(ctx, _passwd_cb, self);

    gpgme_data_t  in_data;
    gpgme_data_new_from_mem(&in_data, [sig UTF8String], [sig length], 0);

    gpgme_data_t out_data;
    gpgErr = gpgme_data_new(&out_data);

    gpgErr = gpgme_op_decrypt(ctx, in_data, out_data);
    if (gpgErr) {
        gpgme_data_release(in_data);
        gpgme_data_release(out_data);
        gpgme_release(ctx);
        [pool drain];

        if (GPG_ERR_NO_DATA == gpgme_err_code(gpgErr)) {
            @throw @"error: [gpg decrypt] not encrypted signature";
        } else {
            @throw @"error: [gpg decrypt] violation error";
        }
    }

    gpgme_decrypt_result_t result;
    result = gpgme_op_decrypt_result(ctx);
    if (result->unsupported_algorithm) {
        //NSLog(@"%d:%s\n", __LINE__, gpg_strerror(gpgErr));
        gpgme_data_release(in_data);
        gpgme_data_release(out_data);
        gpgme_release(ctx);
        [pool drain];
        @throw @"error: [gpg decrypt] unsupported algorithm";
    }

    //[self _print_data:out_data];

    NSData* out_nsdata;
    out_nsdata = [self _data_to_nsdata:out_data];
    if ([out_nsdata length] == 0) {
        //NSLog(@"%d:%s\n", __LINE__, gpg_strerror(gpgErr));
        gpgme_data_release(in_data);
        gpgme_data_release(out_data);
        gpgme_release(ctx);
        [pool drain];
        @throw @"error: [gpg decrypt] nonexistent sig data";
    }

    gpgme_data_release (in_data);
    gpgme_data_release (out_data);
    gpgme_release(ctx);

    const NSStringEncoding* encode;
    encode = [NSString availableStringEncodings];

    NSString* out_string;
    out_string = [[NSString alloc] initWithData:out_nsdata encoding:*encode];
    [pool drain];

    [out_string autorelease];

    return out_string;
}

- (NSString*)sign:(NSString*)txt
{
    id pool = [NSAutoreleasePool new];

    gpgme_ctx_t ctx;
    gpgErr = gpgme_new (&ctx);
    gpgErr = gpgme_set_protocol(ctx, GPGME_PROTOCOL_OpenPGP);
    gpgme_set_passphrase_cb(ctx, _passwd_cb, self);
    gpgme_set_armor(ctx, 1);
    gpgme_set_textmode (ctx, 1);

    gpgme_data_t  in_data;
    gpgme_data_new_from_mem(&in_data, [txt UTF8String], [txt length], 0);

    gpgme_data_t out_data;
    gpgErr = gpgme_data_new(&out_data);

    gpgErr = gpgme_op_sign(ctx, in_data, out_data, GPGME_SIG_MODE_NORMAL);
    //gpgErr = gpgme_op_sign(ctx, in, out, GPGME_SIG_MODE_DETACH);
    //gpgErr = gpgme_op_sign(ctx, in, out, GPGME_SIG_MODE_CLEAR);
    if (gpgErr) {
        gpgme_data_release(in_data);
        gpgme_data_release(out_data);
        gpgme_release(ctx);
        [pool drain];
        @throw @"error: [gpg sign] violation error";
    }

    //gpgme_sign_result_t result;
    //result = gpgme_op_sign_result (ctx);
    
    //[self _print_data:out_data];

    NSData* out_nsdata;
    out_nsdata = [self _data_to_nsdata:out_data];
    if ([out_nsdata length] == 0) {
        gpgme_data_release(in_data);
        gpgme_data_release(out_data);
        gpgme_release(ctx);
        [pool drain];
        @throw @"error: [gpg sign] nonexistent txt data";
    }
    gpgme_data_release(in_data);
    gpgme_data_release(out_data);
    gpgme_release(ctx);

    const NSStringEncoding* encode;
    encode = [NSString availableStringEncodings];

    NSString* out_string;
    out_string = [[NSString alloc] initWithData:out_nsdata encoding:*encode];
    [pool drain];

    [out_string autorelease];

    return out_string;
}

- (NSString*)export:(NSString*)uid
{
#ifdef __MACH__
    // batch mode code

    id pool = [NSAutoreleasePool new];

    NSMutableArray* args;
    args = [ NSMutableArray array];
    [args addObject:@"--homedir"];
    [args addObject:gpgDir];
    //NSLog(@"%@\n", gpgDir);
    [args addObject:@"--armor"];
    [args addObject:@"--export"];
    //[args addObject:@"--export-options"];
    //[args addObject:@"export-minimal"];
    if (uid != nil) {
        //NSLog(@"%@\n", uid);
        [args addObject:uid];
    }

    /*
    // input pipe
    NSPipe* in_pipe;
    in_pipe = [NSPipe pipe];
    NSFileHandle* input;
    input = [in_pipe fileHandleForWriting];
    */

    // output pipe
    NSPipe* out_pipe;
    out_pipe = [NSPipe pipe];
    NSFileHandle* output;
    output = [out_pipe fileHandleForReading];

    // error pipe
    NSFileHandle* devNull;
    devNull = [NSFileHandle fileHandleForWritingAtPath:@"/dev/null"];


    // task
    NSTask* task;
    task = [[NSTask alloc] init];
    [task setLaunchPath:gpgExe];
    [task setStandardOutput:out_pipe];
    [task setStandardError:devNull];
    [task setArguments:args];
    [task launch];

    // encoding environment value
    const NSStringEncoding* encode;
    encode = [NSString availableStringEncodings];

    int ret;
    [task waitUntilExit];
    ret = [task terminationStatus];
    //NSLog(@"ret:%d", ret);

    // stdout
    NSData* out_data;
    out_data = [output readDataToEndOfFile];
    NSString* out_string;
    out_string = [[NSString alloc] initWithData:out_data encoding:*encode];

    //NSLog(@"data_length:%d\n", [out_data length]);
    //NSLog(@"\n%@\n", out_data);

    //NSLog(@"string_length:%d\n", [out_string length]);
    //NSLog(@"\n%@\n", out_string);


    // closefile
    [output closeFile];
    [devNull closeFile];

    [task release];
    [pool drain];

    if ([out_string length] == 0) {
        @throw @"error: [gpg export] nonexistent user";
    }

    if (ret != 0) {
        @throw @"error: [gpg export] violation error";
    }

    [out_string autorelease];

    //NSLog(@"%@\n", out_string);

    return out_string;

#elif __linux__
    // gpgme mode code

    const char* uid_char;
    uid_char = [uid UTF8String];

    gpgme_ctx_t ctx;
    gpgErr = gpgme_new (&ctx);
    gpgErr = gpgme_set_protocol(ctx, GPGME_PROTOCOL_OpenPGP);
    gpgme_set_armor(ctx, 1);

    gpgme_data_t  out_data;
    gpgErr = gpgme_data_new(&out_data);

    gpgErr = gpgme_op_export(ctx, uid_char, 0, out_data);

    if (gpgErr) {
        gpgme_data_release(out_data);
        gpgme_release(ctx);
        @throw @"error: [gpg export] violation error";
    }

    //[self _print_data:out_data];

    NSData* out_nsdata;
    out_nsdata = [self _data_to_nsdata:out_data];

    if ([out_nsdata length] == 0) {
        gpgme_data_release(out_data);
        gpgme_release(ctx);
        @throw @"error: [gpg export] nonexistent user";
    }

    gpgme_data_release(out_data);
    gpgme_release(ctx);

    const NSStringEncoding* encode;
    encode = [NSString availableStringEncodings];

    NSString* out_string;
    out_string = [[NSString alloc] initWithData:out_nsdata encoding:*encode];
    [out_string autorelease];

    return out_string;

#endif
}

- (int)import:(NSString*)key
{

    /*
     *  now, this message return only ture.
     *  error is rising exception.
     */

#ifdef __MACH__
    // batch mode code

    if (key == nil) {
        @throw @"error:[gpg import] nonexistent key";
    }

    if ([key length] == 0) {
        @throw @"error:[gpg import] nonexistent key";
    }

    id pool = [NSAutoreleasePool new];

    NSMutableArray* args;
    args = [ NSMutableArray array];
    [args addObject:@"--homedir"];
    [args addObject:gpgDir];
    //NSLog(@"%@\n", gpgDir);
    [args addObject:@"--fast-import"];

    // input pipe
    NSPipe* in_pipe;
    in_pipe = [NSPipe pipe];
    NSFileHandle* input;
    input = [in_pipe fileHandleForWriting];

    // output pipe
    // error pipe
    NSFileHandle* devNull;
    devNull = [NSFileHandle fileHandleForWritingAtPath:@"/dev/null"];

    // task
    NSTask* task;
    task = [[NSTask alloc] init];
    [task setLaunchPath:gpgExe];
    [task setStandardInput:in_pipe];
    [task setStandardOutput:devNull];
    [task setStandardError:devNull];
    [task setArguments:args];
    [task launch];

    // encoding environment value
    const NSStringEncoding* encode;
    encode = [NSString availableStringEncodings];

    // stdin
    NSString* in_string = [NSString stringWithString:key];
    NSData* in_data;
    in_data = [in_string dataUsingEncoding:*encode];
    [input writeData:in_data];
    [input closeFile];

    int ret;
    [task waitUntilExit];

    // closefile
    [devNull closeFile];

    ret = [task terminationStatus];
    //NSLog(@"ret:%d", ret);

    if (ret != 0) {
        [task release];
        [pool drain];
        @throw @"error:[gpg import] violation error";
    }


    [task release];
    [pool drain];

    return true;     

#elif __linux__
    // gpgme mode code

    gpgme_ctx_t ctx;
    gpgErr = gpgme_new (&ctx);
    gpgErr = gpgme_set_protocol(ctx, GPGME_PROTOCOL_OpenPGP);
    gpgme_set_armor(ctx, 1);

    gpgme_data_t  in_data;
    gpgme_data_new_from_mem(&in_data, [key UTF8String], [key length], 0);


    gpgErr = gpgme_op_import(ctx, in_data);
    if (gpgErr) {
        gpgme_data_release(in_data);
        gpgme_release(ctx);
        @throw @"error:[gpg import] violation error";
    }

    gpgme_import_result_t result;
    result = gpgme_op_import_result(ctx);

    // if you want to understand,, more detail information ..
    // checking in result_check() at gpgme/tests/gpg/t-import.c
    if (result->imports->result != 0) {
        gpgme_data_release(in_data);
        gpgme_release(ctx);
        @throw @"error:[gpg import] violation error";
    }

    //printf("%d\n", result->imports->status );

    // Zero means the key was already known 
    // no new components have been added.
    if (result->imports->status == 0) {
        // status number is contained information ====
        // GPGME_IMPORT_NEW	1
        // GPGME_IMPORT_UID	2
        // GPGME_IMPORT_SIG	4
        // GPGME_IMPORT_SUBKEY	8
        // GPGME_IMPORT_SECRET	16
        // ==========================================
        gpgme_data_release(in_data);
        gpgme_release(ctx);
        @throw @"error:[gpg import] existent users";
    }

    // Number of per status 
    // result->** ,, ** is following ... 
    // considered;
    // no_user_id;
    // imported;
    // imported_rsa;
    // unchanged;
    // new_user_ids;
    // new_sub_keys;
    // new_signatures;
    // new_revocations;
    // secret_read;
    // secret_imported;
    // secret_unchanged;
    // skipped_new_keys;
    // not_imported;

    return true;

#endif
}

- (void)mkKeyParams:(NSString*)key
                   :(NSString*)keylen
                   :(NSString*)sub
                   :(NSString*)sublen
                   :(NSString*)user
                   :(NSString*)mail
{
    @try{
        [gpgDict setObject:key forKey:@"key"];
        [gpgDict setObject:sub forKey:@"sub"];
        [gpgDict setObject:user forKey:@"user"];
        [gpgDict setObject:mail forKey:@"mail"];
        [gpgDict setObject:keylen forKey:@"keylen"];
        [gpgDict setObject:sublen forKey:@"sublen"];
    }

    @catch (id e) {
        @throw @"error:[gpg mkKeyParams]";
    }

    return;
}

- (int)genkey
{
    id pool = [NSAutoreleasePool new];

    char buffer[512];
    memset(buffer, '\0', 512);

    /*
    const char key_type       = "RSA";
    const char key_length     = "2048";
    const char sub_key_type   = "RSA";
    const char sub_key_length = "2048";
    const char user_name      = "bave";
    const char mail_addr      = "bave@aris";
    */

    if ([gpgDict count] != 6) {
        @throw @"error:[gpg genkey]";
    }

    if (gpgPasswd == nil) {
        @throw @"error:[gpg genkey]";
    }

    const char* key;
    key = [[gpgDict objectForKey:@"key"] UTF8String];

    const char* keylen;
    keylen = [[gpgDict objectForKey:@"keylen"] UTF8String];

    const char* sub;
    sub = [[gpgDict objectForKey:@"sub"] UTF8String];

    const char* sublen;
    sublen = [[gpgDict objectForKey:@"sublen"] UTF8String];

    const char* user;
    user = [[gpgDict objectForKey:@"user"] UTF8String];

    const char* mail;
    mail = [[gpgDict objectForKey:@"mail"] UTF8String];


    const char* passwd;
    passwd = [gpgPasswd UTF8String];

    //char* comment;
    //char* expire;

#ifdef __MACH__

    @try{

    //NSString* pubring;
    //pubring = [NSString stringWithFormat:@"%@%@", gpgDir, @"pubring.gpg"];
    //pubring = [NSString stringWithString:@"pubring.gpg"];

    //NSString* secring;
    //secring = [NSString stringWithFormat:@"%@%@", gpgDir, @"secring.gpg"];
    //secring = [NSString stringWithString:@"secring.gpg"];

    sprintf(buffer,
            "Key-Type: %s\n"
            "Key-Length: %s\n"
            "Subkey-Type: %s\n"
            "Subkey-Length: %s\n"
            "Name-Real: %s\n"
            "Name-Email: %s\n"
            "Passphrase: %s\n"
            "Expire-Date: %s\n"
            "Name-Comment: %s\n"
            //"%%pubring %s\n"
            //"%%secring %s\n"
            "%%commit\n",
            key,
            keylen,
            sub,
            sublen,
            user,
            mail,
            passwd,
            "0",
            "RaprinsKey");
            //[pubring UTF8String],
            //[secring UTF8String]);

    //NSLog(@"%s\n", buffer);


    NSString* params;
    params = [NSString stringWithUTF8String:buffer];

    //NSLog(@"\n%@\n", params);

    NSMutableArray* args;
    args = [NSMutableArray array];
    if (gpgDir != nil) {
        [args addObject:@"--homedir"];
        [args addObject:gpgDir];
    }
    [args addObject:@"--batch"];
    [args addObject:@"--no-tty"];
    [args addObject:@"--gen-key"];

    NSPipe* pipe;
    pipe = [NSPipe pipe];

    NSFileHandle* file;
    file = [pipe fileHandleForWriting];

    NSFileHandle* devNull;
    devNull = [NSFileHandle fileHandleForWritingAtPath:@"/dev/null"];

    NSTask* task;
    task = [[NSTask alloc] init];
    /*
    if (gpgDir != nil) {
        [task setCurrentDirectoryPath:gpgDir];
    }
    */
    [task setLaunchPath:gpgExe];
    [task setStandardInput:pipe];
    [task setStandardOutput:devNull];
    [task setStandardError:devNull];
    [task setArguments:args];
    [task launch];

    const NSStringEncoding* encode;
    encode = [NSString availableStringEncodings];
    NSData* data;
    data = [params dataUsingEncoding:*encode];
    [file writeData:data];

    // close files
    [file closeFile];
    [devNull closeFile];

    /*
    while ([task isRunning]) {
        sleep(1);
        //NSLog(@"."); fflush(NULL);
    }
    */

    [task waitUntilExit];


    int ret;
    ret = [task terminationStatus];


    [task release];

    if (ret == 0) { return true;  }
    if (ret != 0) { return false; }

    }

    @catch (id err) {
        [pool drain];
        @throw @"error:[gpg genkey]";
    }

    [pool drain];

    return false;

#elif  __linux__

    sprintf(buffer,
            "<GnupgKeyParms format=\"internal\">\n"
            "Key-Type:      %s\n"
            "Key-Length:    %s\n"
            "Subkey-Type:   %s\n"
            "Subkey-Length: %s\n"
            "Name-Real:     %s\n"
            "Name-Email:    %s\n"
            "Passphrase:    %s\n"
            "</GnupgKeyParms>\n",
            key,
            keylen,
            sub,
            sublen,
            user,
            mail,
            passwd);
     //"Name-Comment: \n"
     //"Expire-Date: 0\n"


    gpgme_ctx_t ctx;
    gpgme_genkey_result_t result;

    //NSLog(@"\n%s", buffer);

    gpgErr = gpgme_new (&ctx);
    gpgErr = gpgme_set_protocol(ctx, GPGME_PROTOCOL_OpenPGP);
    if (gpgErr) {
        gpgme_release(ctx);
        [pool drain];
        @throw @"error:[gpg genkey] new_ctx";
    }
    gpgme_set_armor (ctx, 1);

    gpgErr = gpgme_op_genkey (ctx, buffer, NULL, NULL);
    if (gpgErr) {
        gpgme_release(ctx);
        [pool drain];
        @throw @"error:[gpg genkey] op_genkey";
    }

    result = gpgme_op_genkey_result(ctx);

    if (!result) {
        gpgme_release(ctx);
        [pool drain];
        return false;
    }

    if (!result->fpr) {
        gpgme_release(ctx);
        [pool drain];
        return false;
    }

    gpgme_release(ctx);

    [pool drain];
    return true;

#endif
}

- (BOOL)hasSecring;
{
    id pool = [NSAutoreleasePool new];

    NSFileManager* manager;
    manager = [NSFileManager defaultManager];

    NSString* secring;
    secring = [NSString stringWithFormat:@"%@%@", gpgDir, @"secring.gpg"];
    //NSLog(@"%@\n", secring);
    
    BOOL isExist;
    BOOL isDir;
    isExist = [manager fileExistsAtPath:secring isDirectory:&isDir];

    BOOL ret;

    if (isExist) {
        //exist
        if (isDir) {
            //directory
            ret = false;
        } else {
            //get filesize
            #ifdef __MACH__
                int filesize;
                NSDictionary* item;
                item = [manager attributesOfItemAtPath:secring error:NULL];
                filesize = [item fileSize];
            #elif __linux__
                int filesize;
                struct stat item;
                if (stat([secring UTF8String], &item) == 0) {
                    // success
                    filesize = item.st_size;
                } else {
                    // failure
                    filesize = 0;
                }
            #endif
            if (filesize != 0) {
                ret = true;
            } else {
                ret = false;
            }
        }
    }else{
        // nothing
        ret = false;
    }
    [pool drain];
    return ret;
}

- (BOOL)hasPubring;
{
    id pool = [NSAutoreleasePool new];

    NSFileManager* manager;
    manager = [NSFileManager defaultManager];

    NSString* pubring;
    pubring = [NSString stringWithFormat:@"%@%@", gpgDir, @"pubring.gpg"];
    //NSLog(@"%@\n", pubring);
    
    BOOL isExist;
    BOOL isDir;
    isExist = [manager fileExistsAtPath:pubring isDirectory:&isDir];

    BOOL ret;

    if (isExist) {
        //exist
        if (isDir) {
            //directory
            ret = false;
        } else {
            //get filesize
            #ifdef __MACH__
                int filesize;
                NSDictionary* item;
                item = [manager attributesOfItemAtPath:pubring error:NULL];
                filesize = [item fileSize];
            #elif __linux__
                int filesize;
                struct stat item;
                if (stat([pubring UTF8String], &item) == 0) {
                    // success
                    filesize = item.st_size;
                } else {
                    // failure
                    filesize = 0;
                }
            #endif
            if (filesize != 0) {
                ret = true;
            } else {
                ret = false;
            }
        }
    }else{
        // nothing
        ret = false;
    }
    [pool drain];
    return ret;
}

- (NSString*)verify:(NSString*)sig
{
    [gpgLock lock];
    int valid;
    int trust;
    gpgme_ctx_t ctx;
    gpgme_data_t in_data;
    gpgme_data_t out_data;
    gpgme_verify_result_t result;

    id pool = [NSAutoreleasePool new];

    if ([sig compare:@"No Data"] == NSOrderedSame) {
        [gpgLock unlock];
        [pool drain];
        @throw @"error: [gpg verity] nonexistent sigature file";
    }

    gpgme_new(&ctx);
    gpgme_set_protocol(ctx,GPGME_PROTOCOL_OpenPGP);
    gpgme_data_new_from_mem(&in_data, [sig UTF8String], [sig length], 0);
    gpgme_data_new(&out_data);
    gpgErr = gpgme_op_verify(ctx, in_data, NULL, out_data);

    if (gpgErr) {
        gpgme_data_release(in_data);
        gpgme_data_release(out_data);
        gpgme_release(ctx);
        [gpgLock unlock];
        [pool drain];
        @throw @"error: [gpg verify] violation error";
    }

    //printf("verify data\n");
    //[self _print_data:out_data];

    result = gpgme_op_verify_result(ctx); 
    valid = [self _is_valid:result];
    trust = [self _is_trust:result];
    gpgTrust = trust;
    gpgValid = valid;

    /*
    if (valid != 1) {
        gpgme_data_release(in_data);
        gpgme_data_release(out_data);
        gpgme_release(ctx);
        [pool drain];
        //return @"unValid"
        //@throw @"error: [gpg verity] no validation";
    }
    */

    NSData* out_nsdata = nil;
    out_nsdata = [self _data_to_nsdata:out_data];

    if ([out_nsdata length] == 0) {
        gpgme_data_release(in_data);
        gpgme_data_release(out_data);
        gpgme_release(ctx);
        [gpgLock unlock];
        [pool drain];
        @throw @"error: [gpg decrypt] nonexistent sig data";
    }

    gpgme_data_release(in_data);
    gpgme_data_release(out_data);
    gpgme_release(ctx);

    const NSStringEncoding* encode;
    encode = [NSString availableStringEncodings];
    NSString* out_string = nil;
    out_string = [[NSString alloc] initWithData:out_nsdata encoding:*encode];

    [gpgLock unlock];
    [pool drain];
    [out_string autorelease];

    return out_string;
}

- (NSArray*)throw:(NSString*)key
{
    id pool = [NSAutoreleasePool new];

    if (key == nil) {
        [pool drain];
        @throw @"error:[gpg throw] non key stream";
    }

    NSMutableArray* args = nil;
    args = [ NSMutableArray array];
    [args addObject:@"--homedir"];
    [args addObject:gpgDir];
    [args addObject:@"--throw-keyids"];

    // input pipe
    NSPipe* in_pipe;
    in_pipe = [NSPipe pipe];
    NSFileHandle* input;
    input = [in_pipe fileHandleForWriting];

    // output pipe
    NSPipe* out_pipe;
    out_pipe = [NSPipe pipe];
    NSFileHandle* output;
    output = [out_pipe fileHandleForReading];

    // error pipe
    NSFileHandle* devNull;
    devNull = [NSFileHandle fileHandleForWritingAtPath:@"/dev/null"];

    // task
    NSTask* task;
    task = [[NSTask alloc] init];
    [task setLaunchPath:gpgExe];
    [task setStandardInput:in_pipe];
    [task setStandardOutput:out_pipe];
    [task setStandardError:devNull];
    [task setArguments:args];
    [task launch];

    // encoding environment value
    const NSStringEncoding* encode;
    encode = [NSString availableStringEncodings];

    // stdin
    NSString* in_string = nil;
    in_string = key;
    NSData* in_data = nil;
    in_data = [in_string dataUsingEncoding:*encode];
    [input writeData:in_data];
    [input closeFile];

    int ret;
    [task waitUntilExit];
    ret = [task terminationStatus];
    //NSLog(@"ret:%d", ret);

    // stdout
    NSData* out_data = nil;
    out_data = [output readDataToEndOfFile];
    NSString* key_string = nil;
    key_string = [[NSString alloc] initWithData:out_data encoding:*encode];

    //NSLog(@"data_length:%d\n", [out_data length]);
    //NSLog(@"\n%@\n", out_data);

    //NSLog(@"string_length:%d\n", [out_string length]);
    //NSLog(@"\n%@\n", out_string);


    // closefile
    [output closeFile];
    [devNull closeFile];

    if ([key_string length] == 0) {
        [task release];
        [pool drain];
        @throw @"error:[gpg throw] no key data";
    }

    if (ret != 0) {
        [task release];
        [pool drain];
        @throw @"error: [gpg throw] violation error";
    }


    // nsstring split to nsarray -- from utils.h ---------------------------
    // NSArray* array_split(NSString* string, NSString* delimiter)
    // please ref [NSString componentsSeparatedByString:@""];

    /* format...
    pub  2048R/A470B63E 2010-06-20 hoge <hoge@gmail.com>
    sub  2048R/A4A7D860 2010-06-20
    pub  2048R/85D0E976 2010-06-17 Hage Hage <hagehage@gmail.com>
    sub  2048R/C5A5F02F 2010-06-17
    */

    /*
    NSMutableArray* array = nil;
    NSCharacterSet* chSet = nil;
    NSScanner* scanner = nil;
    NSString* token = nil;

    chSet = [NSCharacterSet characterSetWithCharactersInString:@"\n"];
    array = [[[NSMutableArray alloc] init] autorelease];
    scanner = [NSScanner scannerWithString:out_string];

    NSComparisonResult compResult;
    while(![scanner isAtEnd]) {
        if([scanner scanUpToCharactersFromSet:chSet intoString:&token]) {
            compResult = [token compare:@"sub"
                                options:NSCaseInsensitiveSearch
                                  range:NSMakeRange(0,[@"sub" length])];
            //NSLog(@"%d\n", compResult);
            if (compResult != NSOrderedSame) {
                [array addObject:token];
            }
        }
    }
    */
    //NSLog(@"%@\n", array);
    // ---------------------------------------------------------------------

    [task release];
    [pool drain];
    [key_string autorelease];

    return [self _parse_list_throw:key_string];
}

- (id)init
{

    self = [super init];

    if (self != nil) {
        // --------------
        // initial coding
        BOOL isDir;
        BOOL isExist;

        gpgLock = [NSLock new];

        setlocale(LC_ALL, "");
        gpgme_check_version(NULL);
        gpgme_engine_check_version(GPGME_PROTOCOL_OpenPGP);

        gpgErr = gpgme_set_locale(NULL, LC_CTYPE, setlocale (LC_CTYPE, NULL));
        if (gpgErr) @throw @"error:[gpg init]";

        gpgErr = gpgme_get_engine_info(&gpgInfo);
        if (gpgErr) @throw @"error:[gpg init]";

        if (gpgInfo->home_dir == NULL) {
            gpgDir = [[NSString alloc]
                    initWithFormat:@"%@%@", NSHomeDirectory(), @"/.gnupg/"];
            isExist = [[NSFileManager defaultManager]
                    fileExistsAtPath:gpgDir isDirectory:&isDir];
            if ((isExist && isDir) == false)
                @throw @"error:[gpg init]: nothing home dir";
        }
        else {
            gpgDir = [[NSString alloc]
                    initWithUTF8String:gpgInfo->home_dir];

            isExist = [[NSFileManager defaultManager]
                    fileExistsAtPath:gpgDir isDirectory:&isDir];

            if ((isExist && isDir) == false)
                @throw @"error:[gpg init]: nothing home dir";
        }

        if (gpgInfo->file_name == NULL) {
            gpgExe = nil;
        }
        else {
            gpgExe = [[NSString alloc] initWithUTF8String:gpgInfo->file_name];
        }

        if (gpgInfo->version == NULL) {
            gpgVer = nil;
        }
        else {
            gpgVer = [[NSString alloc] initWithUTF8String:gpgInfo->version];
        }

        gpgDict = [NSMutableDictionary new];

    }
    else {
        @throw @"error:[gpg init]";
    }

    return self;
}

- (id)initWithDir:(NSString*)dir
{
    self = [super init];

    if (self != nil) {
        // --------------
        // initial coding
        // --------------
        BOOL isDir;
        BOOL isExist;

        gpgLock = [NSLock new];

        setlocale(LC_ALL, "");
        gpgme_check_version(NULL);
        gpgme_engine_check_version(GPGME_PROTOCOL_OpenPGP);

        gpgErr = gpgme_set_locale(NULL, LC_CTYPE, setlocale (LC_CTYPE, NULL));
        if (gpgErr) @throw @"error:[gpg initWithDir]";

        gpgErr = gpgme_get_engine_info(&gpgInfo);
        if (gpgErr) @throw @"error:[gpg initWithDir]";

        if (![dir hasSuffix:@"/"]) {
            dir = [NSString stringWithFormat:@"%@%@", dir, @"/"];
        }
        gpgErr = gpgme_set_engine_info(gpgInfo->protocol,
                                       gpgInfo->file_name,
                                       [dir UTF8String]);
        if (gpgErr) @throw @"error:[gpg initWithDir]";

        gpgErr = gpgme_get_engine_info(&gpgInfo);
        if (gpgErr) @throw @"error:[gpg initWithDir]";

        if (gpgInfo->home_dir == NULL) {
            gpgDir = [[NSString alloc]
                        initWithFormat:@"%@%@", NSHomeDirectory(), @"/.gnupg/"];

            isExist = [[NSFileManager defaultManager]
                        fileExistsAtPath:gpgDir isDirectory:&isDir];

            if ((isExist && isDir) == false)
                @throw @"error:[gpg init]: nothing home dir";
        }
        else {
            gpgDir = [[NSString alloc] initWithUTF8String:gpgInfo->home_dir];

            isExist = [[NSFileManager defaultManager]
                        fileExistsAtPath:gpgDir isDirectory:&isDir];
            if ((isExist && isDir) == false)
                @throw @"error:[gpg init]: nothing home dir";
        }

        if (gpgInfo->file_name == NULL) {
            gpgExe = nil;
        }
        else {
            gpgExe = [[NSString alloc] initWithUTF8String:gpgInfo->file_name];
        }

        if (gpgInfo->version == NULL) {
            gpgVer = nil;
        }
        else {
            gpgVer = [[NSString alloc] initWithUTF8String:gpgInfo->version];
        }

        gpgDict = [NSMutableDictionary new];

    }
    else {
        @throw @"error:[gpg initWithDir:]";
    }

    return self;
}

- (void)dealloc
{
    // --------------
    // release coding
    // --------------

    if (gpgDir != nil)  [gpgDir release];
    if (gpgExe != nil)  [gpgExe release];
    if (gpgVer != nil)  [gpgVer release];
    if (gpgPasswd != nil) [gpgPasswd release];

    [gpgLock release];

    [super dealloc];
    return;
}


// private function

- (void)_print_sig_summary:(gpgme_sigsum_t)summary
{
    if (summary & GPGME_SIGSUM_VALID      ) fputs (" valid", stdout);
    if (summary & GPGME_SIGSUM_GREEN      ) fputs (" green", stdout);
    if (summary & GPGME_SIGSUM_RED        ) fputs (" red", stdout);
    if (summary & GPGME_SIGSUM_KEY_REVOKED) fputs (" revoked", stdout);
    if (summary & GPGME_SIGSUM_KEY_EXPIRED) fputs (" key-expired", stdout);
    if (summary & GPGME_SIGSUM_SIG_EXPIRED) fputs (" sig-expired", stdout);
    if (summary & GPGME_SIGSUM_KEY_MISSING) fputs (" key-missing", stdout);
    if (summary & GPGME_SIGSUM_CRL_MISSING) fputs (" crl-missing", stdout);
    if (summary & GPGME_SIGSUM_CRL_TOO_OLD) fputs (" crl-too-old", stdout);
    if (summary & GPGME_SIGSUM_BAD_POLICY ) fputs (" bad-policy", stdout);
    if (summary & GPGME_SIGSUM_SYS_ERROR  ) fputs (" sys-error", stdout);
    fputs ("\n", stdout);
}

- (void)_pinrt_sig_status:(gpgme_error_t)status
{
    switch (status)
    {
        case GPG_ERR_NO_ERROR:
            printf("GPGME_SIG_STAT_GOOD\n");
            break;

        case GPG_ERR_BAD_SIGNATURE:
            printf("GPGME_SIG_STAT_BAD\n");
            break;

        case GPG_ERR_NO_PUBKEY:
            printf("GPGME_SIG_STAT_NOKEY\n");
            break;

        case GPG_ERR_NO_DATA:
            printf("GPGME_SIG_STAT_NOSIG\n");
            break;

        case GPG_ERR_SIG_EXPIRED:
            printf("GPGME_SIG_STAT_GOOD_EXP\n");
            break;

        case GPG_ERR_KEY_EXPIRED:
            printf("GPGME_SIG_STAT_GOOD_EXPKEY\n");
            break;
        default:
            printf("GPGME_SIG_STAT_GOOD_ERROR\n");
            break;
    }
    return;
}

- (int)_is_valid:(gpgme_verify_result_t)result
{
    switch (gpg_err_code(result->signatures->status))
    {
        case GPG_ERR_NO_ERROR:
            return true;

        case GPG_ERR_BAD_SIGNATURE:
        case GPG_ERR_NO_PUBKEY:
        case GPG_ERR_NO_DATA:
        case GPG_ERR_SIG_EXPIRED:
        case GPG_ERR_KEY_EXPIRED:
        default:
            break;
    }
    return false;
}

- (int)_is_trust:(gpgme_verify_result_t)result
{
    gpgme_signature_t sig;
    for (sig = result->signatures; sig; sig = sig->next) 
    {
        gpgme_sigsum_t summary = sig->summary;     
        //[self _print_sig_summary:summary];
        if (summary & GPGME_SIGSUM_VALID      ) { return 1; }
        if (summary & GPGME_SIGSUM_GREEN      ) { return 1; }
        if (summary & GPGME_SIGSUM_RED        ) {}
        if (summary & GPGME_SIGSUM_KEY_REVOKED) {}
        if (summary & GPGME_SIGSUM_KEY_EXPIRED) {}
        if (summary & GPGME_SIGSUM_SIG_EXPIRED) {}
        if (summary & GPGME_SIGSUM_KEY_MISSING) {}
        if (summary & GPGME_SIGSUM_CRL_MISSING) {}
        if (summary & GPGME_SIGSUM_CRL_TOO_OLD) {}
        if (summary & GPGME_SIGSUM_BAD_POLICY ) {}
        if (summary & GPGME_SIGSUM_SYS_ERROR  ) {}
    }
    return 0;
}

- (void)_print_data:(gpgme_data_t)data;
{
    int ret;
    ret = gpgme_data_seek (data, 0, SEEK_SET);

    char buf[1024];
    memset(buf, 0, sizeof(buf));

    while ((ret=gpgme_data_read(data, buf, 1024-1)) > 0)
    {
        fwrite(buf, ret, 1, stdout);
    }
    return;
}

- (NSData*)_data_to_nsdata:(gpgme_data_t)data;
{
    id pool = [NSAutoreleasePool new];

    int ret;
    ret = gpgme_data_seek (data, 0, SEEK_SET);

    char buf[1024];
    memset(buf, 0, sizeof(buf));

    NSMutableData* nsdata = [NSMutableData data];

    while ((ret=gpgme_data_read(data, buf, 1024-1)) > 0)
    {
        [nsdata appendBytes:buf length:ret];
    }

    NSData* ret_nsdata = [[NSData alloc] initWithData:nsdata];


    [pool drain];

    [ret_nsdata autorelease];

    return ret_nsdata;
}

- (NSArray*)_parse_list_pub:(NSString*)pub
{
    id pool = [NSAutoreleasePool new];
    id saved_err = nil;

    NSArray* line_array = nil;
    NSArray* word_array = nil;
    NSEnumerator* line_enum = nil;
    NSEnumerator* word_enum = nil;
    NSMutableArray* ans_array = nil;
    NSMutableArray* output_array = nil;
    NSMutableDictionary* ans_dict = nil;

    @try {
        if (pub == nil) @throw @"nothing input pub_string";

        ans_array = [NSMutableArray new];
        ans_dict = [NSMutableDictionary new];

        line_array = [pub componentsSeparatedByString:@"\n"];
        line_enum = [line_array objectEnumerator];
        //NSLog(@"%@\n", [line_enum allObjects]);

        for (id line_element in line_enum) {

            // - line reconstruction process ------------
            //NSLog(@"%@\n", line_element);

            if ([line_element length] == 0) {
                if ([ans_dict count] != 0) {
                    [ans_array addObject:ans_dict];
                }
                [ans_dict autorelease];
                ans_dict = [NSMutableDictionary new];
                continue;
            }

            word_array = [line_element componentsSeparatedByString:@" "];
            if ([line_element characterAtIndex:0] == 'p') {
                [ans_dict setObject:[word_array objectAtIndex:3] forKey:@"pub"]; 
            }
            if ([line_element characterAtIndex:0] == 's') {
                [ans_dict setObject:[word_array objectAtIndex:3] forKey:@"sub"]; 
            }

            // ------------------------------------------

            word_enum = [word_array objectEnumerator];
            for (id word_element in word_enum) {
                //NSLog(@"%@\n", word_element);

                // - word reconstruction process ------------

                if ([word_element length] == 0) continue;
                if ([word_element characterAtIndex:0] == '/') continue;
                if ([word_element characterAtIndex:0] == '-') continue;
                if ([word_element characterAtIndex:0] == '<') {
                    int start = 1;
                    int end = [word_element length] - 2;
                    NSRange range = NSMakeRange(start, end);
                    word_element = [word_element substringWithRange:range];
                    [ans_dict setObject:word_element forKey:@"user"]; 
                }

                // ------------------------------------------
            }
            //NSLog(@"%@\n", ans_array);
        }
        output_array = [[NSArray alloc] initWithArray:ans_array];
    }
    @catch (NSString* err) {
        NSString* err_string = [NSString stringWithFormat:
                                   @"error: [gpg _parse_list_pub] %@", err];
        saved_err = [err retain];
        @throw err_string;
    }
    @catch (id err) {
        NSLog(@"internal error:%@\n", err);
        saved_err = [err retain];
        @throw err;
    }
    @finally {
        [pool drain];
        [saved_err autorelease];
    }
    [output_array autorelease];

    return output_array;
}

- (NSArray*)_parse_list_sec:(NSString*)sec
{
    id pool = [NSAutoreleasePool new];
    id saved_err = nil;

    NSArray* line_array = nil;
    NSArray* word_array = nil;
    NSEnumerator* line_enum = nil;
    NSEnumerator* word_enum = nil;
    NSMutableArray* ans_array = nil;
    NSMutableArray* output_array = nil;
    NSMutableDictionary* ans_dict = nil;

    @try {
        if (sec == nil) @throw @"nothing input sec_string";

        ans_array = [NSMutableArray new];
        ans_dict = [NSMutableDictionary new];

        line_array = [sec componentsSeparatedByString:@"\n"];
        line_enum = [line_array objectEnumerator];
        //NSLog(@"%@\n", line_array);
        //NSLog(@"%@\n", [line_enum allObjects]);

        for (id line_element in line_enum) {

            // - line reconstruction process ------------
            //NSLog(@"%@\n", line_element);

            if ([line_element length] == 0) {
                if ([ans_dict count] != 0) {
                    [ans_array addObject:ans_dict];
                }
                [ans_dict autorelease];
                ans_dict = [NSMutableDictionary new];
                continue;
            }

            word_array = [line_element componentsSeparatedByString:@" "];

            if ([line_element characterAtIndex:1] == 'e') {
                [ans_dict setObject:[word_array objectAtIndex:3] forKey:@"sec"]; 
            }
            if ([line_element characterAtIndex:1] == 's') {
                [ans_dict setObject:[word_array objectAtIndex:3] forKey:@"sub"]; 
            }

            // ------------------------------------------

            word_enum = [word_array objectEnumerator];
            for (id word_element in word_enum) {
                //NSLog(@"%@\n", word_element);

                // - word reconstruction process ------------

                if ([word_element length] == 0) continue;
                if ([word_element characterAtIndex:0] == '/') continue;
                if ([word_element characterAtIndex:0] == '-') continue;
                if ([word_element characterAtIndex:0] == '<') {
                    int start = 1;
                    int end = [word_element length] - 2;
                    NSRange range = NSMakeRange(start, end);
                    word_element = [word_element substringWithRange:range];
                    [ans_dict setObject:word_element forKey:@"user"]; 
                }

                // ------------------------------------------
            }
            //NSLog(@"%@\n", ans_array);
        }
        output_array = [[NSArray alloc] initWithArray:ans_array];
    }
    @catch (NSString* err) {
        NSString* err_string = [NSString stringWithFormat:
                                   @"error: [gpg _parse_list_sec] %@", err];
        saved_err = [err retain];
        @throw err_string;
    }
    @catch (id err) {
        NSLog(@"internal error:%@\n", err);
        saved_err = [err retain];
        @throw err;
    }
    @finally {
        [pool drain];
        [saved_err autorelease];
    }

    [output_array autorelease];

    return output_array;
}

- (NSArray*)_parse_list_throw:(NSString*)key
{
    id pool = [NSAutoreleasePool new];
    id saved_err = nil;

    NSArray* line_array = nil;
    NSArray* word_array = nil;
    NSEnumerator* line_enum = nil;
    NSEnumerator* word_enum = nil;
    NSMutableArray* ans_array = nil;
    NSMutableArray* output_array = nil;
    NSMutableDictionary* ans_dict = nil;

    @try {
        if (key == nil) @throw @"nothing input key_string";

        ans_array = [NSMutableArray new];
        ans_dict = [NSMutableDictionary new];

        line_array = [key componentsSeparatedByString:@"\n"];
        line_enum = [line_array objectEnumerator];
        //NSLog(@"%@\n", line_array);
        //NSLog(@"%@\n", [line_enum allObjects]);

        for (id line_element in line_enum) {

            word_array = [line_element componentsSeparatedByString:@" "];

            // - line reconstruction process ------------
            //NSLog(@"%@\n", line_element);

            if ([line_element length] == 0) continue;

            if ([line_element characterAtIndex:0] == 'p') {
                [ans_dict setObject:[word_array objectAtIndex:2] forKey:@"pub"]; 
            }

            if ([line_element characterAtIndex:0] == 's') {
                [ans_dict setObject:[word_array objectAtIndex:2] forKey:@"sub"]; 
                if ([ans_dict count] != 0) {
                    [ans_array addObject:ans_dict];
                }
                [ans_dict autorelease];
                ans_dict = [NSMutableDictionary new];
            }

            // ------------------------------------------

            word_enum = [word_array objectEnumerator];
            for (id word_element in word_enum) {
                //NSLog(@"%@\n", word_element);

                // - word reconstruction process ------------

                if ([word_element length] == 0) continue;
                if ([word_element characterAtIndex:0] == '/') continue;
                if ([word_element characterAtIndex:0] == '-') continue;
                if ([word_element characterAtIndex:0] == '<') {
                    int start = 1;
                    int end = [word_element length] - 2;
                    NSRange range = NSMakeRange(start, end);
                    word_element = [word_element substringWithRange:range];
                    [ans_dict setObject:word_element forKey:@"user"]; 
                }

                // ------------------------------------------
            }
            //NSLog(@"%@\n", ans_array);
        }
        //NSLog(@"%@\n", ans_array);
        output_array = [[NSArray alloc] initWithArray:ans_array];
    }

    @catch (NSString* err) {
        NSString* err_string = [NSString stringWithFormat:
                                   @"error: [gpg _parse_list_throw] %@", err];
        saved_err = [err retain];
        @throw err_string;
    }

    @catch (id err) {
        NSLog(@"internal error:%@\n", err);
        saved_err = [err retain];
        @throw err;
    }
    @finally {
        [pool drain];
        [saved_err autorelease];
    }

    [output_array autorelease];
    return output_array;
}
   
- (int)genrkey
{
    return false;
}

- (int)delkey:(NSString*)key
{
    return false;
}

- (int)_mk_directory:(NSString*)dir
{
    [gpgLock lock];
    [gpgLock unlock];
    return false;
}

- (int)_rm_directory:(NSString*)dir
{
    [gpgLock lock];
    [gpgLock unlock];
    return false;
}

@end

#endif //__RAPRINS_GPGME_H_
