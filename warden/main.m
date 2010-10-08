#import <Cocoa/Cocoa.h>

#include "warden.h"
#include "../gpg.h"

void get_path()
{
    id pool = [NSAutoreleasePool new];
    NSString* app_path = [[NSBundle mainBundle] bundlePath];
    NSLog(@"app_path:%@", app_path);
    NSString* real_path =[app_path stringByAppendingPathComponent:@"Contents/MacOS/warden"]; 
    NSLog(@"real_path:%@", real_path);
    unsigned int real_path_length = [real_path lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
    [pool drain];
    return;
}


int main(int argc, char** argv)
{
    if (argc == 3) {
        // ./warden [command] [id]
        return 0;
    }

    else if (argc == 1) {
        return NSApplicationMain(argc,  (const char **) argv);
    }

    else {
        return 1;
    }
}
