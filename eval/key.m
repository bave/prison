#import <Cocoa/Cocoa.h>

#include "../gpg.h"
#include "../utils.h"

#define __PRISON__


void usage(void)
{
    printf("key [homedir]\n");
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

    int ret;
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
    if ((isSec || isPub) != true) {
        @try{
            NSLog(@"start_genkey\n");

            [gpg mkKeyParams:@"RSA"
                            :@"256"
                            :@"RSA"
                            :@"256"
                            :user
                            :uid];

            [gpg setPasswd:user];
            NSLog(@"set_params\n");

            ret = [gpg genkey];

            NSLog(@"genOK?:%d\n", ret);
            NSLog(@"end_genkey\n");
        }
        @catch (id e) {
            NSLog(@"fail_genkey:%@\n", e);
        }
    }

    [pool drain];
    return 0;
}
