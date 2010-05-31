
MIG=/usr/bin/mig

mig: flushcache

flushcache: flushcache.m
	${MIG} ./DSlibinfoMIG.defs
	${CC} -lstdc++ -o flushcache flushcache.m

clean: genclean
	rm DSlibinfoMIGServer.c DSlibinfoMIGUser.c DSlibinfoMIG.h

genclean:
	rm ./flushcache


