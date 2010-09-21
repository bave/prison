#!/bin/sh

make

CAGE="/Users/bayve/git/prison/bin/cage"
CLI="/Users/bayve/git/prison/bin/cli"

SNODE=kris
SPORT=12000

$CAGE &

expect -c "
set timeout 1
spawn $CLI
expect \"send_message:\"
send   \"new,$SNODE,$SPORT,global\n\"
expect \"send_message:\"
send   \"quit\"
"

