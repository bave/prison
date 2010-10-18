#!/bin/sh

host=$1
bias=$2
num=$3

end=`expr $bias + $num`

rm -f log
touch log

i=`expr $bias + 1`
while [ $i -le $end ]
do
    ./measure $host$i.p2p | tee -a log
    i=`expr $i + 1`
done
