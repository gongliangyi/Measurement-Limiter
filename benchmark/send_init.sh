#! /bin/bash

apt-get update
apt install git
apt install make
echo "y" | apt install tshark
echo "y" | apt install python3
apt git clone https://github.com/gongliangyi/Measurement-Limiter
apt git clone https://github.com/magnific0/wondershaper

cd wondershaper
make install


cd ~/Measurement-Limiter/MeasureRate
bash ./install.sh
g++ -o speed speed2file.cpp -lpcap
mv ./speed ~/
cd ../SendPackets
tar -xzvf iperf.tar.gz
cd iperf
./configure
make
make install
cd ~
mv ./Measurement-Limiter/benchmark/cpu_overhead.sh ./
