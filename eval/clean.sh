#!/bin/sh

. ./config.sh

rm -f error &
i=$SPORT
while [ $i -le $EPORT ]
do
    echo $i
    rm -rf $NODE$i
    i=`expr $i + 1`
    sleep 0.1
done
