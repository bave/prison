
CC=g++
MAKE=make
MIG=/usr/bin/mig

#CFLAGS= -Wall -x objective-c++ -framework Foundation -lresolv -lstdc++ -fobjc-gc-only
#CFLAGS= -Wall -x objective-c++ -framework Foundation -lresolv -lstdc++ -fobjc-gc
#CFLAGS= -v -Wall -x objective-c++ -framework Cocoa,AppKit -lresolv -lstdc++

CFLAGS= -O3 -g -Wall -x objective-c++ -framework Cocoa

# for gpg.h
CFLAGS+= -I/opt/local/include -lgpgme -lgpg-error

# for utils.h and name.h
CFLAGS+= -lresolv -lstdc++

all: caged prison

prison:
	$(MAKE) -C ds
	${CC} ${CFLAGS} -o bin/prison main.m

caged:
	$(MAKE) -C cage

.PHONY: clean
clean: clean_ds clean_current

clean_current:
	rm ./bin/prison &
	rm -rf ./bin/prison.dSYM &

clean_ds:
	$(MAKE) -C ds clean
	$(MAKE) -C cage clean
