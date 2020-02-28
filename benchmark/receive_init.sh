#! /bin/bash

apt-get update
apt install make
git clone https://github.com/gongliangyi/Measurement-Limiter
git clone https://github.com/magnific0/wondershaper
mv ./wondershaper ~/

bash ~/Measurement-Limiter/MeasureRate/install.sh

cd ~/Measurement-Limiter/SendPackets/
tar -xvzf ~/Measurement-Limiter/SendPackets/iperf.tar.gz

cd iperf
./configure
make
make install
ldconfig

cd ~
mv ~/Measurement-Limiter/benchmark/receiver.sh ./
