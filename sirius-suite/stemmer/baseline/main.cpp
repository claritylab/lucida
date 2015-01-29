
#include <string.h> /* for memcmp, memmove */
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h> /* for malloc, free */
#include <ctype.h>  /* for isupper, islower, tolower */
#include "porter.h"

static char *s; /* buffer for words to be stemmed */

#define INC 50          /* size units in which s is increased */
static int i_max = INC; /* maximum offset in s */
// change ARRAYSIZE for larger input set
#define ARRAYSIZE 100000
#define A_INC 10000
static int a_max = ARRAYSIZE;

static struct stemmer **stem_list;

#define LETTER(ch) (isupper(ch) || islower(ch))

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
      // word_list[a_size] = s;
      // s[stem(z, s, i - 1) + 1] = 0;
      if (a_size == a_max) {
        a_max += A_INC;
        stem_list = (struct stemmer **)realloc(stem_list, a_max);
      }
      a_size += 1;
    }
  }
  return a_size;
}

void stemfile(struct stemmer *z, FILE *f) {
  while (TRUE) {
    int ch = getc(f);
    if (ch == EOF) return;
    if (LETTER(ch)) {
      int i = 0;
      while (TRUE) {
        if (i == i_max) {
          i_max += INC;
          s = realloc(s, i_max + 1);
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
      s[stem(z, s, i - 1) + 1] = 0;
      /* the previous line calls the stemmer and uses its result to
         zero-terminate the string in s */
      printf("%s",s);
    }
    // else putchar(ch);
  }
}

int main(int argc, char *argv[]) {
  int i;
  struct timeval tv1, tv2;
  unsigned int totalruntime = 0;

  FILE *f = fopen(argv[1], "r");
  if (f == 0) {
    fprintf(stderr, "File %s not found\n", argv[1]);
    exit(1);
  }

  stem_list = (struct stemmer **)malloc(ARRAYSIZE * sizeof(struct stemmer *));
  int array_size = load_data(stem_list, f);
  printf("number of words to stem: %d\n", array_size);

  gettimeofday(&tv1, NULL);
  for (int i = 0; i < array_size; i++) {
    stem_list[i]->b[stem2(stem_list[i]) + 1] = 0;
    // printf("%s\n",stem_list[i]->b);
  }
  gettimeofday(&tv2, NULL);
  totalruntime =
      (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);

  printf("Stemmer CPU time=%4.2f ms\n", (double)totalruntime / 1000);
  free(s);

  // free up allocated data
  for (int i = 0; i < array_size; i++) {
    free(stem_list[i]->b);
    free(stem_list[i]);
  }

  return 0;
}
