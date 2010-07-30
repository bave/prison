#SDK=/Developer/SDKs/MacOSX10.6.sdk
#CFLAGS= -isysroot ${SDK} -x objective-c++ -framework Foundation -Wall
#LDFLAGS= -isysroot ${SDK} -syslibroot,${SDK}
#ENVP= MACOSX_DEPLOYMENT_TARGET=10.6

CC=g++

PLAT := $(shell uname)

ifeq "$(PLAT)" "Darwin"
CFLAGS= -g -Wall 
CFLAGS+= -x objective-c++ -framework Cocoa,AppKit
CFLAGS+= -lresolv -lstdc++
endif

ifeq "$(PLAT)" "Linux"
CFLAGS= -lobjc
CFLAGS+= -lgnustep-base
CFLAGS+= -I/usr/include/GNUstep
CFLAGS+= -fconstant-string-class=NSConstantString
CFLAGS+= -fobjc-exceptions
endif


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

gpg: gpg.m
	${CC} ${CFLAGS} gpg.m -lgpgme


clean:
	rm ./a.out
