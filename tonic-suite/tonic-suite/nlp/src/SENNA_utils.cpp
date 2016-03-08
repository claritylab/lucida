#include "SENNA_utils.h"

static int is_verbose = 0;

static void buffer_reverse_memory(void *ptr_, int block_size, int n_blocks);
static int is_little_endian_cpu();

void SENNA_error(const char *fmt, ...) {
  va_list args;

  fprintf(stderr, "FATAL ERROR: ");
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  va_end(args);
  exit(-1);
}

void SENNA_message(const char *fmt, ...) {
  va_list args;

  if (is_verbose) {
    fprintf(stderr, "[");
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "]\n");
    va_end(args);
  }
}

void SENNA_set_verbose_mode(int verbose) { is_verbose = verbose; }

FILE *SENNA_fopen(const char *path, const char *subpath, const char *mode) {
  FILE *f;
  char *complete_path = NULL;

  if (!path && !subpath)
    SENNA_error("SENNA_fopen: path or subpath should be non NULL");

  if (path && subpath) {
    int pathsize = strlen(path);
    int subpathsize = strlen(subpath);
    complete_path = SENNA_malloc(sizeof(char), pathsize + subpathsize + 1);
    strcpy(complete_path, path);
    strcpy(complete_path + pathsize, subpath);
  }

  f = fopen((complete_path ? complete_path : (path ? path : subpath)), mode);
  if (!f)
    SENNA_error("unable to open file <%s%s>", (path ? path : ""),
                (subpath ? subpath : ""));

  if (sizeof(char) != 1)
    SENNA_error("char size is not 1, sorry can't load binary files");

  if (sizeof(int) != 4)
    SENNA_error("int size is not 4, sorry can't load binary files");

  if (sizeof(float) != 4)
    SENNA_error("float size is not 1, sorry can't load binary files");

  SENNA_free(complete_path);
  return f;
}

void SENNA_fseek(FILE *stream, long offset, int whence) {
  if (fseek(stream, offset, whence)) SENNA_error("unable to seek into a file");
}

long SENNA_ftell(FILE *stream) {
  long res = ftell(stream);
  if (res == -1) SENNA_error("unable to tell where we are in a file");
  return res;
}

void SENNA_fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
  size_t res = fread(ptr, size, nmemb, stream);

  if (res != nmemb)
    SENNA_error("read error: read %ld elements instead of %ld (of size %ld)",
                res, nmemb, size);

  if (size > 1 && !is_little_endian_cpu())
    buffer_reverse_memory(ptr, size, nmemb);
}

void SENNA_fread_tensor_1d(float **ptr, int *n_row, FILE *stream) {
  SENNA_fread(n_row, sizeof(int), 1, stream);
  *ptr = SENNA_malloc(sizeof(float), *n_row);
  SENNA_fread(*ptr, sizeof(float), *n_row, stream);
}

void SENNA_fread_tensor_2d(float **ptr, int *n_row, int *n_column,
                           FILE *stream) {
  SENNA_fread(n_row, sizeof(int), 1, stream);
  SENNA_fread(n_column, sizeof(int), 1, stream);
  *ptr = SENNA_malloc(sizeof(float), (*n_row) * (*n_column));
  SENNA_fread(*ptr, sizeof(float), (*n_row) * (*n_column), stream);
}

char *SENNA_fgetline(char *str, int size, FILE *stream) {
  int str_size;

  if (fgets(str, size, stream)) {
    str_size = strlen(str);
    if ((str_size > 0) && (str[str_size - 1] == '\n')) str[str_size - 1] = '\0';
    return str;
  } else
    return NULL;
}

void SENNA_fclose(FILE *stream) { fclose(stream); }

void *SENNA_malloc(size_t size, size_t nitems) {
  void *res = malloc(size * nitems);
  if (!res)
    SENNA_error("memory allocation error [%ldGB] -- buy new RAM", size << 30);
  return res;
}

void *SENNA_realloc(void *ptr, size_t size, size_t nitems) {
  ptr = realloc(ptr, size * nitems);
  if (!ptr)
    SENNA_error("memory allocation error [%ldGB] -- buy new RAM", size << 30);
  return ptr;
}

void SENNA_free(void *ptr) { free(ptr); }

static void buffer_reverse_memory(void *ptr_, int block_size, int n_blocks) {
  char *ptr;
  char *ptrr;
  char *ptrw;
  int i, j;
  char *buffer_block;

  if (block_size == 1) return;

  ptr = (char *)ptr_;
  buffer_block = SENNA_malloc(sizeof(char), block_size);

  for (i = 0; i < n_blocks; i++) {
    ptrr = ptr + ((i + 1) * block_size);
    ptrw = buffer_block;

    for (j = 0; j < block_size; j++) {
      ptrr--;
      *ptrw++ = *ptrr;
    }

    ptrr = buffer_block;
    ptrw = ptr + (i * block_size);
    for (j = 0; j < block_size; j++) *ptrw++ = *ptrr++;
  }

  SENNA_free(buffer_block);
}

static int is_little_endian_cpu() {
  int x = 7;
  char *ptr = (char *)&x;

  if (ptr[0] == 0)
    return 0;
  else
    return 1;
}

void SENNA_print_tensor_1d(float *tensor, int nrow) {
  int r;

  printf("\n---\n");
  for (r = 0; r < nrow; r++) printf("%f ", tensor[r]);
  printf("[Tensor of size %d]\n", nrow);
}

void SENNA_print_tensor_2d(float *tensor, int nrow, int ncolumn) {
  int c, r;

  printf("\n---\n");
  for (c = 0; c < ncolumn; c++) {
    for (r = 0; r < nrow; r++) printf("%f ", tensor[c * nrow + r]);
    printf("\n---\n");
  }
  printf("[Tensor of size %dx%d]\n", nrow, ncolumn);
}
