#import <Cocoa/Cocoa.h>

#include "../gpg.h"
#include "../utils.h"


int main()
{
    id pool = [NSAutoreleasePool new];

    int ret;

    NSString* home = nil;
    home = [NSString stringWithFormat:@"%@%@", currentdir(), @"../gpg/"];
    NSLog(@"home_dir  :%@\n", home);

    NSString* sig = nil;
    NSString* sigpath = nil;
    sigpath = [NSString stringWithFormat:@"%@%@", home, @"hage/hage.pub.asc"];
    // 10.5 later...
    //NSError*  error;
    //error = nil;
    //sig = [NSString stringWithContentsOfFile:sigpath
    //                                encoding:NSUTF8StringEncoding
    //                                   error:&error];
    // or
    //sig = [NSString stringWithContentsOfFile:sigpath
    //                                encoding:NSUTF8StringEncoding
    //                                   error:nil];
    // before 10.4 or linux objc
    sig = [NSString stringWithContentsOfFile:sigpath];
    NSLog(@"sign_file :%@\n",sigpath);
    NSLog(@"sign_file :\n%@\n",sig);

    NSString* key = nil;
    NSString* keypath = nil;
    keypath = [NSString stringWithFormat:@"%@%@", home, @"hage/hage.pub"];
    key = [NSString stringWithContentsOfFile:keypath];
    NSLog(@"keypath:%@\n",keypath);
    NSLog(@"key:\n%@\n",key);

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
                            :@"test"
                            :@"test@aris"];

            [gpg setPass:@"test"];
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
        NSLog(@"test user keyring export \n%@\n", [gpg export:@"test"]);
    }
    @catch (id e) {
        NSLog(@"%@\n", e);
    }

    // import-key
    @try {
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
