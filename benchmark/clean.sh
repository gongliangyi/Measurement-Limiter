#! /bin/bash

if [ -f "cpu.txt" ];then
	rm cpu.txt
fi

if [ -f "iperf_send.txt" ];then
	rm iperf_send.txt
fi
if [ -f "send.cap" ];then
	rm send.cap
fi
if [ -f "receive.cap" ];then
	rm receive.cap
fi
if [ -f "send.json" ];then
	rm send.json
fi
if [ -f "receive.json" ];then
	rm receive.json
fi