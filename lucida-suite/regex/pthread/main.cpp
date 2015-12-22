#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <pthread.h>

#include "slre.h"

#include "../../utils/memoryman.h"
#include "../../utils/pthreadman.h"
#include "../../utils/timer.h"

/* Data */
char **patts;
char **questions;
int *question_len;
struct slre **s;
struct cap **caps;
int temp[512];

int iterations;
int NTHREADS;
int numQs;
int numPatterns;
int s_max = 512;

void *slre_thread(void *tid) {
  int start, end, *mytid;
  mytid = (int *)tid;
  start = (*mytid * iterations);
  end = start + iterations;

  for (int i = start; i < end; ++i) {
    for (int j = 0; j < numQs; ++j) {
      slre_match(s[i], questions[j], question_len[j], caps[i * numQs + j]);
    }
  }

  return NULL;
}

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
  if (argc < 6) {
    fprintf(stderr, "[ERROR] Invalid arguments provided.\n\n");
    fprintf(stderr, "Usage: %s [THREADS] [NUM PATTERNS] [PATTERN FILE] [NUM QUESTIONS] [QUESTION FILE]\n\n", argv[0]);
    exit(0);
  }

  /* Timing */
  STATS_INIT("kernel", "regular_expression");
  PRINT_STAT_STRING("abrv", "pthread_regex");

  NTHREADS = atoi(argv[1]);
  PRINT_STAT_INT("threads", NTHREADS);

  // fill regex patterns
  int PATTERNS = atoi(argv[2]);
  patts = (char **)sirius_malloc(PATTERNS * sizeof(char *));
  FILE *f = fopen(argv[3], "r");
  if (f == 0) {
    fprintf(stderr, "File %s not found\n", argv[3]);
    exit(1);
  }
  numPatterns = fill(f, patts, temp, PATTERNS);
  PRINT_STAT_INT("expressions", numPatterns);

  // fill questions
  int QUESTIONS = atoi(argv[4]);
  questions = (char **)sirius_malloc(QUESTIONS * sizeof(char *));
  question_len = (int *)sirius_malloc(QUESTIONS * sizeof(int));
  FILE *f1 = fopen(argv[5], "r");
  if (f1 == 0) {
    fprintf(stderr, "File %s not found\n", argv[5]);
    exit(1);
  }
  numQs = fill(f1, questions, question_len, QUESTIONS);
  PRINT_STAT_INT("questions", numQs);

  tic();
  s = (struct slre **)sirius_malloc(numPatterns * sizeof(struct slre *));
  for (int i = 0; i < numPatterns; ++i) {
    s[i] = (struct slre *)sirius_malloc(sizeof(struct slre));
    if (!slre_compile(s[i], patts[i])) {
      printf("error compiling: %s\n", patts[i]);
    }
  }
  PRINT_STAT_DOUBLE("regex_compile", toc());

  caps = (struct cap **)sirius_malloc(numPatterns * numQs * sizeof(struct cap));
  for (int i = 0; i < numPatterns * numQs; ++i) {
    char *s = (char *)sirius_malloc(s_max);
    struct cap *z = (struct cap *)sirius_malloc(sizeof(struct cap));
    z->ptr = s;
    caps[i] = z;
  }
  
  tic();
  int tids[NTHREADS];
  pthread_t threads[NTHREADS];
  pthread_attr_t attr;
  iterations = numPatterns / NTHREADS;
  sirius_pthread_attr_init(&attr);
  sirius_pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  for (int i = 0; i < NTHREADS; i++) {
    tids[i] = i;
    sirius_pthread_create(&threads[i], &attr, slre_thread, (void *)&tids[i]);
  }

  for (int i = 0; i < NTHREADS; i++) sirius_pthread_join(threads[i], NULL);

  PRINT_STAT_DOUBLE("pthread_regex", toc());

  STATS_END();

#ifdef TESTING
  fclose(f);
  f = fopen("../input/regex_slre.pthread", "w");

  for (int i = 0; i < numPatterns * numQs; ++i) fprintf(f, "%s\n", caps[i]->ptr);
#endif
  fclose(f);
  fclose(f1);

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
