#include "darts.h"
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <strstream>

int progress_bar (size_t current, size_t total)
{
  static char bar[] = "*******************************************";
  static int scale = sizeof(bar) - 1;
  static int prev = 0;

  int cur_percentage  = (int)(100.0 * current/total);
  int bar_len         = (int)(1.0   * current*scale/total);

  if (prev != cur_percentage) {
    printf("Making Double-Array: %3d%% |%.*s%*s| ", cur_percentage, bar_len, bar, scale - bar_len, "");
    if (cur_percentage == 100)  printf("\n");
    else                        printf("\r");
    fflush(stdout);
  }
   
  prev = cur_percentage;

  return 1;
};

int main (int argc, char **argv)
{
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " File Index" << std::endl;
    return 0; 
  }

  std::string file  = argv[argc-2];
  std::string index = argv[argc-1];
  std::istream *is;
 
  if (file == "-") is = &std::cin;
  else             is = new std::ifstream (file.c_str());

  if (! *is) {
    std::cerr << "Cannot open: " << file << std::endl;
    return -1; 
  }

  char buf[8192];
  std::vector <char *> str;
  std::vector <int> id;
   
  while (is->getline (buf, 8192)) {
    std::istrstream is (buf, 8192);
    int i; is >> i;
    std::string s;  is >> s;
    char *tmp = new char [s.size()+1];
    strcpy (tmp, s.c_str());
    str.push_back (tmp);
    id.push_back (i);
  }

  Darts::DoubleArray da;
  if (da.build (str.size(), &str[0], 0, &id[0], &progress_bar) < 0) {
    std::cerr << "Cannot build Double-Array" << std::endl;
    return -1; 
  }
   
  if (da.save  (index.c_str()) == -1) {
    std::cerr << "Cannot open: " << index << std::endl;
    return -1; 
  }
   
  for (unsigned int i = 0; i < str.size(); i++) delete [] str[i];
  if (file != "-") delete is;
   
  return 0; 
}
