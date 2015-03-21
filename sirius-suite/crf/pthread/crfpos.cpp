/*
 * $Id$
 */

#include <sys/time.h>
#include <stdio.h>
#include <fstream>
#include <map>
#include <list>
#include <iostream>
#include <sstream>
#include <cmath>
#include <set>
#include "crf.h"
#include "common.h"

using namespace std;

multimap<string, string> WNdic;

void tokenize(const string &s1, list<string> &lt);
string base_form(const string &s, const string &pos);

extern int push_stop_watch();

static string normalize(const string &s) {
  string tmp(s);
  for (size_t i = 0; i < tmp.size(); i++) {
    tmp[i] = tolower(tmp[i]);
    if (isdigit(tmp[i])) tmp[i] = '#';
  }
  return tmp;
}

static CRF_State crfstate(const vector<Token> &vt, int i) {
  CRF_State sample;

  string str = vt[i].str;

  sample.label = vt[i].pos;

  sample.add_feature("W0_" + vt[i].str);

  sample.add_feature("NW0_" + normalize(str));

  string prestr = "BOS";
  if (i > 0) prestr = vt[i - 1].str;

  string prestr2 = "BOS";
  if (i > 1) prestr2 = vt[i - 2].str;

  string poststr = "EOS";
  if (i < (int)vt.size() - 1) poststr = vt[i + 1].str;

  string poststr2 = "EOS";
  if (i < (int)vt.size() - 2) poststr2 = vt[i + 2].str;

  sample.add_feature("W-1_" + prestr);
  sample.add_feature("W+1_" + poststr);

  sample.add_feature("W-2_" + prestr2);
  sample.add_feature("W+2_" + poststr2);

  sample.add_feature("W-10_" + prestr + "_" + str);
  sample.add_feature("W0+1_" + str + "_" + poststr);
  sample.add_feature("W-1+1_" + prestr + "_" + poststr);

  for (size_t j = 1; j <= 10; j++) {
    char buf[1000];
    if (str.size() >= j) {
      sprintf(buf, "SUF%d_%s", (int)j, str.substr(str.size() - j).c_str());
      sample.add_feature(buf);
    }
    if (str.size() >= j) {
      sprintf(buf, "PRE%d_%s", (int)j, str.substr(0, j).c_str());
      sample.add_feature(buf);
    }
  }

  for (size_t j = 0; j < str.size(); j++) {
    if (isdigit(str[j])) {
      sample.add_feature("CTN_NUM");
      break;
    }
  }
  for (size_t j = 0; j < str.size(); j++) {
    if (isupper(str[j])) {
      sample.add_feature("CTN_UPP");
      break;
    }
  }
  for (size_t j = 0; j < str.size(); j++) {
    if (str[j] == '-') {
      sample.add_feature("CTN_HPN");
      break;
    }
  }
  bool allupper = true;
  for (size_t j = 0; j < str.size(); j++) {
    if (!isupper(str[j])) {
      allupper = false;
      break;
    }
  }
  if (allupper) sample.add_feature("ALL_UPP");

  if (!WNdic.empty()) {
    const string n = normalize(str);
    for (map<string, string>::const_iterator i = WNdic.lower_bound(n);
         i != WNdic.upper_bound(n); ++i) {
      sample.add_feature("WN_" + i->second);
    }
  }

  return sample;
}

int crftrain(const CRF_Model::OptimizationMethod method, CRF_Model &m,
             const vector<Sentence> &vs, double gaussian, const bool use_l1) {
  if (method != CRF_Model::BFGS && use_l1) {
    cerr << "error: L1 regularization is currently not supported in this mode. "
            "Please use other optimziation methods." << endl;
    exit(1);
  }

  for (vector<Sentence>::const_iterator i = vs.begin(); i != vs.end(); ++i) {
    const Sentence &s = *i;
    CRF_Sequence cs;
    for (size_t j = 0; j < s.size(); j++) cs.add_state(crfstate(s, j));
    m.add_training_sample(cs);
  }

  if (use_l1)
    m.train(method, 0, 0, 1.0);
  else
    m.train(method, 0, gaussian);

  return 0;
}

void crf_decode_lookahead(Sentence &s, CRF_Model &m,
                          vector<map<string, double> > &tagp) {
  CRF_Sequence cs;
  for (size_t j = 0; j < s.size(); j++) cs.add_state(crfstate(s, j));

  m.decode_lookahead(cs);

  tagp.clear();
  for (size_t k = 0; k < s.size(); k++) {
    s[k].prd = cs.vs[k].label;
    map<string, double> vp;
    vp[s[k].prd] = 1.0;
    tagp.push_back(vp);
  }
}

void crf_decode_forward_backward(Sentence &s, CRF_Model &m,
                                 vector<map<string, double> > &tagp) {
  CRF_Sequence cs;
  for (size_t j = 0; j < s.size(); j++) cs.add_state(crfstate(s, j));

  m.decode_forward_backward(cs, tagp);

  for (size_t k = 0; k < s.size(); k++) s[k].prd = cs.vs[k].label;
}

void crf_decode_nbest(Sentence &s, CRF_Model &m,
                      vector<pair<double, vector<string> > > &nbest_seqs,
                      int n) {
  CRF_Sequence cs;
  for (size_t j = 0; j < s.size(); j++) cs.add_state(crfstate(s, j));

  m.decode_nbest(cs, nbest_seqs, n, 0);

  for (size_t k = 0; k < s.size(); k++) s[k].prd = cs.vs[k].label;
}
