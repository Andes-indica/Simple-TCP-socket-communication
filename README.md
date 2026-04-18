# TCP Server and Client in C

This folder contains a simple TCP socket communication program written in C.
It demonstrates basic client-server communication over IPv4 using `SOCK_STREAM`.

## Overview

- `server.c` creates a TCP server on port `8080`
- `client.c` connects to `127.0.0.1:8080`
- Both programs exchange messages interactively through the terminal
- Typing `exit` ends the chat session

This is a good beginner-friendly example for learning:

- socket creation
- binding and listening
- accepting client connections
- connecting from a client
- reading and writing over TCP

## Files

- `server.c` - TCP server implementation
- `client.c` - TCP client implementation
- `Makefile` - build and cleanup commands
- `.gitignore` - ignores compiled binaries and temporary files

## Requirements

- GCC or any C compiler
- Linux or a Unix-like environment

## Build

Use the provided Makefile:

```bash
make
```

Or compile manually:

```bash
gcc server.c -o server
gcc client.c -o client
```

## Run

Start the server first:

```bash
./server
```

Open another terminal in the same folder and start the client:

```bash
./client
```

## Example Flow

1. Run the server
2. Run the client
3. Type a message in one terminal
4. Reply from the other terminal
5. Type `exit` to close the chat

## Notes

- The client is hardcoded to connect to `127.0.0.1`
- The server listens on port `8080`
- The current implementation handles one client connection at a time
- Messages are exchanged using a fixed-size buffer of 80 bytes
