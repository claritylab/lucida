#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "slre.h"
#include "../../utils/timer.h"
#include "../../utils/memoryman.h"

int fill(FILE *f, char **toFill, int *bufLen, int len) {
  int i = 0;

  while (i < len) {
    int ch = getc(f);
    if (ch == EOF) return i;
    bufLen[i] = 0;
    char *s = (char *)sirius_malloc(5000 + 1);
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
  if (argc < 5) {
    fprintf(stderr, "[ERROR] Invalid arguments provided.\n\n");
    fprintf(stderr, "Usage: %s [NUM PATTERNS] [PATTERN FILE] [NUM QUESTIONS] [QUESTION FILE]\n\n", argv[0]);
    exit(0);
  }

  /* Data */
  int temp[256];
  int s_max = 512;

  /* Timing */
  STATS_INIT("kernel", "regular_expression");
  PRINT_STAT_STRING("abrv", "regex");

  // fill regex patterns
  int PATTERNS = atoi(argv[1]);
  char **patts = (char **)sirius_malloc(PATTERNS * sizeof(char *));
  FILE *f = fopen(argv[2], "r");
  if (f == 0) {
    fprintf(stderr, "File %s not found\n", argv[2]);
    exit(1);
  }
  int numPatterns = fill(f, patts, temp, PATTERNS);
  PRINT_STAT_INT("expressions", numPatterns);

  // fill questions
  int QUESTIONS = atoi(argv[3]);
  char **questions = (char **)sirius_malloc(QUESTIONS * sizeof(char *));
  int *question_len = (int *)sirius_malloc(QUESTIONS * sizeof(int));
  FILE *f1 = fopen(argv[4], "r");
  if (f1 == 0) {
    fprintf(stderr, "File %s not found\n", argv[4]);
    exit(1);
  }
  int numQs = fill(f1, questions, question_len, QUESTIONS);
  PRINT_STAT_INT("questions", numQs);

  tic();
  struct slre **s = (struct slre **)sirius_malloc(numPatterns * sizeof(struct slre *));
  for (int i = 0; i < numPatterns; ++i) {
    s[i] = (struct slre *)sirius_malloc(sizeof(slre));
    if (!slre_compile(s[i], patts[i])) {
      printf("error compiling\n");
    }
  }
  PRINT_STAT_DOUBLE("regex_compile", toc());


  struct cap **caps = (struct cap **)sirius_malloc(numPatterns * numQs * sizeof(struct cap *));
  for (int i = 0; i < numPatterns * numQs; ++i) {
    char *s = (char *)sirius_malloc(s_max);
    struct cap *z = (struct cap *)sirius_malloc(sizeof(struct cap));
    z->ptr = s;
    caps[i] = z;
  }

  tic();
  for (int i = 0; i < numPatterns; ++i) {
    for (int k = 0; k < numQs; ++k) {
      if (slre_match(s[i], questions[k], question_len[k], caps[i * numQs + k]) < -1)
        printf("error\n");
    }
  }
  PRINT_STAT_DOUBLE("regex", toc());

#ifdef TESTING
  fclose(f);
  f = fopen("../input/regex_slre.baseline", "w");

  for (int i = 0; i < numPatterns * numQs; ++i) fprintf(f, "%s\n", caps[i]->ptr);

#endif

  fclose(f);
  fclose(f1);

  STATS_END();

  for (int i = 0; i < PATTERNS; ++i) {
    sirius_free(patts[i]);
    sirius_free(s[i]);
  }

  sirius_free(patts);
  sirius_free(s);

  // for (int i = 0; i < numPatterns * numQs; ++i) {
  //   sirius_free(caps[i]);
  // }

  sirius_free(caps);

  for (int i = 0; i < QUESTIONS; ++i) {
    sirius_free(questions[i]);
  }

  sirius_free(questions);
  sirius_free(question_len);

  return 0;
}
