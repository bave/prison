#!/bin/sh
./seed.sh
#!/bin/sh
./seed.sh

./bootstrap.sh dev 12001 12040 /tmp/boot1
./bootstrap.sh dev 12041 12080 /tmp/boot2
./bootstrap.sh dev 12081 12120 /tmp/boot3
./bootstrap.sh dev 12121 12060 /tmp/boot4
./bootstrap.sh dev 12161 12200 /tmp/boot5
./bootstrap.sh dev 12201 12240 /tmp/boot6
./bootstrap.sh dev 12241 12280 /tmp/boot7
./bootstrap.sh dev 12281 12320 /tmp/boot8

./put.sh dev 12001 12040 /tmp/boot1
./put.sh dev 12041 12080 /tmp/boot2
./put.sh dev 12081 12120 /tmp/boot3
./put.sh dev 12121 12060 /tmp/boot4
./put.sh dev 12161 12200 /tmp/boot5
./put.sh dev 12201 12240 /tmp/boot6
./put.sh dev 12241 12280 /tmp/boot7
./put.sh dev 12281 12320 /tmp/boot8

