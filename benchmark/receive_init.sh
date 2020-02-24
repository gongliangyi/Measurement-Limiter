#! /bin/bash

cd ~

apt-get update
apt install git
apt install make
echo "y" | apt install apache2
apt git clone https://github.com/gongliangyi/Measurement-Limiter
apt git clone https://github.com/magnific0/wondershaper

cd ./Measurement-Limiter/SendPackets
tar -xvzf iperf.tar.gz
cd iperf
./configure
make
make install

cd ~
mv ./Measurement-Limiter/benchmark/receive.sh ./
