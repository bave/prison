#import <Cocoa/Cocoa.h>

#include "../gpg.h"
#include "../utils.h"


int main()
{
    id pool = [NSAutoreleasePool new];

    int ret;

    NSString* home;
    #ifdef __MACH__
    home = [NSString stringWithString:@"/Users/bayve/work/gpg/"];
    #elif __linux__
    home = [NSString stringWithString:@"/home/t-inoue/work/gpg/"];
    #endif
    NSLog(@"home_dir  :%@\n", home);

    NSString* sig;
    NSString* sigpath;
    sigpath = [NSString stringWithFormat:@"%@%@", home, @"Makefile.asc"];
    NSLog(@"sign_file :%@\n",sigpath);

    NSString* key;
    NSString* keypath;
    keypath = [NSString stringWithFormat:@"%@%@", home, @"takano.gpg"];
    NSLog(@"import_key:%@\n",keypath);

    GPGME* gpg;
    //gpg = [GPGME new];
    gpg = [GPGME alloc];
    [gpg initWithDir:home];
    [gpg autorelease];

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
                            :@"1024"
                            :@"RSA"
                            :@"1024"
                            :@"bave"
                            :@"bave@aris"];

            [gpg setPass:@"bave"];
            NSLog(@"set_params\n");

            ret = [gpg genkey];

            NSLog(@"genOK?:%d\n", ret);
            NSLog(@"end_genkey\n");
        }
        @catch (id e) {
            NSLog(@"fail_genkey:%@\n", e);
        }
    }

    // verify test
    @try {
        /*
        // 10.5 later...
        NSError*  error;
        error = nil;
        sig = [NSString stringWithContentsOfFile:sigpath
                                        encoding:NSUTF8StringEncoding
                                           error:&error];
        // or
        sig = [NSString stringWithContentsOfFile:sigpath
                                        encoding:NSUTF8StringEncoding
                                           error:nil];
        */
        // before 10.4 or linux objc
        sig = [NSString stringWithContentsOfFile:sigpath];

        //NSLog(@"sig:\n%@\n",sig);
        NSLog(@"valid:%d\n", [gpg verify:sig]);
    }
    @catch (id e) {
        NSLog(@"%@\n", e);
    }

    // export-key 
    @try {
        // all keyring
        //NSLog(@"all keyring \n%@\n", [gpg export:nil]);

        // user pubring
        //NSLog(@"uid keyring \n%@\n", [gpg export:@"bave"]);
    }
    @catch (id e) {
        NSLog(@"%@\n", e);
    }

    // import-key
    @try {
        key = [NSString stringWithContentsOfFile:keypath];
        NSLog(@"import:%d\n", [gpg import:key]);
    }
    @catch (id e){
        NSLog(@"%@\n", e);
    }

    // throw-keyids
    @try {
        NSLog(@"%@\n",[gpg throw:key]);
    }
    @catch (id e){
        NSLog(@"%@\n", e);
    }


    [pool drain];
    return 0;
}
