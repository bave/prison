
CC=g++
MAKE=make
MIG=/usr/bin/mig


# for release
CFLAGS= -O3 -D_REENTRANT -fomit-frame-pointer -Wall -x objective-c++ -framework Cocoa
# for debug
#CFLAGS= -g -Wall -x objective-c++ -framework Cocoa


# addition libaray and include ---------------------
# for gpg.h
CFLAGS+= -I/opt/local/include -lgpgme -lgpg-error

# for utils.h and name.h
CFLAGS+= -lresolv -lstdc++

# garbage collection 
#CFLAGS+= -fobjc-gc-only
#CFLAGS+= -fobjc-gc
# --------------------------------------------------


# target -------------------------------------------

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
