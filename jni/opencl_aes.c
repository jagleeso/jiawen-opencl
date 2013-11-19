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
#include <time.h>
#include <assert.h>
#include <errno.h>
#include "aes.h"
#include <inttypes.h>

#define CEILING_DIVIDE(value, divisor) (((value) + (divisor) - 1)/(divisor))
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

#define AES_KERNEL			"/data/local/tmp/eng_opencl_aes.cl"
#define MAX_BUFFER_SIZE		DATA_SIZE
//#define MAX_BUFFER_SIZE		128 * 1024 * 1024
 

#define LOOP (1000)

#define DEVICE 0  
int encrypt_cl();

int get_max_work_items(cl_device_id device_id, cl_uint *dims, size_t *max_work_items_dim1);

static const char *load_kernel_source(const char *filename) {
	size_t len = strlen(filename) + 1;
	printf("file len is: %d\n", len);
	char *fullpath = (char *) malloc(len);
	sprintf(fullpath, "%s", filename);
	fullpath[len] = '\0';
	
	printf("full path is: %s\n", fullpath);
	int fd = 0;
	fd = open(fullpath, O_RDONLY);
	if (fd < 0) {
		printf("ERROR: %s\n", strerror(errno));
		exit(1);
	}
	else {
		printf("file opened, fd is %i\n", fd);
	}
	
	struct stat st;
	unsigned int fsize;
	unsigned int readSize = 0;
	if (fstat(fd, &st) < 0) {
		printf("ERROR: %s\n", strerror(errno));
		exit(1);
	}

	fsize = st.st_size;
	printf("file size is %d byte\n", fsize);
	char *buf = (char *) malloc(fsize + 1);
	readSize = read(fd, buf, fsize);
	printf("read in size is: %d\n", readSize);
	/* assert(fsize == read(fd, buf, fsize)); */
	
	buf[fsize] = '\0';
//	printf("source buff is: %s\n", buf);
	
	return buf;
}

int main(int argc, char* argv[])
{

    unsigned int array_size;
    unsigned int global;
    if (argc < 3) {
        /* fprintf(stderr, "Error: expected the size in bytes of the input/output arrays\n"); */
        /* exit(EXIT_FAILURE); */
        array_size = DATA_SIZE;
        global = 1;
    } else {
        int result;

        result = sscanf(argv[1], "%d", &array_size);
        if (result != 1) {
            printf("result == %d\n", result);
            fprintf(stderr, "Error: expected the size in bytes of the input/output arrays, but saw \"%s\"\n", argv[1]);
            exit(EXIT_FAILURE);
        }

        result = sscanf(argv[2], "%d", &global);
        if (result != 1) {
            printf("result == %d\n", result);
            fprintf(stderr, "Error: expected the \"global work size\" (the number of OpenCL AES instances to split the encryption of the input array between), but saw \"%s\"\n", argv[2]);
            exit(EXIT_FAILURE);
        }
    }

	encrypt_cl(array_size, global);
	return 0;
}

#define CHECK_CL_SUCCESS(task_todo, errvar) \
if (errvar != CL_SUCCESS) \
{ \
    printf("Error: %s line %d, Failed to " task_todo ": errcode_ret=%i\n", __FILE__, __LINE__, errvar); \
    exit(EXIT_FAILURE); \
} \

/* entries: the number of "entries" (i.e. groups of size 16) that each OpenCL kernel instance should handle
 * global: the "global work size" (the number of OpenCL AES instances to split the encryption of the input array between).
 */
