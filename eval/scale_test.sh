#!/bin/sh

usage()
{
	echo "Usage: sh scale_test.sh [OPTION]"
	echo "OPTION:"
	echo "	-p: Numbers of process (default:4)"
	echo "	-d: Starting Nodes in 1process (default:10)"
	exit 1;
}

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/mnt/aufs/usr/local/lib

# working directory
WD="/root/git/prison/eval"

SEED_NODE="192.168.1.1"
SEED_PORT="12000"
GROUP="muru"

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

IP=`ip addr | grep "inet " | grep 192.168.1.*/*eth | awk -F" " '{ split($2, ip, "/"); print ip[1] }'`
NAME=`echo $IP | awk -F"." '{ print $4 }'`

if [ ${#NAME} = 1 ]
then
	NODE=${GROUP}0${NAME}"_"
	echo $NODE;
elif [ ${#NAME} = 2 ]
then
	NODE=${GROUP}0${NAME}"_"
	echo $NODE;
fi

cd $WD

# check symbolic link /dev/random to /dev/urandom
if [ ! -L "/dev/random" -a ! "/dev/random" -ef "/dev/urandom" ]
then
	echo "delete /dev/random and create symbolic link /dev/random to /dev/urandom"
	make random
fi

# modify config.sh
cp $WD/config.sh $WD/config.sh.tmp
sed -e s/^SEED_NODE=.*/SEED_NODE=$SEED_NODE/ \
	-e s/^SEED_PORT=.*/SEED_PORT=$SEED_PORT/ \
	-e s/^NODE=.*/NODE=$NODE/ \
	-e s/^#CAGE=/CAGE=/ \
	-e s/^#CLI=/CLI=/ \
	-e s/#INTERNAL_SOCKET/INTERNAL_SOCKET/ \
	$WD/config.sh.tmp > $WD/config.sh
rm $WD/config.sh.tmp

# modify eval.sh
cp $WD/eval.sh $WD/eval.sh.tmp
awk 'BEGIN{
	cflag=0;
	}
	{
	if($0 ~ /\$OS = \"Linux\"/){
		if($0 ~ /^#/){
			print $0;
		}
		else {
			print "#"$0;
			cflag=1;
		}
	}
	else if(cflag && $0 ~ /fi/){
		print "#"$0;
		cflag=0;
	}
	else if(cflag) {
		print "#"$0;
	}
	else {
		print $0;
	}
}' $WD/eval.sh.tmp > $WD/eval.sh

# kill cage process
killall cage

# Start SEED Node
if [ $SEED_NODE = $IP ]
then
	$WD/seed.sh
fi


# run eval.sh
$WD/eval.sh $NODE_NUM $PROCESS
kill `ps auxwww | grep "sshd: root@notty" | awk -F" " '{print $2}'`


