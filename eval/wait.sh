#!/bin/sh

TIMER=$1
shift
COMMAND=$@


if [ -z $TIMER ]; then
    echo "please input timer number arg1"
    exit 1
fi

$@ &
PID=$!

sleep $TIMER

for i in `ps | awk '{print $1}'`
do
    if [ $i = $PID ]; then
        kill $PID
    fi
done
