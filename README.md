# pw_rpc Minimal Demo (Server)

This project demonstrates a minimal setup of Pigweed's `pw_rpc` server using `nanopb` for code generation in a vanilla CMake project, without requiring the full Pigweed bootstrap for the C++ build.

## Prerequisites

Before building, you must install the required dependencies.
On Debian-based systems, you can use the provided script:

```bash
./prereqs.sh
```

For other systems, ensure you have:
*   C/C++ compiler (e.g. GCC, Clang)
*   CMake (>= 3.20)
*   Ninja (or Make)
*   Protobuf compiler (`protoc`)
*   Python 3 with `protobuf` and `pyserial` libraries.

## Build and Run Server

1.  **Build**:
    ```bash
    ./build.sh
    ```
2.  **Run Server**:
    ```bash
    ./build/rpc_demo
    ```
    The server will start and listen on TCP port 33000.

## Interacting with the Server (pw_console)

To interact with the running server, you can use Pigweed's `pw_console` from a bootstrapped Pigweed environment.

1.  **Bootstrap Pigweed**:
    Go to your Pigweed repository root (or the submodule `third_party/pigweed` in this project) and bootstrap it:
    ```bash
    cd third_party/pigweed
    . ./bootstrap.sh
    ```
2.  **Run Console**:
    Run the helper script `run_console.py` from this project *within* the bootstrapped terminal (it defaults to socket connection):
    ```bash
    python /path/to/this/project/run_console.py
    ```
    *Note: Replace `/path/to/this/project` with the actual absolute path to this project's root. You can still pass standard `pw_system.console` flags if needed.*
3.  **Send RPC**:
    In the Python REPL at the bottom of the console, you can send RPCs to the device:
    ```python
    device.rpcs.rpc.ping.PingService.Ping(value="Hello Pigweed RPC!")
    ```
    You should see the response in the console.

## Architecture

This demo implements only the server side of the RPC communication.

### Communication

The server uses `pw_rpc::system_server` (configured with the `host` backend), which listens on a TCP socket (port 33000).
Communication over the socket uses HDLC framing (`pw_hdlc`).

### Python Proto Generation

To allow `pw_console` to interact with our custom `ping.proto` service, the CMake build automatically generates the standard Python protobuf module (`ping_pb2.py`) to `build/generated_python`.
The `run_console.py` script adds this directory to `sys.path` and passes the module to `pw_system.console` which dynamically builds the RPC client at runtime.
