#!/usr/bin/env bash
# Usage: ./sample_opencl_aes_sizes.sh $array_size $max_global_worksize

cd "$(dirname "$0")"
source ./common.sh
cd $ROOT
# set -x
set -e
EXECUTABLE=opencl_aes

ARRAY_SIZE="$1"
MAX_GLOBAL_WORKSIZE="$2"
shift 2

# 128 MB
# ARRAY_SIZE=$((128*1024*1024))
# number of GPU cores * 2
# MAX_GLOBAL_WORKSIZE=8

log2() {
    perl -le "print log($1)/log(2)"
}

sample_global_worksize() {
    local array_size="$1"
    local max_global_worksize="$2"
    shift 2

    build $EXECUTABLE
    copy_program_over $EXECUTABLE

    echo "> Array size (bytes): $array_size"
    echo "> Max global worksize: $max_global_worksize"

    for global_worksize in $(seq 1 $max_global_worksize); do 
        adb shell ./data/local/tmp/$EXECUTABLE $array_size $global_worksize
        echo
    done
}

sample_global_worksize $ARRAY_SIZE $MAX_GLOBAL_WORKSIZE
