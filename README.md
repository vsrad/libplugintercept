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
