#ifndef UTILS_H
#define UTILS_H

#include <vector>

using std::pair;
using std::string;

void convert_images(std::vector<std::pair<string, int> > lines, const char* db_path);

#endif
