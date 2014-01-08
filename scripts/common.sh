#!/bin/bash
ROOT=$(readlink -f ../)

build() {
    local executable="$1"
    shift 1
    cd "$ROOT"
    make templates
    cd -
    ndk-build PROGRAM=$executable
}

copy_program_over() {
    local executable="$1"
    shift 1
    adb push libs/armeabi-v7a/$executable /data/local/tmp
    tmpdir=$(mktemp -d)
    cp jni/*.cl $tmpdir
    adb push $tmpdir /data/local/tmp
    rm -r $tmpdir
}

run_program() {
    local executable="$1"
    shift 1
    adb shell ./data/local/tmp/$executable "$@"
}

device_is_on() {
    adb devices | \
        grep -v "List of devices attached" | \
        grep "device" "$@"
}

wait_for_device_reboot() {
    while true; do
        # poll until the device is available
        device_is_on && break
        sleep 1
    done
}

# Sampling

log2() {
    perl -le "print log($1)/log(2)"
}

sample_powers_of_2_using_constant_start() {
    local executable="$1"
    local start_size="$2"
    local max_array_size="$3"
    shift 3

    echo "> Max input/output array size (bytes): $max_array_size"
    # echo "> Error in max input/output (bytes): $((size / 2))"

    echo "> Sample input/ouput size increment (bytes): powers of two starting at $start_size, ending at $max_array_size"
    local start_samples=$(log2 $start_size)
    local max_samples=$(log2 $max_array_size)
    local SAMPLES=$((max_samples - start_samples + 1))
    echo "> Samples: $SAMPLES"

    build $executable
    copy_program_over $executable

    local array_size=$start_size
    while [ "$array_size" -le "$max_array_size" ]; do 
        run_program $executable $array_size
        array_size=$((array_size * 2))
        echo
    done
}

# sample_array_size_using_constant_increment 100 512 memcopy_with_size
sample_array_size_using_constant_increment() {
    local executable="$1"
    local samples="$2"
    local max_array_size_MB="$3"
    local run_with_size="$4"
    shift 4

    # The max array size rounded down to a number divisible by 16*samples (requirement of uint4 
    # datatype).
    local rounded_max_array_size_bytes=$(( ((max_array_size_MB*1024*1024)/(16*samples))*(16*samples) ))
    # uint -> 4 bytes
    local max_array_size=$((rounded_max_array_size_bytes/4))
    # Sample size is is increment*16*i for i=1..100 
    # i.e. 100 samples spaced evenly apart, each yielding a size divisible by 16
    local increment=$(( max_array_size/samples ))
    echo "> Max array_size (bytes): $rounded_max_array_size_bytes"
    echo "> Max array_size: $rounded_max_array_size_bytes"
    echo "> Sample array_size increment (bytes): $increment"
    echo "> Samples: $samples"

    build $executable
    copy_program_over $executable

    for array_size in $(seq $increment $increment $max_array_size); do 
        $run_with_size $array_size
    done
}
