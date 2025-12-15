# Distributed Shared Memory (DSM) Middleware

A C++ middleware for managing distributed shared memory across multiple nodes using gRPC for inter-node communication and a home-based cache coherence protocol.

## Overview

This DSM middleware provides:
- **Home-based cache coherence**: One designated home node stores the canonical copy of shared objects
- **gRPC networking**: Inter-node RPC calls for data synchronization
- **Lock management**: Distributed lock acquisition/release for synchronized access
- **Object handles**: Type-safe read/write access to shared objects with automatic serialization

## Building

```bash
cd build
cmake ..
make
```

The project requires:
- C++17 or later
- CMake 3.15+
- gRPC and Protocol Buffers (must be pre-installed)

## Configuration

### Cluster Configuration (`cluster.txt`)
Define the cluster topology with node ID, IP, and port:
```
# ID   IP           PORT
0      127.0.0.1    50050
1      127.0.0.1    50051
2      127.0.0.1    50052
```

## Running

Start each node with its node ID (0-indexed) in interactive mode for testing:

```bash
./build/dsm_node 0 cluster.txt interactive    # Start node 0
./build/dsm_node 1 cluster.txt interactive    # Start node 1
./build/dsm_node 2 cluster.txt interactive    # Start node 2
```

## Interactive Commands

- `get <key>` - Read a value
- `put <key> <value>` - Write a value to a key
- `rm <key>` - Remove a key
- `slowget <key>` - Read with 10-second wait after read (to test concurrency)
- `slowput <key> <value>` - Write with 10-second wait after write (to test concurrency)

## Architecture

- **DsmCore**: Main DSM logic with read/write semantics
- **DsmNetwork**: Abstract interface for inter-node communication
- **GrpcDsmNetwork**: gRPC implementation of DsmNetwork
- **LockManager**: Distributed lock coordination
- **ObjectStore**: Local storage for cached objects
- **DsmServiceImpl**: gRPC service handler for incoming messages