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

OS=`uname`

i=1
START_PORT=$SPORT
END_PORT=`expr $START_PORT + $PER_NODE - 1`
while [ $i -le $DIV ]
do
    echo "./bootstrap.sh $NODE $START_PORT  $END_PORT $INTERNAL_SOCKET$i"
    ./bootstrap.sh $NODE $START_PORT  $END_PORT $INTERNAL_SOCKET$i
    echo "./put.sh $NODE $START_PORT $END_PORT $INTERNAL_SOCKET$i"
    if [ $OS = "Linux" ]; then
        ./put.sh $NODE $START_PORT $END_PORT $INTERNAL_SOCKET$i
    else
        ./put_mac.sh $NODE $START_PORT $END_PORT $INTERNAL_SOCKET$i
    fi
    START_PORT=`expr $END_PORT + 1`
    END_PORT=`expr $START_PORT + $PER_NODE - 1`
    i=`expr $i + 1`
done
