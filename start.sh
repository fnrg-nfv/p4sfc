#!/bin/bash
NUM_OF_VNIC=10
SWITCH_INTERFACE_STR = ""
for ((I=9; I<=$NUM_OF_VNIC; I++))
do
    NAME_INDEX=$(( 2 * I ))
    PAIR_NAME_2="veth"$NAME_INDEX
    NAME_INDEX=$((NAME_INDEX - 1))
    PAIR_NAME_1="veth"$NAME_INDEX
    sudo ip link add $PAIR_NAME_1 type veth peer name $PAIR_NAME_2
    sudo ifconfig $PAIR_NAME_1 up
    sudo ifconfig $PAIR_NAME_2 up
    SWITCH_INTERFACE_STR=$SWITCH_INTERFACE_STR" -i "$I"@"$PAIR_NAME_1
done

echo "Create vNIC Successfully."
echo $SWITCH_INTERFACE_STR

sudo simple_switch_grpc $SWITCH_INTERFACE_STR ./configurable_p4_demo/build/p4sfc_server_pkt_distribution.json --thrift-port 9093 --device-id 3 --log-console -- --grpc-server-addr localhost:50054 >switch.log 2>&1 &

echo "Start Server switch Successfully."

# sudo python ./p4sfc_server_daemon/main_controller.py 

# echo "Start P4SFC Server daemon Successfully."

# echo "P4SFC Start successfully."
