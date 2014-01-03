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


#endif /* end of include guard: COMMON_H */
