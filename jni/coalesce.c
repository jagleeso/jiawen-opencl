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
#define KERNEL			"/data/local/tmp/coalesce.cl"

#define LOOP (1000)

#define DEVICE 0  
int coalesce();

char* program_name;
static void usage(void) {

	printf("usage:\n");

	printf("ARRAY_SIZE: the size in bytes of the data to encrypt\n");

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

/* I
*/
unsigned int local_work_size = UINT_MAX; // mark as uninitialized
int provided_local_work_size = 0; 

/* t
*/
unsigned int min_profile_time_ms = UINT_MAX; // mark as uninitialized
int provided_min_profile_time_ms = 0; 

int mode = -1; // mark as uninitialized
#define MODE_COALESCE_OPTIMAL 0

char *modes[] = {
    "MODE_COALESCE_OPTIMAL",
};

#define MAX_KERNEL_NAME 1024
char kernel_name[MAX_KERNEL_NAME];

static char *option_string = "a:G:I:t:";
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
        case 'I':
            if (safe_cmp("NULL", optarg) == 0) {
                local_work_size = 0;
            } else {
                result = sscanf(optarg, "%d", &local_work_size);
                if (result != 1) {
                    printf("result == %d\n", result);
                    fprintf(stderr, "Error: expected the local work size (local_work_size), but saw \"%s\"\n", optarg);
                    usage();
                }
            }
            provided_local_work_size = 1;
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

	coalesce();
	return 0;
}

int runKernel(
        cl_mem in_buffer,
        cl_command_queue commands,
        cl_kernel coalesce_kernel,
        cl_uint work_dim,
        unsigned int global,
        size_t * local_ptr,
        cl_event * event
);

/* entries: the number of "entries" (i.e. groups of size 16) that each OpenCL kernel instance should handle
 * num_work_groups: the "global work size" (the number of OpenCL AES instances to split the encryption of the input array between).
 */
