#!/bin/sh

SEED_NODE=aris.jaist.ac.jp
SEED_PORT=12000

NODE=aris
SPORT=12001
EPORT=12001

CAGE="NO"
CLI="NO"
INTERNAL_SOCKET="NO"

#CAGE="$HOME/git/prison/bin/cage"
#CLI="$HOME/git/prison/bin/cli"
#INTERNAL_SOCKET="/tmp/sock_bootstrap"

if [ $CAGE = "NO" ]; then
    echo "please input path"
    exit 1
fi

if [ $CLI = "NO" ]; then
    echo "please input path"
    exit 1
fi

if [ $INTERNAL_SOCKET = "NO" ]; then
    echo "please input internal socket file path"
    exit 1
fi
