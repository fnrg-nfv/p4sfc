# P4SFC statelib

<!-- optimized from shared library to static library -->
This directory contains the source codes of statelib(`libstate.a`) in P4SFC.
Statelib is used by our customized elements to create states. The state
created can be read by other programs through gRPC.

## Build command

```bash
make
```

## Running test

```bash
make server
make client
# after building
# in one bash
./server
# in another bash
./client
```
