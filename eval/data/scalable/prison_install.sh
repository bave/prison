#!/bin/sh
chroot /mnt/aufs

apt-get -y install gnustep
apt-get -y install git-core
apt-get -y install g++
apt-get -y install gnustep-make
apt-get -y install libgnustep-base-dev
apt-get -y install libgnustep-gui-dev
apt-get -y install libgpgme11-dev
apt-get -y install libevent-dev
apt-get -y install libboost-dev
apt-get -y install libssl-dev
apt-get -y install omake

cd ~/
export LD_LIBRARY_PATH=/lib:/usr/lib:/usr/local/lib
mkdir git
cd git
git clone git://github.com/bave/prison.git
git clone http://github.com/ytakano/libcage.git
cd libcage
git checkout -t origin/libev
omake 
omake install
cd ../prison/cage
make

