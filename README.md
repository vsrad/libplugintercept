# Debug Plug HSA Intercept

## Development

To build the library and the test executable, run:

```sh
mkdir build
cd build
cmake ..
make
```

It is possible to override the code object compiler:

```sh
cmake .. -DCLANG=/opt/rocm/opencl/bin/x86_64/clang
```

## Examples

### Debugging

#### Remote machine

1. Build [libplugintercept](#Development)

2. Build the example:
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

Launch the debugger and open *Debug Visualizer* -- you should see the watches populated with the debug buffer contents.

### Logging code object loads

1. Build [libplugintercept](#Development)

2. Create a `.toml` configuration file with the following contents:
```toml
[agent]
log = "-"

[code-object-dump]
log = "-"
directory = "/co/files/dump/dir"
```

3. Specify path to the configuration file in `$ASM_DBG_CONFIG`:
```sh
export ASM_DBG_CONFIG=/path/to/config.toml
```

4. Add `libplugintercept.so` to `$HSA_TOOLS_LIB`:
```sh
# Assuming the current working directory is the repository root
export HSA_TOOLS_LIB=`pwd`/build/src/libplugintercept.so
```

5. Run the target application

#### Gotchas

Test runners may need special configuration to pass down environment variables to test applications. For example, when using `tox`, you need to specify `passenv = HSA_TOOLS_LIB ASM_DBG_CONFIG`.
