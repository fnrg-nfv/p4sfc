# P4SFC statelib

This directory contains the source codes of statelib(`libstate.so`) in P4SFC.
Statelib is used by our customized elements to create states. The state
created can be read by other programs through gRPC.

## Build command

```bash
make
```

## Running test

```bash
# after building
# in one bash
./server
# in another bash
./client
```
