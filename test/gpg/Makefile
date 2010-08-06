all:
	ls -a

import:
	cat takano.gpg | gpg --homedir ./ --import

export:
	gpg --homedir ./ --armor --export bave

list:
	gpg --homedir ./ --list-keys

verify:
	gpg --homedir ./ --verify Makefile.asc

sign:
	gpg --homedir ./ -sa Makefile

.PHONY: clean
clean: clean_current

clean_current:
	rm ./pubring.gpg~ &
	rm ./pubring.gpg &
	rm ./secring.gpg &
	rm ./trustdb.gpg &
	rm ./random_seed &


