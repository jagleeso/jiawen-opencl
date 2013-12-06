#!/usr/bin/env bash
# Usage: ./sample_aes_work_group_size.sh $array_size $num_work_groups $max_work_group_size

cd "$(dirname "$0")"
source ./common.sh
cd $ROOT
# set -x
set -e
EXECUTABLE=opencl_aes

ARRAY_SIZE="$1"
# ideally (I believe), the number of GPU cores.
NUM_WORK_GROUPS="$2"
# we will experimentally determine this parameter's ideal value.
MAX_WORK_GROUP_SIZE="$3"
shift 3

# 128 MB
# ARRAY_SIZE=$((128*1024*1024))
# ideally (I believe), the number of GPU cores.
# NUM_WORK_GROUPS=4

log2() {
    perl -le "print log($1)/log(2)"
}

sample_global_worksize() {
    local array_size="$1"
    local num_work_groups="$2"
    local max_work_group_size="$3"
    shift 3

    build $EXECUTABLE
    copy_program_over $EXECUTABLE

    echo "> Array size (bytes): $array_size"
    echo "> Number of work groups: $num_work_groups"
    echo "> Max work group size: $max_work_group_size"

    for work_group_size in $(seq 1 $max_work_group_size); do 
        adb shell ./data/local/tmp/$EXECUTABLE -a $array_size -G $num_work_groups -I $work_group_size "$@"
        echo
    done
}

sample_global_worksize $ARRAY_SIZE $NUM_WORK_GROUPS $MAX_WORK_GROUP_SIZE "$@"
