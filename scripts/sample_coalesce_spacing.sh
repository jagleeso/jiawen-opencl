#!/usr/bin/env bash
cd "$(dirname "$0")"
source ./common.sh
cd $ROOT
# set -x

set -e

# e.g. $1 = 512MB -> 512*1024*1024 bytes / 4 bytes per int
max_array_size_MB="$1"
num_work_groups="$2"
min_profile_time_ms="$3"
max_spacing="$4"
prefix="$5"
shift 5 || (echo 1>&2 "ERROR: $0 max_array_size_MB num_work_groups min_profile_time_ms max_spacing prefix" && exit 1)

for spacing in $(seq 0 $max_spacing); do 
    scripts/sample_coalesce_sizes.sh \
        "$max_array_size_MB" "$num_work_groups" "$min_profile_time_ms" \
        -s "$spacing" \
        | tee "$prefix.${spacing}_spacing.txt"
done
