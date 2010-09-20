#import <Cocoa/Cocoa.h>

#include "../gpg.h"
#include "../utils.h"

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

    NSString* home = [NSString stringWithFormat:@"%@%s/", currentdir(), argv[1]];
    NSString* user = [NSString stringWithFormat:@"%s", argv[1]];
    NSString* uid = [NSString stringWithFormat:@"%@@prison/", user];

    NSString* value_export = nil;

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
        // export-key 
        @try {
             value_export = [GPGME trimContentFromArmor:[gpg exportPubring:uid]];
        }
        @catch (id e) {
            NSLog(@"%@\n", e);
        }
    }

    NSString* file_path = [NSString stringWithFormat:@"%@%s", home, "export.txt"];
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
    data = [NSData dataWithBytes:[value_export UTF8String] length:[value_export length]];

    [file writeData:data];
    [file synchronizeFile];
    [file closeFile];

    /*
    */

    [pool drain];
    return 0;
}
