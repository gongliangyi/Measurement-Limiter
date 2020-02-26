#!/bin/bash


usage(){
cat << EOF
the shell scripts descriptions.

-c 144.202.27.162 -p 5001 -i 0.1 -l 1460 -t 30 -b 12m
EOF
}

ISLIMIT=
LIMIT_COMMAND=""
INTERVAL=0.1
IPERF_COMMAND=""
PROTOCOL="none"
STREAM_NUMBER=0
SPEED=10
SRC=
DST=
DEV="ens3"


function iperf3(){
	
	timeout 31 iperf3 $IPERF_COMMAND > iperf_send.txt
}

function testspeed(){
	timeout 35 /root/speed $DEV "dst port 5001" $(echo "scale=0; $INTERVAL*1000" | bc)
}

function latency(){
	timeout 35 tcpdump -i $DEV dst port 5001 -w send.cap > /dev/zero
}

while getopts htun:i:p:l:s:a:b: o
do	case "$o" in
	h)		usage
			exit 1;;
    t)		PROTOCOL="TCP";;
	u)		PROTOCOL="UDP";;
	n)      STREAM_NUMBER=$OPTARG;;
	i)		INTERVAL=$OPTARG;;
	p)      IPERF_COMMAND=$OPTARG;;
	
	l)      LIMIT_COMMAND=$OPTARG
			ISLIMIT=1;;
	s) 		SPEED=$OPTARG;;
	a)		SRC=$OPTARG;;
	b)		DST=$OPTARG;;
	[?])	usage
			exit 1;;
	esac
done

PREFIX=""
if [ "$PROTOCOL" = "TCP" ];then
	PREFIX="T"
elif [ "$PROTOCOL" = "UDP" ];then
	PREFIX="U"
fi
PREFIX=$PREFIX$STREAM_NUMBER'S'$SPEED'Mbps'



if [ ISLIMIT ];then
	wondershaper -c -a ens3
	wondershaper -a ens3 -u 10240
fi


(iperf3 &); (testspeed &); (timeout 35 bash ./cpu_overhead.sh >> cpu.txt &); (latency &)

sleep 45
echo "finished"


###############################################
#
#process relative date
#
###############################################

url="http://$SRC/receive.cap"
wget url

tshark -r send.cap -T json >send.json
tshark -r receive.cap -T json >receive.json

python3 ./pcap.py -s $SRC -d $DST

if [ ! -f "tcp_Retr" ];then
	g++ -o tcp_Retr tcp_Retr.cpp
fi

if [ ! -f "jitter" ];then
	g++ -o jitter jitter.cpp
fi

./tcp_Retr

if [ ! -f "$PREFIX" ];then
	mkdir $PREFIX
fi

mv iperf_send.txt latency.txt speed_data.txt ./$PREFIX
if [ "$PROTOCOL" == "TCP"];then
	mv retr.txt ./$PREFIX
else
	mv jitter.txt ./$PREFIX
fi

