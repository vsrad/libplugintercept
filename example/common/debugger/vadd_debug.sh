#!/bin/bash

USAGE="Usage: $0 -l line -f source_file_path -o debug_buffer_path -w watches -t counter [-a app_args] [-p perl_args]"

while getopts "l:f:o:w:t:a:p:" opt
do
	echo "$opt $OPTARG"
	case "$opt" in
	l) line=$OPTARG ;;
	f) src_path=$OPTARG ;;
	o) buffer_path=$OPTARG ;;
	w) watches=$OPTARG ;;
	t) counter=$OPTARG ;;
	a) app_args=$OPTARG ;;
	p) perl_args=$OPTARG ;;
	esac
done

[[ -z "$line" || -z "$src_path" || -z "$buffer_path" || -z "$watches" ]] && { echo $USAGE; exit 1; }

rm -rf tmp/
mkdir tmp

export ASM_DBG_SCRIPT_OPTIONS="-l $line -t $counter $perl_args"
export ASM_DBG_SCRIPT_WATCHES="$watches"
export INTERCEPT_CONFIG=tmp/config.toml

SCRIPTPATH=`dirname $0`
EXAMPLEPATH=`realpath $SCRIPTPATH/../../`

GFX=`/opt/rocm/bin/rocminfo | grep -om1 gfx9..`
CLANG="/opt/rocm/llvm/bin/clang"
CLANG_ARGS="-x assembler -target amdgcn--amdhsa -mcpu=$GFX -mno-code-object-v3 -I$EXAMPLEPATH/gfx9/include"

cat <<EOF > tmp/config.toml
[logs]
agent-log = "-"
co-log = "-"
co-dump-dir = "tmp"
[[buffer]]
size = 4194304
dump-path = "$buffer_path"
addr-env-name = "ASM_DBG_BUF_ADDR"
size-env-name = "ASM_DBG_BUF_SIZE"
[init-command]
exec = """bash -o pipefail -c '\
  perl $EXAMPLEPATH/common/debugger/breakpoint.pl $src_path $ASM_DBG_SCRIPT_OPTIONS -w "$ASM_DBG_SCRIPT_WATCHES" \
  | tee tmp/debug_src.s \
  | $CLANG $CLANG_ARGS -o tmp/debug.co -'"""
[[code-object-replace]]
condition = { co-load-id = 1 }
co-path = "tmp/debug.co"
EOF

# Path to the compiled https://github.com/vsrad/debug-plug-hsa-intercept
export HSA_TOOLS_LIB=$EXAMPLEPATH/../build/src/libplugintercept.so

CO_PATH="tmp/fp32_v_add.co"

$EXAMPLEPATH/build/gfx9/fp32_v_add -asm "$CLANG $CLANG_ARGS -o $CO_PATH $EXAMPLEPATH/gfx9/fp32_v_add.s" -c $CO_PATH $app_args
echo
