#ifndef __PRISON_GPGME_H_
#define __PRISON_GPGME_H_

/*
 * todo
 * - mk trust
 * - mk revoke
 * - mk templary import
 * - remake import. now,,, import message return only ture...
 * - this class is have to reconstruct signleton.
 * - all method reconstruct mutex lock
 * - at no distant date,,, drop gpgme code,, only ojbc code using gpg command..
 */

// ----- gpg version independent ------------------------------------------
// check
// gpg2 (GnuPG) 2.0.14
// gpg  (GnuPG) 1.4.10
// ------------------------------------------------------------------------

// ----- platform checking ------------------------------------------------
// check
// macosx snow leopard
// Linux2.6.32-22-generic Ubuntu SMP x86_64
// ------------------------------------------------------------------------


#import <Cocoa/Cocoa.h>

#include <stdio.h>
#include <errno.h>
#include <locale.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <gpgme.h>
#include <gpg-error.h>

#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>

#ifdef __linux__
#endif

#ifdef __MACH__
#endif

#include "category.h"
#include "utils.h"
#include "common.h"

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

+ (NSString*)trimContentFromArmor:(NSString*)armorTxt;
+ (NSString*)appendPublicFrame:(NSString*)content;
+ (NSString*)appendPrivateFrame:(NSString*)content;
+ (NSString*)appendMessageFrame:(NSString*)content;
+ (NSString*)appendSignatureFrame:(NSString*)content;

- (id)init;
- (id)initWithDir:(NSString*)dir;
- (void)dealloc;

- (BOOL)getValid;
- (BOOL)getTrust;

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

- (BOOL)genkey;
- (BOOL)signkey:(NSString*)uid;
- (BOOL)delkey:(NSString*)uid;
- (BOOL)delsig:(NSString*)keyuid :(NSString*)siguid;

- (int)import:(NSString*)key;
- (NSString*)exportPubring:(NSString*)uid;
- (NSString*)exportSecring:(NSString*)uid;

- (NSDictionary*)signedlist;
- (NSArray*)userlist;
- (NSArray*)ownerlist;
- (NSArray*)throw:(NSString*)key;

- (NSString*)sign:(NSString*)txt;
- (NSString*)verify:(NSString*)sig;

- (NSString*)decrypt:(NSString*)sig;
- (NSString*)encrypt:(NSString*)txt :(NSString*)uid;
- (NSString*)encryptForce:(NSString*)txt :(NSString*)uid;

// not implementation
- (int)genrkey;

// private function
- (void)_print_sig_summary:(gpgme_sigsum_t)summary;
- (void)_pinrt_sig_status:(gpgme_error_t)status;
- (void)_print_data:(gpgme_data_t)data;

- (int)_is_trust:(gpgme_verify_result_t)result;
- (int)_is_valid:(gpgme_verify_result_t)result;

- (NSData*)_data_to_nsdata:(gpgme_data_t)data;

- (NSDictionary*)_parse_list_sig:(NSString*)sig;
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

// c lang function --------------------------------------------------------------
#define R (0)
#define W (1)

static pid_t popen4(char** args, int* fd_in, int* fd_out, int* fd_err, int* fd_pp)
{
    int ret = 0;

    int pipe_out[2];
    int pipe_err[2];
    int pipe_in[2];
    int pipe_pp[2];

    pid_t pid;

    //Create pipes. -------------------------------
    if (ret == 0) {
        if (pipe(pipe_out) == -1) {
            ret = errno;
        }
    }

    if (ret == 0) {
        if (pipe(pipe_err) == -1) {
            close(pipe_out[R]);
            close(pipe_out[W]);
            ret = errno;
        }
    }

    if (ret == 0) {
        if (pipe(pipe_in) == -1) {
            close(pipe_out[R]);
            close(pipe_out[W]);
            close(pipe_err[R]);
            close(pipe_err[W]);
            ret = errno;
        }
    }

    if (ret == 0) {
        if(pipe(pipe_pp) == -1) {
            close(pipe_out[R]);
            close(pipe_out[W]);
            close(pipe_err[R]);
            close(pipe_err[W]);
            close(pipe_in[R]);
            close(pipe_in[W]);
            ret = errno;
        }
    }

    if (ret != 0) {
        perror("pipe");
        ret = -ret;
    }
    //---------------------------------------------


    // Invoke fork process
    if (ret == 0) {
        if ((pid = fork()) == -1) {
            ret = -errno;
            perror("fork");
        }
    }

    // child process
    if (pid == 0) {

        close(pipe_in[W]);
        close(pipe_out[R]);
        close(pipe_err[R]);
        close(pipe_pp[W]);

        dup2(pipe_in[R], 0);
        close(pipe_in[R]);

        dup2(pipe_out[W], 1);
        close(pipe_out[W]);

        dup2(pipe_err[W], 2);
        close(pipe_err[W]);

        dup2(pipe_pp[R], 3);
        close(pipe_pp[R]);

        /*
        char buffer[4000];
        memset(buffer, 0, 4000);
        read(3, buffer, 4000);
        printf("pass:::%s\n", buffer);
        */

        if (execvp(args[0], args) == -1) {
            close(pipe_in[R]);
            close(pipe_out[W]);
            close(pipe_err[W]);
            close(pipe_pp[R]);
            exit(1);
        }
        exit(0);
    }

    // parent process
    else {
        close(pipe_in[R]);
        close(pipe_out[W]);
        close(pipe_err[W]);
        close(pipe_pp[R]);

        if (fd_in  != NULL) *fd_in  = pipe_in[W];
        if (fd_out != NULL) *fd_out = pipe_out[R];
        if (fd_err != NULL) *fd_err = pipe_err[R];
        if (fd_pp  != NULL) *fd_pp  = pipe_pp[W];
    }

    return(pid);
}
#undef R
#undef W
// ------------------------------------------------------------------------------

// public function
+ (NSString*)appendSignatureFrame:(NSString*)content
{
    NSString* header = @"-----BEGIN PGP SIGNATURE-----\n";
    NSString* footer = @"-----END PGP SIGNATURE-----\n";

    return [NSString stringWithFormat:@"%@\n%@\n%@", header, content, footer];
}

+ (NSString*)appendPrivateFrame:(NSString*)content
{
    NSString* header = @"-----BEGIN PGP PRIVATE KEY BLOCK-----\n";
    NSString* footer = @"-----END PGP PRIVATE KEY BLOCK-----\n";

    return [NSString stringWithFormat:@"%@\n%@\n%@", header, content, footer];
}

