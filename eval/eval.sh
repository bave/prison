#!/bin/sh

. ./config.sh

PER_NODE=$1
DIV=$2
if [ -z $PER_NODE ]; then
    echo "please input PER_NODE arg1"
    exit 1
fi

if [ -z $DIV ]; then
    echo "please input DIV arg2"
    exit 1
fi

i=1
START_PORT=$SPORT
while [ $i -le $DIV ]
do
    echo "./bootstrap.sh $NODE $START_PORT  $END_PORT $INTERNAL_SOCKET$i"
    ./bootstrap.sh $NODE $PORT  `$PORT + $PER_NODE` /tmp/boot$i
    echo "./put.sh $NODE $START_PORT $END_PORT $INTERNAL_SOCKET$i"
    ./put.sh $NODE $PORT  `$PORT + $PER_NODE` /tmp/boot$i
    END_PORT=`expr $START_PORT + $PER_NODE - 1`
    START_PORT=`expr $END_PORT + 1`
    i=`expr $i + 1`
done
