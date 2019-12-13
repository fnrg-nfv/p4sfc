FROM ubuntu:xenial

WORKDIR /usr/src/dpdk
COPY ./build/app /usr/src/dpdk
COPY ./p4_so_libs /usr/local/lib
ENV PATH "$PATH:/usr/src/dpdk/app/"

RUN apt update
RUN apt install -y numactl net-tools iproute2 iputils-ping libboost-all-dev libjudy-dev vim

CMD ["/bin/bash"]
