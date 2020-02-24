#! /bin/bash

interval=0.1
length=10
cnt=0
tot=$(echo "$length/$interval"|bc)
while true
do
	echo `top -b -n1 | fgrep "Cpu(s)" | tail -1 | awk -F'id,' '{split($1, vs, ","); v=vs[length(vs)]; sub(/\s+/, "", v);sub(/\s+/, "", v); printf "%.2f\n", 100-v;}'`
	sleep $interval
	cnt=`expr ${cnt} + 1`
	#echo $cnt
	if [[ "$cnt" = "$tot" ]];then
		break
	fi
done
