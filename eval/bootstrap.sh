#!/bin/sh

CAGE=/Users/bayve/git/prison/bin/cage
CLI=/Users/bayve/git/prison/bin/cli

SNODE=kris
SPORT=12000
EPORT=12100

$CAGE &

rm -f error
touch error

expect -c "
set timeout 1
spawn $CLI
expect \"send_message:\"
send   \"new,$SNODE,$SPORT,global\n\"
expect \"send_message:\"
send   \"quit\"
"

i=`expr $SPORT + 1`
while [ $i -le $EPORT ]
do
    expect -c "
    set timeout 2
    spawn $CLI
    expect \"send_message:\"
    send   \"new,$SNODE$i,$i,global\n\"
    expect \"send_message:\"
    send   \"join,$SNODE$i,localhost,$SPORT\n\"
    expect { 
        \"recv_message:202,\" {
            expect \"send_message:\"
            send   \"quit\"
        }
        \"recv_message:40,\"  {
            system \"echo $i >> error\"
            expect \"send_message:\"
            send   \"quit\"
        }
        timeout { 
            system \"echo $i >> error\"
            expect \"send_message:\"
            send   \"quit\"
        }
    }
    "
    i=`expr $i + 1`
    sleep 0.1
done

LOOP=`cat error`
SIZE=`/bin/ls -al ./error | awk '{print $5}'`

while [ $SIZE -ne 0 ]
do
    rm -f error
    touch error
    for i in $LOOP
    do
        echo $SNODE$i
        expect -c "
        set timeout 5
        spawn $CLI
        expect \"send_message:\"
        send   \"join,$SNODE$i,localhost,$SPORT\n\"
        expect { 
            \"recv_message:202,\" {
                expect \"send_message:\"
                send   \"quit\"
            }
            \"recv_message:40,\"  {
                system \"echo $i >> error\"
                expect \"send_message:\"
                send   \"quit\"
            }
            timeout { 
                system \"echo $i >> error\"
                expect \"send_message:\"
                send   \"quit\"
            }
        }
        "
        i=`expr $i + 1`
        sleep 0.2
    done
    SIZE=`/bin/ls -al error | awk '{print $5}'`
done
