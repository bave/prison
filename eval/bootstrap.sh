#!/bin/zsh

CAGE="NO"
CLI="NO"

CAGE=/Users/bayve/git/prison/bin/cage
CLI=/Users/bayve/git/prison/bin/cli
INTERNAL_SOCKET=/tmp/boot1

if [ $CAGE = "NO" ]; then
    echo "please input cage path"
    exit 1
fi

if [ $CLI = "NO" ]; then
    echo "please input cli path"
    exit 1
fi

SEED_NODE=localhost
SEED_PORT=12000

SNODE=kris
SPORT=12001
EPORT=12010

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
    send   \"new,$SNODE$i,$i,global\n\"
    expect { 
        \"recv_message:200,\" {
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
        spawn $CLI $INTERNAL_SOCKET
        expect \"send_message:\"
        send   \"new,$SNODE$i,$i,global\n\"
        expect { 
            \"recv_message:200,\" {
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
    send   \"join,$SNODE$i,$SEED_NODE,$SEED_PORT\n\"
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
        spawn $CLI $INTERNAL_SOCKET
        expect \"send_message:\"
        send   \"join,$SNODE$i,$SEED,$SPORT\n\"
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
# ============================================================

exit 1

# put for dht ================================================
rm -f error
touch error
i=$SPORT
while [ $i -le $EPORT ]
do
    mkdir $SNODE$i
    ./key $SNODE$i
    ./sign $SNODE$i
    ./export $SNODE$i

    # value of sign
    VS=`cat $SNODE$i/sign.txt`

    # value of export
    VE=`cat $SNODE$i/export.txt`
    echo $VE
    sleep 1

    expect -c "
    set timeout 2
    spawn $CLI $INTERNAL_SOCKET
    expect \"send_message:\"
    send   \"put,$SNODE$i,$SNODE$i@prison,$VE,7200,unique\n\"
    expect { 
        \"recv_message:203,\" {
            expect \"send_message:\"
            expect {
                \"recv_message:203,\" {
                    send   \"put,$SNODE$i,$SNODE$i.p2p,$VS,7200,unique\n\"
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
    }"

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
        VS=`cat $SNODE$i/sign.txt`

        # value of export
        VE=`cat $SNODE$i/export.txt`

        expect -c "
        set timeout 2
        spawn $CLI $INTERNAL_SOCKET
        expect \"send_message:\"
        send   \"put,$SNODE$i,$SNODE$i@prison,$VE,7200,unique\n\"
        expect { 
            \"recv_message:203,\" {
                expect \"send_message:\"
                send   \"put,$SNODE$i,$SNODE$i.p2p,$VS,7200,unique\n\"
                expect {
                    \"recv_message:203,\" {
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
        }"
        i=`expr $i + 1`
        sleep 0.1
    done
    SIZE=`/bin/ls -al error | awk '{print $5}'`
done
# ============================================================

