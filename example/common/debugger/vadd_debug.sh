#!/bin/bash

USAGE="Usage: $0 -l line -f source_file -o debug_buffer_path -w watches -t counter -p perl_args"

while getopts "l:f:o:w:t:" opt
do
	echo "-$opt $OPTARG"
	case "$opt" in
	l) line=$OPTARG ;;
	f) source_file=$OPTARG ;;
	o) debug_buffer_path=$OPTARG ;;
	w) watches=$OPTARG ;;
	t) counter=$OPTARG ;;
	p) perl_args=$OPTARG ;;
	esac
done

[[ -z "$line" || -z "$source_file" || -z "$debug_buffer_path" || -z "$watches" ]] && { echo $USAGE; exit 1; }

rm -rf tmp/
mkdir tmp

export INTERCEPT_CONFIG=tmp/config.toml
cat <<EOF > tmp/config.toml
[logs]
agent-log = "-"
co-log = "-"
co-dump-dir = "tmp"
[[buffer]]
size = 1048576
dump-path = "$debug_buffer_path"
addr-env-name = "ASM_DBG_BUF_ADDR"
size-env-name = "ASM_DBG_BUF_SIZE"
[init-command]
exec = """bash -o pipefail -c '\
  perl common/debugger/breakpoint.pl -ba \$ASM_DBG_BUF_ADDR -bs \$ASM_DBG_BUF_SIZE \
    -l $line -w "$watches" -s 96 -r s0 -t ${counter:=0} -p $perl_args $source_file \
  | /opt/rocm/llvm/bin/clang -x assembler -target amdgcn--amdhsa -mcpu=gfx900 -mno-code-object-v3 \
    -Igfx9/include -o tmp/debug.co -'"""
[[code-object-replace]]
condition = { co-load-id = 1 }
co-path = "tmp/debug.co"
EOF

# Path to the compiled https://github.com/vsrad/debug-plug-hsa-intercept
export HSA_TOOLS_LIB=../build/src/libplugintercept.so

./build/gfx9/fp32_v_add --asm "$source_file" --include gfx9/include --output_path tmp/fp32_v_add.co --clang /opt/rocm/llvm/bin/clang
echo
