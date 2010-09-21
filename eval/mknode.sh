#!/bin/sh

DIR=$1

mkdir $DIR 
./key $DIR
./sign $DIR
./export $DIR
