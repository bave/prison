all:cage cli un

cage: cage.cpp
	#g++ -O3 -fomit-frame-pointer -Wall -levent -lcage -o ../bin/cage cage.cpp
	g++ -g -Wall -levent -lcage -o ../bin/cage cage.cpp 

cli: cli.m
	gcc -O3 -fomit-frame-pointer -Wall -o ../bin/cli cli.m
	#gcc -g -Wall -o ../bin/cli cli.m

un: un.cpp
	g++ -O3 -fomit-frame-pointer -Wall -o ../bin/un un.cpp -lpthread

.PHONY: clean
clean: clean_current

clean_current:
	rm base64_test &
	rm un &
	rm ../bin/cli &
	rm ../bin/cage &
	rm ../bin/un &
	rm -rf ../bin/cli.dSYM &
	rm -rf ../bin/cage.dSYM &

