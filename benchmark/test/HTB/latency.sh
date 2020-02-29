DEV="ens3"
SRC="144.202.27.162"
DST="155.138.226.26"

if [ -f "latency1.txt" ];then
	rm latency1.txt
fi

#TCP
for ((i=0;i < 100; i=i+1))
do	
	
	speed="100m"
	speedw="102400"
	IPERF_COMMAND="-c $DST -i 0.1 -l 1448 -t 10 -p 5001 -b $speed --pacing-timer 1"
	wondershaper -c -a $DEV
	
	wondershaper -a $DEV -u $speedw
	temp=$(echo $(echo "scale=2; 0.01*$i" | bc) | awk '{print $1}')
	DELAY=$temp'ms'
	echo $DELAY
	tc qd del dev $DEV root > /dev/null
	tc qd add dev $DEV root netem delay $DELAY
	
	timeout 12 tcpdump -i $DEV ip host $SRC and ip host $DST -w send.pcap > /dev/zero &
	timeout 12 iperf3 $IPERF_COMMAND > iperf_send.txt
	tshark -r send.pcap -2Y "tcp.analysis.ack_rtt" -T fields -e tcp.analysis.ack_rtt > latency.txt
	cat latency.txt | awk 'BEGIN{tot=0;sum=0}{ if ($1>=$temp)tot+=1;sum+=$1 } END{printf "%.9lf\n", sum/tot}' >> latency1.txt
	rm iperf_send.txt
done

for ((i=1;i <= 40; i=i+1))
do	
	speed="100m"
	speedw="102400"
	IPERF_COMMAND="-c $DST -i 0.1 -l 1448 -t 10 -p 5001 -b $speed --pacing-timer 1"
	wondershaper -c -a $DEV
	tc qd del dev $DEV root > /dev/null
	wondershaper -a $DEV -u $speedw
	DELAY=$i'ms'
	echo "$DELAY"
	tc qd add dev ens3 root netem delay $DELAY
	
	timeout 12 tcpdump -i $DEV ip host $SRC and ip host $DST -w send.pcap > /dev/zero &
	timeout 12 iperf3 $IPERF_COMMAND > iperf_send.txt
	tshark -r send.pcap -2Y "tcp.analysis.ack_rtt" -T fields -e tcp.analysis.ack_rtt > latency.txt
	cat latency.txt | awk 'BEGIN{tot=0;sum=0}{ if ($1>=$i)tot+=1;sum+=$1 } END{printf "%.9lf\n", sum/tot}' >> latency1.txt
	rm iperf_send.txt
done
