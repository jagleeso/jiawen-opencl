/**********************************************************************
Copyright (c) 2013  GSS Mahadevan
Copyright ©2012 Advanced Micro Devices, Inc. All rights reserved.

********************************************************************/

//For clarity,error checking has been omitted.
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
/* #include <sys/time.h> */
/* #include <ctime> */
#include <assert.h>
#include <errno.h>
#include "aes.h"
#include <inttypes.h>
#include <limits.h>

#include "common.h"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

//128 bit key
static const unsigned char key[] = {
    0x00, 0x11, 0x22, 0x33, 
    0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xaa, 0xbb,
    0xcc, 0xdd, 0xee, 0xff,
};

#define DEBUG
 // Use a static data size for simplicity
 //
#define DATA_SIZE (128)
//#define DATA_SIZE (512)
//#define DATA_SIZE (1024)
//#define DATA_SIZE (2048)
//#define DATA_SIZE (4096)
//#define DATA_SIZE (8192)
//#define DATA_SIZE (16384)
//#define DATA_SIZE (32768)
//#define DATA_SIZE (65536)
//#define DATA_SIZE (131072)
//#define DATA_SIZE (262144)
//#define DATA_SIZE (524288)
//define DATA_SIZE (1048576)// segmentation fault

#define AES_KERNEL            "/data/local/tmp/eng_opencl_aes.cl"
#define MAX_BUFFER_SIZE        DATA_SIZE
//#define MAX_BUFFER_SIZE        128 * 1024 * 1024
 

#define LOOP (1000)

#define DEVICE 0  
int encrypt_cl();

char* program_name;
static void usage(void) {

    printf("usage:\n");

    printf("%s [-e ENTRIES] -a ARRAY_SIZE -G NUM_WORK_GROUPS -I WORK_GROUP_SIZE\n", program_name);
    printf("ENTRIES: the number of uint4 (16 byte) entries each work-item should encrypt in the OpenCL AES kernel\n");
    printf("ARRAY_SIZE: the size in bytes of the data to encrypt\n");
    printf("NUM_WORK_GROUPS: the number of work groups to use\n");
    printf("WORK_GROUP_SIZE: the number of work-items per work group\n");

    exit(EXIT_FAILURE);
}

/* e
*/
size_t entries = UINT_MAX;
int provided_entries = 0;

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
int local_tbox = 0; // false

/* T
*/
int coalesced_local_tbox = 0; // false

/* R 
*/
int runtime_local_worksize = 0;

/* r
*/
unsigned int min_profile_time_ms = UINT_MAX; // mark as uninitialized
int provided_min_profile_time_ms = 0; 

/* w
 * Include both the CPU-to-GPU and GPU-to-CPU copy.
*/
int with_copy = 0;

/* b
 * Reuse the input buffer for each run.
*/
int reuse_buffer = 0;

/* s 
*/
/* unsigned int runs = 2; // mark as uninitialized */
/* int provided_runs = 0;  */

int mode = -1; // mark as uninitialized
#define MODE_STRIDED_EQUAL_WORK_ITEM_PARTITION 0
#define MODE_RUNTIME_LOCAL_WORK_SIZE 1
#define MODE_COALESCED_EQUAL_WORK_ITEM_PARTITION 2
#define MODE_STRIDED_TBOX_LOCAL_MEMORY 3
#define MODE_COALESCED_TBOX_LOCAL_MEMORY 4

char *modes[] = {
    "MODE_STRIDED_EQUAL_WORK_ITEM_PARTITION",
    "MODE_RUNTIME_LOCAL_WORK_SIZE",
    "MODE_COALESCED_EQUAL_WORK_ITEM_PARTITION",
    "MODE_STRIDED_TBOX_LOCAL_MEMORY",
    "MODE_COALESCED_TBOX_LOCAL_MEMORY",
};

#define MAX_KERNEL_NAME 1024
char kernel_name[MAX_KERNEL_NAME];

