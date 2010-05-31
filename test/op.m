#import <Cocoa/Cocoa.h>
#include "../op.h"
#include <unistd.h>
#include <nameser.h>
#include <resolv.h>

#include <sys/socket.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>


int main(int argc, char** argv) {
    id pool = [[NSAutoreleasePool alloc] init];

    int divertFD;
    struct sockaddr_in sin_divert;

    divertFD = socket(AF_INET, SOCK_RAW, IPPROTO_DIVERT);
    if (divertFD < 0) {
        perror("socket");
        exit(1);
    }

    memset(&sin_divert, 0,sizeof(sin_divert));
    sin_divert.sin_family = AF_INET;
    sin_divert.sin_port   = htons(53);

    if (bind(divertFD, (SA*)&sin_divert, sizeof(sin_divert)) < 0) {
        perror("bind");
        exit(1);
    }

    NSOperationQueue *queue;
    queue = [[NSOperationQueue alloc] init];
    [queue setMaxConcurrentOperationCount:10];
    [queue setName:@"testQueue"];

    Operation *op = [[[Operation alloc] init] autorelease];
    op.socketFD=divertFD;
    op.selector=OP_NAME;
    [queue addOperation:op];
    sleep(10);
    [pool drain];
    return 0;
}
