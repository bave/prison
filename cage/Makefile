all:cage cli

cage: cage.cpp
	g++ -g -Wall -levent -lcage -o cage cage.cpp

cli: cli.m
	gcc -g -Wall -framework Cocoa,AppKIT -o cli cli.m

.PHONY: clean
clean: clean_current

clean_current:
	rm ./cli &
	rm ./cage &
	rm -rf ./cli.dSYM &
	rm -rf ./cage.dSYM &

