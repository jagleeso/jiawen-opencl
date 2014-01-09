#include <CL/cl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "aopencl.h"
#include <fcntl.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <errno.h>
#include "aes.h"
#include <inttypes.h>
#include <limits.h>

#include "common.h"

#define DEBUG
#define KERNEL			"/data/local/tmp/memcopy.cl"

#define LOOP (1000)

#define DEVICE 0  
int memcopy();

char* program_name;
static void usage(void) {

	printf("usage: %s -a ARRAY_SIZE -G WORK_GROUP_SIZE -t MIN_PROFILE_TIME_MS\n", program_name);

	printf("ARRAY_SIZE: the size in bytes of the data to encrypt\n");
	printf("WORK_GROUP_SIZE: the local_work_size parameter to clEnqueueNDRangeKernel\n");
	printf("MIN_PROFILE_TIME_MS: the minimum amount of time needed for the runs of a benchmark (we report an average over such runs)\n");

	exit(EXIT_FAILURE);
}

/* a
*/
unsigned int array_size;
int provided_array_size = 0;

/* G
*/
unsigned int num_work_groups = UINT_MAX; // mark as uninitialized
int provided_num_work_groups = 0; 

/* t
*/
unsigned int min_profile_time_ms = UINT_MAX; // mark as uninitialized
int provided_min_profile_time_ms = 0; 

/* r
*/
int reuse_buffer = 0;

int mode = -1; // mark as uninitialized
#define MODE_CPU_GPU 0
#define MODE_REUSE_BUFFER 1

char *modes[] = {
    "MODE_CPU_GPU",
    "MODE_REUSE_BUFFER",
};

#define MAX_KERNEL_NAME 1024
char kernel_name[MAX_KERNEL_NAME];

#define ASSERT_OR_PRINTF(condition, ...) \
    if (!(condition)) { \
        printf(__VA_ARGS__); \
        assert(condition); \
    } \

void check_output(cl_uint* in, cl_uint* out, size_t count) {
    int i;
    for (i = 0; i < count; i++) {
        ASSERT_OR_PRINTF(out[i] == in[i] + 1, "in[%i] == %i, out[%i] == %i\n", i, in[i], out[i], i);
    }
}

void check_input(cl_uint* in, size_t count) {
    int i;
    for (i = 0; i < count; i++) {
        assert(in[i] == 0);
    }
}

static char *option_string = "G:t:a:r";
int main(int argc, char* argv[])
{
    program_name = argv[0];
	int option;

    int result;
	while((option = getopt(argc, argv, option_string)) != -1) {
		switch((char)option) {
        case 'a':
            result = sscanf(optarg, "%d", &array_size);
            if (result != 1) {
                printf("result == %d\n", result);
                fprintf(stderr, "Error: expected the size in bytes of the input/output arrays, but saw \"%s\"\n", optarg);
                usage();
            }
            provided_array_size = 1;
            break;
        case 'G':
            result = sscanf(optarg, "%d", &num_work_groups);
            if (result != 1) {
                printf("result == %d\n", result);
                fprintf(stderr, "Error: expected the number of work groups (num_work_groups), but saw \"%s\"\n", optarg);
                usage();
            }
            provided_num_work_groups = 1;
            break;
        case 't':
            if (safe_cmp("NULL", optarg) == 0) {
                min_profile_time_ms = 0;
            } else {
                result = sscanf(optarg, "%d", &min_profile_time_ms);
                if (result != 1) {
                    printf("result == %d\n", result);
                    fprintf(stderr, "Error: expected the minimum profile time (min_profile_time_ms), but saw \"%s\"\n", optarg);
                    usage();
                }
            }
            provided_min_profile_time_ms = 1;
            break;
		case '?':
            fprintf(stderr, "\n");
			usage();
            abort();
			break;
		}
	}

    if (option == -1) {
        /* Read all the options.
        */
        if (optind < argc) {
            fprintf(stderr, "Read the options but there was extra stuff:\n");
            while (optind < argc) {
                fprintf(stderr, "%s ", argv[optind++]);
            }
            fprintf(stderr, "\n");
            usage();
            abort();
        }
    }

	memcopy();
	return 0;
}

