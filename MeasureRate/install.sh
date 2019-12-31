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

if grep -Eqi "CentOS" /etc/issue || grep -Eq "CentOS" /etc/*-release; then
	echo 'y' | yum install flex
	echo 'y' | yum install bison
	echo 'y' | yum install git
	echo 'y' | yum install gcc-c++
elif grep -Eqi "Ubuntu" /etc/issue || grep -Eq "Ubuntu" /etc/*-release; then
    sudo apt install flex
	sudo apt install bison
	sudo apt install git
	sudo apt install g++
fi

cd bison-2.4.1
./configure
sudo make
sudo make install
cd ..

ldconfig

cd libpcap-1.9.1
ldconfig
./configure
sudo make
sudo make install
cd ..

ldconfig
