#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#include "common.h"

void print_data(const char* name, unsigned int count, unsigned char* data) {
    unsigned int i;
    printf("%s length is %u\n", name, count);
    printf("%s data is\n", name);
    for (i=0; i<count; i++) {
        printf("%02X", data[i]);
    }
    printf("\n");
}

void check_encrypted_data(const char* name, unsigned int count, unsigned char* data) {
    size_t max_num_consec = 4;
    size_t num_consec = 1;
    unsigned char last;
    unsigned int i;
    for (i=0; i<count; i++) {
        if (i != 0 && data[i] == last) {
            num_consec += 1;
            if (num_consec > max_num_consec) {
                printf("Error: encrypted output had more than the maximum number of consecutive repeated bytes %zd. in particular, we saw a repeated sequence of %zd.\n", max_num_consec, num_consec);
                print_data(name, count, data);
                exit(EXIT_FAILURE);
            }
        } else {
            num_consec = 1;
        }
        last = data[i];
    }
}

void check_equal(const char* name1, const char* name2, unsigned int count, unsigned char* data1, unsigned char* data2) {
    unsigned int i;
    for (i=0; i<count; i++) {
        if (data1[i] != data2[i]) {
            printf("Error: %s data doesn't match %s data at byte %u\n", name1, name2, i);
            print_data(name1, count, data1);
            print_data(name2, count, data2);
            exit(EXIT_FAILURE);
        }
    }
}

const char *load_kernel_source(const char *filename) {
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

int safe_cmp(const char * safe_str, const char * user_str) {
    size_t len = strlen(safe_str);
    return strncmp(safe_str, user_str, len);
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

double milliseconds(struct timeval t) {
    return (t.tv_sec*1e3) + (((double)t.tv_usec)/1e3);
}

double get_time_ms(struct timeval start) {
    struct timeval end;
	gettimeofday(&end, NULL);
    return milliseconds(end) - milliseconds(start);
}

