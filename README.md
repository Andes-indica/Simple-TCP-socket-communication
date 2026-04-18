# Multi-Client TCP Server in C

This project implements a multi-client TCP echo server using POSIX sockets and `pthreads`.
The server binds to `127.0.0.1`, accepts multiple simultaneous clients, and creates one thread per client connection.

## Features

- TCP server using `AF_INET` and `SOCK_STREAM`
- Configurable port with default port `8080`
- One detached thread per client using `pthread_create()`
- Thread-safe active connection counter using a mutex
- Maximum client limit of `100`
- Graceful handling of disconnects and socket errors
- Echo-style acknowledgment for every received message
- Optional artificial delay to simulate network latency
- Optional random packet drop simulation for testing

## Files

- `server.c` - multi-client threaded TCP server
- `client.c` - simple test client for interactive messaging
- `Makefile` - build and cleanup commands
- `DEVELOPER_GUIDE.md` - detailed implementation walkthrough for contributors

## Build

Compile with the Makefile:

```bash
make
```

Manual compilation:

```bash
gcc -Wall -Wextra -std=c11 server.c -o server -lpthread
gcc -Wall -Wextra -std=c11 client.c -o client
```

## Run

Start the server:

```bash
./server
```

Run with custom options:

```bash
./server 9090 2 20
```

Arguments:

- `port` - listening port on `127.0.0.1` (default `8080`)
- `delay_seconds` - artificial delay before echoing back messages (default `0`)
- `drop_rate_percent` - percent chance to simulate dropping a received message (default `0`)

Start a client in another terminal:

```bash
./client
```

Or connect to a custom port:

```bash
./client 9090
```

## Example

1. Start the server with `./server 8080 1 10`
2. Open multiple terminals
3. Run `./client 8080` in each terminal
4. Send messages from each client
5. Watch the server log client IP, port, messages, disconnects, and active connection count

## Notes

- The server only listens on `localhost` (`127.0.0.1`)
- If more than `100` clients connect, new clients are rejected cleanly
- Simulated packet drops intentionally skip echo replies for some messages
- Press `Ctrl+C` to stop the server
