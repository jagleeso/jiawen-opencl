#!/bin/bash
ROOT=../

build() {
    local executable="$1"
    ndk-build PROGRAM=$executable
}

copy_program_over() {
    local executable="$1"
    adb push libs/armeabi-v7a/$executable /data/local/tmp
    adb push jni/$opencl /data/local/tmp
}

device_is_on() {
    adb devices | \
        grep -v "List of devices attached" | \
        grep "device" --quiet
}

wait_for_device_reboot() {
    while true; do
        # poll until the device is available
        device_is_on && break
        sleep 1
    done
}
