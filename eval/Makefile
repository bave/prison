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

key: key.m
	${CC} ${CFLAGS} -o key key.m -lgpgme -lgpg-error

sign: sign.m
	${CC} ${CFLAGS} -o sign sign.m -lgpgme -lgpg-error

export: export.m
	${CC} ${CFLAGS} -o export export.m -lgpgme -lgpg-error

clean:
	rm ./key &
	rm -rf ./key.dSYM &
	rm ./sign &
	rm -rf ./sign.dSYM &
	rm ./export &
	rm -rf ./export.dSYM &

