#!/bin/bash

if [ -f "receive.cap" ];then
	rm ./receive.cap
fi
ntpdate -u ntp.api.bz > /dev/zero
timeout 40 tcpdump -i ens3 dst port 5001 -w receive.cap > /dev/zero &
sleep 45
if [ -f "/var/www/html/receive.cap" ];then
	rm /var/www/html/receive.cap
fi

mv ./receive.cap /var/www/html