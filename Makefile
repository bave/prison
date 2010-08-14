
CC=g++
MAKE=make
MIG=/usr/bin/mig

#CFLAGS= -Wall -x objective-c++ -framework Foundation -lresolv -lstdc++ -fobjc-gc-only
#CFLAGS= -Wall -x objective-c++ -framework Foundation -lresolv -lstdc++ -fobjc-gc
#CFLAGS= -v -Wall -x objective-c++ -framework Cocoa,AppKit -lresolv -lstdc++

CFLAGS= -g -Wall -x objective-c++ -framework Cocoa,AppKit

# for gpg.h
CFLAGS+= -I/opt/local/include -lgpgme -lgpg-error

# for utils.h and name.h
CFLAGS+= -lresolv -lstdc++

all:raprins

raprins:
	$(MAKE) -C ds
	${CC} ${CFLAGS} -o bin/raprins main.m
	$(MAKE) -C cage

.PHONY: clean
clean: clean_ds clean_current

clean_current:
	rm ./bin/raprins &
	rm -rf ./bin/raprins.dSYM &

clean_ds:
	$(MAKE) -C ds clean
	$(MAKE) -C cage clean
