all:cage cli

cage: cage.cpp
	g++ -O3 -fomit-frame-pointer -Wall -levent -lcage -o ../bin/cage cage.cpp base64.cpp
	#g++ -g -Wall -levent -lcage -o ../bin/cage cage.cpp

cli: cli.m
	gcc -O3 -fomit-frame-pointer -Wall -o ../bin/cli cli.m
	#gcc -g -Wall -o ../bin/cli cli.m

base64: base64.cpp base64.hpp base64_test.cpp
	g++ -O3 -fomit-frame-pointer -Wall -o base64_test base64.cpp base64_test.cpp

un: un.c
	g++ -O3 -fomit-frame-pointer -Wall -o un un.c

.PHONY: clean
clean: clean_current

clean_current:
	rm base64_test &
	rm un &
	rm ../bin/cli &
	rm ../bin/cage &
	rm -rf ../bin/cli.dSYM &
	rm -rf ../bin/cage.dSYM &

