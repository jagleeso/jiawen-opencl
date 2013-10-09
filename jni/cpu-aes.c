#include <stdio.h>
#include <stdlib.h>
#include <linux/kernel.h>
#include "aes.h"

//128 bit key
static const unsigned char key[] = {
	0x00, 0x11, 0x22, 0x33, 
	0x44, 0x55, 0x66, 0x77,
	0x88, 0x99, 0xaa, 0xbb,
	0xcc, 0xdd, 0xee, 0xff,
};

void encrypt_data(FILE* input_file, FILE* output_file);
void decrypt_data(FILE* input_file, FILE* output_file);

int main(int argc, char* argv[])
{
	//Check for valid number of arguments
	if (argc != 4)
	{
		printf("Invalid number of arguments. %d arguments were supplied.\n", argc);
		printf("Usage: %s inputfile outputfile\n", argv[0]); //Usage: ./xortest inputfile outputfile
		exit(0);
	}

	FILE* input;
	FILE* output;

	//Open input and output files
	input = fopen(argv[1], "r");
	output = fopen(argv[2], "w");

	//Check input file
	if (input == NULL)
	{
		printf("Input file cannot be read.\n");
		exit(0);
	}

	//Check output file
	if (output == NULL)
	{
		printf("Output file cannot be written to.\n");
		exit(0);
	}

	printf("Encrypting %s\n", argv[1]);
	encrypt_data(input, output);
	printf("Encrypted data written to %s\n", argv[2]);

	//Close files
	fclose(input);
	fclose(output);
	
	//Open input and output files
	input = fopen(argv[2], "r");
	output = fopen(argv[3], "w");

	//Check input file
	if (input == NULL)
	{
		printf("Input file cannot be read.\n");
		exit(0);
	}

	//Check output file
	if (output == NULL)
	{
		printf("Output file cannot be written to.\n");
		exit(0);
	}

	printf("Decrypting %s\n", argv[2]);
	decrypt_data(input, output);
	printf("Decrypted data written to %s\n", argv[3]);

	//Close files
	fclose(input);
	fclose(output);

	return 0;
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

void encrypt_data(FILE* input_file, FILE* output_file)
{
	unsigned char inbuf[80];
	unsigned char outbuf[80];
	unsigned char decbuf[80];
	int inlen, outlen;

	AES_KEY enc_key;
	AES_KEY dec_key;
	AES_set_encrypt_key(key, 128, &enc_key);

	//while(1) {
		int i;
		inlen = 80;
		outlen = 80;
//		inlen = fread(inbuf, 1, 80, input_file);
		for (i=0; i<inlen; i++) {
			inbuf[i]=0;
		}
		printf("input data is \n");
		for (i = 0; i < inlen; i++) {
			printf("%c ", inbuf[i]);
		}
		printf("\n");

		AES_encrypt(inbuf, outbuf, &enc_key);  

		printf("encrypted data is \n");
		for (i = 0; i < inlen; i++) {
			printf("%X ", outbuf[i]);
		}
		printf("\n");
		
		AES_set_decrypt_key(key, 128, &dec_key);
		AES_decrypt(outbuf, decbuf, &dec_key);  

		printf("decrypted data is \n");
		for (i = 0; i < inlen; i++) {
			printf("%c ", decbuf[i]);
		}
		printf("\n");

		outlen = fwrite(outbuf, 1, inlen, output_file);

//		if (outlen < AES_BLOCK_SIZE)
//		{
//			break;
//		}
	//}
}

void decrypt_data(FILE* input_file, FILE* output_file)
{
	unsigned char inbuf[80];
	unsigned char outbuf[80];
	int inlen, outlen;

	AES_KEY dec_key;
	AES_set_decrypt_key(key, 128, &dec_key);

	//while(1) {
		int i;
		inlen = fread(inbuf, 1, 80, input_file);
		AES_decrypt(inbuf, outbuf, &dec_key);

		for (i = 0; i < inlen; i++) {
			printf("%c ", outbuf[i]);
		}
		printf("\n");

		outlen = fwrite(outbuf, 1, inlen, output_file);
	//	if (outlen < 80)
	//	{
	//		break;
	//	}
//	}
}
