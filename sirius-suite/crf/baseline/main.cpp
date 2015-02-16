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
    fprintf(stderr,
            "Usage: %s [MODEL] [INPUT DATA]\n\n",
            argv[0]);
    exit(0);
  }
  
  STATS_INIT ("kernel", "conditional_random_fields");
  PRINT_STAT_STRING ("abrv", "crf");

  string model = "-m " + (string)argv[1] + " -v 3 -n2";
  CRFPP::Tagger *tagger =
    CRFPP::createTagger(model.c_str());

  if (!tagger) {
    cerr << CRFPP::getTaggerError() << endl;
    return -1;
  }

  // clear internal context
  tagger->clear();

  // count number of sentences and add context
  char buf[100];
  filebuf fb;
  fb.open (argv[2], ios::in);
  istream file(&fb);
  int sentences = 0;
  while(file) {
    if(buf[0] == '.')
      ++sentences;
    file.getline(buf, 100);
    tagger->add(buf);
  }

  PRINT_STAT_INT("sentences", sentences);

  // parse and change internal stated as 'parsed'
  tic ();
  tagger->parse();
  PRINT_STAT_DOUBLE ("crf", toc());

#ifdef TESTING
  FILE *f = fopen("../input/crf.baseline", "w");

  for (size_t i = 0; i < tagger->size(); ++i) {
    for (size_t j = 0; j < tagger->xsize(); ++j) {
      fprintf(f, "%s ", tagger->x(i, j));
    }
    fprintf(f, "%s\n", tagger->y2(i));
  }

  fclose(f);
#endif

  delete tagger;

  STATS_END ();

  return 0;
}
