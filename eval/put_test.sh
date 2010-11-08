#!/bin/sh

PROCESS="4"
NODE_NUM="10"

while getopts p:d:h OPT
do
	case $OPT in
		"p" ) PROCESS="$OPTARG" ;;
		"d" ) NODE_NUM="$OPTARG" ;;
		"h" ) usage;;
	esac
done

# make key
cd /root/git/prison/eval/
make key
make sign
make export

NODE=`cat /root/git/prison/eval/config.sh | grep ^NODE= | awk -F"=" '{ print $2}'`
rm -rf ${NODE}*
start_port=12001

i=1
while [ $i -le $PROCESS ]
do
	#end_port=`expr $start_port + $NODE_NUM - 1`
	end_port=`expr $start_port`
	echo "/root/git/prison/eval/put.sh $NODE $start_port $end_port /tmp/sock_bootstrap$i"
	sh /root/git/prison/eval/put.sh $NODE $start_port $end_port /tmp/sock_bootstrap$i
	start_port=`expr $start_port + $NODE_NUM`
	i=`expr $i + 1`
done
