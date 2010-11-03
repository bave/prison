all:cage cli

cage: cage.cpp
	g++ -O3 -fomit-frame-pointer -Wall -levent -lcage -o ../bin/cage cage.cpp
	#g++ -g -Wall -levent -lcage -o ../bin/cage cage.cpp

cli: cli.m
	gcc -O3 -fomit-frame-pointer -Wall -o ../bin/cli cli.m
	#gcc -g -Wall -o ../bin/cli cli.m

.PHONY: clean
clean: clean_current

clean_current:
	rm ../bin/cli &
	rm ../bin/cage &
	rm -rf ../bin/cli.dSYM &
	rm -rf ../bin/cage.dSYM &

