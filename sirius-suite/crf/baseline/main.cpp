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

int main(int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr, "[ERROR] Invalid arguments provided.\n\n");
    fprintf(stderr, "Usage: %s [CONTEXTS] [MODEL] [INPUT DATA]\n\n", argv[0]);
    exit(0);
  }

  STATS_INIT("kernel", "conditional_random_fields");
  PRINT_STAT_STRING("abrv", "crf");

  // contexts to force baseline to see data some way as pthreaded version
  int CONTEXTS = atoi(argv[1]);

  string model = "-m " + (string)argv[2] + " -v 3 -n2";

  // count number of sentences
  int sentences = 0;
  char buf[100];
  filebuf fb;
  fb.open(argv[3], ios::in);
  istream file(&fb);
  while (file) {
    file.getline(buf, 100);
    if (buf[0] == '.') ++sentences;
  }

  PRINT_STAT_INT("sentences", sentences);

  // add equal number of sentences to each context
  vector<CRFPP::Tagger *> taggers;
  int iterations = sentences / CONTEXTS;
  filebuf fb1;
  fb1.open(argv[3], ios::in);
  istream file1(&fb1);
  for (int i = 0; i < CONTEXTS; ++i) {
    CRFPP::Tagger *tagger = CRFPP::createTagger(model.c_str());
    if (!tagger) return -1;

    // clear internal context
    tagger->clear();

    int s = 0;
    while (s < iterations) {
      file1.getline(buf, 100);
      tagger->add(buf);
      if (buf[0] == '.') ++s;
    }
    taggers.push_back(tagger);
  }

  // parse and change internal stated to 'parsed'
  tic();
  for (int i = 0; i < CONTEXTS; ++i) taggers[i]->parse();
  PRINT_STAT_DOUBLE("crf", toc());

#ifdef TESTING
  FILE *f = fopen("../input/crf_tag.baseline", "w");

  for (int i = 0; i < CONTEXTS; i++) {
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

  STATS_END();

  for (int i = 0; i < CONTEXTS; i++) {
    CRFPP::Tagger *tagger = taggers[i];
    delete tagger;
  }

  return 0;
}