+ (NSString*)appendPublicFrame:(NSString*)content
{
    NSString* header = @"-----BEGIN PGP PUBLIC KEY BLOCK-----\n";
    NSString* footer = @"-----END PGP PUBLIC KEY BLOCK-----\n";

    return [NSString stringWithFormat:@"%@\n%@\n%@", header, content, footer];
}

+ (NSString*)appendMessageFrame:(NSString*)content
{
    NSString* header = @"-----BEGIN PGP MESSAGE-----\n";
    NSString* footer = @"-----END PGP MESSAGE-----\n";

    return [NSString stringWithFormat:@"%@\n%@\n%@", header, content, footer];
}

+ (NSString*)trimContentFromArmor:(NSString*)armorTxt
{
    id pool = [NSAutoreleasePool new];
    NSMutableString* tmp_string = [NSMutableString new];

    NSArray* armor_array = [armorTxt componentsSeparatedByString:@"\n"]; 
    NSEnumerator* line_enum = [armor_array objectEnumerator];
    ITERATE(line_element, line_enum) {
        //NSLog(@"%@\n", line_element);

        if ([line_element length] == 0) {
            continue;
        }

        NSComparisonResult compResult;
        if ([line_element length] >= [@"-----" length]) {
            compResult = [line_element compare:@"-----"
                                       options:NSCaseInsensitiveSearch
                                         range:NSMakeRange(0,[@"-----" length])];
            if (compResult == NSOrderedSame) {
                continue;
            }
        }

        if ([line_element length] >= [@"Version:" length]) {
            compResult = [line_element compare:@"Version:"
                                       options:NSCaseInsensitiveSearch
                                         range:NSMakeRange(0,[@"Version:" length])];
            if (compResult == NSOrderedSame) {
                continue;
            }
        }

        [tmp_string appendString:line_element];

    }
    NSString* ret_string = [[NSString alloc] initWithString:tmp_string];
    [pool drain];
    [ret_string autorelease];
    return ret_string;
}

- (BOOL)getValid
{
    return gpgValid;
}

- (BOOL)getTrust
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
    NSFileHandle* out_file = nil;

    NSPipe* err_pipe = nil;
    NSFileHandle* err_file = nil;


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
        out_file = [out_pipe fileHandleForReading];

        // error pipe
        err_pipe = [NSPipe pipe];
        err_file = [err_pipe fileHandleForReading];

        task = [[NSTask alloc] init];
        [task setLaunchPath:gpgExe];
        //[task setStandardInput:in_pipe];
        [task setStandardOutput:out_pipe];
        [task setStandardError:err_pipe];
        [task setArguments:args];
        [task launch];

        [task waitUntilExit];
        ret = [task terminationStatus];

        if (ret != 0) {
            @throw @"violation error";
        }

        // stdout
        encode = [NSString availableStringEncodings];
        out_data = [out_file readDataToEndOfFile];
        out_string = [[NSString alloc] initWithData:out_data encoding:*encode];

        // stderr
        /*
        NSData* err_data = nil;
        err_data = [err_file readDataToEndOfFile];
        NSString* err_string;
        err_string = [[NSString alloc] initWithData:err_data encoding:*encode];
        [err_string autorelease];
        NSLog(@"err:\n%@\n", err_string);
        */


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
        [out_file closeFile];
        [err_file closeFile];
        [task release];
        [pool drain];
        [saved_err autorelease];
    }

    [out_string autorelease];
    //NSLog(@"%@\n", out_string);

    return [self _parse_list_sec:out_string];
}

- (NSDictionary*)signedlist
{
    id pool = [NSAutoreleasePool new];
    id saved_err = nil;

    //NSPipe* in_pipe = nil;;
    //NSFileHandle* input = nil;

    NSPipe* out_pipe = nil;
    NSFileHandle* out_file = nil;

    NSPipe* err_pipe = nil;
    NSFileHandle* err_file = nil;

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
        [args addObject:@"--list-sig"];

        // output pipe
        out_pipe = [NSPipe pipe];
        out_file = [out_pipe fileHandleForReading];

        // output pipe
        err_pipe = [NSPipe pipe];
        err_file = [err_pipe fileHandleForReading];


        task = [[NSTask alloc] init];
        [task setLaunchPath:gpgExe];
        //[task setStandardInput:in_pipe];
        [task setStandardOutput:out_pipe];
        [task setStandardError:err_pipe];
        [task setArguments:args];
        [task launch];

        [task waitUntilExit];
        ret = [task terminationStatus];

        if (ret != 0) {
            @throw @"violation error";
        }

        // stdout
        encode = [NSString availableStringEncodings];
        out_data = [out_file readDataToEndOfFile];
        out_string = [[NSString alloc] initWithData:out_data encoding:*encode];

        if ([out_string length] == 0) {
            @throw @"no data";
        }
    }

    @catch (NSString* err) {
        NSString* err_string = [NSString stringWithFormat:
                                @"error: [gpg signedlist] %@", err];
        saved_err = [err retain];
        @throw err_string;
    }

    @catch (id err) {
        saved_err = [err retain];
        @throw err;
    }
    @finally {
        [out_file closeFile];
        [err_file closeFile];
        [task release];
        [pool drain];
        [saved_err autorelease];
    }

    [out_string autorelease];
    //NSLog(@"%@\n", out_string);

    return [self _parse_list_sig:out_string];
}

