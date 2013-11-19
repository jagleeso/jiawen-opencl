#include <stdio.h>
#include <stdlib.h>
#include <linux/kernel.h>
#include "aes.h"
#include <assert.h>

#include "common.h"

//128 bit key
static const unsigned char key[] = {
	0x00, 0x11, 0x22, 0x33, 
	0x44, 0x55, 0x66, 0x77,
	0x88, 0x99, 0xaa, 0xbb,
	0xcc, 0xdd, 0xee, 0xff,
};

/* void encrypt_data(unsigned char* input_file, unsigned char* output_file); */
void encrypt_data(unsigned int array_size, unsigned char* inbuf, unsigned char* outbuf, unsigned char* decbuf);
/* void decrypt_data(unsigned char* input_file, unsigned char* output_file); */
void decrypt_data(FILE* input_file, FILE* output_file);

void print_data(const char* name, unsigned int count, unsigned char* data);

int main(int argc, char* argv[])
{

    int NUM_ARGS = 1;
    unsigned int array_size;

    if (argc < NUM_ARGS + 1) {
        fprintf(stderr, "Error: usage is\n");
        fprintf(stderr, "./cpu-aes array_size\n");
        exit(EXIT_FAILURE);
    }
    int result;
    result = sscanf(argv[1], "%d", &array_size);
    if (result != 1) {
        fprintf(stderr, "Error: expected the size in bytes of the input/output arrays, but saw \"%s\"\n", argv[1]);
        exit(EXIT_FAILURE);
    }

	printf("Encrypting %s\n", argv[1]);
    unsigned char* inbuf = malloc(sizeof(unsigned char)*array_size);
    unsigned char* outbuf = malloc(sizeof(unsigned char)*array_size);
    unsigned char* decbuf = malloc(sizeof(unsigned char)*array_size);

    int i;
    for (i=0; i<array_size; i++) {
        /* inbuf[i] = 0; */
        inbuf[i] = (unsigned char) 0xFF;
    }

    encrypt_data(array_size, inbuf, outbuf, decbuf);
    print_data("input", array_size, inbuf);
    print_data("encrypted", array_size, outbuf);
    print_data("decrypted", array_size, decbuf);
    check_encrypted_data("encrypted", array_size, outbuf);

	return 0;
}

void encrypt_data(unsigned int array_size, unsigned char* inbuf, unsigned char* outbuf, unsigned char* decbuf)
{
    int outlen = array_size;

    AES_KEY enc_key;
    AES_KEY dec_key;
    AES_set_encrypt_key(key, 128, &enc_key);
    printf("rd_key %s\n", enc_key.rd_key);

    AES_encrypt(inbuf, outbuf, &enc_key);  

    AES_set_decrypt_key(key, 128, &dec_key);
    AES_decrypt(outbuf, decbuf, &dec_key);  
}

int amain(int argc, char** argv) {

	int i;
	unsigned char text[]="hello world!";
	unsigned char *enc_out = malloc(80*sizeof(char)); 
	unsigned char *dec_out = malloc(80*sizeof(char));

	AES_KEY enc_key, dec_key;
	AES_set_encrypt_key(key, 128, &enc_key);
	AES_encrypt(text, enc_out, &enc_key);  
	
	AES_set_decrypt_key(key,128,&dec_key);
	AES_decrypt(enc_out, dec_out, &dec_key);

	printf("original:\t");
	for(i=0; *(text+i)!=0x00; i++)
		printf("%X ", *(text+i));

	printf("\nencrypted:\t");
	for(i=0; *(enc_out+i)!=0x00; i++)
		printf("%X ", *(enc_out+i));
	
	printf("\ndecrypted:\t");
	for(i=0;*(dec_out+i)!=0x00;i++)
		printf("%X ",*(dec_out+i));

	printf("\n");
	free(enc_out);
	free(dec_out);

	return 0;
}

/* void decrypt_data(unsigned int array_size, unsigned char* inbuf, unsigned char* outbuf, unsigned char* decbuf) */
/* void decrypt_data(unsigned char* input_file, unsigned char* output_file) */
void decrypt_data(FILE* input_file, FILE* output_file)
{
	unsigned char inbuf[80];
	unsigned char outbuf[80];
	int inlen, outlen;

	AES_KEY dec_key;
	AES_set_decrypt_key(key, 128, &dec_key);

    int i;
    inlen = fread(inbuf, 1, 80, input_file);
    AES_decrypt(inbuf, outbuf, &dec_key);

    for (i = 0; i < inlen; i++) {
        printf("%c ", outbuf[i]);
    }
    printf("\n");

    outlen = fwrite(outbuf, 1, inlen, output_file);
}
