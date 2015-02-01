
#include <string.h> /* for memcmp, memmove */
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h> /* for malloc, free */
#include <ctype.h>  /* for isupper, islower, tolower */
#include <pthread.h>

#include "porter.h"

static char *s; /* buffer for words to be stemmed */

#define INC 50          /* size units in which s is increased */
static int i_max = INC; /* maximum offset in s */

// change ARRAYSIZE for larger input set
#define ARRAYSIZE 1000000
#define A_INC 10000
static int a_max = ARRAYSIZE;
int iterations;
int NTHREADS;

static struct stemmer **stem_list;

#define LETTER(ch) (isupper(ch) || islower(ch))

void *stem_thread(void *tid) {
  int i, start, *mytid, end;

  mytid = (int *)tid;
  start = (*mytid * iterations);
  end = start + iterations;
  // printf ("Thread %d doing iterations %d to %d\n", *mytid, start, end-1);

  for (i = start; i < end; i++) {
    stem_list[i]->b[stem2(stem_list[i]) + 1] = 0;
  }

  pthread_exit(NULL);
}

int load_data(struct stemmer **stem_list, FILE *f) {
  int a_size = 0;
  while (a_size < ARRAYSIZE) {
    int ch = getc(f);
    if (ch == EOF) return a_size;
    char *s = (char *)malloc(i_max + 1);
    if (LETTER(ch)) {
      int i = 0;
      while (TRUE) {
        if (i == i_max) {
          i_max += INC;
          s = (char *)realloc(s, i_max + 1);
        }
        ch = tolower(ch); /* forces lower case */

        s[i] = ch;
        i++;
        ch = getc(f);
        if (!LETTER(ch)) {
          ungetc(ch, f);
          break;
        }
      }
      struct stemmer *z = create_stemmer();
      z->b = s;
      z->k = i - 1;
      stem_list[a_size] = z;
      if (a_size == a_max) {
        a_max += A_INC;
        stem_list = (struct stemmer **)realloc(stem_list, a_max);
      }
      a_size += 1;
    }
  }
  return a_size;
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    printf("%s <threads> <input>\n", argv[0]);
    exit(0);
  }
  struct timeval tv1, tv2;
  unsigned int totalruntimeseq = 0;
  unsigned int totalruntimepar = 0;

  NTHREADS = atoi(argv[1]);
  FILE *f = fopen(argv[2], "r");
  if (f == 0) {
    fprintf(stderr, "File %s not found\n", argv[1]);
    exit(1);
  }

  stem_list = (struct stemmer **)malloc(ARRAYSIZE * sizeof(struct stemmer *));
  int array_size = load_data(stem_list, f);
  printf("Number of words to stem: %d\n", array_size);

  gettimeofday(&tv1, NULL);
  for (int i = 0; i < array_size; i++) {
    stem_list[i]->b[stem2(stem_list[i]) + 1] = 0;
  }
  gettimeofday(&tv2, NULL);
  totalruntimeseq =
      (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);

  int start, tids[NTHREADS];
  pthread_t threads[NTHREADS];
  pthread_attr_t attr;

  // load the data again
  fclose(f);
  f = fopen(argv[2], "r");
  load_data(stem_list, f);

  gettimeofday(&tv1, NULL);
  iterations = array_size / NTHREADS;

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  for (int i = 0; i < NTHREADS; i++) {
    tids[i] = i;
    pthread_create(&threads[i], &attr, stem_thread, (void *)&tids[i]);
  }

  for (int i = 0; i < NTHREADS; i++) {
    pthread_join(threads[i], NULL);
  }

  gettimeofday(&tv2, NULL);
  totalruntimepar =
      (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);

  printf("Stemmer CPU time=%4.2f ms\n", (double)totalruntimeseq / 1000);
  printf("Stemmer CPU PThread time=%4.2f ms\n", (double)totalruntimepar / 1000);
  printf("Speedup=%4.2f \n", (double)totalruntimeseq / (float)totalruntimepar);

  fclose(f);
  free(s);

  // free up allocated data
  for (int i = 0; i < array_size; i++) {
    free(stem_list[i]->b);
    free(stem_list[i]);
  }

  return 0;
}
