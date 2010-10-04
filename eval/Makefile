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

all: key sign export

key: key.m
	${CC} ${CFLAGS} -o key key.m -lgpgme -lgpg-error

sign: sign.m
	${CC} ${CFLAGS} -o sign sign.m -lgpgme -lgpg-error

export: export.m
	${CC} ${CFLAGS} -o export export.m -lgpgme -lgpg-error

siocgifaddr:siocgifaddr.c
	${CC} ${CFLAGS} -o siocgifaddr siocgifaddr.c

measure:measure.m
	${CC} ${CFLAGS} -o measure measure.m

random:
	rm /dev/random
	ln -s /dev/urandom /dev/ramdom

rerandom:
	mknod -m 644 /dev/random c 1 8
	mknod -m 644 /dev/urandom c 1 9
	chown root:root /dev/random /dev/urandom

clean:
	rm ./key &
	rm -rf ./key.dSYM &
	rm ./sign &
	rm -rf ./sign.dSYM &
	rm ./export &
	rm -rf ./export.dSYM &
	rm ./siocgifaddr &
	rm -rf ./siocgifaddr.dSYM &
	rm ./measure &
	rm -rf ./measure.dSYM &
	./clean.sh

