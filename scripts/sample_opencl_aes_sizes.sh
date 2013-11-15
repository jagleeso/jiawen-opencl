#!/usr/bin/env bash
# Usage: ./sample_opencl_aes_sizes.sh $global_worksize

cd "$(dirname "$0")"
source ./common.sh
cd $ROOT
# set -x
set -e
executable=opencl_aes
global_worksize="$1"
shift 1

# Guess the maximum input/output array size by starting with an initial value then 
# incrementing by constant value.  Error is at most the constant value.
max_array_size=""
guess_max_array_size_const_inc() {
    build $executable
    copy_program_over $executable
    local total_memory=$((2*1024*1024*1024))
    local increment=$(round_down_to_closest_factor $((total_memory/10)) 16)
    local size=$((16*1024*250))
    local i=0;
    while true; do
        echo "> Trying size == $size"
        if adb shell ./data/local/tmp/$executable $size $global_worksize 2>&1 | grep "Segmentation fault"; then
            if [ "$i" -eq "0" ]; then
                echo "ERROR: starting size ($size) is already too large and caused an error already" 
                exit 1
            fi
            max_array_size=$((size - increment))
            echo "> Max input/output array size (bytes): $max_array_size"
            echo "> Error in max input/output (bytes): $increment"
            break
        fi
        size=$((size + increment))
        i=$((i+1))
    done
}

# Guess the maximum input/output array size by starting with an initial value then 
# incrementing by a factor of the current value. Error is at most the largest value used. 
start_size=16
# max_array_size=""
start_guess_size=$((512*1024*1024))
guess_max_array_size_bin_exp() {
    build $executable
    copy_program_over $executable
    # local increment=10000
    local size=$start_guess_size
    local i=0
    while true; do
        echo "SIZE == $size"
        # adb shell ./data/local/tmp/$executable $size 2>&1
        # echo "...."
        if adb shell ./data/local/tmp/$executable $size $global_worksize 2>&1 | grep "Segmentation fault\|^Error:" || ! device_is_on; then
            if [ "$i" -eq "0" ]; then
                echo -n "ERROR: starting size ($size) is already too large and caused an error:" 
                if device_is_on --quiet; then
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

# sample_using_constant_increment_guess() {
# 
#     guess_max_array_size_const_inc
# 
#     SAMPLES=100
# 
#     INCREMENT=$((max_array_size / SAMPLES))
#     echo "> Sample input/ouput size increment (bytes): $INCREMENT"
#     echo "> Samples: $SAMPLES"
# 
#     build $executable
#     copy_program_over $executable
# 
#     array_size=$INCREMENT
#     for i in $(seq 1 $SAMPLES); do 
#         adb shell ./data/local/tmp/$executable $array_size
#         array_size=$((array_size + INCREMENT))
#         echo
#     done
# }

round_down_to_closest_factor() {
    number="$1"
    factor="$2"
    shift 2
    echo $(( (number/$factor)*$factor ))
}


sample_using_constant_increment_guess() {

    guess_max_array_size_const_inc

    # We need to sample using sizes divisible by 16.
    # Hence, we sample 100 samples from 1*floor(max_array_size/100) ... 100*floor(max_array_size/100).

    SAMPLES=100

    # The max array size rounded down to a number divisible by 16 (requirement of opencl_aes).
    ROUNDED_MAX_ARRAY_SIZE=$(( (max_array_size/(16*SAMPLES))*(16*SAMPLES) ))
    # Sample size is is INCREMENT*16*i for i=1..100 
    # i.e. 100 samples spaced evenly apart, each yielding a size divisible by 16
    INCREMENT=$(( ROUNDED_MAX_ARRAY_SIZE/SAMPLES ))
    echo "> Rounded down array size to closest 16*$SAMPLES bytes (bytes): $ROUNDED_MAX_ARRAY_SIZE"
    echo "> Sample input/ouput size increment (bytes): $INCREMENT"
    echo "> Samples: $SAMPLES"

    build $executable
    copy_program_over $executable

    for array_size in $(seq $INCREMENT $INCREMENT $ROUNDED_MAX_ARRAY_SIZE); do 
        adb shell ./data/local/tmp/$executable $array_size $global_worksize
        echo
    done
}


sample_using_binary_exponentiation_guess() {

    # guess_max_array_size_bin_exp

    max_array_size=$((512*1024*1024))
    echo "> Max input/output array size (bytes): $max_array_size"
    # echo "> Error in max input/output (bytes): $((size / 2))"

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
        adb shell ./data/local/tmp/$executable $array_size $global_worksize
        array_size=$((array_size * 2))
        echo
    done
}

# sample_using_constant_increment_guess
sample_using_binary_exponentiation_guess
