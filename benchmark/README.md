#  Usage





# Useful data files:

+ `speed_data.txt`: throughput rate, unit is `Mbps`
+ `latency.txt`:  RTT, unit is `ms`
+ `cpu.txt`: CPU overhead
+ `retr.txt`: retransmission numbers.



# Todo

+ generalize the function,e.g. move the variables to command.
with iperf command 
src ip
dst ip
tcp or udp
是否使用限速,
interval

T1S10M



问题记录：

安装Tshark的时候会有交互的界面

receiver:

iperf3 -s -p 5001



bash ./sender.sh -a 144.202.27.162 -b 155.138.226.26 -t -n 1 -i 0.1 -p "-c 155.138.226.26 -p 5001 -i 0.1 -l 1460 -t 30 -b 12m" -l "wondershaper -a ens3 -u 10240" -s 10



bash ./sender.sh -a 144.202.27.162 -b 155.138.226.26 -t -n 5 -i 0.1 -p "-c 155.138.226.26 -p 5001 -i 0.1 -l 1460 -t 30 -b 1.2m" -l "wondershaper -a ens3 -u 10240" -s 1



bash ./sender.sh -a 144.202.27.162 -b 155.138.226.26 -u -n 5 -i 0.1 -p "-c 155.138.226.26 -p 5001 -i 0.1 -l 1460 -t 30 -b 1.2m" -l "wondershaper -a ens3 -u 10240" -s 1





捕到的速度数据不全

更换限速的算法

测试多流