int encrypt_cl(unsigned int array_size, unsigned int global) {
#ifdef DEBUG 
	printf("start of encrypt_cl\n");
#endif
	
	unsigned int count = array_size;
	/* unsigned int count = 1048576; */

	int err;                            // error code returned from api calls
	unsigned int correct;               // number of correct results returned
	/* size_t global;                      // global domain size for our calculation */
	size_t local;                       // local domain size for our calculation

	cl_device_id *device_id;             // compute device id 
	cl_context context;                 // compute context
	cl_command_queue commands;          // compute command queue
	cl_program program;                 // compute program
	cl_program decrypt_program;                 // compute program
	cl_kernel encrypt_kernel;                   // compute kernel
	//cl_kernel decrypt_kernel;                   // compute kernel
	cl_event event;
	
	static cl_mem buffer_state;
	static cl_mem buffer_roundkeys;

#ifdef DEBUG 
	printf("data, keydata, results\n");
#endif
	/* float results[count];           // results returned from device */
	
	/* unsigned char in[count];              //plain text */
	/* unsigned char out[count];              // encryped text */

	unsigned char* in = malloc(sizeof(unsigned char)*count);              //plain text
	unsigned char* out = malloc(sizeof(unsigned char)*count);              // encryped text

#ifdef DEBUG 
	printf("initFns\n");
#endif

	initFns();
	cl_platform_id platform = NULL;//the chosen platform
	err = clGetPlatformIDs(1, &platform, NULL);
    CHECK_CL_SUCCESS("clGetPlatformIDs", err);

	// Connect to a compute device
#ifdef DEBUG 
	printf("Connect to a compute device\n");
#endif
	//
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

    /* James: Check if the device is available
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
	if (err != CL_SUCCESS)
	{
		size_t len;
		char b[2048];

		printf("Error: Failed to build program executable!\n");
		err = clGetProgramBuildInfo(program, device_id[DEVICE], CL_PROGRAM_BUILD_LOG, sizeof(b), b, &len);
        CHECK_CL_SUCCESS("clGetProgramBuildInfo", err);
		printf("%s\n", b);
		exit(1);
	}

	// Create the compute kernel in the program we wish to run
#ifdef DEBUG 
	printf("Create the compute kernel in the program we wish to run\n");
#endif
	encrypt_kernel = clCreateKernel(program, "AES_encrypt", &err);
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
	// dynamic buffer size please
	buffer_state = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, count, NULL, &err);
    CHECK_CL_SUCCESS("clCreateBuffer", err);
	buffer_roundkeys = clCreateBuffer(context, CL_MEM_READ_ONLY, 16 * 15, NULL, &err);
    CHECK_CL_SUCCESS("clCreateBuffer", err);
	if (!buffer_state || !buffer_roundkeys)
	{
		printf("Error: Failed to allocate device memory!\n");
		exit(1);
	}    

	// Get the maximum work group size for executing the kernel on the device
#ifdef DEBUG 
	printf("Get the maximum work group size for executing the kernel on the device\n");
#endif
	//
	err = clGetKernelWorkGroupInfo(encrypt_kernel, device_id[DEVICE], CL_KERNEL_WORK_GROUP_SIZE, sizeof(local), &local, NULL);
	if (err != CL_SUCCESS)
	{
		printf("Error: Failed to retrieve kernel work group info! %d\n", err);
		exit(1);
	}
	printf("local can be %d\n", local);
    local = 1;
	printf("local is %d\n", local);


	unsigned int i = 0;

	clock_t tStartF = clock();
	// Fill our data set with random float values
	i = 0;
	printf("encrypt_cl: count = %d\n", count);
	for(i = 0; i < count; i++) {
		in[i] = 0;
		//in[i] = rand();
	}
	//tFill += (double)(clock() - tStartF)/CLOCKS_PER_SEC;

	clock_t tStart = clock();
	unsigned int k = 0;
	double tFill = 0;
	double tMemory = 0;
	double tArgument = 0;
	double tExecute = 0;
	double tRead = 0;

	int ret;
	AES_KEY ks;
	ret = AES_set_encrypt_key(key, 128, &ks);
	//for (k = 0; k<LOOP; k++) {
		//printf("encrypt_cl: i = %d\n", i);

		// Write our data set into the input array in device memory 
		//printf("Write our data set into the input array in device memory\n");
		//

		clock_t tStartM = clock();
		err = clEnqueueWriteBuffer(commands, buffer_state, CL_TRUE, 0, count * sizeof(*in), in, 0, NULL, NULL);
        CHECK_CL_SUCCESS("clEnqueueWriteBuffer", err);
		err = clEnqueueWriteBuffer(commands, buffer_roundkeys, CL_TRUE, 0, 16 * 15, &ks.rd_key, 0, NULL, NULL);
        CHECK_CL_SUCCESS("clEnqueueWriteBuffer", err);
		printf("rd_key %s\n", ks.rd_key);
		//err = clEnqueueWriteBuffer(commands, input, CL_TRUE, 0, sizeof(float) * count, data, 0, NULL, NULL);
		//err = clEnqueueWriteBuffer(commands, key, CL_TRUE, 0, sizeof(float) * count, keyData, 0, NULL, NULL);
		if (err != CL_SUCCESS)
		{
			printf("Error: Failed to write to source array!\n");
			exit(1);
		}
		err = clFinish(commands);
        CHECK_CL_SUCCESS("clFinish", err);
		tMemory += (double)(clock() - tStartM)/CLOCKS_PER_SEC;

		// Set the arguments to our compute kernel
		//printf("Set the arguments to our compute kernel\n");
		//
		clock_t tStartA = clock();
        size_t N;


        /* 1 thread operates on 4 (uint4 vector type has 4 uints, (x, y, w, z)) consecutive 32 bit 
         * (uint) fields at a time.  
         *
         * That is, 1 thread operates on 4*4 == 16 bytes (chars) of the input array.
         *
         * An input of N bytes needs N / 16 threads.
         */
        assert(array_size % 16 == 0);
        /* unsigned int entries = 2; */
        /* unsigned int entries = ((array_size / 16) + global - 1) / global; */
        size_t entries_to_encrypt = array_size / 16;
        unsigned int entries = CEILING_DIVIDE(entries_to_encrypt, global);
		/* global = (entries_to_encrypt + entries - 1)/entries; */
		/* global = 1024; */

        printf("entries == %lu\n", entries);
        printf("entries_to_encrypt == %lu\n", entries_to_encrypt);

		err = 0;
		err  = clSetKernelArg(encrypt_kernel, 0, sizeof(cl_mem), &buffer_state);
        CHECK_CL_SUCCESS("clSetKernelArg", err);
		err |= clSetKernelArg(encrypt_kernel, 1, sizeof(cl_mem), &buffer_roundkeys);
        CHECK_CL_SUCCESS("clSetKernelArg", err);
		err |= clSetKernelArg(encrypt_kernel, 2, sizeof(ks.rounds), &ks.rounds);

        CHECK_CL_SUCCESS("clSetKernelArg", err);
		err |= clSetKernelArg(encrypt_kernel, 3, sizeof(cl_uint), &entries);
        CHECK_CL_SUCCESS("clSetKernelArg", err);
		err |= clSetKernelArg(encrypt_kernel, 4, sizeof(cl_uint), &entries_to_encrypt);
        CHECK_CL_SUCCESS("clSetKernelArg", err);

		if (err != CL_SUCCESS)
		{
			printf("Error: Failed to set kernel arguments! %d\n", err);
			exit(1);
		}
		tArgument += (double)(clock() - tStartA)/CLOCKS_PER_SEC;


		// Execute the kernel over the entire range of our 1d input data set
		// using the maximum number of work group items for this device

#ifdef DEBUG 
		printf("global is %d\n", global);
#endif

		clock_t tStartE = clock();
		cl_float t = 0.;
		cl_ulong start = 0, end = 0;
        cl_uint work_dim = 1;

        size_t max_kernel_work_group_size;
        err = clGetKernelWorkGroupInfo(encrypt_kernel, device_id[DEVICE], CL_KERNEL_WORK_GROUP_SIZE,
                sizeof(size_t), &max_kernel_work_group_size, NULL);


        cl_uint dims;
        size_t max_work_items_dim1;
        err = get_max_work_items(device_id[DEVICE], &dims, &max_work_items_dim1);
        CHECK_CL_SUCCESS("get_max_work_items", err);
		printf("> max dimensions: %i\n", dims);
		printf("> max_work_items_dim1: %zd\n", max_work_items_dim1);
		printf("> max_kernel_work_group_size: %zd\n", max_kernel_work_group_size);

		//for (i = 0; i<LOOP; i++) {
            // local call
            /* unsigned int global_one = MIN(max_kernel_work_group_size, max_work_items_dim1); */
            /* Just one maxmimally sized work group.
             */
            /* unsigned int local_global = global_one; */
            unsigned int global_one = global;
            unsigned int local_global = global;
            printf("> GLOBAL = %lu\n", global_one);
            printf("> LOCAL = %lu\n", local_global);
			err = clEnqueueNDRangeKernel(commands, encrypt_kernel, work_dim, NULL, &global_one, &local_global, 0, NULL, &event);
            // global call
			/* err = clEnqueueNDRangeKernel(commands, encrypt_kernel, work_dim, NULL, &global, &local, 0, NULL, &event); */
            CHECK_CL_SUCCESS("clEnqueueNDRangeKernel", err);
			//err = clEnqueueNDRangeKernel(commands, kernel, 1, NULL, &global, NULL, 0, NULL, NULL);
			if (err)
			{
				printf("Error: Failed to execute kernel!\n");
				return EXIT_FAILURE;
			}
			err = clWaitForEvents(1, &event);
            CHECK_CL_SUCCESS("clWaitForEvents", err);
			err = clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
            CHECK_CL_SUCCESS("clGetEventProfilingInfo", err);
			err = clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);
            CHECK_CL_SUCCESS("clGetEventProfilingInfo", err);
			//END-START gives you hints on kind of “pure HW execution time”
			//the resolution of the events is 1e-09 sec
			t += (cl_float)(end - start)*(cl_float)(1e-06);
		//}
		printf("profile time: %f ms\n",t);
		err = clFinish(commands);
        CHECK_CL_SUCCESS("clFinish", err);
		// Wait for the command commands to get serviced before reading back results
		//
		tExecute += (double)(clock() - tStartE)/CLOCKS_PER_SEC;

		// Read back the results from the device to verify the output
		//
		clock_t tStartR = clock();
		//err = clEnqueueReadBuffer( commands, input, CL_TRUE, 0, sizeof(float) * count, data, 0, NULL, NULL );  
		err = clEnqueueReadBuffer(commands, buffer_state, CL_FALSE, 0, count, out, 0, NULL, NULL);
		if (err != CL_SUCCESS)
		{
			printf("Error: Failed to read output array! %d\n", err);
			exit(1);
		}

        /* print_data("input", count, in); */
        /* print_data("encrypted", count, out); */

		/* printf("input data is\n"); */
		for (i=0; i<count; i++) {
            if (in[i] != '\0') {
                printf("Error: input array wasn't all '\\0's\n");
                exit(EXIT_FAILURE);
            }
			/* printf("%02X", in[i]); */
		}
        /* printf("\n"); */
		/* printf("encrypted data is\n"); */
        size_t max_num_consec = 4;
        size_t num_consec = 1;
        unsigned char last;
		for (i=0; i<count; i++) {
            if (i != 0 && out[i] == last) {
                num_consec += 1;
                if (num_consec > max_num_consec) {
                    printf("Error: encrypted output had more than the maximum number of consecutive repeated bytes %zd. in particular, we saw a repeated sequence of %zd.\n", max_num_consec, num_consec);
                    print_data("input", count, in);
                    print_data("encrypted", count, out);
                    exit(EXIT_FAILURE);
                }
            } else {
                num_consec = 1;
            }
            last = out[i];
			/* printf("%02X", out[i]); */
		}
        /* printf("\n"); */
		tRead += (double)(clock() - tStartR)/CLOCKS_PER_SEC;
	//}




	printf("-----------------------------------------------\n");
	printf("encrypt_cl Time taken: %.2fs\n", (double)(clock() - tStart)/CLOCKS_PER_SEC);
	printf("cl Fill data Time taken: %.2fs\n", tFill); 
	printf("cl memory copy Time taken: %.2fs\n", tMemory); 
	printf("cl set Argument Time taken: %.2fs\n", tArgument); 
	printf("cl Execute kernel time taken: %.2fs\n", tExecute); 
	printf("cl read memory taken: %.2fs\n", tRead); 

	// Validate our results
	//
	correct = 0;
	for(i = 0; i < count; i++)
	{
		//if( data[i] - sqrt(keyData[i]) < 0.001)
			correct++;
	}

	// Print a brief summary detailing the results
