mkdir -p `pwd`/example/tmp_dir

export HSA_TOOLS_LIB=`pwd`/build/src/libplugintercept.so
export ASM_DBG_PATH=`pwd`/example/tmp_dir/debug_output
export ASM_DBG_BUF_SIZE=400000
export ASM_DBG_WRITE_PATH=`pwd`/example/tmp_dir
export ASM_DBG_CO_LOG_PATH="-"