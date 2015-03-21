
#include <string.h> /* for memcmp, memmove */
#include <stdio.h>
#include <stdlib.h> /* for malloc, free */
#include <ctype.h>  /* for isupper, islower, tolower */
#include <errno.h>

#include "../../utils/timer.h"
#include "../../utils/memoryman.h"
#include "porter.h"

static char *s; /* buffer for words to be stemmed */

#define INC 50          /* size units in which s is increased */
static int i_max = INC; /* maximum offset in s */
// $cmt: change ARRAYSIZE for larger input set
#define ARRAYSIZE 1000000
#define A_INC 10000
static int a_max = ARRAYSIZE;

static struct stemmer **stem_list;

#define LETTER(ch) (isupper(ch) || islower(ch))

int load_data(struct stemmer **stem_list, FILE *f) {
  int a_size = 0;
  while (a_size < ARRAYSIZE) {
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
      // word_list[a_size] = s;
      // s[stem(z, s, i - 1) + 1] = 0;
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
  if (argc < 2) {
    fprintf(stderr, "[ERROR] Input file required.\n\n");
    fprintf(stderr, "Usage: %s [INPUT FILE]\n\n", argv[0]);
    exit(0);
  }

  /* Timing */
  STATS_INIT("kernel", "porter_stemming");
  PRINT_STAT_STRING("abrv", "stemmer");

  FILE *f = fopen(argv[1], "r");
  if (f == 0) {
    fprintf(stderr, "File %s not found\n", argv[1]);
    exit(1);
  }

  stem_list =
      (struct stemmer **)sirius_malloc(ARRAYSIZE * sizeof(struct stemmer *));
  int words = load_data(stem_list, f);
  if (words < 0)
    goto out;

  fclose(f);
  PRINT_STAT_INT("words", words);

  tic();
  for (int i = 0; i < words; i++) {
    stem_list[i]->b[stem2(stem_list[i]) + 1] = 0;
    // printf("%s\n",stem_list[i]->b);
  }
  PRINT_STAT_DOUBLE("stemmer", toc());

  STATS_END();

#ifdef TESTING
  f = fopen("../input/stem_porter.baseline", "w");

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
