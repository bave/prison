all:cage cli un

cage: cage.cpp
	g++ -O3 -fomit-frame-pointer -Wall -levent -lcage -o ../bin/cage cage.cpp

cli: cli.c
	gcc -O3 -fomit-frame-pointer -Wall -o ../bin/cli cli.c

un: un.cpp
	g++ -O3 -fomit-frame-pointer -Wall -o ../bin/un un.cpp -lpthread

.PHONY: clean
clean: clean_current

clean_current:
	rm ../bin/un &
	rm ../bin/cli &
	rm ../bin/cage &
	rm -rf ../bin/un.dSYM &
	rm -rf ../bin/cli.dSYM &
	rm -rf ../bin/cage.dSYM &

