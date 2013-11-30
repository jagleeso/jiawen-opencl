#ifndef COMMON_H

#define COMMON_H

#define CEILING_DIVIDE(value, divisor) (((value) + (divisor) - 1)/(divisor))
/* Round value up to the nearest multiple of divisor.
 */
#define ROUND_UP(value, divisor) ((((value) + (divisor) - 1)/(divisor))*(divisor))
void print_data(const char* name, unsigned int count, unsigned char* data);
void check_encrypted_data(const char* name, unsigned int count, unsigned char* data);
void check_equal(const char* name1, const char* name2, unsigned int count, unsigned char* data1, unsigned char* data2);

#endif /* end of include guard: COMMON_H */
