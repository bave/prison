
CC=g++
MAKE=make
MIG=/usr/bin/mig
#CFLAGS= -Wall -x objective-c++ -framework Foundation -lresolv -lstdc++ -fobjc-gc-only
#CFLAGS= -Wall -x objective-c++ -framework Foundation -lresolv -lstdc++ -fobjc-gc
#CFLAGS= -v -Wall -x objective-c++ -framework Cocoa,AppKit -lresolv -lstdc++
CFLAGS= -g -Wall -x objective-c++ -framework Cocoa,AppKit -lresolv -lstdc++

all:raprins

raprins:
	$(MAKE) -C ds
	$(MAKE) -C cage
	${CC} ${CFLAGS} -o ppnat ppnat.m

.PHONY: clean
clean: clean_ds clean_current

clean_current:
	rm ./ppnat &
	rm -rf ./ppnat.dSYM &

clean_ds:
	$(MAKE) -C ds clean
	$(MAKE) -C cage clean
