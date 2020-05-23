#!/bin/bash
NUM_OF_VNIC=50
for ((I=1; I<=$NUM_OF_VNIC; I++))
do
    NAME_INDEX=$(( 2 * I ))
    PAIR_NAME_2="veth"$NAME_INDEX
    NAME_INDEX=$((NAME_INDEX - 1))
    PAIR_NAME_1="veth"$NAME_INDEX
    sudo ip link add $PAIR_NAME_1 type veth peer name $PAIR_NAME_2
    sudo ifconfig $PAIR_NAME_1 up
    sudo ifconfig $PAIR_NAME_2 up
done