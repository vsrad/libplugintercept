# Debug Plug HSA Intercept

*(Early work in progress)*

The following API calls are intercepted:

1. When `hsa_queue_create` is called, a debug buffer is allocated if the `ASM_DBG_BUF_SIZE` environment variable is set.
2. During `hsa_code_object_reader_create_from_memory`, the code object is searched for instructions issued by the plug to setup debug buffer:
```
s_mov_b32 s[xx], 0x7f7f7f7f
s_mov_b32 s[xx+1], 0x7f7f7f7f
```
(`0x7f7f7f7f` is a placeholder sequence. It is replaced with the address of the buffer allocated at step one.)

3. When `hsa_signal_wait_scacquire` is called, copy local debug buffer to system memory and dump to the file if kernel execution completed and `ASM_DBG_PATH` environment variable is set.

## Development

To build the library and the test executable, run:

```
mkdir build
cd build
cmake ..
make
```

Before running the test executable, set `HSA_TOOLS_LIB`:
```
export HSA_TOOLS_LIB=`pwd`/build/src/libplugintercept.so
```
## Example 

### Remote machine

1. Build [debug interrupter](#Development)
2. Build example:
    * Go to `example` direcotry
    * mkdir build && cd build
    * cmake ..
    * make

### Host machine

1. Create new *Radeon Asm Project*
2. Go to *Tools* -> *RAD Debug* -> *Options*
3. Click the *Edit* button in the opened window to edit the active debug profile
4. Set *Remote Machine Address*
5. In the *Debugger* tab, set *Working Directory* to the absolute path
to `example` on the remote machine
6. Press *Apply* to save the changes and *OK* to close the profile editor

Let's start debugger!

