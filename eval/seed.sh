#!/bin/sh

make
. ./config.sh

/usr/bin/killall -9 cage
/usr/bin/killall -9 expect
$CAGE &

expect -c "
set timeout 2
spawn $CLI
expect \"send_message:\"
send   \"new,$SEED_NODE,$SEED_PORT,global\n\"
expect \"send_message:\"
send   \"quit\n\"
interact
"

