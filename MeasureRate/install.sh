#! /bin/bash

wget http://www.tcpdump.org/release/libpcap-1.9.1.tar.gz
wget https://github.com/westes/flex/archive/flex-2-5-35.tar.gz
wget http://ftp.gnu.org/gnu/bison/bison-2.4.1.tar.gz
wget ftp://ftp.gnu.org/gnu/m4/m4-1.4.13.tar.gz
tar -xvzf libpcap-1.9.1.tar.gz
tar -xvzf flex-2-5-35.tar.gz
tar -xvzf bison-2.4.1.tar.gz
tar -xvzf m4-1.4.13.tar.gz
# rm ./libpcap-1.9.1.tar.gz ./flex-2-5-35.tar.gz ./bison-2.4.1.tar.gz ./m4-1.4.13.tar.gz
cd m4-1.4.13
./configure
sudo make
sudo make install
cd ..

cd flex-2-5-35
./configure
sudo make
sudo make install
cd ..

cd bison-2.4.1
./configure
sudo make
sudo make install
cd ..

cd libpcap-1.9.1
./configure
sudo make
sudo make install
cd ..

ldconfig

wget https://github.com/baolintian/shaper/blob/master/interval.c
gcc -o interval interval.c -lpcap
