#! /bin/bash

cd ~

apt-get update
apt install make
apt install ntpdate
echo "Yes" | apt install tshark
echo "y" | apt install python3


cd ~
git clone https://github.com/magnific0/wondershaper
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
ldconfig
cd ~
mv ~/Measurement-Limiter/benchmark/cpu_overhead.sh ./
mv ~/Measurement-Limiter/benchmark/pcap.py ./
mv ~/Measurement-Limiter/benchmark/sender.sh ./
mv ~/Measurement-Limiter/benchmark/tcp_Retr.cpp ./
mv ~/Measurement-Limiter/benchmark/jitter.cpp ./