int memcopy(void) {

    /* Determine our mode of operation (need to load the right kernel).
     */

    if (provided_array_size && provided_num_work_groups && provided_min_profile_time_ms && reuse_buffer) {
        mode = MODE_CPU_GPU;
    } else if (provided_array_size && provided_num_work_groups && provided_min_profile_time_ms) {
        mode = MODE_REUSE_BUFFER;
    } else {
        fprintf(stderr, "Invalid mode of operation\n");
        usage();
        abort();
    }
    strncpy(kernel_name, "memcopy", MAX_KERNEL_NAME);
    assert(mode != -1);
    assert(strlen(kernel_name) != 0);
	
	int err;
	cl_device_id *device_id;
	cl_context context;
	cl_command_queue commands;
	cl_program program;
	cl_kernel memcopy_kernel;
	cl_event event;
	
	cl_mem in_buffer;

	initFns();
	cl_platform_id platform = NULL;
	err = clGetPlatformIDs(1, &platform, NULL);
    CHECK_CL_SUCCESS("clGetPlatformIDs", err);

	cl_uint numDevices = 0;
	device_id = (cl_device_id*)malloc(2 * sizeof(cl_device_id));
	err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 2, device_id, &numDevices);
	if (err != CL_SUCCESS)
	{
		printf("Error: Failed to create a device group!\n");
		return EXIT_FAILURE;
	}

#ifdef DEBUG 
	printf("has %d devices\n", numDevices);
#endif

    //IAH();
    //err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, numDevices, devices, NULL);

	//cl_device_info device_info;
	char buffer[1024];
	clGetDeviceInfo(device_id[0], CL_DEVICE_NAME, sizeof(buffer), buffer, NULL);

    /* Check if the device is available
     */
    cl_bool device_available = CL_FALSE;
	clGetDeviceInfo(device_id[0], CL_DEVICE_AVAILABLE, sizeof(cl_bool), &device_available, NULL);
    if (device_available != CL_TRUE) 
    {
		printf("Error: Device %i is not available\n", 0);
		return EXIT_FAILURE;
    }

#ifdef DEBUG 
	printf("Device name is %s\n", buffer);
#endif
	clGetDeviceInfo(device_id[1], CL_DEVICE_NAME, sizeof(buffer), buffer, NULL);
#ifdef DEBUG 
	printf("Device name is %s\n", buffer);
