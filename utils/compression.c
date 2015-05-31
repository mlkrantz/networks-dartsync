// FILE: utils/compression.c
//
// Description: this file contains function definitions for file compression and decompression. Thanks to Ethan for 
// the help!
//
// Date: May 29, 2015

#include <zlib.h>
#include <stdlib.h>
#include <stdio.h>
#include "compression.h"

// decompressed_length must already been assigned
char* compress_stream(char *original, unsigned long int *decompressed_length, unsigned long int *compressed_length) {
    *compressed_length = compressBound(*decompressed_length); // Upper bound on compressed string length
    char *compress_buf = calloc(*compressed_length, sizeof(char));
    int ret = compress((unsigned char*) compress_buf, compressed_length, (unsigned char*) original, *decompressed_length);
    if (ret != Z_OK) {
        switch (ret) {
            case Z_BUF_ERROR:
                printf("Buffer was not large enough to hold decompressed data\n");
                break;
            case Z_MEM_ERROR:
                printf("Out of memory!\n");
                break;
            case Z_DATA_ERROR:
                printf("Data is corrupted!\n");
                break;
            default:
                printf("Unknown error\n");
                break;
        }
        fflush(stdout);
        free(compress_buf);
        return NULL;
    }
    compress_buf = (char*) realloc(compress_buf, *compressed_length);
    return compress_buf;
}

char* decompress_stream(char* compressed, unsigned long int *compressed_length, unsigned long int *decompressed_length) {
    char *decompress_buf = calloc(*decompressed_length, sizeof(char));
    int ret = uncompress((unsigned char*) decompress_buf, decompressed_length, (unsigned char*) compressed, *compressed_length);
    if (ret != Z_OK) {
        free(decompress_buf);
        return NULL;
    }
    decompress_buf = (char*) realloc(decompress_buf, *decompressed_length);
    return decompress_buf;
}