- (NSArray*)userlist
{
    id pool = [NSAutoreleasePool new];
    id saved_err = nil;

    //NSPipe* in_pipe = nil;;
    //NSFileHandle* input = nil;

    NSPipe* out_pipe = nil;
    NSFileHandle* out_file = nil;

    NSPipe* err_pipe = nil;
    NSFileHandle* err_file = nil;

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
        out_file = [out_pipe fileHandleForReading];

        // error pipe
        err_pipe = [NSPipe pipe];
        err_file = [err_pipe fileHandleForReading];


        task = [[NSTask alloc] init];
        [task setLaunchPath:gpgExe];
        //[task setStandardInput:in_pipe];
        [task setStandardOutput:out_pipe];
        [task setStandardError:err_pipe];
        [task setArguments:args];
        [task launch];

        [task waitUntilExit];
        ret = [task terminationStatus];

        if (ret != 0) {
            @throw @"violation error";
        }

        // stdout
        encode = [NSString availableStringEncodings];
        out_data = [out_file readDataToEndOfFile];
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
        [out_file closeFile];
        [err_file closeFile];
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
#ifdef __PRISON__

    pid_t ch;
    int ch_fd_in;
    int ch_fd_out;
    int ch_fd_err;
    int ch_fd_pp;
    int status;

    if (txt == nil) {
        @throw @"error:[gpg encrypt] nonexistent txt";
    }

    if ([txt length] == 0) {
        @throw @"error:[gpg encrypt] nonexistent txt";
    }

    if (uid == nil) {
        @throw @"error:[gpg encrypt] nonexistent uid";
    }

    id pool = [NSAutoreleasePool new];

    NSMutableArray* arg_array = [NSMutableArray array];
    [arg_array addObject:gpgExe];
    [arg_array addObject:@"--homedir"];
    [arg_array addObject:gpgDir];
    [arg_array addObject:@"--batch"];
    [arg_array addObject:@"--armor"];
    [arg_array addObject:@"-r"];
    [arg_array addObject:uid];
    [arg_array addObject:@"--encrypt"];

    //NSLog(@"%@\n", arg_array);

    unsigned int count = [arg_array count];
    char* args[count];
    for (unsigned int i=0; i<count; i++) {
        args[i] = (char*)[[arg_array objectAtIndex:i] UTF8String];
    }
    args[count] = NULL;

    ch = popen4(args, &ch_fd_in, &ch_fd_out, &ch_fd_err, &ch_fd_pp);

    write(ch_fd_in, [txt UTF8String], [txt length]);
    close(ch_fd_in);

    //NSLog(@"%s\n", [[self getPass] UTF8String]);
    //NSLog(@"%d\n", [[self getPass] length]);
    write(ch_fd_pp, [[self getPass] UTF8String], [[self getPass] length]);
    close(ch_fd_pp);

    waitpid(ch, &status, 0);

    int is_correct_terminate;
    is_correct_terminate = WIFEXITED(status);

    if (is_correct_terminate != 1) {
        close(ch_fd_out);
        close(ch_fd_err);
        @throw @"error: [gpg encrypt] violation error";
    }

    int terminate_status;
    terminate_status = WEXITSTATUS(status);

    if (terminate_status != 0) {
        close(ch_fd_out);
        close(ch_fd_err);
        @throw @"error: [gpg encrypt] miss passphrase";
    }

    NSFileHandle* out_file = [[NSFileHandle alloc] initWithFileDescriptor:ch_fd_out];
    const NSStringEncoding* encode = [NSString availableStringEncodings];
    NSData* out_data = [out_file readDataToEndOfFile];
    NSString* out_string = [[NSString alloc] initWithData:out_data encoding:*encode];
    //NSLog(@"%@\n", out_string);

    close(ch_fd_out);
    close(ch_fd_err);
    [out_file release];
    [pool drain];
    [out_string autorelease];

    return out_string;

#else

    if (txt == nil) {
        @throw @"error: [gpg encrypt] nonexistent txt";
    }

    if ([txt length] == 0) {
        @throw @"error: [gpg encrypt] nonexistent txt";
    }

    if (uid == nil) {
        @throw @"error: [gpg encrypt] nonexistent uid";
    }

    if ([uid length] == 0) {
        @throw @"error: [gpg encrypt] nonexistent uid";
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
    NSFileHandle* in_file;
    in_pipe = [NSPipe pipe];
    in_file = [in_pipe fileHandleForWriting];

    // output pipe
    NSPipe* out_pipe;
    NSFileHandle* out_file;
    out_pipe = [NSPipe pipe];
    out_file = [out_pipe fileHandleForReading];

    // error pipe
    NSPipe* err_pipe;
    NSFileHandle* err_file;
    err_pipe = [NSPipe pipe];
    err_file = [err_pipe fileHandleForReading];

    // task
    NSTask* task;
    task = [[NSTask alloc] init];
    [task setLaunchPath:gpgExe];
    [task setStandardInput:in_pipe];
    [task setStandardOutput:out_pipe];
    [task setStandardError:err_pipe];
    [task setArguments:args];
    [task launch];

    // encoding environment value
    const NSStringEncoding* encode;
    encode = [NSString availableStringEncodings];

    // stdin processing
    NSString* in_string = [NSString stringWithString:txt];
    NSData* in_data;
    in_data = [in_string dataUsingEncoding:*encode];
    [in_file writeData:in_data];
    [in_file closeFile];

    // stdout processing
    NSData* out_data;
    out_data = [out_file readDataToEndOfFile];
    NSString* out_string;
    out_string = [[NSString alloc] initWithData:out_data encoding:*encode];

    int ret;
    [task waitUntilExit];

    // closefile
    [out_file closeFile];
    [err_file closeFile];

    ret = [task terminationStatus];
    //NSLog(@"ret:%d", ret);

    if (ret != 0) {
        [task release];
        [pool drain];
        @throw @"error: [gpg encrypt] violation error";
    }


    [task release];
    [pool drain];

    [out_string autorelease];

    return out_string;

#endif
}

- (NSString*)encryptForce:(NSString*)txt :(NSString*)uid
{
#ifdef __PRISON__
    pid_t ch;
    int ch_fd_in;
    int ch_fd_out;
    int ch_fd_err;
    int ch_fd_pp;
    int status;

    if (txt == nil) {
        @throw @"error:[gpg encrypt] nonexistent txt";
    }

    if ([txt length] == 0) {
        @throw @"error:[gpg encrypt] nonexistent txt";
    }

    if (uid == nil) {
        @throw @"error:[gpg encrypt] nonexistent uid";
    }

    id pool = [NSAutoreleasePool new];

    NSMutableArray* arg_array = [NSMutableArray array];
    [arg_array addObject:gpgExe];
    [arg_array addObject:@"--homedir"];
    [arg_array addObject:gpgDir];
    [arg_array addObject:@"--batch"];
    [arg_array addObject:@"--always-trust"];
    [arg_array addObject:@"--armor"];
    [arg_array addObject:@"-r"];
    [arg_array addObject:uid];
    [arg_array addObject:@"--encrypt"];

    //NSLog(@"%@\n", arg_array);

    unsigned int count = [arg_array count];
    char* args[count];
    for (unsigned int i=0; i<count; i++) {
        args[i] = (char*)[[arg_array objectAtIndex:i] UTF8String];
    }
    args[count] = NULL;

    ch = popen4(args, &ch_fd_in, &ch_fd_out, &ch_fd_err, &ch_fd_pp);

    write(ch_fd_in, [txt UTF8String], [txt length]);
    close(ch_fd_in);

    //NSLog(@"%s\n", [[self getPass] UTF8String]);
    //NSLog(@"%d\n", [[self getPass] length]);
    write(ch_fd_pp, [[self getPass] UTF8String], [[self getPass] length]);
    close(ch_fd_pp);

    waitpid(ch, &status, 0);

    int is_correct_terminate;
    is_correct_terminate = WIFEXITED(status);

    if (is_correct_terminate != 1) {
        close(ch_fd_out);
        close(ch_fd_err);
        @throw @"error: [gpg encrypt] violation error";
    }

    int terminate_status;
    terminate_status = WEXITSTATUS(status);

    if (terminate_status != 0) {
        close(ch_fd_out);
        close(ch_fd_err);
        @throw @"error: [gpg encrypt] miss passphrase";
    }

    NSFileHandle* out_file = [[NSFileHandle alloc] initWithFileDescriptor:ch_fd_out];
    const NSStringEncoding* encode = [NSString availableStringEncodings];
    NSData* out_data = [out_file readDataToEndOfFile];
    NSString* out_string = [[NSString alloc] initWithData:out_data encoding:*encode];
    //NSLog(@"%@\n", out_string);

    close(ch_fd_out);
    close(ch_fd_err);
    [out_file release];
    [pool drain];
    [out_string autorelease];

    return out_string;

#else

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
    NSFileHandle* in_file;
    in_pipe = [NSPipe pipe];
    in_file = [in_pipe fileHandleForWriting];

    // output pipe
    NSPipe* out_pipe;
    out_pipe = [NSPipe pipe];
    NSFileHandle* out_file;
    out_file = [out_pipe fileHandleForReading];

    // error pipe
    NSPipe* err_pipe;
    err_pipe = [NSPipe pipe];
    NSFileHandle* err_file;
    err_file = [err_pipe fileHandleForReading];

    // task
    NSTask* task;
    task = [[NSTask alloc] init];
    [task setLaunchPath:gpgExe];
    [task setStandardInput:in_pipe];
    [task setStandardOutput:out_pipe];
    [task setStandardError:err_pipe];
    [task setArguments:args];
    [task launch];

    // encoding environment value
    const NSStringEncoding* encode;
    encode = [NSString availableStringEncodings];

    // stdin processing
    NSString* in_string = [NSString stringWithString:txt];
    NSData* in_data;
    in_data = [in_string dataUsingEncoding:*encode];
    [in_file writeData:in_data];
    [in_file closeFile];

    // stdout processing
    NSData* out_data;
    out_data = [out_file readDataToEndOfFile];
    NSString* out_string;
    out_string = [[NSString alloc] initWithData:out_data encoding:*encode];

    int ret;
    [task waitUntilExit];

    // file close
    [out_file closeFile];
    [err_file closeFile];

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

#endif
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

#ifndef  __PRISON__

    pid_t ch;
    int ch_fd_in;
    int ch_fd_out;
    int ch_fd_err;
    int ch_fd_pp;
    int status;

    if (txt == nil) {
        @throw @"error: [gpg sign] txt data is nil";
    }

    if ([txt length] == 0) {
        @throw @"error: [gpg sign] nonexistent txt data";
    }

    id pool = [NSAutoreleasePool new];

    NSMutableArray* arg_array = [NSMutableArray array];
    [arg_array addObject:gpgExe];
    [arg_array addObject:@"--homedir"];
    [arg_array addObject:gpgDir];
    [arg_array addObject:@"--passphrase-fd"];
    [arg_array addObject:@"3"];
    [arg_array addObject:@"--no-tty"];
    [arg_array addObject:@"--batch"];
    [arg_array addObject:@"--armor"];
    [arg_array addObject:@"--sign"];

    //NSLog(@"%@\n", arg_array);

    unsigned int count = [arg_array count];
    char* args[count];
    for (unsigned int i=0; i<count; i++) {
        args[i] = (char*)[[arg_array objectAtIndex:i] UTF8String];
    }
    args[count] = NULL;

    ch = popen4(args, &ch_fd_in, &ch_fd_out, &ch_fd_err, &ch_fd_pp);

    write(ch_fd_in, [txt UTF8String], [txt length]);
    close(ch_fd_in);

    //NSLog(@"%s\n", [[self getPass] UTF8String]);
    //NSLog(@"%d\n", [[self getPass] length]);
    write(ch_fd_pp, [[self getPass] UTF8String], [[self getPass] length]);
    close(ch_fd_pp);

    waitpid(ch, &status, 0);

    int is_correct_terminate;
    is_correct_terminate = WIFEXITED(status);

    if (is_correct_terminate != 1) {
        close(ch_fd_out);
        close(ch_fd_err);
        @throw @"error: [gpg sign] violation error";
    }

    int terminate_status;
    terminate_status = WEXITSTATUS(status);

    if (terminate_status != 0) {
        close(ch_fd_out);
        close(ch_fd_err);
        @throw @"error: [gpg sign] miss passphrase";
    }

    NSFileHandle* out_file = [[NSFileHandle alloc] initWithFileDescriptor:ch_fd_out];
    const NSStringEncoding* encode = [NSString availableStringEncodings];
    NSData* out_data = [out_file readDataToEndOfFile];
    NSString* out_string = [[NSString alloc] initWithData:out_data encoding:*encode];
    //NSLog(@"%@\n", out_string);

    close(ch_fd_out);
    close(ch_fd_err);
    [out_file release];
    [pool drain];
    [out_string autorelease];

    return out_string;

#else

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

#endif
}

- (NSString*)exportPubring:(NSString*)uid
{
#ifdef __MACH__
    // batch mode code

    id pool = [NSAutoreleasePool new];

    NSMutableArray* args;
    args = [NSMutableArray array];
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
    NSFileHandle* input;
    in_pipe = [NSPipe pipe];
    input = [in_pipe fileHandleForWriting];
    */

    // output pipe
    NSPipe* out_pipe;
    NSFileHandle* out_file;
    out_pipe = [NSPipe pipe];
    out_file = [out_pipe fileHandleForReading];

    // error pipe
    NSPipe* err_pipe;
    NSFileHandle* err_file;
    err_pipe = [NSPipe pipe];
    err_file = [err_pipe fileHandleForReading];

    // task
    NSTask* task;
    task = [[NSTask alloc] init];
    [task setLaunchPath:gpgExe];
    [task setStandardOutput:out_pipe];
    [task setStandardError:err_pipe];
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
    out_data = [out_file readDataToEndOfFile];
    NSString* out_string;
    out_string = [[NSString alloc] initWithData:out_data encoding:*encode];

    //NSLog(@"data_length:%d\n", [out_data length]);
    //NSLog(@"\n%@\n", out_data);

    //NSLog(@"string_length:%d\n", [out_string length]);
    //NSLog(@"\n%@\n", out_string);


    // closefile
    [out_file closeFile];
    [err_file closeFile];

    [task release];
    [pool drain];

    if ([out_string length] == 0) {
        @throw @"error: [gpg exportPubring] nonexistent user";
    }

    if (ret != 0) {
        @throw @"error: [gpg exportPubring] violation error";
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
        @throw @"error: [gpg exportPubring] violation error";
    }

    //[self _print_data:out_data];

    NSData* out_nsdata;
    out_nsdata = [self _data_to_nsdata:out_data];

    if ([out_nsdata length] == 0) {
        gpgme_data_release(out_data);
        gpgme_release(ctx);
        @throw @"error: [gpg exportPubring] nonexistent user";
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
    [args addObject:@"--fast-import"];

    // input pipe
    NSPipe* in_pipe;
    in_pipe = [NSPipe pipe];
    NSFileHandle* in_file;
    in_file = [in_pipe fileHandleForWriting];

    // output pipe
    /*
    NSPipe* out_pipe;
    out_pipe = [NSPipe pipe];
    NSFileHandle* out_file;
    out_file = [out_pipe fileHandleForReading];
    */

    // error pipe
    NSPipe* err_pipe;
    err_pipe = [NSPipe pipe];
    NSFileHandle* err_file;
    err_file = [err_pipe fileHandleForReading];

    // task
    NSTask* task;
    task = [[NSTask alloc] init];
    [task setLaunchPath:gpgExe];
    [task setStandardInput:in_pipe];
    //[task setStandardOutput:out_pipe];
    [task setStandardError:err_pipe];
    [task setArguments:args];
    [task launch];

    // encoding environment value
    const NSStringEncoding* encode;
    encode = [NSString availableStringEncodings];

    // stdin
    NSString* in_string = [NSString stringWithString:key];
    NSData* in_data;
    in_data = [in_string dataUsingEncoding:*encode];
    [in_file writeData:in_data];
    [in_file closeFile];

    int ret;
    [task waitUntilExit];

    // closefile
    //[out_file closeFile];
    [err_file closeFile];

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

- (NSString*)exportSecring:(NSString*)uid
{
    id pool = [NSAutoreleasePool new];
    id saved_err = nil;

    const NSStringEncoding* encode;

    /*
    NSPipe* in_pipe = nil;;
    NSFileHandle* in_file = nil;
    NSData* in_data = nil;
    */

    NSPipe* out_pipe = nil;
    NSFileHandle* out_file = nil;
    NSData* out_data = nil;

    NSPipe* err_pipe = nil;
    NSFileHandle* err_file = nil;
    //NSData* err_data = nil;


    NSString* out_string = nil;
    //NSString* err_string = nil;

    NSTask* task = nil;

    @try{

        /*
        // input pipe
        in_pipe = [NSPipe pipe];
        in_file = [in_pipe fileHandleForWriting];
        */

        // output pipe
        out_pipe = [NSPipe pipe];
        out_file = [out_pipe fileHandleForReading];

        // error pipe
        err_pipe = [NSPipe pipe];
        err_file = [err_pipe fileHandleForReading];

        NSMutableArray* args;
        args = [NSMutableArray array];
        [args addObject:@"--armor"];
        [args addObject:@"--homedir"];
        [args addObject:gpgDir];
        [args addObject:@"--export-secret-keys"];
        if (uid != nil) {
            [args addObject:uid];
        }

        // task
        NSTask* task;
        task = [[NSTask alloc] init];
        [task setLaunchPath:gpgExe];
        //[task setStandardInput:in_pipe];
        [task setStandardOutput:out_pipe];
        [task setStandardError:err_pipe];
        [task setArguments:args];
        [task launch];

        // encoding
        encode = [NSString availableStringEncodings];

        /*
        // stdin
        in_data = [buf_str dataUsingEncoding:*encode];
        [in_file writeData:in_data];
        [in_file closeFile];
        */

        int ret;
        [task waitUntilExit];
        ret = [task terminationStatus];

        if (ret != 0) {
            @throw @"violation error";
        }



        // stdout
        //out_data = [out_file availableData];
        out_data = [out_file readDataToEndOfFile];
        out_string = [[NSString alloc] initWithData:out_data encoding:*encode];
        //NSLog(@"out:\n%@\n", out_string);

        /*
        // stderr
        err_data = [err_file readDataToEndOfFile];
        err_string = [[NSString alloc] initWithData:err_data encoding:*encode];
        [err_string autorelease];
        //NSLog(@"err:\n%@\n", err_string);
        */
    }

    @catch (NSString* err) {
        NSString* err_string = [NSString stringWithFormat:
                                @"error: [gpg exportSecring] %@", err];
        saved_err = [err retain];
        @throw err_string;
    }

    @catch (id err) {
        saved_err = [err retain];
        @throw err;
    }
    @finally {
        [out_file closeFile];
        [err_file closeFile];
        [task release];
        [pool drain];
        [saved_err autorelease];
    }

    [out_string autorelease];
    return out_string;
}

- (BOOL)signkey:(NSString*)uid
{
    id pool = [NSAutoreleasePool new];
    id saved_err = nil;

    const NSStringEncoding* encode;

    NSPipe* in_pipe = nil;;
    NSFileHandle* in_file = nil;
    NSData* in_data = nil;
    NSString* inFD = nil;

    NSPipe* out_pipe = nil;
    NSFileHandle* out_file = nil;
    //NSData* out_data = nil;
    NSString* outFD = nil;

    NSPipe* err_pipe = nil;
    NSFileHandle* err_file = nil;
    //NSData* err_data = nil;
    NSString* errFD = nil;

    NSTask* task = nil;

    //NSString* out_string = nil;
    //NSString* err_string = nil;
    NSMutableString* buf_str = [NSMutableString new];

    int ret = 1;

    @try{

        // input pipe
        in_pipe = [NSPipe pipe];
        in_file = [in_pipe fileHandleForWriting];

        // output pipe
        out_pipe = [NSPipe pipe];
        out_file = [out_pipe fileHandleForReading];

        // error pipe
        err_pipe = [NSPipe pipe];
        err_file = [err_pipe fileHandleForReading];

        // in
        inFD = [NSString stringWithFormat:@"%d", [in_file fileDescriptor]];

        // out
        outFD = [NSString stringWithFormat:@"%d", [out_file fileDescriptor]]; 

        // error
        errFD = [NSString stringWithFormat:@"%d", [err_file fileDescriptor]];

        //NSLog(@"in  %@\n", inFD);
        //NSLog(@"out %@\n", outFD);
        //NSLog(@"err %@\n", errFD);

        NSMutableArray* args;
        args = [NSMutableArray array];
        [args addObject:@"--homedir"];
        [args addObject:gpgDir];
        [args addObject:@"--yes"];
        [args addObject:@"--batch"];
        [args addObject:@"--passphrase-fd"];
        [args addObject:@"0"];
        [args addObject:@"--sign-key"];
        [args addObject:uid];

        // make input string 
        [buf_str appendString:[self getPass]];
        //NSLog(@"%@\n", buf_str);

        // task
        NSTask* task;
        task = [[NSTask alloc] init];
        [task setLaunchPath:gpgExe];
        [task setStandardInput:in_pipe];
        [task setStandardOutput:out_pipe];
        [task setStandardError:err_pipe];
        [task setArguments:args];
        [task launch];

        // encoding
        encode = [NSString availableStringEncodings];

        // stdin
        in_data = [buf_str dataUsingEncoding:*encode];
        [in_file writeData:in_data];
        [in_file closeFile];

        [task waitUntilExit];
        ret = [task terminationStatus];

        // stdout
        //out_data = [out_file readDataToEndOfFile];
        /*
        out_data = [out_file availableData];
        out_string = [[NSString alloc] initWithData:out_data encoding:*encode];
        [out_string autorelease];
        NSLog(@"out:\n%@\n", out_string);
        */

        // stderr
        /*
        err_data = [err_file readDataToEndOfFile];
        err_string = [[NSString alloc] initWithData:err_data encoding:*encode];
        [err_string autorelease];
        NSLog(@"err:\n%@\n", err_string);
        */

    }

    @catch (NSString* err) {
        NSString* err_string = [NSString stringWithFormat:
                                @"error: [gpg signkey] %@", err];
        saved_err = [err retain];
        @throw err_string;
    }

    @catch (id err) {
        saved_err = [err retain];
        @throw err;
    }
    @finally {
        [out_file closeFile];
        [err_file closeFile];
        [task release];
        [pool drain];
        [saved_err autorelease];
    }

    //NSLog(@"ret:%d\n", ret);
    if (ret == 0) {
        return true;
    } else {
        return false;
    }

}


- (BOOL)delsig:(NSString*)keyuid :(NSString*)siguid
{
    id pool = [NSAutoreleasePool new];
    id saved_err = nil;

    const NSStringEncoding* encode;

    NSPipe* in_pipe = nil;;
    NSFileHandle* in_file = nil;
    NSData* in_data = nil;

    //NSPipe* com_pipe = nil;;
    //NSFileHandle* com_file = nil;
    //NSData* com_data = nil;

    NSPipe* out_pipe = nil;
    NSFileHandle* out_file = nil;
    //NSData* out_data = nil;

    NSPipe* err_pipe = nil;
    NSFileHandle* err_file = nil;
    NSData* err_data = nil;

    //NSPipe* stat_pipe = nil;
    //NSFileHandle* stat_file = nil;
    //NSData* stat_data = nil;

    NSTask* task = nil;

    //NSString* out_string = nil;
    NSString* err_string = nil;
    //NSString* stat_string = nil;

    NSMutableString* buf_str = [NSMutableString new];

    int ret = 1;
    int valid_count = 0;

    NSArray* ret_array;
    NSEnumerator* line_enum;

    @try{

        // input pipe
        in_pipe = [NSPipe pipe];
        in_file = [in_pipe fileHandleForWriting];

        // output pipe
        out_pipe = [NSPipe pipe];
        out_file = [out_pipe fileHandleForReading];

        // error pipe
        err_pipe = [NSPipe pipe];
        err_file = [err_pipe fileHandleForReading];

        // command pipe
        //com_pipe = [NSPipe pipe];
        //com_file = [com_pipe fileHandleForWriting];

        // status pipe
        //stat_pipe = [NSPipe pipe];
        //stat_file = [stat_pipe fileHandleForReading];

        // in
        NSString* inFD = nil;
        inFD = [NSString stringWithFormat:@"%d", [in_file fileDescriptor]];

        // out
        NSString* outFD = nil;
        outFD = [NSString stringWithFormat:@"%d", [out_file fileDescriptor]]; 

        // error
        NSString* errFD = nil;
        errFD = [NSString stringWithFormat:@"%d", [err_file fileDescriptor]];

        /*
        // command
        NSString* comFD = nil;
        comFD = [NSString stringWithFormat:@"%d", [com_file fileDescriptor]];
        */

        /*
        // stat
        NSString* statFD = nil;
        statFD = [NSString stringWithFormat:@"%d", [stat_file fileDescriptor]]; 
        */


        //NSLog(@"in  %@\n", inFD);
        //NSLog(@"out %@\n", outFD);
        //NSLog(@"err %@\n", errFD);
        //NSLog(@"com %@\n", comFD);
        //NSLog(@"stat%@\n", statFD);

        NSMutableArray* args;
        args = [NSMutableArray array];
        [args addObject:@"-q"];
        [args addObject:@"--no-tty"];
        [args addObject:@"--with-colons"];
        [args addObject:@"--homedir"];
        [args addObject:gpgDir];
        [args addObject:@"--command-fd"];
        [args addObject:@"0"];
        [args addObject:@"--status-fd"];
        [args addObject:@"2"];
        //[args addObject:@"--logger-fd"];
        //[args addObject:@"2"];
        //[args addObject:@"--attribute-fd"];
        //[args addObject:@"2"];
        [args addObject:@"--edit-key"];
        [args addObject:keyuid];
        //[args addObject:@"--"];


        [buf_str appendString:@"uid 1\n"];
        [buf_str appendString:@"delsig\n"];
        id keyuid_list = [self signedlist];
        id siguid_list = [keyuid_list valueForKey:keyuid];
        int count = 0;
        while (1) {
            count++;
            id num = [NSNumber numberWithInt:count];
            id siguid_each = [siguid_list objectForKey:num];
            if (siguid_each == nil) {
                break;
            }
            if ([siguid compare:siguid_each] == NSOrderedSame) {
                [buf_str appendString:@"y\n"];
                if ([siguid compare:keyuid] == NSOrderedSame) {
                    [buf_str appendString:@"y\n"];
                    valid_count++;
                }
                valid_count++;
            }
            else if ([@"unknown" compare:siguid_each] == NSOrderedSame) {
                [buf_str appendString:@"y\n"];
                valid_count++;
            }
            else {
                [buf_str appendString:@"N\n"];
                valid_count++;
            }
        }
        [buf_str appendString:@"save\n"];
        //NSLog(@"valid_count:%d\n", valid_count);
        //NSLog(@"%@\n", buf_str);

        // task
        NSTask* task;
        task = [[NSTask alloc] init];
        [task setLaunchPath:gpgExe];
        //[task setLaunchPath:@"/tmp/a.out"];
        [task setStandardInput:in_pipe];
        [task setStandardOutput:out_pipe];
        [task setStandardError:err_pipe];
        [task setArguments:args];
        [task launch];

        // encoding
        encode = [NSString availableStringEncodings];

        // stdin
        in_data = [buf_str dataUsingEncoding:*encode];
        [in_file writeData:in_data];
        [in_file closeFile];

        // command
        /*
        com_data = [buf_str dataUsingEncoding:*encode];
        [com_file writeData:com_data];
        [com_file closeFile];
        */

        [task waitUntilExit];
        ret = [task terminationStatus];

        // stat
        /*
        stat_data = [stat_file availableData];
        stat_string = [[NSString alloc] initWithData:out_data encoding:*encode];
        [stat_string autorelease];
        NSLog(@"stat:\n%@\n", stat_string);
        */

        // stdout
        /*
        //out_data = [out_file availableData];
        out_data = [out_file readDataToEndOfFile];
        out_string = [[NSString alloc] initWithData:out_data encoding:*encode];
        [out_string autorelease];
        NSLog(@"out:\n%@\n", out_string);
        */

        // stderr
        err_data = [err_file readDataToEndOfFile];
        err_string = [[NSString alloc] initWithData:err_data encoding:*encode];
        [err_string autorelease];
        //NSLog(@"err:\n%@\n", err_string);

        ret_array = [err_string componentsSeparatedByString:@"\n"]; 
        line_enum = [ret_array objectEnumerator];
        #ifdef __MACH__
        for (id line_element in line_enum) {
        #else
        id line_element;
        while (line_element = [line_enum nextObject]) {
        #endif
            if ([line_element hasSuffix:@".delsig.valid"]) {
                valid_count--;

            }
            if ([line_element hasSuffix:@".delsig.selfsig"]) {
                valid_count--;
            }
        }
    }

    @catch (NSString* err) {
        NSString* err_string = [NSString stringWithFormat:
                                @"error: [gpg delsig] %@", err];
        saved_err = [err retain];
        @throw err_string;
    }

    @catch (id err) {
        saved_err = [err retain];
        @throw err;
    }
    @finally {
        //[stat_file closeFile];
        [out_file closeFile];
        [err_file closeFile];
        [task release];
        [pool drain];
        [saved_err autorelease];
    }

    // maybe ... checking really is deleted ... ----------
    // processing.....
    // ----------------------------------------------------

    //NSLog(@"ret:%d\n", ret);
    //NSLog(@"valid_count:%d\n", valid_count);
    if (ret == 0) {
        if (valid_count == 0) {
            return true;
        } else {
        return false;
        }
    } else {
        return false;
    }

}

- (BOOL)delkey:(NSString*)uid
{
    id pool = [NSAutoreleasePool new];
    id saved_err = nil;

    NSPipe* out_pipe = nil;
    NSFileHandle* out_file = nil;

    NSPipe* err_pipe = nil;
    NSFileHandle* err_file = nil;

    NSTask* task = nil;

    int ret;

    @try{

        out_pipe = [NSPipe pipe];
        out_file = [out_pipe fileHandleForReading];

        err_pipe = [NSPipe pipe];
        err_file = [err_pipe fileHandleForReading];


        NSMutableArray* args;
        args = [NSMutableArray array];
        [args addObject:@"--homedir"];
        [args addObject:gpgDir];
        [args addObject:@"--batch"];
        [args addObject:@"--yes"];
        [args addObject:@"--delete-keys"];
        [args addObject:uid];


        task = [[NSTask alloc] init];
        [task setLaunchPath:gpgExe];
        [task setStandardOutput:out_pipe];
        [task setStandardError:err_pipe];
        [task setArguments:args];
        [task launch];

        [task waitUntilExit];
        ret = [task terminationStatus];

    }

    @catch (NSString* err) {
        NSString* err_string = [NSString stringWithFormat:
                                @"error: [gpg delkey] %@", err];
        saved_err = [err retain];
        @throw err_string;
    }

    @catch (id err) {
        saved_err = [err retain];
        @throw err;
    }
    @finally {
        [out_file closeFile];
        [err_file closeFile];
        [task release];
        [pool drain];
        [saved_err autorelease];
    }

    if (ret == 0) {
        return true;
    } else {
        return false;
    }

}

- (BOOL)genkey
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
            "PrisonKey");
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

    NSPipe* in_pipe;
    NSFileHandle* in_file;
    in_pipe = [NSPipe pipe];
    in_file = [in_pipe fileHandleForWriting];

    /*
    NSPipe* out_pipe;
    NSFileHandle* out_file;
    out_pipe = [NSPipe pipe];
    out_file = [out_pipe fileHandleForReading];
    */

    NSPipe* err_pipe;
    NSFileHandle* err_file;
    err_pipe = [NSPipe pipe];
    err_file = [err_pipe fileHandleForReading];

    NSTask* task;
    task = [[NSTask alloc] init];
    /*
    if (gpgDir != nil) {
        [task setCurrentDirectoryPath:gpgDir];
    }
    */
    [task setLaunchPath:gpgExe];
    [task setStandardInput:in_pipe];
    //[task setStandardOutput:out_pipe];
    [task setStandardError:err_pipe];
    [task setArguments:args];
    [task launch];

    const NSStringEncoding* encode;
    encode = [NSString availableStringEncodings];

    NSData* data;
    data = [params dataUsingEncoding:*encode];
    [in_file writeData:data];
    [in_file closeFile];

    // close files
    //[out_file closeFile];
    [err_file closeFile];

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
    int valid = 0;
    int trust = 0;
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
    NSFileHandle* in_file;
    in_file = [in_pipe fileHandleForWriting];

    // output pipe
    NSPipe* out_pipe;
    out_pipe = [NSPipe pipe];
    NSFileHandle* out_file;
    out_file = [out_pipe fileHandleForReading];

    // error pipe
    NSPipe* err_pipe;
    err_pipe = [NSPipe pipe];
    NSFileHandle* err_file;
    err_file = [err_pipe fileHandleForReading];

    // task
    NSTask* task;
    task = [[NSTask alloc] init];
    [task setLaunchPath:gpgExe];
    [task setStandardInput:in_pipe];
    [task setStandardOutput:out_pipe];
    [task setStandardError:err_pipe];
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
    [in_file writeData:in_data];
    [in_file closeFile];


    int ret;
    [task waitUntilExit];
    ret = [task terminationStatus];
    //NSLog(@"ret:%d", ret);

    // stdout
    NSData* out_data = nil;
    out_data = [out_file readDataToEndOfFile];
    NSString* key_string = nil;
    key_string = [[NSString alloc] initWithData:out_data encoding:*encode];

    // file close
    [out_file closeFile];
    [err_file closeFile];

    //NSLog(@"data_length:%d\n", [out_data length]);
    //NSLog(@"\n%@\n", out_data);

    //NSLog(@"string_length:%d\n", [out_string length]);
    //NSLog(@"\n%@\n", out_string);


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

        #ifdef __MACH__
        for (id line_element in line_enum) {
        #else
        id line_element;
        while (line_element = [line_enum nextObject]) {
        #endif

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
            #ifdef __MACH__
            for (id word_element in word_enum) {
            #else
            id word_element;
            while (word_element = [word_enum nextObject]) {
            #endif
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
        //NSLog(@"internal error:%@\n", err);
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

        #ifdef __MACH__
        for (id line_element in line_enum) {
        #else
        id line_element;
        while (line_element = [line_enum nextObject]) {
        #endif

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
            #ifdef __MACH__
            for (id word_element in word_enum) {
            #else
            id word_element;
            while (word_element = [word_enum nextObject]) {
            #endif
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
        //NSLog(@"internal error:%@\n", err);
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
        
        #ifdef __MACH__
        for (id line_element in line_enum) {
        #else
        id line_element;
        while (line_element = [line_enum nextObject]) {
        #endif

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
            #ifdef __MACH__
            for (id word_element in word_enum) {
            #else
            id word_element;
            while (word_element = [word_enum nextObject]) {
            #endif
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
        //NSLog(@"internal error:%@\n", err);
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

- (NSDictionary*)_parse_list_sig:(NSString*)sig
{
    id pool = [NSAutoreleasePool new];
    id saved_err = nil;

    NSArray* line_array = nil;
    NSArray* word_array = nil;
    NSDictionary* ans_dict = nil;
    NSEnumerator* line_enum = nil;
    NSEnumerator* word_enum = nil;

    NSMutableDictionary* ans_mdict = [NSMutableDictionary new];

    //NSString* sig = [NSString stringWithContentsOfFile:@"sig.txt"];
    //NSLog(@"%@\n", pub);

    @try {
        if (sig == nil) @throw @"nothing input sig_string";

        line_array = [sig componentsSeparatedByString:@"\n"];
        line_enum = [line_array objectEnumerator];
        #ifdef __MACH__
        for (id line_element in line_enum) {
        #else
        id line_element;
        while (line_element = [line_enum nextObject]) {
        #endif

            if ([line_element length] == 0) {
                continue;
            }

            // - line reconstruction process ------------
            //NSLog(@"%@\n", line_element);

            int counter = 0;
            NSNumber* ns_counter = nil;
            NSString* user = nil;
            NSMutableDictionary* sig_dict = [NSMutableDictionary new];
            NSComparisonResult compResult;

            compResult = [line_element compare:@"pub"
                                       options:NSCaseInsensitiveSearch
                                         range:NSMakeRange(0,[@"pub" length])];

            if (compResult == NSOrderedSame) {
                line_element = [line_enum nextObject];

                word_array = [line_element componentsSeparatedByString:@" "];
                word_enum = [word_array objectEnumerator];
                #ifdef __MACH__
                for (id word_element in word_enum) {
                #else
                id word_element;
                while (word_element = [word_enum nextObject]) {
                #endif
                    // - word reconstruction process ------------
                    //NSLog(@"%@\n", word_element);
                    if ([word_element length] == 0) {
                        continue;
                    }
                    if ([word_element characterAtIndex:0] == '<') {
                        int start = 1;
                        int end = [word_element length] - 2;
                        NSRange range = NSMakeRange(start, end);
                        user = [word_element substringWithRange:range];
                        break;
                    }
                    // ------------------------------------------
                }

                while (1) {
                    line_element = [line_enum nextObject];
                    if ([line_element length] == 0) {
                        break;
                    }
                    compResult = [line_element compare:@"sub"
                                               options:NSCaseInsensitiveSearch
                                                 range:NSMakeRange(0,[@"sub" length])];
                    if (compResult == NSOrderedSame) {
                        break;
                    }
                    counter++;
                    ns_counter = [NSNumber numberWithInt:counter];
                    word_array = [line_element componentsSeparatedByString:@" "];
                    word_enum = [word_array objectEnumerator];
                    #ifdef __MACH__
                    for (id word_element in word_enum) {
                    #else
                    id word_element;
                    while (word_element = [word_enum nextObject]) {
                    #endif
                        // - word reconstruction process ------------
                        //NSLog(@"%@\n", word_element);
                        if ([word_element length] == 0) {
                            continue;
                        }
                        if ([word_element characterAtIndex:0] == '<') {
                            int start = 1;
                            int end = [word_element length] - 2;
                            NSRange range = NSMakeRange(start, end);
                            word_element = [word_element substringWithRange:range];
                            [sig_dict setObject:word_element forKey:ns_counter];
                        }
                        if ([word_element characterAtIndex:0] == '[') {
                            [sig_dict setObject:@"unknown" forKey:ns_counter];
                        }
                        // ------------------------------------------
                    }
                }
                [ans_mdict setObject:sig_dict forKey:user];
            }
        }
        //NSLog(@"per dict:%@\n", ans_mdict);
        ans_dict = [[NSDictionary alloc] initWithDictionary:ans_mdict];

    }
    @catch (NSString* err) {
        NSString* err_string = [NSString stringWithFormat:
                                   @"error: [gpg _parse_list_sig] %@", err];
        saved_err = [err retain];
        @throw err_string;
    }
    @catch (id err) {
        //NSLog(@"internal error:%@\n", err);
        saved_err = [err retain];
        @throw err;
    }
    @finally {
        [pool drain];
        [saved_err autorelease];
    }
    [ans_dict autorelease];
    return ans_dict;
}



   
- (int)genrkey
{
    return false;
}


@end

#endif //__PRISON_GPGME_H_
