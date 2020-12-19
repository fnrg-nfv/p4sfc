# P4SFC Elements

This directory contains the source codes of customized elements in P4SFC.
These elements should be built together with fastclick.

## Dependency

1. *.[cc]/[hh] be symbolically linked in <fastclick>/elements/local.

2. Add the following library dependency statements into <fastclick>/userlevel/Makefile.in.

    ```bash
    # for statelib
    INCLUDES += -I. -I$(top_builddir) -I$(srcdir) $(CLICKINCLUDES) -I/home/sonic/p4sfc/click_nf/statelib
    LDFLAGS += -L/home/sonic/p4sfc/click_nf/statelib
    LIBS += -L/home/sonic/p4sfc/click_nf/statelib -lstate -lprotobuf
    ```

## Building command in fastclick

```bash
cd <fastclick>
autoconf
./configure --enable-dpdk --enable-intel-cpu --verbose --enable-select=poll CFLAGS="-O3" CXXFLAGS="-std=c++11 -O3"  --disable-dynamic-linking --enable-poll --enable-bound-port-transfer --enable-local --enable-flow --disable-task-stats --disable-cpu-load
make -j
sudo make install
```