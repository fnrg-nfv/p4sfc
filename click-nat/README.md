# CLICK DEMO

说明：这个文件夹分为两部分，一部分是click与mininet以及p4的网络连通，第二部分为自己构建click element。暂时未将自己构建的click element用于mininet。

## 第一部分

1. 首先使用以下命令启动p4-mininet。（注：我修改了utils中的启动代码，只要在topology.json中加入 "hostIsDocker": false，即可使用P4host作为host而不是docker作为host）

    ```bash
    make run
    ```

2. 分别为两个hosts启动xterm

    ```bash
    xterm h1 h2
    ```

3. 分别在两个xterm中启动source.click与mjt-nat.click

    ```bash
    # h1
    click source.click

    # h2
    click mjt-nat.click
    ```

## 第二部分

第二部分主要是自行构建click element并进行测试（之后会将第二部分融入第一部分）。目前已将utils中的libswitch加入编译与链接中，并成功编译。主要需要修改的源文件为.cc .hh， .in .ac文件为配置文件，可以模仿click内的源码构造更复杂的element。

1. 进入p4sfc-click-package文件夹，构建package。（详情请看p4sfc-click-package/README）

    ```bash
    ./configure

    sudo make install
    ```

2. run test.click to test the generated element.

    ```bash
    click test.click
    ```
