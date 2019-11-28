# Simple demo

A simple demo program to test whether your environment has been set up correctly

## Step to Run

1. Compile p4 program and start Containernet topo
   
    ```bash
    make run
    ```

2. Open another terminal, run <u>mycontroller.py</u> as a controller to inject rules to bmv2.

    ```bash
    python mycontroller.py
    ```

3. For each created container, attach to its bash and start dpdk app: (replace <> with your own arguments)

    ```bash
    <dpdk-app> -l <core-list> -n <channel_number> --file-prefix <prefix> --no-pci --vdev 'eth_af_packet,iface=<veth>' -- <app-args>
    ```

    In this example, the command for the first docker container can be:
    ```bash
    pktgen -l 0-1 -n 4 --file-prefix pktgen1 --no-pci --vdev 'eth_af_packet,iface=h1-eth0' -- -P -T -m"1.0"
    ```

    and the command for the second docker container can be
    ```bash
    pktgen -l 2-3 -n 4 --file-prefix pktgen2 --no-pci --vdev 'eth_af_packet,iface=h2-eth0' -- -P -T -m"3.0"
    ```

    Now, the pktgen program should be started. For simplicity, we call them pktgen1 and pktgen2

4. In pktgen, set packets' dst_ip to correct value. For example, in pktgen1, you can input:
    ```bash
    set 0 dst ip 10.0.1.10
    ```
    where 10.0.1.10 is the ip address of the second docker container.<br>
    Similarly, you can input the following command for pktgen2:
    ```bash
    set 0 dst ip 10.0.0.10
    ``` 

5. In one of pktgen program, use command:
    ```bash
    start 0
    ```
    to let pktgen start generating packages.<br>
    
    Then, in another pktgen program, you should see that the program starts to receive packets. If you don't, it means that something is wrong with your environment setting.

6. Congratulations! your environment has been set up correctly and you can start your own exploration!