// FILE: utils/compression.h
//
// Description: this file contains prototypes for file compression and decompression
//
// Date: May 29, 2015

#ifndef COMPRESSION_H
#define COMPRESSION_H

char* compress_stream(char *original, unsigned long int *uncompressed_length, unsigned long int *compressed_length);
char* decompress_stream(char* compressed, unsigned long int *compressed_length, unsigned long int *decompressed_length);

#endif
