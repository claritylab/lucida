#ifndef SENNA_UTILS_H
#define SENNA_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

/* messaging */
void SENNA_error(const char *fmt, ...);
void SENNA_message(const char *fmt, ...);
void SENNA_set_verbose_mode(int verbose);

/* disk */
FILE *SENNA_fopen(const char *path, const char *subpath, const char *mode);
void SENNA_fseek(FILE *stream, long offset, int whence);
long SENNA_ftell(FILE *stream);
void SENNA_fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
void SENNA_fread_tensor_1d(float **ptr, int *n_row, FILE *stream);
void SENNA_fread_tensor_2d(float **ptr, int *n_row, int *n_column,
                           FILE *stream);
char *SENNA_fgetline(char *str, int size, FILE *stream);
void SENNA_fclose(FILE *stream);

/* memory */
void *SENNA_malloc(size_t size, size_t nitems);
void *SENNA_realloc(void *ptr, size_t size, size_t nitems);
void SENNA_free(void *ptr);

/* debug */
void SENNA_print_tensor_1d(float *tensor, int nrow);
void SENNA_print_tensor_2d(float *tensor, int nrow, int ncolumn);

#endif
