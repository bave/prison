#!/bin/sh

CAGE="/Users/bayve/git/prison/bin/cage -f /tmp/boot1"
CLI="/Users/bayve/git/prison/bin/cli /tmp/boot1"

SEED_NODE=localhost
SEED_PORT=12000

SNODE=kris
SPORT=12001
EPORT=12010


$CAGE &
rm -f error
touch error

# new node ===================================================
i=$SPORT
while [ $i -le $EPORT ]
do
    expect -c "
    set timeout 2
    spawn $CLI
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
        spawn $CLI
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
    spawn $CLI
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
        spawn $CLI
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
    VE1=`cat $SNODE$i/export.txt | cut -c 1-300`
    VE2=`cat $SNODE$i/export.txt | cut -c 301-600`
    VE3=`cat $SNODE$i/export.txt | cut -c 601-900`
    VE4=`cat $SNODE$i/export.txt | cut -c 901-1200`

    expect -c "
    set timeout 2
    spawn $CLI
    expect \"send_message:\"
    send   \"put,$SNODE$i,$SNODE$i.p2p,$VS,7200,unique\n\"
    expect { 
        \"recv_message:203,\" {
            expect \"send_message:\"
            send   \"put,$SNODE$i,$SNODE$i@prison,$VE1\"
            send   \"$V2E\"
            send   \"$V3E\"
            send   \"$VE4,7200,unique\n\"
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
        VE1=`cat $SNODE$i/export.txt | cut -c 1-300`
        VE2=`cat $SNODE$i/export.txt | cut -c 301-600`
        VE3=`cat $SNODE$i/export.txt | cut -c 601-900`
        VE4=`cat $SNODE$i/export.txt | cut -c 901-1200`

        expect -c "
        set timeout 2
        spawn $CLI
        expect \"send_message:\"
        send   \"put,$SNODE$i,$SNODE$i.p2p,$VS,7200,unique\n\"
        expect { 
            \"recv_message:203,\" {
                expect \"send_message:\"
                send   \"put,$SNODE$i,$SNODE$i@prison,$VE1\"
                send   \"$V2E\"
                send   \"$V3E\"
                send   \"$VE4,7200,unique\n\"
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

