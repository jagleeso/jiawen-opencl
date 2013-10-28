#!/usr/bin/env bash

cd "$(dirname "$0")"
source ./common.sh
cd $ROOT
# set -x
set -e
executable=opencl_aes

max_array_size=""
start_size=128
guess_max_array_size() {
    build opencl_aes
    copy_program_over opencl_aes
    # local increment=10000
    local size=$start_size
    local i=0
    while true; do
        if adb shell ./data/local/tmp/opencl_aes $size 2>&1 | grep --quiet "Segmentation fault\|^Error:" || ! device_is_on; then
            if [ "$i" -eq "0" ]; then
                echo -n "ERROR: starting size ($size) is already too large and caused an error:" 
                if ! device_is_on; then
                    echo "a segmentation fault or OpenCL error"
                else
                    echo "the device was forcefully rebooted"
                fi
                exit 1
            fi
            max_array_size=$((size / 2))
            echo "> Max input/output array size (bytes): $max_array_size"
            echo "> Error in max input/output (bytes): $((size / 2))"
            break
        fi
        size=$((size * 2))
        i=$((i+1))
    done
}

log2() {
    perl -le "print log($1)/log(2)"
}

guess_max_array_size

echo "> Sample input/ouput size increment (bytes): powers of two starting at $start_size, ending at $max_array_size"
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
    adb shell ./data/local/tmp/$executable $array_size
    array_size=$((array_size * 2))
    echo
done
