# Basic

A simple demo program to test whether your environment has been set up correctly

## Step to Run

1. Compile p4 program and start Containernet

    ```bash
    make run
    ```

1. For each created container, attach to its bash and run dpdk app: (replace <> with your own arguments)

    ```bash
    <dpdk-app> -l <core-list> -n <channel_number> --file-prefix <prefix> --no-pci --vdev 'eth_af_packet,iface=<veth>' -- <app-args>
    ```

    In this pktgen example, the command for the first docker container can be:

    ```bash
    pktgen -l 0-1 -n 4 --file-prefix pktgen1 --no-pci --vdev 'eth_af_packet,iface=h1-eth0' -- -P -T -m"1.0"
    ```

    and the command for the second can be

    ```bash
    pktgen -l 2-3 -n 4 --file-prefix pktgen2 --no-pci --vdev 'eth_af_packet,iface=h2-eth0' -- -P -T -m"3.0"
    ```

    Now, the pktgen program should be started. For simplicity, we call them pktgen1 and pktgen2

1. In pktgen, set packets' dst_ip to correct value. For example, in the first pktgen, you can input:

    ```bash
    set 0 dst ip 10.0.2.2
    ```

    where 10.0.1.1 is the ip address of the second docker container.

    Similarly, you can input the following command for pktgen2:

    ```bash
    set 0 dst ip 10.0.1.1
    ```

1. In one of pktgen program, use command:

    ```bash
    str
    ```

    to let pktgen start generating packages.

    Then, the other pktgen program starts to receive packets. Otherwise, it means that something is wrong in your environment setting.

1. Congratulations! your environment has been set up correctly and you can start your own exploration!

## TODO

- h1 and h2 in the above containernet cannot ping with each other.
