#!/bin/bash

DEV="ens3"
DST="155.138.226.26"
for ((i=1;i <= 1000; i=i+2))
do	
	speed=$i'm'
	speedw=$(echo $(echo "scale=0; $i*1024" | bc) | awk -F. '{print $1}')
	IPERF_COMMAND="-c $DST -i 0.1 -l 1448 -t 10 -p 5001 -b $speed --pacing-timer 1 -u"
	wondershaper -c -a $DEV
	tc qd del dev $DEV root > /dev/null
	wondershaper -a $DEV -u $speedw
	timeout 11 iperf3 $IPERF_COMMAND > iperf_send.txt
	cat iperf_send.txt | grep % | awk -F')' '{split($1, vs, " "); v=vs[length(vs)]; split(v, v1, "("); v=v1[length(v1)];split(v, v1, "%"); v=v1[1];print v;}'| tail -1  >> loss.txt
	rm iperf_send.txt
done