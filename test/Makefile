#SDK=/Developer/SDKs/MacOSX10.6.sdk
#CFLAGS= -isysroot ${SDK} -x objective-c++ -framework Foundation -Wall
#LDFLAGS= -isysroot ${SDK} -syslibroot,${SDK}
#ENVP= MACOSX_DEPLOYMENT_TARGET=10.6

CC=g++
CFLAGS= -Wall -x objective-c++ -framework Cocoa,Foundation,AppKit
#CFLAGS= -w -x objective-c++ -framework Foundation

all:
	ls

name: name.m
	${CC} ${CFLAGS} name.m

obs: obs.m
	${CC} ${CFLAGS} obs.m

fw: fw.m
	${CC} ${CFLAGS} fw.m

utils: utils.m
	${CC} ${CFLAGS} utils.m

th: th.m
	${CC} ${CFLAGS} th.m

op: op.m
	${CC} ${CFLAGS} op.m -lresolv

divert: divert.m
	${CC} ${CFLAGS} divert.m

pbuf: pbuf.m
	${CC} ${CFLAGS} pbuf.m

ni: ni.m
	${CC} ${CFLAGS} ni.m

timer: timer.m
	${CC} ${CFLAGS} timer.m

kvt: kvt.m
	${CC} ${CFLAGS} kvt.m

mgmt: mgmt.m
	${CC} ${CFLAGS} mgmt.m


clean:
	rm ./a.out
