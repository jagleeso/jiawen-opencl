#!/usr/bin/env bash
# Usage: ./sample_aes_entries.sh $array_size $entries

cd "$(dirname "$0")"
source ./common.sh
cd $ROOT
# set -x
set -e
EXECUTABLE=opencl_aes

ARRAY_SIZE="$1"
MAX_ENTRIES="$2"
shift 2

# 128 MB
# ARRAY_SIZE=$((128*1024*1024))
# ideally (I believe), the number of GPU cores.
# NUM_WORK_GROUPS=4

log2_round_up() {
    perl -le "
    \$x = log($1)/log(2); 
    if (\$x == int(\$x)) { 
        print $1;
    } else { 
        print (2**(int(\$x) + 1)); 
    }"
}

sample_entries() {
    local array_size="$1"
    local max_entries="$2"
    shift 2

    build $EXECUTABLE
    copy_program_over $EXECUTABLE

    echo "> Array size (bytes): $array_size"
    echo "> Max entries: $max_entries"

    # for entries in $(seq $max_entries -1 1); do 
    # for entries in $(seq 1 100 $max_entries); do 
    for entries in $(seq 1 $max_entries); do 
        adb shell ./data/local/tmp/$EXECUTABLE -a $array_size -e $entries
        echo
    done
}

sample_entries_exp() {
    local array_size="$1"
    local max_entries="$2"
    shift 2
    # local max_entries="104857.6"

    local power_2_max_entries=$(log2_round_up $max_entries)

    build $EXECUTABLE
    copy_program_over $EXECUTABLE

    echo "> Array size (bytes): $array_size"
    echo "> Max entries: $max_entries"

    local entries=1
    # for entries in $(seq 1 $max_entries); do 
    while [ "$entries" -lt "$power_2_max_entries" ]; do
        adb shell ./data/local/tmp/$EXECUTABLE -a $array_size -e $entries
        entries=$((entries*2))
    done

}

# sample_entries_exp $ARRAY_SIZE $MAX_ENTRIES
sample_entries $ARRAY_SIZE $MAX_ENTRIES
