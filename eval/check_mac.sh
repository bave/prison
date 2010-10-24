#!/bin/sh

. ./config.sh

if [ $# != 0 ]; then
    host=$1
    bias=$2
    num=$3
    INTERNAL_SOCKET=$4
    EPORT=`expr $bias + $num`
fi

echo "start"

#rm -f error
touch error

i=`expr $bias + 1`
while [ $i -le $EPORT ]
do
    expect -c "
    set timeout 10
    spawn $CLI $INTERNAL_SOCKET
    expect \"send_message:\"
    send   \"get,prison,$host$i.p2p\n\"
    expect {
        \"recv_message:204,\" {
            expect \"send_message:\"
            send   \"quit\n\"
            interact
        }
        timeout { 
            system \"echo $i >> error\"
            expect \"send_message:\"
            send   \"quit\n\"
            expect \"XXX\"
        }
    }
    "
    i=`expr $i + 1`
done

