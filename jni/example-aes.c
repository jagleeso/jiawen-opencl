/* Taken from:
 * http://wiki.opensslfoundation.com/index.php/EVP_Symmetric_Encryption_and_Decryption
 */
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <string.h>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <linux/kernel.h>
#include "common.h"

//128 bit key
static const unsigned char key[] = {
    0x00, 0x11, 0x22, 0x33, 
    0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xaa, 0xbb,
    0xcc, 0xdd, 0xee, 0xff,
};

int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
        unsigned char *iv, unsigned char **ciphertext_ptr, struct timespec* start_of_encryption, struct timespec* end_of_encryption);
int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
        unsigned char *iv, unsigned char **plaintext_ptr);

double milliseconds(struct timespec t) {
    return (t.tv_sec*1e3) + (((double)t.tv_nsec)/1e6);
}

int main(int argc, char *argv[])
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

	printf("> Array input size (bytes): %u\n", array_size);
    unsigned char* plaintext = malloc(sizeof(unsigned char)*array_size);
    assert(plaintext != NULL);

    int i;
    for (i=0; i<array_size; i++) {
        plaintext[i] = 0;
        /* plaintext[i] = 0xFF; */
    }

    /* Set up the key and iv. Do I need to say to not hard code these in a
     * real application? :-)
     */

    /* A 256 bit key */
    unsigned char *key = (unsigned char*) "01234567890123456789012345678901";

    /* A 128 bit IV */
    unsigned char *iv = (unsigned char*) "01234567890123456";

    /* Message to be encrypted */
    /* unsigned char *plaintext = */
    /*     "The quick brown fox jumps over the lazy dog"; */

    /* Buffer for ciphertext. Ensure the buffer is long enough for the
     * ciphertext which may be longer than the plaintext, dependant on the
     * algorithm and mode
     */
    /* unsigned char ciphertext[128]; */

    /* Buffer for the decrypted text */
    /* unsigned char decryptedtext[128]; */

    /* Initialise the library */
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
    OPENSSL_config(NULL);

    /* print_data("data", array_size, plaintext); */

    /* Encrypt the plaintext */
    unsigned char* ciphertext;
    struct timespec start_of_encryption, end_of_encryption;
    int ciphertext_len = encrypt(plaintext, array_size, key, iv,
            &ciphertext, &start_of_encryption, &end_of_encryption);
    double encrypt_time = milliseconds(end_of_encryption) - milliseconds(start_of_encryption);
    printf("> Encryption time (ms): %f\n", encrypt_time);

    /* unsigned char* ciphertext = malloc(sizeof(unsigned char)*ciphertext_len); */

    /* Do something useful with the ciphertext here */
    /* print_data("encrypted", ciphertext_len, ciphertext); */
    /* printf("Ciphertext is:\n"); */
    /* BIO_dump_fp(stdout, ciphertext, ciphertext_len); */

    /* Decrypt the ciphertext */
    unsigned char* decryptedtext;
    int decryptedtext_len = decrypt(ciphertext, ciphertext_len, key, iv,
            &decryptedtext);

    (void)decryptedtext_len;

    /* Add a NULL terminator. We are expecting printable text */
    /* decryptedtext[decryptedtext_len] = '\0'; */

    /* Show the decrypted text */
    /* print_data("decrypted", decryptedtext_len, decryptedtext); */
    /* printf("Decrypted text is:\n"); */
    /* printf("%s\n", decryptedtext); */

    /* check_encrypted_data("encrypted", array_size, ciphertext); */
    check_equal("input", "decrypted", array_size, plaintext, decryptedtext);
	printf("Done\n");

    /* Clean up */
    EVP_cleanup();
    ERR_free_strings();

    return 0;

}

/* int example_main(int arc, unsigned char *argv[]) */
/* { */
/*     #<{(| Set up the key and iv. Do I need to say to not hard code these in a */
/*      * real application? :-) */
/*      |)}># */
/*  */
/*     #<{(| A 256 bit key |)}># */
/*     unsigned char *key = "01234567890123456789012345678901"; */
/*  */
/*     #<{(| A 128 bit IV |)}># */
/*     unsigned char *iv = "01234567890123456"; */
/*  */
/*     #<{(| Message to be encrypted |)}># */
/*     unsigned char *plaintext = */
/*         "The quick brown fox jumps over the lazy dog"; */
/*  */
/*     #<{(| Buffer for ciphertext. Ensure the buffer is long enough for the */
/*      * ciphertext which may be longer than the plaintext, dependant on the */
/*      * algorithm and mode */
/*      |)}># */
/*     unsigned char ciphertext[128]; */
/*  */
/*     #<{(| Buffer for the decrypted text |)}># */
/*     unsigned char decryptedtext[128]; */
/*  */
/*     int decryptedtext_len, ciphertext_len; */
/*  */
/*     #<{(| Initialise the library |)}># */
/*     ERR_load_crypto_strings(); */
/*     OpenSSL_add_all_algorithms(); */
/*     OPENSSL_config(NULL); */
/*  */
/*     #<{(| Encrypt the plaintext |)}># */
/*     ciphertext_len = encrypt(plaintext, strlen(plaintext), key, iv, */
/*             ciphertext); */
/*  */
/*     #<{(| Do something useful with the ciphertext here |)}># */
/*     printf("Ciphertext is:\n"); */
/*     BIO_dump_fp(stdout, ciphertext, ciphertext_len); */
/*  */
/*     #<{(| Decrypt the ciphertext |)}># */
/*     decryptedtext_len = decrypt(ciphertext, ciphertext_len, key, iv, */
/*             decryptedtext); */
/*  */
/*     #<{(| Add a NULL terminator. We are expecting printable text |)}># */
/*     decryptedtext[decryptedtext_len] = '\0'; */
/*  */
/*     #<{(| Show the decrypted text |)}># */
/*     printf("Decrypted text is:\n"); */
/*     printf("%s\n", decryptedtext); */
/*  */
/*     #<{(| Clean up |)}># */
/*     EVP_cleanup(); */
/*     ERR_free_strings(); */
/*  */
/*     return 0; */
/* } */

