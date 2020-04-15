# libplugintercept

`libplugintecept` is an HSA tools library for replacing GPU kernels at runtime without modifying host applications. It works by intercepting calls to specific `hsa_` functions and (optionally) substituting the values returned to an application.

## Features

* Saving each [_code object_](http://www.hsafoundation.com/html/Content/Runtime/Topics/02_Core/code_objects_and_executables.htm) loaded by an application to disk for further inspection
* Replacing specific code objects with external binaries
* Setting up a trap handler from an external binary
* Allocating auxiliary GPU buffers
* Running a custom shell command before loading a replacement code object

The addresses of all auxiliary buffers can be read (as environment variables) by the shell command, which enables, for instance, a [source-level debugging workflow](#debugging-assembly-code).

## Setup

Build the library and the test executable:

```sh
mkdir build
cd build
cmake ..
make
```

Optionally run the tests:

```sh
./build/tests/tests
```

Before launching the host application, add `libplugintercept.so` to the `HSA_TOOLS_LIB` environment variable. To avoid specifying an absolute path to the library, create a symbolic link to it in `/opt/rocm/lib`:

```sh
ln -s `pwd`/src/libplugintercept.so /opt/rocm/lib
export HSA_TOOLS_LIB=libplugintercept.so
```

## Configuration

At launch, the library loads a configuration file from the path specified in the `INTERCEPT_CONFIG` environment variable. Supported options and their intended usage are listed in the example file located in [`tests/fixtures/config.toml`](https://github.com/vsrad/debug-plug-hsa-intercept/blob/master/tests/fixtures/config.toml).

## Examples

### Logging code object loads

1. Create a `.toml` configuration file with the following contents:
```toml
[logs]
# log info messages to stdout:
agent-log = "-"
# log code object loads and hsa_executable_symbol_get_info calls to stdout:
co-log = "-"
# dump code objects to /tmp/co_loads:
co-dump-dir = "/tmp/co_loads"
```

2. Specify path to the configuration file in `INTERCEPT_CONFIG`:
```sh
export INTERCEPT_CONFIG=/path/to/config.toml
```

3. Add `libplugintercept.so` to `HSA_TOOLS_LIB`:
```sh
ln -s `pwd`/src/libplugintercept.so /opt/rocm/lib
export HSA_TOOLS_LIB=libplugintercept.s
```

4. Run the target application

### Debugging generated GPU kernels

Refer to the [documented sample](https://github.com/vsrad/Tensile/blob/kernel-debug/debug/README.md) for debugging [Tensile](https://github.com/ROCmSoftwarePlatform/Tensile)-generated HIP and assembly kernels.

### Debugging assembly code

#### Remote machine

Build the example:
```sh
cd example
mkdir build && cd build
cmake ..
make
```

#### Host machine

1. Create new *Radeon Asm Project*
2. Go to *Tools* -> *RAD Debug* -> *Options*
3. Click the *Edit* button in the opened window to edit the active debug profile
4. Set *Remote Machine Address*
5. In the *Debugger* tab, set *Working Directory* to the absolute path
to `example` on the remote machine
6. Press *Apply* to save the changes and *OK* to close the profile editor

Launch the debugger and open *Debug Visualizer* — you should see the watches populated with the debug buffer contents.
