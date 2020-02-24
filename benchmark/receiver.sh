#!/bin/bash

rm ./receive.cap
timeout 35 tcpdump -i ens3 dst port 5001 -w receive.cap > /dev/zero &
sleep 37
rm /var/www/html/receive.cap
mv ./receive.cap /var/www/html