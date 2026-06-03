# Prototype: RPC + CMake - bootstrap

A minimal application built with CMake that sets up a `pw_rpc` server
and uses Nanopb for protobuf codegen. The app and protobuf codegen builds
without a Pigweed bootstrap. I use a bootstrap to spin up `pw_console` and
verify that I can send an RPC to the app.

## Dependencies

On Debian-based systems, running `./install.sh` should get you
most or everything you need. This list was generated after the
fact so I may have missed something.

## Build

```
./build.sh
```

The build script adds a bunch of Pigweed module paths to the Python
system path. Other than that it's a normal CMake build.

## Run

1. Open a console tab and run `./build/rpc_demo`.

   The app boots up and starts listening on `localhost:33000`.

2. Open another console tab, `cd` into the root of the Pigweed repo, bootstrap,
   and then start a `pw_console` session so that you can test sending an
   RPC to the app.

   ```
   cd third_party/pigweed
   . bootstrap.sh
   python3 ../../run_console.py`
   ```

   We are only using bootstrap here to simplify the prototype. In real-world
   usage I imagine you'd have other ways to send RPCs to devices. I'm not sure
   if it's a big lift to get `pw_console` working without a bootstrap.

3. In the Python REPL of `pw_console` send an RPC to the app.

   ```
   device.rpcs.rpc.ping.PingService.Ping(value="rpc")
   ```

4. In the console tab running the app, you should see `rpc` printed to
   `stdout`.

## Architecture

This demo implements only the server side of the RPC communication.

### Communication

The server uses `pw_rpc::system_server` (configured with the `host` backend),
which listens on a TCP socket (port 33000). Communication over the socket
uses HDLC framing (`pw_hdlc`).

### Proto generation

To allow `pw_console` to interact with our custom `ping.proto` service, the
CMake build automatically generates the standard Python protobuf module
(`ping_pb2.py`) to `build/generated_python`. The `run_console.py` script adds
this directory to `sys.path` and passes the module to `pw_system.console` which
dynamically builds the RPC client at runtime.
