FROM ubuntu:xenial

WORKDIR /usr/src/dpdk
COPY ./build/app /usr/src/dpdk
ENV PATH "$PATH:/usr/src/dpdk/app/"

RUN apt update
RUN apt install -y numactl

CMD ["/bin/bash"]
