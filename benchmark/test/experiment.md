## 选取的限速的算法：

①HTB

使用的工具是wondershaper:

命令：

```
wondershaper -a ens3 -u rate
```



② FQ/pacing:

使用tc

命令：

```
tc qdisc change dev ens3 root fq pacing flow_limit 1000 maxrate 1024kbit
```

reference: [tc-fq](<http://man7.org/linux/man-pages/man8/tc-fq.8.html>)



③ TSF

tc+iproute



## 一些常数的设置

+ 一组测试时长为30S
+ 速率采样窗口为100ms
+ 发包速率=改成1倍的速率发送。

原因：经过实际的测试，发现所有的限速的工具都很难达到这个上限。

之所以设置成一倍的原因，

一是希望测试看看限速的算法能否达到预期的限速的目标，

二是尽量的减少多余的包在系统栈中造成的排队，使得限速的算法性能进一步的降低的可能。



## 测量指标



### throughtput accuracy

本指标主要在测试能否精确的进行限速，达到预期的限速目标。

粗略的观察，HTB是只能达到95%上限速度, FQ/pacing仍需进一步的测量。

因为是吞吐的准确性的测量，我们仅使用UDP包进行实验。

+ 单流：

UDP 1Mbps:

wondershaper:

```
bash ./sender.sh -a 144.202.27.162 -b 155.138.226.26 -u -n 1 -i 0.1 -p "-c 155.138.226.26 -p 5001 -i 0.1 -l 1460 -t 30 -b 1.2m --pacing-timer 1 -u" -l "wondershaper -a ens3 -u 1024" -s 1
```

FQ:

```
bash ./sender.sh -a 144.202.27.162 -b 155.138.226.26 -u -n 1 -i 0.1 -p "-c 155.138.226.26 -p 5001 -i 0.1 -l 1460 -t 30 -b 1.2m --pacing-timer 1 -u" -l "tc qdisc add dev ens3 root fq pacing flow_limit 1000 maxrate 1024kbit" -s 1
```

TSF: (需要接口)

```
bash ./sender.sh -a 144.202.27.162 -b 155.138.226.26 -u -n 1 -i 0.1 -p "-c 155.138.226.26 -p 5001 -i 0.1 -l 1460 -t 30 -b 1.2m --pacing-timer 1 -u" -l "(unfinished)" -s 1
```



UDP 10Mbps:

wondershaper:

```
bash ./sender.sh -a 144.202.27.162 -b 155.138.226.26 -u -n 1 -i 0.1 -p "-c 155.138.226.26 -p 5001 -i 0.1 -l 1460 -t 30 -b 12m --pacing-timer 1 -u" -l "wondershaper -a ens3 -u 10240" -s 10
```

FQ:

```
bash ./sender.sh -a 144.202.27.162 -b 155.138.226.26 -u -n 1 -i 0.1 -p "-c 155.138.226.26 -p 5001 -i 0.1 -l 1460 -t 30 -b 12m --pacing-timer 1 -u" -l "tc qdisc add dev ens3 root fq pacing flow_limit 1000 maxrate 10240kbit" -s 10
```

TSF: (需要接口)

```
bash ./sender.sh -a 144.202.27.162 -b 155.138.226.26 -u -n 1 -i 0.1 -p "-c 155.138.226.26 -p 5001 -i 0.1 -l 1460 -t 30 -b 12m --pacing-timer 1 -u" -l "(unfinished)" -s 10
```



UDP 100Mbps:

wondershaper:

```
bash ./sender.sh -a 144.202.27.162 -b 155.138.226.26 -u -n 1 -i 0.1 -p "-c 155.138.226.26 -p 5001 -i 0.1 -l 1460 -t 30 -b 120m --pacing-timer 1 -u" -l "wondershaper -a ens3 -u 102400" -s 100
```

FQ:

```
bash ./sender.sh -a 144.202.27.162 -b 155.138.226.26 -u -n 1 -i 0.1 -p "-c 155.138.226.26 -p 5001 -i 0.1 -l 1460 -t 30 -b 120m --pacing-timer 1 -u" -l "tc qdisc add dev ens3 root fq pacing flow_limit 1000 maxrate 102400kbit" -s 100
```

TSF: (需要接口)

```
bash ./sender.sh -a 144.202.27.162 -b 155.138.226.26 -u -n 1 -i 0.1 -p "-c 155.138.226.26 -p 5001 -i 0.1 -l 1460 -t 30 -b 120m --pacing-timer 1 -u" -l "(unfinished)" -s 100
```

根据需要，可以增加更多的测试的速率。



+ 多流

10条流，总带宽10Mbps，UDP:

wondershaper:

```
bash ./sender.sh -a 144.202.27.162 -b 155.138.226.26 -u -n 10 -i 0.1 -p "-c 155.138.226.26 -p 5001 -i 0.1 -l 1460 -t 30 -b 12m --pacing-timer 1 -u -P 10" -l "wondershaper -a ens3 -u 10240" -s 10
```

FQ:

```
bash ./sender.sh -a 144.202.27.162 -b 155.138.226.26 -u -n 10 -i 0.1 -p "-c 155.138.226.26 -p 5001 -i 0.1 -l 1460 -t 30 -b 12m --pacing-timer 1 -u -P 10" -l "tc qdisc add dev ens3 root fq pacing flow_limit 1000 maxrate 10240kbit" -s 10
```

TSF: (需要接口)

```
bash ./sender.sh -a 144.202.27.162 -b 155.138.226.26 -u -n 10 -i 0.1 -p "-c 155.138.226.26 -p 5001 -i 0.1 -l 1460 -t 30 -b 12m --pacing-timer 1 -u -P 10" -l "(unfinished)" -s 10
```



100条流，总带宽10Mbps，UDP:

wondershaper:

```
bash ./sender.sh -a 144.202.27.162 -b 155.138.226.26 -u -n 100 -i 0.1 -p "-c 155.138.226.26 -p 5001 -i 0.1 -l 1460 -t 30 -b 12m --pacing-timer 1 -u -P 100" -l "wondershaper -a ens3 -u 10240" -s 10
```

FQ:

```
bash ./sender.sh -a 144.202.27.162 -b 155.138.226.26 -u -n 100 -i 0.1 -p "-c 155.138.226.26 -p 5001 -i 0.1 -l 1460 -t 30 -b 12m --pacing-timer 1 -u -P 100" -l "tc qdisc add dev ens3 root fq pacing flow_limit 1000 maxrate 10240kbit" -s 10
```

TSF: (需要接口)

```
bash ./sender.sh -a 144.202.27.162 -b 155.138.226.26 -u -n 100 -i 0.1 -p "-c 155.138.226.26 -p 5001 -i 0.1 -l 1460 -t 30 -b 12m --pacing-timer 1 -u -P 100" -l "(unfinished)" -s 10
```



### jitter

这个只有一个值，可以使用UDP协议，用不同的速率测试来看，jitter $$\approx $$ 0.010-0.015ms并且不受发送速率的影响，说明链路环境是比较的稳定的，不会较大幅度的影响latency



### latency

<可能还需要找一个能改变rtt的工具，netem?>

之前使用时钟同步，然后计算发送接收端的相同数据包的时间差，但是NTP时钟是存在时间漂移的问题的，时间漂移能达到20-30ms, 然而rtt基本只有0.2-0.8ms,可见误差是相当大的。

找各种指标，使用的是tcp.analysis.ack_rtt字段进行计算的。



```
tc qd add dev ens3 root netem delay 20ms
```





在链路空闲的情况下，使用ping命令可以测出理想情况下的RTT时间。

![](./rtt.png)

latency=电磁波传输时间t1+链路传输时间t2+排队时间t3



上图说明，当发送速率较小的时候，latency$\approx$RTT, t2占主导的作用。

当速率较大的时候，t3占主导的作用。

我们的限速器是在发送端末端的，因此不会增加链路中间的排队时间，能够说明TSF能缓解中间链路的排队latency。



先用ping测出空闲链路的latency，得到一个基准线.

100流 50Mbps TCP, 画CDF图像：



wondershaper:

```
bash ./sender.sh -a 144.202.27.162 -b 155.138.226.26 -t -n 100 -i 0.1 -p "-c 155.138.226.26 -p 5001 -i 0.1 -l 1460 -t 30 -b 60m --pacing-timer 1 -P 100" -l "wondershaper -a ens3 -u 10240" -s 50
```

FQ:

```
bash ./sender.sh -a 144.202.27.162 -b 155.138.226.26 -t -n 100 -i 0.1 -p "-c 155.138.226.26 -p 5001 -i 0.1 -l 1460 -t 30 -b 60m --pacing-timer 1 -P 100" -l "tc qdisc add dev ens3 root fq pacing flow_limit 1000 maxrate 10240kbit" -s 50
```

TSF: (需要接口)

```
bash ./sender.sh -a 144.202.27.162 -b 155.138.226.26 -t -n 100 -i 0.1 -p "-c 155.138.226.26 -p 5001 -i 0.1 -l 1460 -t 30 -b 60m --pacing-timer 1 -P 100" -l "(unfinished)" -s 50
```



100流 50Mbps UDP, 画CDF图像：

wondershaper:

```
bash ./sender.sh -a 144.202.27.162 -b 155.138.226.26 -u -n 100 -i 0.1 -p "-c 155.138.226.26 -p 5001 -i 0.1 -l 1460 -t 30 -b 60m --pacing-timer 1 -u -P 100" -l "wondershaper -a ens3 -u 10240" -s 50
```

FQ:

```
bash ./sender.sh -a 144.202.27.162 -b 155.138.226.26 -u -n 100 -i 0.1 -p "-c 155.138.226.26 -p 5001 -i 0.1 -l 1460 -t 30 -b 60m --pacing-timer 1 -u -P 100" -l "tc qdisc add dev ens3 root fq pacing flow_limit 1000 maxrate 10240kbit" -s 50
```

TSF: (需要接口)

```
bash ./sender.sh -a 144.202.27.162 -b 155.138.226.26 -u -n 100 -i 0.1 -p "-c 155.138.226.26 -p 5001 -i 0.1 -l 1460 -t 30 -b 60m --pacing-timer 1 -u -P 100" -l "(unfinished)" -s 50
```



### CPU overhead(不好直接做)

感觉使用top的粒度太粗了，先测测看吧。

CPU overhead在每一组测试样例中都会有记录。

单流：



### loss rate

106Mbps开始丢包了

### 多流公平竞争(针对TCP)







## 需要在限速模块进行的测量



### 实际发送时间-理想时间

检测数据包时候按照想要的行为发送出去



### TSF中保存了多少个包，来衡量内存的使用的效率





## 问题

怎么wondershaper TCP UDP限速之间差距非常的大



