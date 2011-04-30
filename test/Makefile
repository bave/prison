#SDK=/Developer/SDKs/MacOSX10.6.sdk
#CFLAGS= -isysroot ${SDK} -x objective-c++ -framework Foundation -Wall
#LDFLAGS= -isysroot ${SDK} -syslibroot,${SDK}
#ENVP= MACOSX_DEPLOYMENT_TARGET=10.6

CC=g++

PLAT := $(shell uname)

ifeq "$(PLAT)" "Darwin"
CFLAGS= -g -Wall 
CFLAGS+= -I/opt/local/include
CFLAGS+= -x objective-c++ -framework Cocoa,AppKit
CFLAGS+= -Wno-deprecated-declarations -Wno-deprecated
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

rdp: rdp.m
	${CC} ${CFLAGS} rdp.m


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
	${CC} ${CFLAGS} kvt.m -lgpgme -lgpg-error

gpgme: gpg.m
	${CC} ${CFLAGS} gpg.m -lgpgme -lgpg-error

event: event.m
	${CC} ${CFLAGS} event.m -lgpgme -lgpg-error

rc: rc.m
	${CC} ${CFLAGS} rc.m

task: task.m
	${CC} ${CFLAGS} task.m

mgmt: mgmt.m
	${CC} ${CFLAGS} mgmt.m -lgpgme -lgpg-error

base64: ../base64.hpp base64.cpp
		g++ -O3 -fomit-frame-pointer -Wall base64.cpp

clean:
	rm ./a.out
	rm -rf ./a.out.dSYM

