#!/bin/sh

usage()
{
	echo "sh run_scale_test.sh [OPTION]"
	echo "OPTION:"
	echo "	-p: Numbers of process(default:4)"
	echo "	-d: Starting Nodes in 1process (default:10)" 
	echo "	-s: Start Address. example: 192.168.1.1(default)"
	echo "	-n: Numbers of Node(default:10)"
	echo "	-h: usage message"
	exit 0
}

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


for i in 1 2 3 4 5 6 7 8 9 10
do
	echo "ssh 192.168.1.${i} sh /mnt/aufs/root/git/prison/eval/put_test.sh"
	ssh -o HashKnownHosts=yes 192.168.1.${i} "export LD_LIBRARY_PATH=/lib:/usr/lib:/usr/local/lib; chroot /mnt/aufs env"
	ssh -o HashKnownHosts=yes 192.168.1.${i} "export LD_LIBRARY_PATH=/lib:/usr/lib:/usr/local/lib; chroot /mnt/aufs sh /root/git/prison/eval/put_test.sh -p $PROCESS -d $NODE_NUM"
	i=`expr $i + 1`
done
