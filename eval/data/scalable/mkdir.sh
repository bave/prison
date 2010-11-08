#!/bin/sh

if [ -d $1 ]
then
	echo "Derectory is exist"
	exit 1
fi

mkdir $1
touch $1/1check.log
touch $1/1check-p.log
touch $1/2check.log
touch $1/2check-p.log
