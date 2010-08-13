
CC=g++
CFLAGS= -g -Wall -levent -lcage

all:cage

cage:
	${CC} ${CFLAGS} -o cage cage.cpp 

.PHONY: clean
clean: clean_current

clean_current:
	rm ./a.out &
	rm ./cage &
	rm -rf ./a.out.dSYM &
	rm -rf ./cage.dSYM &

