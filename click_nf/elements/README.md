# P4SFC Elements

This directory contains the source codes of customized elements in P4SFC.
These elements should be built together with fastclick.

## Dependency

\*.[cc]/[hh] must be symbolically linked in <fastclick>/elements/local.

```bash
cd <fastclick>/elements/local
ln -s <p4sfc>/click_nf/elements/*.cc .
ln -s <p4sfc>/click_nf/elements/*.hh .
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
