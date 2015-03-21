#include <list>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include "common.h"

using namespace std;

static void replace(string& s, const string& s1, const string& s2,
                    const char skip = 0, bool left = true);

static void separate_commas(string& s) {
  const int n = s.size();

  string t;
  for (int i = 0; i < n; i++) {
    if (s[i] == ',') {
      if (!(i > 0 && isdigit(s[i - 1]) && i < n - 1 && isdigit(s[i + 1]))) {
        t += " , ";
        continue;
      }
    }
    t += string(1, s[i]);
  }

  s = t;
}

void tokenize(const string& s1, vector<string>& lt) {
  if (s1.size() == 0) return;

  lt.clear();

  string s(s1);

  replace(s, "``", " `` ");
  if (s[0] == '"') s.replace(0, 1, "`` ");
  if (s.size() > 2 && s[0] == '`' && s[1] != '`') s.replace(0, 1, "` ");
  replace(s, " \"", "  `` ");
  replace(s, "(\"", "( `` ");
  replace(s, "[\"", "[ `` ");
  replace(s, "{\"", "{ `` ");
  replace(s, "<\"", "< `` ");
  replace(s, " `", "  ` ", '`', false);
  replace(s, "(`", "( ` ", '`', false);
  replace(s, "[`", "[ ` ", '`', false);
  replace(s, "{`", "{ ` ", '`', false);
  replace(s, "<`", "< ` ", '`', false);

  replace(s, "...", " ... ");

  separate_commas(s);
  replace(s, ";", " ; ");
  replace(s, ":", " : ");
  replace(s, "@", " @ ");
  replace(s, "#", " # ");
  replace(s, "$", " $ ");
  replace(s, "%", " % ");
  replace(s, "&", " & ");

  int pos = s.size() - 1;
  while (pos > 0 && s[pos] == ' ') pos--;
  while (pos > 0) {
    char c = s[pos];
    if (c == '[' || c == ']' || c == ')' || c == '}' || c == '>' || c == '"' ||
        c == '\'') {
      pos--;
      continue;
    }
    break;
  }
  if (s[pos] == '.' && !(pos > 0 && s[pos - 1] == '.')) s.replace(pos, 1, " .");

  replace(s, "?", " ? ");
  replace(s, "!", " ! ");

  replace(s, "[", " [ ");
  replace(s, "]", " ] ");
  replace(s, "(", " ( ");
  replace(s, ")", " ) ");
  replace(s, "{", " { ");
  replace(s, "}", " } ");
  replace(s, "<", " < ");
  replace(s, ">", " > ");

  replace(s, "--", " -- ");

  s.replace(string::size_type(0), 0, " ");
  s.replace(s.size(), 0, " ");

  replace(s, "''", " '' ");
  replace(s, "\"", " '' ");

  replace(s, "' ", " ' ", '\'');
  replace(s, "'s ", " 's ");
  replace(s, "'S ", " 'S ");
  replace(s, "'m ", " 'm ");
  replace(s, "'M ", " 'M ");
  replace(s, "'d ", " 'd ");
  replace(s, "'D ", " 'D ");
  replace(s, "'ll ", " 'll ");
  replace(s, "'re ", " 're ");
  replace(s, "'ve ", " 've ");
  replace(s, "n't ", " n't ");
  replace(s, "'LL ", " 'LL ");
  replace(s, "'RE ", " 'RE ");
  replace(s, "'VE ", " 'VE ");
  replace(s, "N'T ", " N'T ");

  replace(s, " Cannot ", " Can not ");
  replace(s, " cannot ", " can not ");
  replace(s, " D'ye ", " D' ye ");
  replace(s, " d'ye ", " d' ye ");
  replace(s, " Gimme ", " Gim me ");
  replace(s, " gimme ", " gim me ");
  replace(s, " Gonna ", " Gon na ");
  replace(s, " gonna ", " gon na ");
  replace(s, " Gotta ", " Got ta ");
  replace(s, " gotta ", " got ta ");
  replace(s, " Lemme ", " Lem me ");
  replace(s, " lemme ", " lem me ");
  replace(s, " More'n ", " More 'n ");
  replace(s, " more'n ", " more 'n ");
  replace(s, "'Tis ", " 'T is ");
  replace(s, "'tis ", " 't is ");
  replace(s, "'Twas ", " 'T was ");
  replace(s, "'twas ", " 't was ");
  replace(s, " Wanna ", " Wan na ");
  replace(s, " wanna ", " wan na ");

  if (s[s.size() - 1] == '\'') s.replace(s.size() - 1, 1, " ' ");

  istringstream is(s);
  string t;
  while (is >> t) {
    lt.push_back(t);
  }
}

static void replace(string& s, const string& s1, const string& s2,
                    const char skip, bool left) {
  string::size_type pos = 0;
  while (1) {
    string::size_type i = s.find(s1, pos);
    if (i == string::npos) break;
    if (skip) {
      if (left && i > 0 && s[i - 1] == skip) {
        pos = i + 1;
        continue;
      } else if (i < s.size() - 1 && s[i + 1] == skip) {
        pos = i + 1;
        continue;
      }
    }
    s.replace(i, s1.size(), s2);
    pos = i + s2.size();
  }
}

void tokenize(const string& s, vector<Token>& vt,
              const bool use_upenn_tokenizer) {
  vector<string> vs;
  if (use_upenn_tokenizer) {
    tokenize(s, vs);
  } else {
    istringstream is(s);
    string token;
    while (is >> token) {
      vs.push_back(token);
    }
  }

  int begin = 0;
  for (vector<string>::const_iterator i = vs.begin(); i != vs.end(); ++i) {
    string::size_type x = s.find(*i, begin);
    int strlen = i->size();
    if (*i == "''") {
      string::size_type y = s.find("\"", begin);
      if (y != string::npos && (x == string::npos || y < x)) {
        x = y;
        strlen = 1;
      }
    }
    if (*i == "``") {
      string::size_type y = s.find("\"", begin);
      if (y != string::npos && (x == string::npos || y < x)) {
        x = y;
        strlen = 1;
      }
    }
    if (x == string::npos) {
      cerr << "internal error: tokenization failed." << endl;
      cerr << "input = " << s << endl;
      exit(1);
    }
    const int end = x + strlen;
    vt.push_back(Token(*i, x, end));
    begin = end;
  }
}