static char *option_string = "e:a:G:I:tTRs:wb";
int main(int argc, char* argv[])
{
    program_name = argv[0];
    int option;

    int result;
    while((option = getopt(argc, argv, option_string)) != -1) {
        switch((char)option) {
        case 'e':
            result = sscanf(optarg, "%zd", &entries);
            if (result < 0) {
                printf("result == %d\n", result);
                fprintf(stderr, "Error: the number of entries (i.e. groups of size 16 bytes) that each work item should encrypt\n");
                usage();
            }
            provided_entries = 1;
            break;
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
            local_tbox = 1;
            break;
        case 'R':
            runtime_local_worksize = 1;
            break;
        case 'w':
            with_copy = 1;
            break;
        case 'b':
            reuse_buffer = 1;
            break;
        case 's':
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
        case 'T':
            coalesced_local_tbox = 1;
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

    encrypt_cl();
    return 0;
}

/* entries: the number of "entries" (i.e. groups of size 16) that each OpenCL kernel instance should handle
 * num_work_groups: the "global work size" (the number of OpenCL AES instances to split the encryption of the input array between).
 */
int encrypt_cl(void) {
#ifdef DEBUG 
    printf("start of encrypt_cl\n");
#endif
    if (!provided_min_profile_time_ms) {
        fprintf(stderr, "Must provided MIN_PROFILE_TIME_MS [-s]\n");
        usage();
        abort();
    }

    /* Determine our mode of operation (need to load the right kernel).
     */
    if (provided_array_size && provided_num_work_groups && coalesced_local_tbox) {
        strncpy(kernel_name, "AES_encrypt_coalesced_local_tbox", MAX_KERNEL_NAME);
        mode = MODE_COALESCED_TBOX_LOCAL_MEMORY;
    } else if (provided_array_size && provided_num_work_groups && local_tbox) {
        strncpy(kernel_name, "AES_encrypt_strided_local_tbox", MAX_KERNEL_NAME);
        mode = MODE_STRIDED_TBOX_LOCAL_MEMORY;
    } else if (provided_array_size && provided_num_work_groups) {
        strncpy(kernel_name, "AES_encrypt_strided", MAX_KERNEL_NAME);
        mode = MODE_STRIDED_EQUAL_WORK_ITEM_PARTITION;
    } else if (provided_array_size && provided_entries && runtime_local_worksize) { 
        strncpy(kernel_name, "AES_encrypt", MAX_KERNEL_NAME);
        mode = MODE_RUNTIME_LOCAL_WORK_SIZE;
    } else if (provided_array_size && provided_entries) {
        strncpy(kernel_name, "AES_encrypt_strided", MAX_KERNEL_NAME);
        mode = MODE_COALESCED_EQUAL_WORK_ITEM_PARTITION;
    } else {
        fprintf(stderr, "Invalid mode of operation\n");
        usage();
        abort();
    }
    assert(mode != -1);
    assert(strlen(kernel_name) != 0);
    
    int err;                            // error code returned from api calls
    unsigned int correct;               // number of correct results returned

    cl_device_id *device_id;             // compute device id 
    cl_context context;                 // compute context
    cl_command_queue commands;          // compute command queue
    cl_program program;                 // compute program
    cl_program decrypt_program;                 // compute program
    cl_kernel encrypt_kernel;                   // compute kernel
    //cl_kernel decrypt_kernel;                   // compute kernel
    /* cl_event event; */
    
    static cl_mem buffer_state;
    static cl_mem buffer_roundkeys;

#ifdef DEBUG 
    printf("data, keydata, results\n");
#endif

    unsigned char* in = malloc(sizeof(unsigned char)*array_size); // plain text
    unsigned char* out = malloc(sizeof(unsigned char)*array_size); // encryped text
    assert(in != NULL);
    assert(out != NULL);

#ifdef DEBUG 
    printf("initFns\n");
#endif

    initFns();
    cl_platform_id platform = NULL;//the chosen platform
    err = clGetPlatformIDs(1, &platform, NULL);
    CHECK_CL_SUCCESS("clGetPlatformIDs", err);

#ifdef DEBUG 
    printf("Connect to a compute device\n");
#endif
    cl_uint numDevices = 0;
    device_id = (cl_device_id*)malloc(2 * sizeof(cl_device_id));
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 2, device_id, &numDevices);
    if (err != CL_SUCCESS) {
        printf("Error: Failed to create a device group!\n");
        return EXIT_FAILURE;
    }

#ifdef DEBUG 
    printf("has %d devices\n", numDevices);
#endif

    char buffer[1024];
    clGetDeviceInfo(device_id[0], CL_DEVICE_NAME, sizeof(buffer), buffer, NULL);

    /* Check if the device is available.
     */
    cl_bool device_available = CL_FALSE;
    clGetDeviceInfo(device_id[0], CL_DEVICE_AVAILABLE, sizeof(cl_bool), &device_available, NULL);
    if (device_available != CL_TRUE) {
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
    if (!context) {
        printf("Error: Failed to create a compute context!\n");
        return EXIT_FAILURE;
    }

    if (err != CL_SUCCESS) {
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
    if (!commands) {
        printf("Error: Failed to create a command commands!\n");
        return EXIT_FAILURE;
    }

    // Create the compute program from the source buffer
#ifdef DEBUG 
    printf("Create the compute program from the source buffer\n");
#endif
    const char *kernel_source = load_kernel_source(AES_KERNEL);
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
    if (err != CL_SUCCESS) {
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
    encrypt_kernel = clCreateKernel(program, kernel_name, &err);
    /* encrypt_kernel = clCreateKernel(program, "AES_encrypt_coalesced", &err); */
    /* encrypt_kernel = clCreateKernel(program, "AES_encrypt_old", &err); */
    if (!encrypt_kernel || err != CL_SUCCESS) {
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

    unsigned int i = 0;

    printf("encrypt_cl: count = %d\n", array_size);
    for(i = 0; i < array_size; i++) {
        in[i] = 0;
        /* in[i] = rand(); */
    }

    int ret;
    AES_KEY ks;
    ret = AES_set_encrypt_key(key, 128, &ks);

    /* if (!with_copy) { */
    /*     copy_cpu_to_gpu; */
    /* } */

    printf("rd_key %s\n", (char*) ks.rd_key);

    // Execute the kernel over the entire range of our 1d input data set
    // using the maximum number of work group items for this device

    clock_t tStartE = clock();

    size_t max_kernel_work_group_size;
    err = clGetKernelWorkGroupInfo(encrypt_kernel, device_id[DEVICE], CL_KERNEL_WORK_GROUP_SIZE,
            sizeof(size_t), &max_kernel_work_group_size, NULL);

    size_t preferred_multiple = -1;
    err = clGetKernelWorkGroupInfo(encrypt_kernel,
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
    printf("> CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE: %zd\n", preferred_multiple);
    printf("> max_kernel_work_group_size: %zd\n", max_kernel_work_group_size);

    /* 1 thread operates on 4 (uint4 vector type has 4 uints, (x, y, w, z)) consecutive 32 bit 
     * (uint) fields at a time.  
     *
     * That is, 1 thread operates on 4*4 == 16 bytes (chars) of the input array.
     *
     * An input of N bytes needs N / 16 threads.
     */
    assert(array_size % 16 == 0);
    unsigned int num_processing_elements;
    /* The total number of entries to encrypt in the input array.
    */
    unsigned int entries_to_encrypt;

    /* Modes of operation:
    */
    entries_to_encrypt = array_size / 16;
    if (!provided_local_work_size) {
        local_work_size = preferred_multiple;
    }
    switch (mode) {
        case MODE_COALESCED_TBOX_LOCAL_MEMORY:
            /* Same as MODE_STRIDED_TBOX_LOCAL_MEMORY, but with coalesced accesses to the input as in 
             * MODE_COALESCED_EQUAL_WORK_ITEM_PARTITION.
             */
        case MODE_STRIDED_TBOX_LOCAL_MEMORY:
            /* Each work item gets an equal sized chunk of the input data to encrypt (in the same 
             * manner as MODE_STRIDED_EQUAL_WORK_ITEM_PARTITION), and the work items within a group 
             * synchronize using a local memory barrier so they can share the 4 T-boxes instead of 
             * re-reading them from constant memory (which is just a read-only global memory).
             */
        case MODE_STRIDED_EQUAL_WORK_ITEM_PARTITION:
            /* Each work item gets an equal sized chunk of the input data to encrypt.
             *
             * e.g. 2 work groups with 1 processing element each, with each processing element encrypting 
             * half the input
             *
             * ./opencl_aes -a 128 -G 2 -I 1
             *
             * - User provides: 
             *   - array size
             *   - number of work groups
             *   - work group size
             *   We calculate:
             *   - the number of entries each work item encrypts
             */
            num_processing_elements = num_work_groups * local_work_size;
            entries = CEILING_DIVIDE(entries_to_encrypt, num_processing_elements);
            break;
        case MODE_RUNTIME_LOCAL_WORK_SIZE:
            /* Each work item encrypts the number of entries specified, where an entry is 16 bytes, and the 
             * local work size is specified as NULL to the OpenCL runtime so it can decide (hopefully 
             * optimally) what work group size to use (though it is irrelavent to us since we run the 
             * AES_encrypt kernel which uses only global id).
             *
             * e.g. 
             *
             * ./opencl_aes -a 128 -e 2 -I NULL
             *
             * - User provides: 
             *   - array size
             *   - the number of entries each work item encrypts
             *   - the local worksize (as NULL)
             *   We calculate:
             *   - work group size = determined by OpenCL runtime (set local_work_size == NULL)
             *   - number of work groups = # work items needed given each encrypt <entries>
             */
            assert(local_work_size == 0);
            num_work_groups = entries_to_encrypt / entries;
            break;
        case MODE_COALESCED_EQUAL_WORK_ITEM_PARTITION:
            /* Each work item encrypts the number of entries specified, where an entry is 16 bytes.
             *
             * e.g. 
             * - 128/16 = 8 entries to encrypt in total
             * - so we have 8/2 = 4 work items needed to encrypt 
             * - we want to fully utilize each compute unit, so we use 80 processing elements in a work 
             *   group
             * - we only need 1 work group
             *
             * ./opencl_aes -a 128 -e 2
             *
             * - User provides: 
             *   - array size
             *   - the number of entries each work item encrypts
             *   We calculate:
             *   - work group size = max supported processing elements
             *   - number of work groups = # needed to encrypt all of array size (function of 
             *     array size and number entries encrypted by each work item)
             */
            num_processing_elements = CEILING_DIVIDE(entries_to_encrypt, entries);
            num_work_groups = ROUND_UP(num_processing_elements, local_work_size)/local_work_size;
            break;
        default:
            fprintf(stderr, "Invalid mode of operation\n");
            usage();
            abort();
            break;
    }
    assert(num_work_groups != UINT_MAX);
    assert(local_work_size != UINT_MAX);

    unsigned int global = UINT_MAX;
    unsigned int local = UINT_MAX;
    size_t * local_ptr = (size_t*) 0xffffffff;
    switch (mode) {
        case MODE_COALESCED_TBOX_LOCAL_MEMORY:
        case MODE_STRIDED_TBOX_LOCAL_MEMORY:
        case MODE_STRIDED_EQUAL_WORK_ITEM_PARTITION:
        case MODE_COALESCED_EQUAL_WORK_ITEM_PARTITION:
            global = num_work_groups * local_work_size;
            local = local_work_size;
            local_ptr = &local;
            break;
        case MODE_RUNTIME_LOCAL_WORK_SIZE:
            global = num_work_groups;
            local = 0;
            local_ptr = NULL;
            break;
        default:
            assert(0);
            break;
    }
    assert(global != UINT_MAX);
    assert(local != UINT_MAX);
    assert(local_ptr != (size_t*) 0xffffffff);

    printf("> mode = %s\n", modes[mode]);
    printf("num_work_groups is %d\n", num_work_groups);

    if (entries != UINT_MAX) {
        printf("entries == %zd\n", entries);
    }
    printf("entries_to_encrypt == %u\n", entries_to_encrypt);
    printf("num_processing_elements == %u\n", num_processing_elements);

    printf("> GLOBAL = %u\n", global);
    printf("> LOCAL = %u\n", local);

    err = 0;

#define CREATE_BUFFERS \
    buffer_state = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, array_size, NULL, &err); \
    CHECK_CL_SUCCESS("clCreateBuffer", err); \
    buffer_roundkeys = clCreateBuffer(context, CL_MEM_READ_ONLY, 16 * 15, NULL, &err); \
    CHECK_CL_SUCCESS("clCreateBuffer", err); \
    if (!buffer_state || !buffer_roundkeys) { \
        printf("Error: Failed to allocate device memory!\n"); \
        exit(1); \
    } \

    /* Set the kernel arguments.
     */
#define SET_KERNEL_ARGUMENTS \
    err = clSetKernelArg(encrypt_kernel, 0, sizeof(cl_mem), &buffer_state); \
    CHECK_CL_SUCCESS("clSetKernelArg", err); \
    err |= clSetKernelArg(encrypt_kernel, 1, sizeof(cl_mem), &buffer_roundkeys); \
    CHECK_CL_SUCCESS("clSetKernelArg", err); \
    err |= clSetKernelArg(encrypt_kernel, 2, sizeof(ks.rounds), &ks.rounds); \
    CHECK_CL_SUCCESS("clSetKernelArg", err); \
 \
    if (strcmp("AES_encrypt_strided", kernel_name) == 0 || \
        strcmp("AES_encrypt_coalesced", kernel_name) == 0 || \
        strcmp("AES_encrypt_strided_local_tbox", kernel_name) == 0 || \
        strcmp("AES_encrypt_coalesced_local_tbox", kernel_name) == 0) { \
        err |= clSetKernelArg(encrypt_kernel, 3, sizeof(cl_uint), &entries); \
        CHECK_CL_SUCCESS("clSetKernelArg", err); \
        err |= clSetKernelArg(encrypt_kernel, 4, sizeof(cl_uint), &entries_to_encrypt); \
        CHECK_CL_SUCCESS("clSetKernelArg", err); \
    } \
 \
    if (err != CL_SUCCESS) { \
        printf("Error: Failed to set kernel arguments! %d\n", err); \
        exit(1); \
    } \

#define COPY_CPU_TO_GPU \
    err = clEnqueueWriteBuffer(commands, buffer_state, CL_TRUE, 0, array_size * sizeof(*in), in, 0, NULL, NULL); \
    CHECK_CL_SUCCESS("clEnqueueWriteBuffer", err); \
    err = clEnqueueWriteBuffer(commands, buffer_roundkeys, CL_TRUE, 0, 16 * 15, &ks.rd_key, 0, NULL, NULL); \
    CHECK_CL_SUCCESS("clEnqueueWriteBuffer", err); \
    if (err != CL_SUCCESS) { \
        printf("Error: Failed to write to source array!\n"); \
        exit(1); \
    } \

#define RUN_KERNEL \
    err = clEnqueueNDRangeKernel(commands, encrypt_kernel, work_dim, NULL, &global, local_ptr, 0, NULL, NULL); \
    CHECK_CL_SUCCESS("clEnqueueNDRangeKernel", err); \
    if (err) { \
        printf("Error: Failed to execute kernel!\n"); \
        return EXIT_FAILURE; \
    } \
    err = clFinish(commands); \
    CHECK_CL_SUCCESS("clFinish", err); \

#define COPY_GPU_TO_CPU \
    err = clEnqueueReadBuffer(commands, buffer_state, CL_TRUE, 0, array_size, out, 0, NULL, NULL); \
    if (err != CL_SUCCESS) { \
        printf("Error: Failed to read output array! %d\n", err); \
        exit(1); \
    } \

#define CLEANUP_BUFFERS \
    clReleaseMemObject(buffer_state); \
    CHECK_CL_SUCCESS("clReleaseMemObject", err); \
    clReleaseMemObject(buffer_roundkeys); \
    CHECK_CL_SUCCESS("clReleaseMemObject", err); \

    cl_uint work_dim = 1;

    double total_kernel_run_time;
    double avg_kernel_run_time;
    unsigned int runs;
    if (with_copy && reuse_buffer) {
        printf("> including copy time in profile time and reusing input buffer...\n");
        CREATE_BUFFERS;
        BENCHMARK_THEN_RUN(
            /* Nothing to initialize.
             */
            ,

            /* Copy the input to the buffer, run the kernel, copy to the output buffer.
             */
            COPY_CPU_TO_GPU;
            SET_KERNEL_ARGUMENTS;
            RUN_KERNEL;
            COPY_GPU_TO_CPU;
            ,

            /* Nothing to cleanup.
             */
            ,

            total_kernel_run_time,
            avg_kernel_run_time,
            runs,
            min_profile_time_ms);
    } else if (with_copy && !reuse_buffer) {
        printf("> including copy time in profile time and not reusing input buffer...\n");
        BENCHMARK_THEN_RUN(
            /* Nothing to initialize.
             */
            ,

            /* Create the input buffer, copy the input to the buffer, run the kernel, copy 
             * to the output buffer.
             */
            CREATE_BUFFERS;
            COPY_CPU_TO_GPU;
            SET_KERNEL_ARGUMENTS;
            RUN_KERNEL;
            COPY_GPU_TO_CPU;
            ,

            /* Cleanup the buffers.
             */
            CLEANUP_BUFFERS;
            ,

            total_kernel_run_time,
            avg_kernel_run_time,
            runs,
            min_profile_time_ms);
   } else {
        printf("> not including copy time in profile time...\n");
        CREATE_BUFFERS;
        COPY_CPU_TO_GPU;
        SET_KERNEL_ARGUMENTS;
        BENCHMARK_THEN_RUN(
            /* Nothing to initialize.
             */
            ,

            /* Run the kernel.
             */
            RUN_KERNEL;
            ,

            /* Nothing to cleanup.
             */
            ,

            total_kernel_run_time,
            avg_kernel_run_time,
            runs,
            min_profile_time_ms);
        COPY_GPU_TO_CPU;
    }

    /* err = clFinish(commands); */
    /* CHECK_CL_SUCCESS("clFinish", err); */

    /* print_data("input", array_size, in); */
    /* print_data("encrypted", array_size, out); */

    printf("-----------------------------------------------\n");
    printf("average profile time: %.2f ms\n", avg_kernel_run_time); 

    /* Cleanup.
     */
    CLEANUP_BUFFERS;
    clReleaseProgram(program);
    CHECK_CL_SUCCESS("clReleaseProgram", err);
    clReleaseKernel(encrypt_kernel);
    CHECK_CL_SUCCESS("clReleaseKernel", err);
    clReleaseCommandQueue(commands);
    CHECK_CL_SUCCESS("clReleaseCommandQueue", err);
    clReleaseContext(context);
    CHECK_CL_SUCCESS("clReleaseContext", err);

}