void handleErrors(void)
{
    ERR_print_errors_fp(stderr);
    abort();
}

int ciphertext_length(int plaintext_len, EVP_CIPHER_CTX *ctx) {
    /* http://stackoverflow.com/questions/3716691/relation-between-input-and-ciphertext-length-in-aes
     */
    return (
        /* Round plaintext length up to closest block size.
         */
        /* (plaintext_len / (EVP_CIPHER_CTX_block_size(ctx) + 1))*EVP_CIPHER_CTX_block_size(ctx) + */
        CEILING_DIVIDE(plaintext_len, EVP_CIPHER_CTX_block_size(ctx)) * EVP_CIPHER_CTX_block_size(ctx) +
        /* IV length (assume == block size).
         */
        EVP_CIPHER_CTX_block_size(ctx)
    );
}

int decrypted_plaintext_length(int ciphertext_len, EVP_CIPHER_CTX *ctx) {
    /* The parameters and restrictions are identical to the encryption operations except that if padding is 
     * enabled the decrypted data buffer out passed to EVP_DecryptUpdate() should have sufficient room for 
     * (inl + cipher_block_size) bytes unless the cipher block size is 1 in which case inl bytes is 
     * sufficient. 
     * 
     * http://www.openssl.org/docs/crypto/EVP_EncryptInit.html
     */
    return (
        ciphertext_len + EVP_CIPHER_CTX_block_size(ctx)
    );
}

/* Measure the time of encryption with:
 * The data to encrypt possibly in the cache (since it's been intialized).
 * The encrypted data not in the cache.
 *
 * So, CPU probably has the advantage over GPU (GPU may or may not have input data to encrypt cached).
 */
int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
        unsigned char *iv, unsigned char **ciphertext_ptr, struct timespec* start_of_encryption, struct timespec* end_of_encryption)
{
    EVP_CIPHER_CTX *ctx;

    int len;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new())) handleErrors();

    /* Initialise the encryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits */
    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
        handleErrors();

    int ciphertext_len_seen;
    int ciphertext_len = ciphertext_length(plaintext_len, ctx);
    unsigned char* ciphertext = malloc(sizeof(unsigned char)*ciphertext_len);
    assert(ciphertext != NULL);
    *ciphertext_ptr = ciphertext; 

    /* Provide the message to be encrypted, and obtain the encrypted output.
     * EVP_EncryptUpdate can be called multiple times if necessary
     *
     * NOTE: "This function can be called multiple times to encrypt successive blocks of data".
     * http://linux.die.net/man/3/evp_encryptinit_ex
     */
    int result = 0;
    result = clock_gettime(CLOCK_REALTIME, start_of_encryption);
    assert(result == 0);

    if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        handleErrors();

    result = clock_gettime(CLOCK_REALTIME, end_of_encryption);
    assert(result == 0);

    /* ciphertext_len = len; */
    ciphertext_len_seen = len;
    assert(ciphertext_len_seen <= ciphertext_len);

    /* Finalise the encryption. Further ciphertext bytes may be written at
     * this stage.
     */
    if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) handleErrors();
    ciphertext_len_seen += len;
    assert(ciphertext_len_seen <= ciphertext_len);

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    assert(ciphertext_len_seen <= ciphertext_len);
    return ciphertext_len_seen;
}


int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
        unsigned char *iv, unsigned char **plaintext_ptr)
{
    EVP_CIPHER_CTX *ctx;

    int len;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new())) handleErrors();

    /* Initialise the decryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits */
    if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
        handleErrors();

    int plaintext_len_seen;
    int plaintext_len = decrypted_plaintext_length(ciphertext_len, ctx);

    unsigned char* plaintext = malloc(sizeof(unsigned char)*plaintext_len);
    assert(plaintext != NULL);
    *plaintext_ptr = plaintext;

    /* Provide the message to be decrypted, and obtain the plaintext output.
     * EVP_DecryptUpdate can be called multiple times if necessary
     */
    if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
        handleErrors();
    plaintext_len_seen = len;
    assert(plaintext_len_seen <= plaintext_len);
    /* plaintext_len = len; */

    /* Finalise the decryption. Further plaintext bytes may be written at
     * this stage.
     */
    if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) handleErrors();
    plaintext_len_seen += len;
    assert(plaintext_len_seen <= plaintext_len);
    /* plaintext_len += len; */

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    assert(plaintext_len_seen <= plaintext_len);
    return plaintext_len_seen;
}
