#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "../../timer/timer.h"
#include "slre.h"

#define EXPRESSIONS 100
#define QUESTIONS 400
#define MAXCAPS 60000

/* Data */
char *exps[256];
struct slre *slre[512];
char *bufs[MAXCAPS];
int temp[256], buf_len[512];
struct cap caps[MAXCAPS];

int fill(FILE *f, char **toFill, int *bufLen, int len) {
  int i = 0;

  while (i < len) {
    int ch = getc(f);
    if (ch == EOF) return i;
    bufLen[i] = 0;
    char *s = (char *)malloc(5000 + 1);
    while (1) {
      s[bufLen[i]] = ch;
      ++bufLen[i];
      ch = getc(f);
      if (ch == '\n') {
        s[bufLen[i]] = 0;
        toFill[i] = s;
        ++i;
        break;
      }
    }
  }
  return i;
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "[ERROR] Invalid arguments provided.\n\n");
    fprintf(stderr, "Usage: %s [LIST FILE] [QUESTION FILE]\n\n", argv[0]);
    exit(0);
  }
  /* Timing */
  STATS_INIT ("kernel", "regular_expression");
  PRINT_STAT_STRING ("abrv", "regex");

  FILE *f = fopen(argv[1], "r");
  if (f == 0) {
    fprintf(stderr, "File %s not found\n", argv[1]);
    exit(1);
  }
  int numExps = fill(f, exps, temp, EXPRESSIONS);
  PRINT_STAT_INT ("expressions", numExps);

  FILE *f1 = fopen(argv[2], "r");
  if (f1 == 0) {
    fprintf(stderr, "File %s not found\n", argv[2]);
    exit(1);
  }
  int numQs = fill(f1, bufs, buf_len, QUESTIONS);
  PRINT_STAT_INT ("questions", numQs);

  tic ();
  for (int i = 0; i < numExps; ++i) {
    slre[i] = (struct slre *)malloc(sizeof(slre));
    if (!slre_compile(slre[i], exps[i])) {
      printf("error compiling\n");
    }
  }
  PRINT_STAT_DOUBLE ("regex_compile", toc ());

  tic ();
  for (int i = 0; i < numExps; ++i) {
    for (int k = 0; k < numQs; ++k) {
      if (slre_match(slre[i], bufs[k], buf_len[k], caps) < -1)
        printf("error\n");
    }
  }
  PRINT_STAT_DOUBLE ("regex", toc ());

  STATS_END ();

  return 0;
}
