#ifndef COMMON_H
#define COMMON_H

#include <CL/cl.h>
#include <sys/types.h>
#include <unistd.h>
#include "aopencl.h"


#define CEILING_DIVIDE(value, divisor) (((value) + (divisor) - 1)/(divisor))
/* Round value up to the nearest multiple of divisor.
 */
#define ROUND_UP(value, divisor) ((((value) + (divisor) - 1)/(divisor))*(divisor))

#define CHECK_CL_SUCCESS(task_todo, errvar) \
if (errvar != CL_SUCCESS) \
{ \
    printf("Error: %s line %d, Failed to " task_todo ": errcode_ret=%i\n", __FILE__, __LINE__, errvar); \
    exit(EXIT_FAILURE); \
} \

void print_data(const char* name, unsigned int count, unsigned char* data);
void check_encrypted_data(const char* name, unsigned int count, unsigned char* data);
void check_equal(const char* name1, const char* name2, unsigned int count, unsigned char* data1, unsigned char* data2);

const char *load_kernel_source(const char *filename);
int safe_cmp(const char * safe_str, const char * user_str);
int get_max_work_items(cl_device_id device_id, cl_uint *dims, size_t *max_work_items_dim1);
double milliseconds(struct timeval t);
double get_time_ms(struct timeval start);

#define BENCHMARK(code, total_run_time, avg_run_time, runs, min_profile_time_ms) \
    total_run_time = 0; \
    avg_run_time = 0; \
    runs = 1; \
    while (1) { \
        printf("trying %u runs...\n", runs); \
        printf("run once to handle cache effects...\n"); \
        code; \
        printf("start real runs...\n"); \
        struct timeval before_runs; \
        gettimeofday(&before_runs, NULL); \
        for (i = 0; i < runs; i++) { \
            code; \
        } \
        total_run_time = get_time_ms(before_runs); \
        if (total_run_time >= min_profile_time_ms) { \
            avg_run_time = total_run_time / (double) runs; \
            break; \
        } \
        runs = runs * 2; \
    } \



/* If skip_benchmark is false, first benchmark:
 * - setup, code, teardown
 * Then, run:
 * - setup, code 
 */
#define BENCHMARK_THEN_RUN(setup, code, teardown, total_run_time, avg_run_time, runs, min_profile_time_ms) \
    BENCHMARK(setup; \
            code; \
            teardown;, \
            total_run_time, \
            avg_run_time, \
            runs, \
            min_profile_time_ms); \
    setup; \
    code; \
    
#endif /* end of include guard: COMMON_H */
