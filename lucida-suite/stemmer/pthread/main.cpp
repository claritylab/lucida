
#include <string.h> /* for memcmp, memmove */
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h> /* for malloc, free */
#include <ctype.h>  /* for isupper, islower, tolower */
#include <pthread.h>
#include <errno.h>

#include "../../utils/memoryman.h"
#include "../../utils/pthreadman.h"
#include "../../utils/timer.h"

#include "porter.h"

static char *s; /* buffer for words to be stemmed */

#define INC 50          /* size units in which s is increased */
static int i_max = INC; /* maximum offset in s */

// change WORDS for larger input set
#define A_INC 10000
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

  sirius_pthread_exit(NULL);
}

int load_data(int WORDS, struct stemmer **stem_list, FILE *f) {
  static int a_max = WORDS;
  int a_size = 0;
  while (a_size < WORDS) {
    int ch = getc(f);
    if (ch == EOF) return a_size;
    char *s = (char *)sirius_malloc(i_max + 1);
    if (LETTER(ch)) {
      int i = 0;
      while (TRUE) {
        if (i == i_max) {
          i_max += INC;
          void *_realloc = NULL;
          if ((_realloc = realloc(s, i_max + 1)) == NULL) {
            fprintf(stderr, "realloc() failed!\n");
            return -ENOMEM; 
          }
          s = (char*)_realloc;
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
        void *_realloc = NULL;
        if ((_realloc = realloc(stem_list, a_max)) == NULL) {
            fprintf(stderr, "realloc() failed!\n");
            return -ENOMEM; 
        }
        stem_list = (struct stemmer **)_realloc;
      }
      a_size += 1;
    }
  }
  return a_size;
}

int main(int argc, char *argv[]) {
  if (argc < 4) {
    fprintf(stderr, "[ERROR] Invalid arguments provided.\n\n");
    fprintf(stderr, "Usage: %s [NUMBER OF THREADS] [WORDS] [INPUT FILE]\n\n", argv[0]);
    exit(0);
  }

  /* Timing */
  STATS_INIT("kernel", "pthread_porter_stemming");
  PRINT_STAT_STRING("abrv", "pthread_stemmer");

  NTHREADS = atoi(argv[1]);
  int WORDS = atoi(argv[2]);
  PRINT_STAT_INT("threads", NTHREADS);
  FILE *f = fopen(argv[3], "r");
  if (f == 0) {
    fprintf(stderr, "File %s not found\n", argv[1]);
    exit(1);
  }

  stem_list =
      (struct stemmer **)sirius_malloc(WORDS * sizeof(struct stemmer *));
  int words = load_data(WORDS, stem_list, f);
  fclose(f);
 
 if (words < 0)
    goto out;

  PRINT_STAT_INT("words", words);

  tic();
  int start, tids[NTHREADS];
  pthread_t threads[NTHREADS];
  pthread_attr_t attr;
  iterations = words / NTHREADS;

  sirius_pthread_attr_init(&attr);
  sirius_pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  for (int i = 0; i < NTHREADS; i++) {
    tids[i] = i;
    sirius_pthread_create(&threads[i], &attr, stem_thread, (void *)&tids[i]);
  }

  for (int i = 0; i < NTHREADS; i++) {
    sirius_pthread_join(threads[i], NULL);
  }
  PRINT_STAT_DOUBLE("pthread_stemmer", toc());

  STATS_END();

#ifdef TESTING
  f = fopen("../input/stem_porter.pthread", "w");

  for (int i = 0; i < words; ++i) fprintf(f, "%s\n", stem_list[i]->b);

  fclose(f);
#endif

out:
  sirius_free(s);

  // free up allocated data
  for (int i = 0; i < words; i++) {
    sirius_free(stem_list[i]->b);
    sirius_free(stem_list[i]);
  }

  return 0;
}
