#!/bin/bash

(bash iperf3.sh &) ; (bash bmon.sh & ) ;  (bash sar.sh &) ; (bash ifstat.sh & ) ;  (bash iftop.sh &) ; (bash monitor1.sh &)
echo "here"
