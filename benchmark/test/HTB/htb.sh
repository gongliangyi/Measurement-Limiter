#! /bin/bash


SRC=144.202.27.162
DST=155.138.226.26

# single stream
bash ./sender.sh -a $SRC -b $DST -u -n 1 -i 0.1 -p "-c $DST -i 0.1 -l 1448 -t 30 -p 5001 -b 1m --pacing-timer 1 -u" -l "wondershaper -a ens3 -u 1024" -s 1
bash ./sender.sh -a $SRC -b $DST -u -n 1 -i 0.1 -p "-c $DST -i 0.1 -l 1448 -t 30 -p 5001 -b 10m --pacing-timer 1 -u" -l "wondershaper -a ens3 -u 10240" -s 10
bash ./sender.sh -a $SRC -b $DST -u -n 1 -i 0.1 -p "-c $DST -i 0.1 -l 1448 -t 30 -p 5001 -b 100m --pacing-timer 1 -u" -l "wondershaper -a ens3 -u 102400" -s 100
bash ./sender.sh -a $SRC -b $DST -u -n 1 -i 0.1 -p "-c $DST -i 0.1 -l 1448 -t 30 -p 5001 -b 500m --pacing-timer 1 -u" -l "wondershaper -a ens3 -u 512000" -s 500
bash ./sender.sh -a $SRC -b $DST -u -n 1 -i 0.1 -p "-c $DST -i 0.1 -l 1448 -t 30 -p 5001 -b 1000m --pacing-timer 1 -u" -l "wondershaper -a ens3 -u 1024000" -s 1000

# 10 streams with 100Mbps

bash ./sender.sh -a $SRC -b $DST -u -n 10 -i 0.1 -p "-c $DST -i 0.1 -l 1448 -t 30 -p 5001 -b 1m --pacing-timer 1 -u -P 10" -l "wondershaper -a ens3 -u 1024" -s 1
bash ./sender.sh -a $SRC -b $DST -u -n 10 -i 0.1 -p "-c $DST -i 0.1 -l 1448 -t 30 -p 5001 -b 10m --pacing-timer 1 -u -P 10" -l "wondershaper -a ens3 -u 10240" -s 10
bash ./sender.sh -a $SRC -b $DST -u -n 10 -i 0.1 -p "-c $DST -i 0.1 -l 1448 -t 30 -p 5001 -b 100m --pacing-timer 1 -u -P 10" -l "wondershaper -a ens3 -u 102400" -s 100
bash ./sender.sh -a $SRC -b $DST -u -n 10 -i 0.1 -p "-c $DST -i 0.1 -l 1448 -t 30 -p 5001 -b 500m --pacing-timer 1 -u -P 10" -l "wondershaper -a ens3 -u 512000" -s 500
bash ./sender.sh -a $SRC -b $DST -u -n 10 -i 0.1 -p "-c $DST -i 0.1 -l 1448 -t 30 -p 5001 -b 1000m --pacing-timer 1 -u -P 10" -l "wondershaper -a ens3 -u 1024000" -s 1000