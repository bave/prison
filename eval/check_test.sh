#!/bin/sh

process=$1
div=$2
end_node="10"

num=`expr $process \* $div`
start="12001"
end=`expr 12000 + $num`

rm -f log
touch log

i=$start
h="1"
#echo $i
#echo $h
#echo $end
#echo $end_node

while [ $h -le $end_node ]
do
    while [ $i -le $end ]
    do
        ./measure muru0${h}_$i.p2p | tee -a log
        i=`expr $i + $div`
    done
    i=$start
    h=`expr $h + 1`
done
