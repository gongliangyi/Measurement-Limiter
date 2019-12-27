#! /bin/bash
iperf3 -c 192.168.179.134 -p 5001 -i 1 -b 100m -u -l 1500 -A 2 -t 120 -Z --pacing-timer=1 > iperf3.txt