#endif
	
	// Create a compute context 
	context = clCreateContext(0, 1, &device_id[DEVICE], NULL, NULL, &err);
	//context = clCreateContext(0, 1, device_id, NULL, NULL, &err);
	if (!context)
	{
		printf("Error: Failed to create a compute context!\n");
		return EXIT_FAILURE;
	}

	if (err != CL_SUCCESS)
	{
		printf("Error: Failed to create a compute context: errcode_ret=%i\n", err);
		return EXIT_FAILURE;
	}

	// Create a command commands
	commands = clCreateCommandQueue(context, device_id[DEVICE], CL_QUEUE_PROFILING_ENABLE, &err);
    CHECK_CL_SUCCESS("clCreateCommandQueue", err);
	if (!commands)
	{
		printf("Error: Failed to create a command commands!\n");
		return EXIT_FAILURE;
	}

	// Create the compute program from the source buffer
	const char *kernel_source = load_kernel_source(KERNEL);
	//printf("kernel source is:\n %s", kernel_source);
	program = clCreateProgramWithSource(context, 1, &kernel_source, NULL, &err);
	if (!program || err != CL_SUCCESS) {
		printf("Error: Failed to create compute program!\n");
		return EXIT_FAILURE;
	}

	// Build the program executable
	int build_program_err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    size_t log_size;
    err = clGetProgramBuildInfo(program, device_id[DEVICE], CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
    CHECK_CL_SUCCESS("clGetProgramBuildInfo", err);
    char* program_log = (char*) malloc((log_size + 1)*sizeof(char));
    clGetProgramBuildInfo(program, device_id[DEVICE], CL_PROGRAM_BUILD_LOG, log_size + 1, program_log, NULL);
    printf("%s\n", program_log);
    free(program_log);
	if (build_program_err != CL_SUCCESS)
	{
		printf("Error: Failed to build program executable!\n");
		exit(1);
	}

	// Create the compute kernel in the program we wish to run
	printf("> kernel_name = %s\n", kernel_name);
	memcopy_kernel = clCreateKernel(program, kernel_name, &err);

	if (!memcopy_kernel || err != CL_SUCCESS) {
		printf("Error: Failed to create compute kernel! err = %d\n", err);
		size_t len;
		char b[2048];
		err = clGetProgramBuildInfo(program, device_id[DEVICE], CL_PROGRAM_BUILD_LOG, sizeof(b), b, &len);
        CHECK_CL_SUCCESS("clGetProgramBuildInfo", err);
		printf("%s\n", b);
		exit(1);
	}

    size_t preferred_multiple = -1;
    err = clGetKernelWorkGroupInfo(memcopy_kernel,
            device_id[0],
            CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
            sizeof(size_t),
            &preferred_multiple,
            NULL);
    CHECK_CL_SUCCESS("clGetKernelWorkGroupInfo", err);

    /* Modes of operation:
    */
    unsigned int global = UINT_MAX;
    unsigned int local = UINT_MAX;
    size_t * local_ptr = (size_t*) 0xffffffff;
    switch (mode) {
        case MODE_CPU_GPU:
        case MODE_REUSE_BUFFER:
            local = preferred_multiple;
            local_ptr = &local;
            global = num_work_groups * local;
            array_size = ROUND_UP(array_size, 4*global);
            break;
        default:
            fprintf(stderr, "Invalid mode of operation\n");
            usage();
            abort();
            break;
    }
    assert(num_work_groups != UINT_MAX);
    assert(global != UINT_MAX);
    assert(local != UINT_MAX);
    assert(local_ptr != (size_t*) 0xffffffff);

    /* Allocate the host memory input array (initialized to all zeros),
     * then allocate a OpenCL memory buffer to copy it to.
     */
	cl_uint* in = malloc(sizeof(cl_uint)*array_size);
	cl_uint* out = malloc(sizeof(cl_uint)*array_size);
    assert(in != NULL);
	unsigned int i = 0;
	for(i = 0; i < array_size; i++) {
		in[i] = 0;
	}


#define CREATE_BUFFER \
    in_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, array_size * sizeof(cl_uint), NULL, &err); \
    CHECK_CL_SUCCESS("clCreateBuffer", err); \
    if (!in_buffer) \
    { \
        printf("Error: Failed to allocate device memory!\n"); \
        exit(1); \
    } \

    double total_cpu_to_gpu_copy_time;
    double avg_cpu_to_gpu_copy_time;
    int cpu_to_gpu_runs;
    if (mode == MODE_REUSE_BUFFER) {
        /* Create buffer
         */
        CREATE_BUFFER;
        BENCHMARK_THEN_RUN(
            /* Nothing to initialize.
             */
            ,

            /* Perform the copy.
             */
            err = clEnqueueWriteBuffer(commands, in_buffer, CL_TRUE, 0, array_size * sizeof(cl_uint), in, 0, NULL, NULL);
            CHECK_CL_SUCCESS("clEnqueueWriteBuffer", err);
            if (err != CL_SUCCESS)
            {
                printf("Error: Failed to write to source array!\n");
                exit(1);
            }
            err = clFinish(commands);
            CHECK_CL_SUCCESS("clFinish", err);
            ,

            /* Nothing to cleanup.
             */
            ,

            total_cpu_to_gpu_copy_time,
            avg_cpu_to_gpu_copy_time,
            cpu_to_gpu_runs,
            min_profile_time_ms);

    } else {
        BENCHMARK_THEN_RUN(

            /* Create buffer
             */
            CREATE_BUFFER
            ,

            /* Perform the copy.
             */
            err = clEnqueueWriteBuffer(commands, in_buffer, CL_TRUE, 0, array_size * sizeof(cl_uint), in, 0, NULL, NULL);
            CHECK_CL_SUCCESS("clEnqueueWriteBuffer", err);
            if (err != CL_SUCCESS)
            {
                printf("Error: Failed to write to source array!\n");
                exit(1);
            }
            err = clFinish(commands);
            CHECK_CL_SUCCESS("clFinish", err);
            ,

            /* Cleanup buffer.
             */
            clReleaseMemObject(in_buffer);
            CHECK_CL_SUCCESS("clReleaseMemObject", err);
            ,

            total_cpu_to_gpu_copy_time,
            avg_cpu_to_gpu_copy_time,
            cpu_to_gpu_runs,
            min_profile_time_ms);
    }

    /* err = clEnqueueWriteBuffer(commands, in_buffer, CL_TRUE, 0, array_size * sizeof(cl_uint), in, 0, NULL, NULL); */
    /* CHECK_CL_SUCCESS("clEnqueueWriteBuffer", err); */
    /* if (err != CL_SUCCESS) */
    /* { */
    /*     printf("Error: Failed to write to source array!\n"); */
    /*     exit(1); */
    /* } */
    /* err = clFinish(commands); */
    /* CHECK_CL_SUCCESS("clFinish", err); */
    /* double copy_time = get_time_ms(before_copy_t); */

    // Execute the kernel over the entire range of our 1d input data set
    // using the maximum number of work group items for this device

    cl_uint dims;
    size_t max_work_items_dim1;
    err = get_max_work_items(device_id[DEVICE], &dims, &max_work_items_dim1);
    CHECK_CL_SUCCESS("get_max_work_items", err);
    printf("> max dimensions: %i\n", dims);
    printf("> max_work_items_dim1: %zd\n", max_work_items_dim1);
    printf("> CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE: %zd\n", preferred_multiple);

	printf("> array_size: %u\n", array_size);
	printf("> array size in bytes: %u bytes\n", array_size*sizeof(cl_uint));
    printf("> mode = %s\n", modes[mode]);
    printf("num_work_groups is %d\n", num_work_groups);

    printf("> GLOBAL = %u\n", global);
    printf("> LOCAL = %u\n", local);

    err = 0;

    err |= clSetKernelArg(memcopy_kernel, 0, sizeof(cl_mem), &in_buffer);
    CHECK_CL_SUCCESS("clSetKernelArg", err);
    err |= clSetKernelArg(memcopy_kernel, 1, sizeof(cl_uint), &array_size);
    CHECK_CL_SUCCESS("clSetKernelArg", err);

    /* Run the kernel a minimum number of times so that it can be averaged over the min_profile_time_ms.
     */
    cl_uint work_dim = 1;
    err = clEnqueueNDRangeKernel(commands, memcopy_kernel, work_dim, NULL, &global, local_ptr, 0, NULL, &event);
    CHECK_CL_SUCCESS("clEnqueueNDRangeKernel", err);
    if (err) {
        printf("Error: Failed to execute kernel!\n");
        return EXIT_FAILURE;
    }
    err = clFinish(commands);
    CHECK_CL_SUCCESS("clFinish", err);
    err = clReleaseEvent(event);
    CHECK_CL_SUCCESS("clReleaseEvent", err);

    double total_gpu_to_cpu_copy_time;
    double avg_gpu_to_cpu_copy_time;
    int gpu_to_cpu_runs;
    BENCHMARK_THEN_RUN(

        /* Setup (nothing to do).
         */
        ,

        /* Perform the copy.
         */
        err = clEnqueueReadBuffer(commands, in_buffer, CL_TRUE, 0, array_size * sizeof(cl_uint), out, 0, NULL, NULL);
        if (err != CL_SUCCESS) {
            printf("Error: Failed to read output array! %d\n", err);
            exit(1);
        }
        err = clFinish(commands);
        CHECK_CL_SUCCESS("clFinish", err);
        ,

        /* Cleanup (nothing to do).
         */
        ,

        total_gpu_to_cpu_copy_time,
        avg_gpu_to_cpu_copy_time,
        gpu_to_cpu_runs,
        min_profile_time_ms)
    check_input(in, array_size);
    check_output(in, out, array_size);

	printf("-----------------------------------------------\n");
    /* printf("average profile time: %f ms\n", avg_kernel_run_time); */
	printf("average CPU-to-GPU copy time: %.2f ms\n", avg_cpu_to_gpu_copy_time); 
	printf("average GPU-to-CPU copy time: %.2f ms\n", avg_gpu_to_cpu_copy_time); 

	// Shutdown and cleanup
	clReleaseMemObject(in_buffer);
    CHECK_CL_SUCCESS("clReleaseMemObject", err);
	clReleaseProgram(program);
    CHECK_CL_SUCCESS("clReleaseProgram", err);
	clReleaseKernel(memcopy_kernel);
    CHECK_CL_SUCCESS("clReleaseKernel", err);
	clReleaseCommandQueue(commands);
    CHECK_CL_SUCCESS("clReleaseCommandQueue", err);
	clReleaseContext(context);
    CHECK_CL_SUCCESS("clReleaseContext", err);

    return EXIT_SUCCESS;
}
