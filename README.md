# Peer Time Sync

A peer-to-peer network clock synchronization system that implements the **Network Time Protocol** (NTP) algorithm to allow nodes to synchronize their clocks by accounting for network packet travel times.


## Overview

This application implements a peer-to-peer network for clock synchronization. Each node in the network synchronizes with other nodes by accounting for packet transmission delays. The network operates with equal rights for all nodes, with one node serving as a synchronization leader.


## 📌 Features

- ⏱️ Precise clock synchronization using timestamp exchanges
- 🌐 Peer-to-peer communication model over UDP
- 🧠 Implements NTP-like offset and delay estimation:
  ```
  offset = (T2 - T1 + T3 - T4) / 2
  delay  = (T4 - T1) - (T3 - T2)
  ```
- 🧪 Includes Python testing scripts for simulation and validation
- 🔄 Leader election and correction propagation between nodes


## How It Works

### Node Synchronization Levels

Each node maintains a synchronization level:
- **255**: Node is not synchronized with any node
- **0**: Node is the synchronization source for other nodes (leader)
- **1**: Node is synchronized directly with the leader
- **2+**: Node is synchronized with a node that is synchronized with the leader (up to 254)

### 📦 Building and Running

```bash
# Navigate to the src directory
cd src

# Build the project
make

# The binary will be available in the 'build' directory
../build/peer-time-sync [options]
```

To see the log messages:
```bash
make debug
```

To clean build artifacts:
```bash
make clean
```

### 🧪 Testing

The `tests/` folder includes Python scripts for simulating different scenarios.
To run tests:

```bash
python3 tests/test{test_id}.py
```

## Command-Line Parameters

The program accepts the following command-line parameters:

- `-b bind_address` - IP address on which the node listens (optional, default: all host addresses)
- `-p port` - Port on which the node listens (optional, default: 0 for any available port)
- `-a peer_address` - IP address or hostname of another node to connect with (optional)
- `-r peer_port` - Port of another node to connect with (optional, required if -a is specified)

Parameters can be specified in any order. Both `-a` and `-r` must be provided together when connecting to an existing node.

## Network Protocol

### Messages

Nodes communicate using the following message types:

#### Network Joining
- **HELLO (1)**: Initiates communication with another node
- **HELLO_REPLY (2)**: Response to HELLO, containing information about other known nodes
- **CONNECT (3)**: Requests connection with a node
- **ACK_CONNECT (4)**: Confirms connection establishment

#### Time Synchronization
- **SYNC_START (11)**: Contains synchronization level and current timestamp
- **DELAY_REQUEST (12)**: Requests delay measurement
- **DELAY_RESPONSE (13)**: Contains synchronization level and response timestamp

#### Leader Selection
- **LEADER (21)**: Used to designate a node as leader (0) or remove leader status (255)

#### Time Information
- **GET_TIME (31)**: Requests current time from a node
- **TIME (32)**: Contains synchronization level and current timestamp

### Joining the Network

A node can join the network in two ways:
1. If started without `-a` and `-r` parameters, it waits for new participants
2. If started with `-a` and `-r` parameters, it sends a HELLO message to the specified node

### Time Synchronization Process

1. Nodes with synchronization level < 254 periodically send SYNC_START messages
2. A receiving node responds with DELAY_REQUEST if the sender meets certain criteria
3. The original sender responds with DELAY_RESPONSE
4. The synchronizing node calculates the offset: `(T2 - T1 + T3 - T4) / 2`
5. The node becomes synchronized at a level one higher than the synchronization source

The implementation uses the Network Time Protocol (NTP) synchronization algorithm, which accounts for network delay by calculating the offset between clocks using the formula:

```
offset = (T2 - T1 + T3 - T4) / 2
```

Where:
- T1: Timestamp when the source node sends SYNC_START
- T2: Timestamp when the receiving node receives SYNC_START
- T3: Timestamp when the receiving node sends DELAY_REQUEST
- T4: Timestamp when the source node receives DELAY_REQUEST

### Leader Selection

A node becomes a leader when it receives a LEADER message with value 0, and stops being a leader when it receives a LEADER message with value 255.
