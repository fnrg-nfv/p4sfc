#!/bin/bash
VETH=$(ifconfig | grep -m 1 veth | cut -d " " -f 1)
while [ $VETH ];
do
    sudo ip link del $VETH
    VETH=$(ifconfig | grep -m 1 veth | cut -d " " -f 1)
done