int coalesce(void) {
#ifdef DEBUG 
	printf("start of coalesce\n");
#endif

    /* Determine our mode of operation (need to load the right kernel).
     */
    if (provided_array_size && provided_min_profile_time_ms && provided_num_work_groups) {
        strncpy(kernel_name, "coalesce_optimal", MAX_KERNEL_NAME);
        mode = MODE_COALESCE_OPTIMAL;
    } else {
        fprintf(stderr, "Invalid mode of operation\n");
        usage();
        abort();
    }
    assert(mode != -1);
    assert(strlen(kernel_name) != 0);
	
	int err;                            // error code returned from api calls
	/* size_t global;                      // global domain size for our calculation */
	/* size_t local;                       // local domain size for our calculation */

	cl_device_id *device_id;             // compute device id 
	cl_context context;                 // compute context
	cl_command_queue commands;          // compute command queue
	cl_program program;                 // compute program
	cl_kernel coalesce_kernel;                   // compute kernel
	//cl_kernel decrypt_kernel;                   // compute kernel
	cl_event event;
	
	cl_mem in_buffer;

	cl_uint* in = malloc(sizeof(cl_uint)*array_size);              //plain text
    assert(in != NULL);

	initFns();
	cl_platform_id platform = NULL;//the chosen platform
	err = clGetPlatformIDs(1, &platform, NULL);
    CHECK_CL_SUCCESS("clGetPlatformIDs", err);

	cl_uint numDevices = 0;
	//int gpu = 1;
	//err = clGetDeviceIDs(NULL, gpu ? CL_DEVICE_TYPE_GPU : CL_DEVICE_TYPE_CPU, 1, &device_id, NULL);
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
#ifdef DEBUG 
	printf("Create a compute context\n");
#endif
	//
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
#ifdef DEBUG 
	printf("Create a command commands\n");
#endif
	//
	commands = clCreateCommandQueue(context, device_id[DEVICE], CL_QUEUE_PROFILING_ENABLE, &err);
    CHECK_CL_SUCCESS("clCreateCommandQueue", err);
	if (!commands)
	{
		printf("Error: Failed to create a command commands!\n");
		return EXIT_FAILURE;
	}

	// Create the compute program from the source buffer
#ifdef DEBUG 
	printf("Create the compute program from the source buffer\n");
#endif
	const char *kernel_source = load_kernel_source(KERNEL);
	//printf("kernel source is:\n %s", kernel_source);
	program = clCreateProgramWithSource(context, 1, &kernel_source, NULL, &err);
	if (!program || err != CL_SUCCESS) {
		printf("Error: Failed to create compute program!\n");
		return EXIT_FAILURE;
	}

	// Build the program executable
#ifdef DEBUG 
	printf("Build the program executable\n");
#endif
	//
	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	if (err != CL_SUCCESS)
	{
		size_t log_size;
        err = clGetProgramBuildInfo(program, device_id[DEVICE], CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        CHECK_CL_SUCCESS("clGetProgramBuildInfo", err);
        char* program_log = (char*) malloc((log_size + 1)*sizeof(char));
        clGetProgramBuildInfo(program, device_id[DEVICE], CL_PROGRAM_BUILD_LOG, log_size + 1, program_log, NULL);
        printf("%s\n", program_log);
        free(program_log);
		printf("Error: Failed to build program executable!\n");
		exit(1);
	}

	// Create the compute kernel in the program we wish to run
#ifdef DEBUG 
	printf("Create the compute kernel in the program we wish to run\n");
#endif
	printf("> kernel_name = %s\n", kernel_name);
	coalesce_kernel = clCreateKernel(program, kernel_name, &err);

	/* coalesce_kernel = clCreateKernel(program, "AES_encrypt_coalesced", &err); */
	/* coalesce_kernel = clCreateKernel(program, "AES_encrypt_old", &err); */
	if (!coalesce_kernel || err != CL_SUCCESS) {
		printf("Error: Failed to create compute kernel! err = %d\n", err);
		size_t len;
		char b[2048];
		err = clGetProgramBuildInfo(program, device_id[DEVICE], CL_PROGRAM_BUILD_LOG, sizeof(b), b, &len);
        CHECK_CL_SUCCESS("clGetProgramBuildInfo", err);
		printf("%s\n", b);
		exit(1);
	}

	// Create the input and output arrays in device memory for our calculation
#ifdef DEBUG 
	printf("Create the input and output arrays in device memory for our calculation\n");
#endif
	printf("> array_size: %u\n", array_size);
	printf("> array size in bytes: %u bytes\n", array_size*sizeof(cl_uint));
	// dynamic buffer size please
	in_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, array_size * sizeof(cl_uint), NULL, &err);
    CHECK_CL_SUCCESS("clCreateBuffer", err);
	if (!in_buffer)
	{
		printf("Error: Failed to allocate device memory!\n");
		exit(1);
	}    

	// Get the maximum work group size for executing the kernel on the device
#ifdef DEBUG 
	printf("Get the maximum work group size for executing the kernel on the device\n");
#endif

	unsigned int i = 0;
	for(i = 0; i < array_size; i++) {
		in[i] = 0;
	}

    /* The timestamp just before we perform a copy the input array to global memory.
     */
    struct timeval before_copy_t;
    gettimeofday(&before_copy_t, NULL);
    err = clEnqueueWriteBuffer(commands, in_buffer, CL_TRUE, 0, array_size * sizeof(cl_uint), in, 0, NULL, NULL);
    CHECK_CL_SUCCESS("clEnqueueWriteBuffer", err);
    if (err != CL_SUCCESS)
    {
        printf("Error: Failed to write to source array!\n");
        exit(1);
    }
    err = clFinish(commands);
    CHECK_CL_SUCCESS("clFinish", err);
    double copy_time = get_time_ms(before_copy_t);

    // Execute the kernel over the entire range of our 1d input data set
    // using the maximum number of work group items for this device

    /* cl_ulong start = 0, end = 0; */

    size_t max_kernel_work_group_size;
    err = clGetKernelWorkGroupInfo(coalesce_kernel, device_id[DEVICE], CL_KERNEL_WORK_GROUP_SIZE,
            sizeof(size_t), &max_kernel_work_group_size, NULL);

    size_t preferred_multiple = -1;
    err = clGetKernelWorkGroupInfo(coalesce_kernel,
            device_id[0],
            CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
            sizeof(size_t),
            &preferred_multiple,
            NULL);
    CHECK_CL_SUCCESS("clGetKernelWorkGroupInfo", err);

    cl_uint dims;
    size_t max_work_items_dim1;
    err = get_max_work_items(device_id[DEVICE], &dims, &max_work_items_dim1);
    CHECK_CL_SUCCESS("get_max_work_items", err);
    printf("> max dimensions: %i\n", dims);
    printf("> max_work_items_dim1: %zd\n", max_work_items_dim1);
    printf("> max_kernel_work_group_size: %zd\n", max_kernel_work_group_size);
    printf("> CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE: %zd\n", preferred_multiple);

    /* Modes of operation:
    */
    unsigned int global = UINT_MAX;
    unsigned int local = UINT_MAX;
    size_t * local_ptr = (size_t*) 0xffffffff;
    switch (mode) {
        case MODE_COALESCE_OPTIMAL:
            local = preferred_multiple;
            global = num_work_groups * local;
            local_ptr = &local;
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

    printf("> mode = %s\n", modes[mode]);

    printf("num_work_groups is %d\n", num_work_groups);

    printf("> GLOBAL = %u\n", global);
    printf("> LOCAL = %u\n", local);

    err = 0;
    /* err |= clSetKernelArg(coalesce_kernel, 0, sizeof(cl_mem), &in_buffer); */
    /* CHECK_CL_SUCCESS("clSetKernelArg", err); */
    /* err |= clSetKernelArg(coalesce_kernel, 1, sizeof(cl_uint), &array_size); */
    /* CHECK_CL_SUCCESS("clSetKernelArg", err); */

    /* Run the kernel a minimum number of times so that it can be averaged over the min_profile_time_ms.
     */
    cl_uint work_dim = 1;

    double total_kernel_run_time = 0;
    double avg_kernel_run_time = 0;
    int runs = 1;
    while (1) {
        printf("trying %u runs...\n", runs);

        printf("run once to handle cache effects...\n");
        runKernel(in_buffer, commands, coalesce_kernel, work_dim, global, local_ptr, &event);

        printf("start real runs...\n");
        struct timeval before_runs;
        gettimeofday(&before_runs, NULL);
        for (i = 0; i < runs; i++) {
            runKernel(in_buffer, commands, coalesce_kernel, work_dim, global, local_ptr, &event);
        }
        total_kernel_run_time = get_time_ms(before_runs);
        if (total_kernel_run_time >= min_profile_time_ms) {
            avg_kernel_run_time = total_kernel_run_time / (double) runs;
            break;
        }
        runs = runs * 2;
    }

    /* err = clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL); */
    /* CHECK_CL_SUCCESS("clGetEventProfilingInfo", err); */
    /* err = clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL); */
    /* CHECK_CL_SUCCESS("clGetEventProfilingInfo", err); */
    // the resolution of the events is 1e-09 sec

    err = clFinish(commands);
    CHECK_CL_SUCCESS("clFinish", err);

    /* print_data("input", array_size, in); */
    /* print_data("encrypted", array_size, out); */

	printf("-----------------------------------------------\n");
    printf("average profile time: %f ms\n", avg_kernel_run_time);
	printf("copy time: %.2f ms\n", copy_time); 

	// Shutdown and cleanup
	clReleaseMemObject(in_buffer);
    CHECK_CL_SUCCESS("clReleaseMemObject", err);
	clReleaseProgram(program);
    CHECK_CL_SUCCESS("clReleaseProgram", err);
	clReleaseKernel(coalesce_kernel);
    CHECK_CL_SUCCESS("clReleaseKernel", err);
	clReleaseCommandQueue(commands);
    CHECK_CL_SUCCESS("clReleaseCommandQueue", err);
	clReleaseContext(context);
    CHECK_CL_SUCCESS("clReleaseContext", err);

    return EXIT_SUCCESS;
}

int runKernel(
        cl_mem in_buffer,
        cl_command_queue commands,
        cl_kernel coalesce_kernel,
        cl_uint work_dim,
        unsigned int global,
        size_t * local_ptr,
        cl_event * event
)
{
    int err = 0;
    err |= clSetKernelArg(coalesce_kernel, 0, sizeof(cl_mem), &in_buffer);
    CHECK_CL_SUCCESS("clSetKernelArg", err);
    err |= clSetKernelArg(coalesce_kernel, 1, sizeof(cl_uint), &array_size);
    CHECK_CL_SUCCESS("clSetKernelArg", err);
    err = clEnqueueNDRangeKernel(commands, coalesce_kernel, work_dim, NULL, &global, local_ptr, 0, NULL, event);
    CHECK_CL_SUCCESS("clEnqueueNDRangeKernel", err);
    if (err) {
        printf("Error: Failed to execute kernel!\n");
        return EXIT_FAILURE;
    }
    err = clFinish(commands);
    CHECK_CL_SUCCESS("clFinish", err);
    err = clReleaseEvent(*event);
    CHECK_CL_SUCCESS("clReleaseEvent", err);
    /* err = clWaitForEvents(1, event); */
    /* CHECK_CL_SUCCESS("clWaitForEvents", err); */
    return err;
}
