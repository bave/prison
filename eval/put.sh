#!/bin/sh

# $# args count
# $@ arg all
# $1 NODE
# $2 SPORT
# $3 EPORT
# $4 INTERNAL_SOCKET

. ./config.sh

if [ $# != 0 ]; then
    if [ -z $1 ]; then
        echo "put.sh [node_name] [start_port] [end_port] [internal_sock]"
        exit 1
    else
        NODE=$1
    fi

    if [ -z $2 ]; then
        echo "put.sh [node_name] [start_port] [end_port] [internal_sock]"
        exit 1
    else
        SPORT=$2
    fi

    if [ -z $3 ]; then
        echo "put.sh [node_name] [start_port] [end_port] [internal_sock]"
        exit 1
    else
        EPORT=$3
    fi

    if [ -z $4 ]; then
        echo "put.sh [node_name] [start_port] [end_port] [internal_sock]"
        exit 1
    else
        INTERNAL_SOCKET=$4
    fi
fi

# put for dht ================================================
rm -f error
touch error
i=$SPORT
while [ $i -le $EPORT ]
do

    mkdir $NODE$i
    ./key $NODE$i
    ./sign $NODE$i
    ./export $NODE$i

    # value of sign
    VS=`cat $NODE$i/sign.txt`

    # value of export
    VE=`cat $NODE$i/export.txt`
    #echo $VE
    expect -c "
    set timeout 2
    spawn $CLI $INTERNAL_SOCKET
    expect \"send_message:\"
    send   \"put,$NODE$i,$NODE$i@prison,$VE,7200,unique\n\"
    expect { 
        \"recv_message:203,\" {
            send   \"put,$NODE$i,$NODE$i.p2p,$VS,7200,unique\n\"
            expect {
                \"recv_message:203,\" {
                    expect \"send_message:\"
                    send   \"quit\n\"
                    interact
                }
                \"recv_message:40\"  {
                    system \"echo $i >> error\"
                    expect \"send_message:\"
                    send   \"quit\n\"
                    interact
                }
                timeout { 
                    system \"echo $i >> error\"
                    expect \"send_message:\"
                    send   \"quit\n\"
                    interact
                }
            }
        }
        \"recv_message:40\"  {
            system \"echo $i >> error\"
            expect \"send_message:\"
            send   \"quit\n\"
            interact
        }
        timeout { 
            system \"echo $i >> error\"
            expect \"send_message:\"
            send   \"quit\n\"
            interact
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
        # value of sign
        VS=`cat $NODE$i/sign.txt`

        # value of export
        VE=`cat $NODE$i/export.txt`
        expect -c "
        set timeout 2
        spawn $CLI $INTERNAL_SOCKET
        expect \"send_message:\"
        send   \"put,$NODE$i,$NODE$i@prison,$VE,7200,unique\n\"
        expect { 
            \"recv_message:203,\" {
                send   \"put,$NODE$i,$NODE$i.p2p,$VS,7200,unique\n\"
                expect {
                    \"recv_message:203,\" {
                        expect \"send_message:\"
                        send   \"quit\n\"
                        interact
                    }
                    \"recv_message:40\"  {
                        system \"echo $i >> error\"
                        expect \"send_message:\"
                        send   \"quit\n\"
                        interact
                    }
                    timeout { 
                        system \"echo $i >> error\"
                        expect \"send_message:\"
                        send   \"quit\n\"
                        interact
                    }
                }
            }
            \"recv_message:40\"  {
                system \"echo $i >> error\"
                expect \"send_message:\"
                send   \"quit\n\"
                interact
            }
            timeout { 
                system \"echo $i >> error\"
                expect \"send_message:\"
                send   \"quit\n\"
                interact
            }
        }
        "
        i=`expr $i + 1`
        sleep 0.1
    done
    SIZE=`/bin/ls -al error | awk '{print $5}'`
done
# ============================================================

