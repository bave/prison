
MIG=/usr/bin/mig

mig: flushcache

flushcache: flushcache.m
	${MIG} ./DSlibinfoMIG.defs
	${CC} -O3 -fomit-frame-pointer -lstdc++ -o flushcache flushcache.m

clean: genclean
	rm DSlibinfoMIGServer.c DSlibinfoMIGUser.c DSlibinfoMIG.h

genclean:
	rm ./flushcache &


