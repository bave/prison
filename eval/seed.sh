#!/bin/sh

make

CAGE="NO"
CLI="NO"

CAGE="/Users/bayve/git/prison/bin/cage"
CLI="/Users/bayve/git/prison/bin/cli"

if [ $CAGE = "NO" ]; then
    echo "please input cage path"
    exit 1
fi

if [ $CLI = "NO" ]; then
    echo "please input cli path"
    exit 1
fi


SNODE=kris
SPORT=12000

/usr/bin/killall -9 cage
/usr/bin/killall -9 expect
$CAGE &

expect -c "
set timeout 1
spawn $CLI
expect \"send_message:\"
send   \"new,$SNODE,$SPORT,global\n\"
expect \"send_message:\"
send   \"quit\"
"

