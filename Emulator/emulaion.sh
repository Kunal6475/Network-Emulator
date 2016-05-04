#!/bin/sh
rm .cs*
xterm -T "Bridge cs1" -e bridge cs1 8 &
sleep 1
xterm -T "Bridge cs2" -e bridge cs2 8 &
sleep 1
xterm -T "Bridge cs3" -e bridge cs3 8 &
sleep 1
xterm -T "Host A" -e station -no ./ifaces/ifaces.a ./rtables/rtable.a hosts &
sleep 1
xterm -T "Host B" -e station -no ./ifaces/ifaces.b ./rtables/rtable.b hosts &
sleep 1
xterm -T "Host C" -e station -no ./ifaces/ifaces.c ./rtables/rtable.c hosts &
sleep 1
xterm -T "Host D" -e station -no ./ifaces/ifaces.d ./rtables/rtable.d hosts &
sleep 1
xterm -T "Host E" -e station -no ./ifaces/ifaces.e ./rtables/rtable.e hosts &
sleep 1
xterm -T "Router r1" -e station -route ./ifaces/ifaces.r1 ./rtables/rtable.r1 hosts &
sleep 1
xterm -T "Router r2" -e station -route ./ifaces/ifaces.r2 ./rtables/rtable.r2 hosts &

