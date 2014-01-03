#!/usr/bin/env bash
# Usage: ./sample_coalesce_sizes.sh $global_worksize

cd "$(dirname "$0")"
source ./common.sh
cd $ROOT
# set -x

set -e
executable=coalesce

max_array_size=$(( $1 * 1024 * 1024 ))
num_work_groups="$2"
min_profile_time_ms="$3"
shift 3 || (echo 1>&2 "ERROR: $0 max_array_size_MB num_work_groups min_profile_time_ms" && exit 1)
# start_size=16
start_size=$((128*1024*1024))

sample_using_binary_exp() {

    echo "> Max input array size (bytes): $max_array_size"
    # echo "> Error in max input/output (bytes): $((size / 2))"

    echo "> Sample input size increment (bytes): powers of two starting at $start_size, ending at $max_array_size"
    start_samples=$(log2 $start_size)
    max_samples=$(log2 $max_array_size)
    SAMPLES=$((max_samples - start_samples + 1))
    echo "> Samples: $SAMPLES"

    # when guessing we might have caused the device to hang
    wait_for_device_reboot

    build $executable
    copy_program_over $executable

    array_size=$start_size
    while [ "$array_size" -le "$max_array_size" ]; do 
        adb shell ./data/local/tmp/$executable -a $array_size -G $num_work_groups -t $min_profile_time_ms "$@"
        array_size=$((array_size * 2))
        echo
    done
}

sample_using_binary_exp "$@"
