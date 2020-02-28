#  Usage

```bash
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
```





# Useful data files:

+ `speed_data.txt`: throughput rate, unit is `Mbps`
+ `latency.txt`:  RTT, unit is `ms`
+ `cpu.txt`: CPU overhead
+ `retr.txt`: retransmission numbers.




