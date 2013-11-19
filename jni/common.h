#ifndef COMMON_H

#define COMMON_H

#define CEILING_DIVIDE(value, divisor) (((value) + (divisor) - 1)/(divisor))
void print_data(const char* name, unsigned int count, unsigned char* data);
void check_encrypted_data(const char* name, unsigned int count, unsigned char* data);

#endif /* end of include guard: COMMON_H */