#ifdef DEBUG 
	printf("Computed '%d/%d' correct values!\n", correct, count);
#endif

	// Shutdown and cleanup
	//
	clReleaseMemObject(buffer_state);
    CHECK_CL_SUCCESS("clReleaseMemObject", err);
	clReleaseMemObject(buffer_roundkeys);
    CHECK_CL_SUCCESS("clReleaseMemObject", err);
	clReleaseProgram(program);
    CHECK_CL_SUCCESS("clReleaseProgram", err);
	clReleaseKernel(encrypt_kernel);
    CHECK_CL_SUCCESS("clReleaseKernel", err);
	clReleaseCommandQueue(commands);
    CHECK_CL_SUCCESS("clReleaseCommandQueue", err);
	clReleaseContext(context);
    CHECK_CL_SUCCESS("clReleaseContext", err);
}

void print_data(const char* name, unsigned int count, unsigned char* data) {
    unsigned int i;
    printf("%s data is\n", name);
    for (i=0; i<count; i++) {
        printf("%02X", data[i]);
    }
    printf("\n");
}

int get_max_work_items(cl_device_id device_id, cl_uint *dims, size_t *max_work_items_dim1) {
    /* cl_uint dims; */
    int err = 0;
    err = clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(*dims), dims, NULL);
    CHECK_CL_SUCCESS("clGetDeviceInfo", err);
    if(err != CL_SUCCESS){
        /* index += sprintf(&result[index],"  Max work item dimensions ERR: %d\n", err); */
        return err;
    }else{
        //checkErr(err, "clGetDeviceInfo(CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS)");
        //printf("  Max work item dimensions:\t\t\t %d\n", dims);

        size_t sizes[*dims];
        err = clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(size_t)*(*dims), sizes, NULL);
        CHECK_CL_SUCCESS("clGetDeviceInfo", err);

        if (*dims >= 1) {
            *max_work_items_dim1 = sizes[0];
        }
        //checkErr(err, "clGetDeviceInfo(CL_DEVICE_MAX_WORK_ITEM_SIZES)");
        if(err != CL_SUCCESS){
            return err;
            /* index += sprintf(&result[index],"  Max work item dimensions ERR: %d\n", err); */
        }else{
            /* index += sprintf(&result[index],"  Max work item dimensions: %d\n", *dims); */
            /* { */
            /*     unsigned int k; */
            /*     index += sprintf(&result[index],"    Max work items: ("); */
            /*     for (k=0; k<*dims; k++) { */
            /*         index += sprintf(&result[index], "%u", (unsigned int)sizes[k]); */
            /*         if (k != (*dims)-1) */
            /*             index += sprintf(&result[index],","); */
            /*     } */
            /*     index += sprintf(&result[index],")\n"); */
            /* } */
        }
    }
    return 0;
}
