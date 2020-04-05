#! /bin/bash

apt-get update
echo 'y' | apt install make
echo 'y' | apt install g++
git clone https://github.com/esnet/iperf.git
cd iperf
./configure
make
make install
ldconfig
iperf3 -v
cd ~
apt install lrzsz

