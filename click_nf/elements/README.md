# P4SFC Elements

This directory contains the source codes of customized elements in P4SFC.
These elements should be built together with fastclick.

## Dependency

./src/\* must be symbolically linked in <fastclick>/elements/local.

```bash
ln -s <p4sfc>/click_nf/elements/* <fastclick>/elements/local/
```

## Building command in fastclick

```bash
cd <fastclick>
autoconf
./configure --enable-dpdk --enable-intel-cpu --verbose --enable-select=poll CFLAGS="-O3" CXXFLAGS="-std=c++11 -O3"  --disable-dynamic-linking --enable-poll --enable-bound-port-transfer --enable-local --enable-flow --disable-task-stats --disable-cpu-load
# if memory is not sufficient: make -j6 or make -j4
make -j
sudo make install
```

## Test

To test whether one element can work correctly, running the following command under test directory:
```bash
sudo bash
~/fastclick/bin/click --dpdk -l 0-1 -n 4 -- <NF>.click
```
