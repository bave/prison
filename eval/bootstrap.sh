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

$CAGE -f $INTERNAL_SOCKET &
rm -f error
touch error
# new node ===================================================
i=$SPORT
while [ $i -le $EPORT ]
do
    expect -c "
    set timeout 2
    spawn $CLI $INTERNAL_SOCKET
    expect \"send_message:\"
    send   \"new,$NODE$i,$i,global\n\"
    expect { 
        \"recv_message:200,\" {
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
        }
    }
    "
    i=`expr $i + 1`
done

LOOP=`cat error`
SIZE=`/bin/ls -al ./error | awk '{print $5}'`

while [ $SIZE -ne 0 ]
do
    rm -f error
    touch error
    for i in $LOOP
    do
        echo $NODE$i
        expect -c "
        set timeout 5
        spawn $CLI $INTERNAL_SOCKET
        expect \"send_message:\"
        send   \"new,$NODE$i,$i,global\n\"
        expect { 
            \"recv_message:200,\" {
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
            }
        }
        "
        i=`expr $i + 1`
    done
    SIZE=`/bin/ls -al error | awk '{print $5}'`
done
# ============================================================


# join dht ===================================================
rm -f error
touch error
i=$SPORT
while [ $i -le $EPORT ]
do
    expect -c "
    set timeout 2
    spawn $CLI $INTERNAL_SOCKET
    expect \"send_message:\"
    send   \"join,$NODE$i,$SEED_NODE,$SEED_PORT\n\"
    expect { 
        \"recv_message:202,\" {
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
        }
    }
    "
    i=`expr $i + 1`
done

LOOP=`cat error`
SIZE=`/bin/ls -al ./error | awk '{print $5}'`

while [ $SIZE -ne 0 ]
do
    rm -f error
    touch error
    for i in $LOOP
    do
        echo $NODE$i
        expect -c "
        set timeout 5
        spawn $CLI $INTERNAL_SOCKET
        expect \"send_message:\"
        send   \"join,$NODE$i,$SEED,$SPORT\n\"
        expect { 
            \"recv_message:202,\" {
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
            }
        }
        "
        i=`expr $i + 1`
    done
    SIZE=`/bin/ls -al error | awk '{print $5}'`
done
# ============================================================

