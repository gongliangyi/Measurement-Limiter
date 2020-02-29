#!/bin/bash



usage(){
cat <<EOF

Automatic test scripts includes throughput, cpu overhead, latency, tcp retransmission rate, udp jitter.


OPTIONS:
	-h		show usage
    -t		use TCP protocol to send packet
	-u		use UDP protocol to send packet
	-n      set stream numbers
	-i		set interval time, unit is sec
	-p      set iperf command
	
	-l      set rate limiter and its command
	-s 		set speed (Mbps)
	-a		set source ip
	-b		set destination ip
	
EXAMPLES:
	TCP 1 stream 10Mbps:
		bash ./sender.sh -a 144.202.27.162 -b 155.138.226.26 -t -n 1 -i 0.1 -p "-c 155.138.226.26 -p 5001 -i 0.1 -l 1460 -t 30 -b 12m" -l "wondershaper -a ens3 -u 10240" -s 10
	TCP 5 stream 1.2Mbps:
		bash ./sender.sh -a 144.202.27.162 -b 155.138.226.26 -t -n 5 -i 0.1 -p "-c 155.138.226.26 -p 5001 -i 0.1 -l 1460 -t 30 -b 1.2m" -l "wondershaper -a ens3 -u 1024" -s 1
	UDP 5 stream 1.2Mbps:
		bash ./sender.sh -a 144.202.27.162 -b 155.138.226.26 -u -n 5 -i 0.1 -p "-c 155.138.226.26 -p 5001 -i 0.1 -l 1460 -t 30 -b 1.2m -p 5" -l "wondershaper -a ens3 -u 1024" -s 1


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
	temp=$(echo $(echo "scale=0; $INTERVAL*1000" | bc) | awk -F. '{print $1}')
	echo "speedtest interval" $temp
	timeout 35 /root/speed $DEV "dst port 5001" $temp
}

function latency(){
	timeout 35 tcpdump -i $DEV ip host $SRC and ip host $DST -w send.pcap > /dev/zero
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
	wondershaper -c -a $DEV
	tc qd del dev $DEV root

	$LIMIT_COMMAND
fi

(iperf3 &); (testspeed &); (timeout 35 bash ./cpu_overhead.sh >> cpu.txt &); (latency &)

sleep 45
echo "finished"


###############################################
#
#process relative date
#
###############################################

tshark -r send.pcap -2Y "tcp.analysis.ack_rtt" -T fields -e tcp.analysis.ack_rtt > latency.txt


if [ ! -f "tcp_Retr" ];then
	g++ -o tcp_Retr tcp_Retr.cpp
fi

if [ ! -f "jitter" ];then
	g++ -o jitter jitter.cpp
fi

if [ "$PROTOCOL" == "TCP" ];then
	./tcp_Retr
else
	./jitter
fi

if [  -d "$PREFIX/" ];then
	rm -rf "$PREFIX/"
fi
mkdir $PREFIX
mv iperf_send.txt latency.txt speed_data.txt cpu.txt ./$PREFIX
if [ "$PROTOCOL" == "TCP" ];then
	mv retr.txt ./$PREFIX
else
	mv jitter.txt ./$PREFIX
fi

if [ -f *.txt ];then
	rm *.txt
fi
if [ -f *.cap ];then
	rm *.cap
fi
