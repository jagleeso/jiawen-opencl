#include <stdio.h>
#include <stdlib.h>
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