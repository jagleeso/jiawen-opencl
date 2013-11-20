#!/bin/bash
ROOT=../

build() {
    local executable="$1"
    shift 1
    ndk-build PROGRAM=$executable
}

copy_program_over() {
    local executable="$1"
    shift 1
    adb push libs/armeabi-v7a/$executable /data/local/tmp
    adb push jni/$opencl /data/local/tmp
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
