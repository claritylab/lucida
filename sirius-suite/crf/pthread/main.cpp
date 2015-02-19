#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <pthread.h>
#include <vector>

#include "crfpp.h"
#include "../../utils/timer.h"

using namespace std;

vector<CRFPP::Tagger *> taggers;

void *slre_thread(void *tid) {
  int *mytid;
  mytid = (int *)tid;

  // parse and change internal stated to 'parsed'
  taggers[*mytid]->parse();
}

int main(int argc, char **argv) {
  if (argc < 4) {
    fprintf(stderr, "[ERROR] Invalid arguments provided.\n\n");
    fprintf(stderr,
        "Usage: %s [NUMBER OF THREADS] [MODEL] [INPUT DATA]\n\n",
        argv[0]);
    exit(0);
  }

  STATS_INIT ("kernel", "pthread_conditional_random_fields");
  PRINT_STAT_STRING ("abrv", "pthread_crf");

  int NTHREADS = atoi(argv[1]);
  PRINT_STAT_INT("threads", NTHREADS);

  string model = "-m " + (string)argv[2] + " -v 3 -n2";

  char buf[100];
  filebuf fb;
  fb.open (argv[3], ios::in);
  istream file(&fb);

  // count number of sentences
  int sentences = 0;
  while(file) {
    file.getline(buf, 100);
    if(buf[0] == '.') ++sentences;
  }

  PRINT_STAT_INT("sentences", sentences);

  // add to each context
  int iterations = sentences / NTHREADS;
  filebuf fb1;
  fb1.open (argv[3], ios::in);
  istream file1(&fb1);
  for(int i = 0; i < NTHREADS; ++i) {
    CRFPP::Tagger *tagger = CRFPP::createTagger(model.c_str());
    if (!tagger) return -1;

    // clear internal context
    tagger->clear();

    int s = 0;
    while(s < iterations) {
      file1.getline(buf, 100);
      tagger->add(buf);
      if(buf[0] == '.')
        ++s;
    }
    taggers.push_back(tagger);
  }

  tic ();
  int tids[NTHREADS];
  pthread_t threads[NTHREADS];
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  for (int i = 0; i < NTHREADS; i++) {
    tids[i] = i;
    pthread_create(&threads[i], &attr, slre_thread, (void *)&tids[i]);
  }

  for (int i = 0; i < NTHREADS; i++) pthread_join(threads[i], NULL);

  PRINT_STAT_DOUBLE ("crf_pthread", toc());

#ifdef TESTING
  FILE *f = fopen("../input/crf_tag.pthread", "w");

  for (int i = 0; i < NTHREADS; i++) {
    CRFPP::Tagger *tagger = taggers[i];
    for (size_t i = 0; i < tagger->size(); ++i) {
      for (size_t j = 0; j < tagger->xsize(); ++j) {
        fprintf(f, "%s ", tagger->x(i, j));
      }
      fprintf(f, "%s\n", tagger->y2(i));
    }
  }

  fclose(f);
#endif

  STATS_END ();

  for (int i = 0; i < NTHREADS; i++) {
    CRFPP::Tagger *tagger = taggers[i];
    delete tagger;
  }

  return 0;
}
