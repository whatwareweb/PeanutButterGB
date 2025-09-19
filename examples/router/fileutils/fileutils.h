/*
    This file was adapted from the SDL project
*/

#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <stdio.h>


/**
 * Load all the data from a file path.
 *
 * The data is allocated with a zero byte at the end (null terminated) for
 * convenience.
 */
uint8_t * load_file(const char *filename);


/**
 * Retrieve the size of the file passed by the pointer
 */
long get_file_size(FILE *f);

#endif