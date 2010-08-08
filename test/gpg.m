#import <Cocoa/Cocoa.h>

#include "../gpg.h"
#include "../utils.h"


int main()
{
    id pool = [NSAutoreleasePool new];

    int ret;

    NSString* home = nil;
    home = [NSString stringWithFormat:@"%@%@", currentdir(), @"gpg/"];
    NSLog(@"home_dir  :%@\n", home);

    NSString* sig = nil;
    NSString* sigpath = nil;
    sigpath = [NSString stringWithFormat:@"%@%@", home, @"hage/message.asc"];
    #ifdef LEOPARD
    sig = [NSString stringWithContentsOfFile:sigpath
                                    encoding:NSUTF8StringEncoding error:nil];
    #else
    sig = [NSString stringWithContentsOfFile:sigpath];
    #endif
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

            [gpg setPasswd:@"test"];
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
        NSLog(@"verify message:\n%@\n", [gpg verify:sig]);
        NSLog(@"valid:%d\n", [gpg getValid]);
        NSLog(@"trust:%d\n", [gpg getTrust]);
    }
    @catch (id e) {
        NSLog(@"%@\n", e);
    }

    // export-key 
    @try {
        // all keyring
        //NSLog(@"all keyring \n%@\n", [gpg export:nil]);
        // user pubring
        NSLog(@"export keyring\n%@\n", [gpg export:@"test"]);
    }
    @catch (id e) {
        NSLog(@"%@\n", e);
    }

    // import-key
    @try {
        NSLog(@"import message:%d\n", [gpg import:key]);
    }
    @catch (id e){
        NSLog(@"%@\n", e);
    }

    // sign
    @try {
        [gpg setPasswd:@"test"];
        NSLog(@"sign message\n%@\n", [gpg sign:@"tetete\n"]);
    }
    @catch (id e){
        NSLog(@"%@\n", e);
    }

    //encryptForce
    @try {
        NSLog(@"force encrypt message\n%@\n", [gpg encryptForce:@"tetete" :@"hage"]);
    }
    @catch (id e){
        NSLog(@"%@\n", e);
    }

    //encrypt
    @try {
        NSLog(@"encrypt mesg\n%@\n", [gpg encrypt:@"tetete" :@"test"]);
    }
    @catch (id e){
        NSLog(@"%@\n", e);
    }

    // decrypt
    @try {
        NSLog(@"decrypt mesg\n%@\n", [gpg decrypt:[gpg encrypt:@"tetete" :@"test"]]);
    }
    @catch (id e){
        NSLog(@"%@\n", e);
    }

    // ----- gpg version independent ------------------------------------------
    // check
    // gpg2 (GnuPG) 2.0.14
    // gpg  (GnuPG) 1.4.10

    // throw-keyids
    @try {
        NSLog(@"%@\n",[gpg throw:key]);
    }
    @catch (id e){
        NSLog(@"%@\n", e);
    }


    // userlist
    @try {
        NSLog(@"userlist mesg\n%@\n", [gpg userlist]);
    }
    @catch (id e){
        NSLog(@"%@\n", e);
    }

    [pool drain];
    return 0;
}
