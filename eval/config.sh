#!/bin/sh

SEED_NODE=127.0.0.1
SEED_PORT=12000

NODE=prison
SPORT=12001
EPORT=12001

#CAGE="$HOME/git/prison/bin/cage"
#CLI="$HOME/git/prison/bin/cli"
#INTERNAL_SOCKET="/tmp/sock_bootstrap"

if [ -z $CAGE ]; then
    echo "please input path in config.sh"
    exit 1
fi

if [ -z $CLI ]; then
    echo "please input path in config.sh"
    exit 1
fi

if [ -z $INTERNAL_SOCKET ]; then
    echo "please input internal socket file path in config.sh"
    exit 1
fi
