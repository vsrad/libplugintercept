# Debug Plug HSA Intercept

## Development

To build the library and the test executable, run:

```sh
mkdir build
cd build
cmake ..
make
```

## Example

### Remote machine

1. Build [libplugintercept](#Development)
2. Build the example:
```sh
cd example
mkdir build && cd build
cmake ..
make
```

### Host machine

1. Create new *Radeon Asm Project*
2. Go to *Tools* -> *RAD Debug* -> *Options*
3. Click the *Edit* button in the opened window to edit the active debug profile
4. Set *Remote Machine Address*
5. In the *Debugger* tab, set *Working Directory* to the absolute path
to `example` on the remote machine
6. Press *Apply* to save the changes and *OK* to close the profile editor

Launch the debugger and open *Debug Visualizer* -- you should see the watches populated with the debug buffer contents.
