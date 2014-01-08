#!/usr/bin/env bash
cd "$(dirname "$0")"
source ./common.sh
cd $ROOT
# set -x

set -e

# e.g. $1 = 512MB -> 512*1024*1024 bytes / 4 bytes per int
samples="$1"
max_array_size_MB="$2"
num_work_groups="$3"
min_profile_time_ms="$4"

shift 4 || (echo 1>&2 "ERROR: $0 samples max_array_size_MB num_work_groups min_profile_time_ms" && exit 1)

args=("$@")

memcopy_with_size() {
    local array_size="$1"
    shift 1
    run_program memcopy -G $num_work_groups -t $min_profile_time_ms -a $array_size "${args[@]}" 
}

sample_array_size_using_constant_increment memcopy $samples $max_array_size_MB memcopy_with_size
