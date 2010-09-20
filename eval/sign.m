#import <Cocoa/Cocoa.h>

#include "../gpg.h"
#include "../utils.h"
#include "../ni.h"

#define __PRISON__


void usage(void)
{
    printf("sign [homedir]\n");
    exit(1);
}

int main(int argc, char** argv)
{
    id pool = [NSAutoreleasePool new];

    if (argc != 2) {
        usage();
    }

    NSString* ip;

    #ifdef __MACH__

    NetInfo* ni = [NetInfo new];
    ip = [ni defaultIP4];
    NSLog(@"%@", ip);

    #else __LINUX__
    {
        int ret;
        const NSStringEncoding* encode;
        NSTask* task = nil;
        NSData* out_data = nil;
        NSString* out_string = nil;
        NSPipe* out_pipe = nil;
        NSFileHandle* out_file = nil;
        NSMutableArray* args;

        args = [NSMutableArray array];
        NSString* command = [NSString stringWithFormat:@"%@ip eth0", currentdir()];
        [args addObject:@"-c"];
        [args addObject:command];


        out_pipe = [NSPipe pipe];
        out_file = [out_pipe fileHandleForReading];

        task = [[NSTask alloc] init];
        [task setLaunchPath:@"/bin/sh"];
        //[task setStandardInput:in_pipe];
        [task setStandardOutput:out_pipe];
        //[task setStandardError:err_pipe];
        [task setArguments:args];
        [task launch];

        [task waitUntilExit];
        ret = [task terminationStatus];

        encode = [NSString availableStringEncodings];
        out_data = [out_file readDataToEndOfFile];
        out_string = [[NSString alloc] initWithData:out_data encoding:*encode];
        ip = out_string;
        NSLog(@"%@", ip);
    }
    #endif


    NSString* home = [NSString stringWithFormat:@"%@%s/", currentdir(), argv[1]];
    NSString* user = [NSString stringWithFormat:@"%s", argv[1]];
    //NSString* uid = [NSString stringWithFormat:@"%@@prison/", user];

    NSString* phrase = [NSString stringWithFormat:@"%@:NULL:NULL", ip];

    NSString* value_sign = nil;

    GPGME* gpg = [[[GPGME alloc] initWithDir:home] autorelease];

    // hasSecuring test
    int isSec;
    isSec = [gpg hasSecring];
    NSLog(@"secring:%d\n", isSec);

    // has Pubring test
    int isPub;
    isPub = [gpg hasPubring];
    NSLog(@"pubring:%d\n", isPub);

    // key generation test
    if ((isSec || isPub) == true) {
        // sign
        NSLog(@"sign");
        @try {
            [gpg setPasswd:user];
            value_sign = [GPGME trimContentFromArmor:[gpg sign:phrase]];
        }
        @catch (id e){
            NSLog(@"%@\n", e);
        }
    }

    NSString* file_path = [NSString stringWithFormat:@"%@%s", home, "sign.txt"];
    NSLog(@"%@", file_path);

    BOOL ret;
    ret = [[NSFileManager defaultManager]
                createFileAtPath:file_path
                contents:nil
                attributes:nil];

    if (!ret) {
        printf("cant create file!!\n");
        exit(1);
    }

    NSFileHandle* file = [NSFileHandle fileHandleForWritingAtPath:file_path];

    if (!file) {
        printf("cant create file handle!!\n");
        exit(1);
    }

    NSData* data = nil;
    data = [NSData dataWithBytes:[value_sign UTF8String] length:[value_sign length]];

    [file writeData:data];
    [file synchronizeFile];
    [file closeFile];

    [pool drain];
    return 0;
}
