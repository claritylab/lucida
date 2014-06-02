//
//  CRF++ -- Yet Another CRF toolkit
//
//  $Id: feature_index.cpp 1587 2007-02-12 09:00:36Z taku $;
//
//  Copyright(C) 2005-2007 Taku Kudo <taku@chasen.org>
//
#include <iostream>
#include <fstream>
#include <cstring>
#include <set>
#include "common.h"
#include "feature_index.h"

namespace CRFPP {
namespace {
const char *read_ptr(const char **ptr, size_t size) {
  const char *r = *ptr;
  *ptr += size;
  return r;
}

template <class T> static inline void read_static(const char **ptr,
                                                  T *value) {
  const char *r = read_ptr(ptr, sizeof(T));
  memcpy(value, r, sizeof(T));
}

void make_templs(const std::vector<std::string> unigram_templs,
                 const std::vector<std::string> bigram_templs,
                 std::string *templs) {
  templs->clear();
  for (size_t i = 0; i < unigram_templs.size(); ++i) {
    templs->append(unigram_templs[i]);
    templs->append("\n");
  }
  for (size_t i = 0; i < bigram_templs.size(); ++i) {
    templs->append(bigram_templs[i]);
    templs->append("\n");
  }
}
}  // namespace

char *Allocator::strdup(const char *p) {
  const size_t len = std::strlen(p);
  char *q = char_freelist_->alloc(len + 1);
  std::strcpy(q, p);
  return q;
}

Allocator::Allocator(size_t thread_num)
    : thread_num_(thread_num),
      feature_cache_(new FeatureCache),
      char_freelist_(new FreeList<char>(8192)) {
  init();
}

Allocator::Allocator()
    : thread_num_(1),
      feature_cache_(new FeatureCache),
      char_freelist_(new FreeList<char>(8192)) {
  init();
}

Allocator::~Allocator() {}

Path *Allocator::newPath(size_t thread_id) {
  return path_freelist_[thread_id].alloc();
}

Node *Allocator::newNode(size_t thread_id) {
  return node_freelist_[thread_id].alloc();
}

void Allocator::clear() {
  feature_cache_->clear();
  char_freelist_->free();
  for (size_t i = 0; i < thread_num_; ++i) {
    path_freelist_[i].free();
    node_freelist_[i].free();
  }
}

void Allocator::clear_freelist(size_t thread_id) {
  path_freelist_[thread_id].free();
  node_freelist_[thread_id].free();
}

FeatureCache *Allocator::feature_cache() const {
  return feature_cache_.get();
}

size_t Allocator::thread_num() const {
  return thread_num_;
}

void Allocator::init() {
  path_freelist_.reset(new FreeList<Path> [thread_num_]);
  node_freelist_.reset(new FreeList<Node> [thread_num_]);
  for (size_t i = 0; i < thread_num_; ++i) {
    path_freelist_[i].set_size(8192 * 16);
    node_freelist_[i].set_size(8192);
  }
}

int DecoderFeatureIndex::getID(const char *key) const {
  return da_.exactMatchSearch<Darts::DoubleArray::result_type>(key);
}

int EncoderFeatureIndex::getID(const char *key) const {
  std::map <std::string, std::pair<int, unsigned int> >::iterator
      it = dic_.find(key);
  if (it == dic_.end()) {
    dic_.insert(std::make_pair
                (std::string(key),
                 std::make_pair(maxid_, static_cast<unsigned int>(1))));
    const int n = maxid_;
    maxid_ += (key[0] == 'U' ? y_.size() : y_.size() * y_.size());
    return n;
  } else {
    it->second.second++;
    return it->second.first;
  }
  return -1;
}

bool EncoderFeatureIndex::open(const char *template_filename,
                               const char *train_filename) {
  check_max_xsize_ = true;
  return openTemplate(template_filename) && openTagSet(train_filename);
}

bool EncoderFeatureIndex::openTemplate(const char *filename) {
  std::ifstream ifs(WPATH(filename));
  CHECK_FALSE(ifs) << "open failed: "  << filename;

  std::string line;
  while (std::getline(ifs, line)) {
    if (!line[0] || line[0] == '#') {
      continue;
    }
    if (line[0] == 'U') {
      unigram_templs_.push_back(line);
    } else if (line[0] == 'B') {
      bigram_templs_.push_back(line);
    } else {
      CHECK_FALSE(true) << "unknown type: " << line << " " << filename;
    }
  }

  make_templs(unigram_templs_, bigram_templs_, &templs_);

  return true;
}

bool EncoderFeatureIndex::openTagSet(const char *filename) {
  std::ifstream ifs(WPATH(filename));
  CHECK_FALSE(ifs) << "no such file or directory: " << filename;

  scoped_fixed_array<char, 8192> line;
  scoped_fixed_array<char *, 1024> column;
  size_t max_size = 0;
  std::set<std::string> candset;

  while (ifs.getline(line.get(), line.size())) {
    if (line[0] == '\0' || line[0] == ' ' || line[0] == '\t') {
      continue;
    }
    const size_t size = tokenize2(line.get(), "\t ",
                                  column.get(), column.size());
    if (max_size == 0) {
      max_size = size;
    }
    CHECK_FALSE(max_size == size)
        << "inconsistent column size: "
        << max_size << " " << size << " " << filename;
    xsize_ = size - 1;
    candset.insert(column[max_size-1]);
  }

  y_.clear();
  for (std::set<std::string>::iterator it = candset.begin();
       it != candset.end(); ++it) {
    y_.push_back(*it);
  }

  ifs.close();

  return true;
}

bool DecoderFeatureIndex::open(const char *model_filename) {
  CHECK_FALSE(mmap_.open(model_filename)) << mmap_.what();
  return openFromArray(mmap_.begin(), mmap_.file_size());
}

bool DecoderFeatureIndex::openFromArray(const char *ptr, size_t size) {
  unsigned int version_ = 0;
  const char *end = ptr + size;
  read_static<unsigned int>(&ptr, &version_);

  CHECK_FALSE(version_ / 100 == version / 100)
      << "model version is different: " << version_
      << " vs " << version;
  int type = 0;
  read_static<int>(&ptr, &type);
  read_static<double>(&ptr, &cost_factor_);
  read_static<unsigned int>(&ptr, &maxid_);
  read_static<unsigned int>(&ptr, &xsize_);

  unsigned int dsize = 0;
  read_static<unsigned int>(&ptr, &dsize);

  unsigned int y_str_size;
  read_static<unsigned int>(&ptr, &y_str_size);
  const char *y_str = read_ptr(&ptr, y_str_size);
  size_t pos = 0;
  while (pos < y_str_size) {
    y_.push_back(y_str + pos);
    while (y_str[pos++] != '\0') {}
  }

  unsigned int tmpl_str_size;
  read_static<unsigned int>(&ptr, &tmpl_str_size);
  const char *tmpl_str = read_ptr(&ptr, tmpl_str_size);
  pos = 0;
  while (pos < tmpl_str_size) {
    const char *v = tmpl_str + pos;
    if (v[0] == '\0') {
      ++pos;
    } else if (v[0] == 'U') {
      unigram_templs_.push_back(v);
    } else if (v[0] == 'B') {
      bigram_templs_.push_back(v);
    } else {
      CHECK_FALSE(true) << "unknown type: " << v;
    }
    while (tmpl_str[pos++] != '\0') {}
  }

  make_templs(unigram_templs_, bigram_templs_, &templs_);

  da_.set_array(const_cast<char *>(ptr));
  ptr += dsize;

  alpha_float_ = reinterpret_cast<const float *>(ptr);
  ptr += sizeof(alpha_float_[0]) * maxid_;

  CHECK_FALSE(ptr == end) << "model file is broken.";

  return true;
}

void EncoderFeatureIndex::shrink(size_t freq, Allocator *allocator) {
  if (freq <= 1) {
    return;
  }

  std::map<int, int> old2new;
  int new_maxid = 0;

  for (std::map<std::string, std::pair<int, unsigned int> >::iterator
           it = dic_.begin(); it != dic_.end();) {
    const std::string &key = it->first;

    if (it->second.second >= freq) {
      old2new.insert(std::make_pair(it->second.first, new_maxid));
      it->second.first = new_maxid;
      new_maxid += (key[0] == 'U' ? y_.size() : y_.size() * y_.size());
      ++it;
    } else {
      dic_.erase(it++);
    }
  }

  allocator->feature_cache()->shrink(&old2new);

  maxid_ = new_maxid;
}

bool EncoderFeatureIndex::convert(const char *text_filename,
                                  const char *binary_filename) {
  std::ifstream ifs(WPATH(text_filename));

  y_.clear();
  dic_.clear();
  unigram_templs_.clear();
  bigram_templs_.clear();
  xsize_ = 0;
  maxid_ = 0;

  CHECK_FALSE(ifs) << "open failed: " << text_filename;

  scoped_fixed_array<char, 8192> line;
  char *column[8];

  // read header
  while (true) {
    CHECK_FALSE(ifs.getline(line.get(), line.size()))
        << " format error: " << text_filename;

    if (std::strlen(line.get()) == 0) {
      break;
    }

    const size_t size = tokenize(line.get(), "\t ", column, 2);

    CHECK_FALSE(size == 2) << "format error: " << text_filename;

    if (std::strcmp(column[0], "xsize:") == 0) {
      xsize_ = std::atoi(column[1]);
    }

    if (std::strcmp(column[0], "maxid:") == 0) {
      maxid_ = std::atoi(column[1]);
    }
  }

  CHECK_FALSE(maxid_ > 0) << "maxid is not defined: " << text_filename;

  CHECK_FALSE(xsize_ > 0) << "xsize is not defined: " << text_filename;

  while (true) {
    CHECK_FALSE(ifs.getline(line.get(), line.size()))
        << "format error: " << text_filename;
    if (std::strlen(line.get()) == 0) {
      break;
    }
    y_.push_back(line.get());
  }

  while (true) {
    CHECK_FALSE(ifs.getline(line.get(), line.size()))
        << "format error: " << text_filename;
    if (std::strlen(line.get()) == 0) {
      break;
    }
    if (line[0] == 'U') {
      unigram_templs_.push_back(line.get());
    } else if (line[0] == 'B') {
      bigram_templs_.push_back(line.get());
    } else {
      CHECK_FALSE(true) << "unknown type: " << line.get()
                        << " " << text_filename;
    }
  }

  while (true) {
    CHECK_FALSE(ifs.getline(line.get(), line.size()))
        << "format error: " << text_filename;
    if (std::strlen(line.get()) == 0) {
      break;
    }

    const size_t size = tokenize(line.get(), "\t ", column, 2);
    CHECK_FALSE(size == 2) << "format error: " << text_filename;

    dic_.insert(std::make_pair
                (std::string(column[1]),
                 std::make_pair(std::atoi(column[0]),
                                static_cast<unsigned int>(1))));
  }

  std::vector<double> alpha;
  while (ifs.getline(line.get(), line.size())) {
    alpha.push_back(std::atof(line.get()));
  }

  alpha_ = &alpha[0];

  CHECK_FALSE(alpha.size() == maxid_) << " file is broken: "  << text_filename;

  return save(binary_filename, false);
}


bool EncoderFeatureIndex::save(const char *filename,
                               bool textmodelfile) {
  std::vector<char *> key;
  std::vector<int>    val;

  std::string y_str;
  for (size_t i = 0; i < y_.size(); ++i) {
    y_str += y_[i];
    y_str += '\0';
  }

  std::string templ_str;
  for (size_t i = 0; i < unigram_templs_.size(); ++i) {
    templ_str += unigram_templs_[i];
    templ_str += '\0';
  }

  for (size_t i = 0; i < bigram_templs_.size(); ++i) {
    templ_str += bigram_templs_[i];
    templ_str += '\0';
  }

  while ((y_str.size() + templ_str.size()) % 4 != 0) {
    templ_str += '\0';
  }

  for (std::map<std::string, std::pair<int, unsigned int> >::iterator
           it = dic_.begin();
       it != dic_.end(); ++it) {
    key.push_back(const_cast<char *>(it->first.c_str()));
    val.push_back(it->second.first);
  }

  Darts::DoubleArray da;

  CHECK_FALSE(da.build(key.size(), &key[0], 0, &val[0]) == 0)
      << "cannot build double-array";

  std::ofstream bofs;
  bofs.open(WPATH(filename), OUTPUT_MODE);

  CHECK_FALSE(bofs) << "open failed: " << filename;

  unsigned int version_ = version;
  bofs.write(reinterpret_cast<char *>(&version_), sizeof(unsigned int));

  int type = 0;
  bofs.write(reinterpret_cast<char *>(&type), sizeof(type));
  bofs.write(reinterpret_cast<char *>(&cost_factor_), sizeof(cost_factor_));
  bofs.write(reinterpret_cast<char *>(&maxid_), sizeof(maxid_));

  if (max_xsize_ > 0) {
    xsize_ = std::min(xsize_, max_xsize_);
  }
  bofs.write(reinterpret_cast<char *>(&xsize_), sizeof(xsize_));
  unsigned int dsize = da.unit_size() * da.size();
  bofs.write(reinterpret_cast<char *>(&dsize), sizeof(dsize));
  unsigned int size = y_str.size();
  bofs.write(reinterpret_cast<char *>(&size),  sizeof(size));
  bofs.write(const_cast<char *>(y_str.data()), y_str.size());
  size = templ_str.size();
  bofs.write(reinterpret_cast<char *>(&size),  sizeof(size));
  bofs.write(const_cast<char *>(templ_str.data()), templ_str.size());
  bofs.write(reinterpret_cast<const char *>(da.array()), dsize);

  for (size_t i  = 0; i < maxid_; ++i) {
    float alpha = static_cast<float>(alpha_[i]);
    bofs.write(reinterpret_cast<char *>(&alpha), sizeof(alpha));
  }

  bofs.close();

  if (textmodelfile) {
    std::string filename2 = filename;
    filename2 += ".txt";

    std::ofstream tofs(WPATH(filename2.c_str()));

    CHECK_FALSE(tofs) << " no such file or directory: " << filename2;

    // header
    tofs << "version: "     << version_ << std::endl;
    tofs << "cost-factor: " << cost_factor_ << std::endl;
    tofs << "maxid: "       << maxid_ << std::endl;
    tofs << "xsize: "       << xsize_ << std::endl;

    tofs << std::endl;

    // y
    for (size_t i = 0; i < y_.size(); ++i) {
      tofs << y_[i] << std::endl;
    }

    tofs << std::endl;

    // template
    for (size_t i = 0; i < unigram_templs_.size(); ++i) {
      tofs << unigram_templs_[i] << std::endl;
    }

    for (size_t i = 0; i < bigram_templs_.size(); ++i) {
      tofs << bigram_templs_[i] << std::endl;
    }

    tofs << std::endl;

    // dic
    for (std::map<std::string, std::pair<int, unsigned int> >::iterator
             it = dic_.begin();
         it != dic_.end(); ++it) {
      tofs << it->second.first << " " << it->first << std::endl;
    }

    tofs << std::endl;

    tofs.setf(std::ios::fixed, std::ios::floatfield);
    tofs.precision(16);

    for (size_t i  = 0; i < maxid_; ++i) {
      tofs << alpha_[i] << std::endl;
    }
  }

  return true;
}

const char *FeatureIndex::getTemplate() const {
  return templs_.c_str();
}

void FeatureIndex::calcCost(Node *n) const {
  n->cost = 0.0;

#define ADD_COST(T, A)                                                  \
  do { T c = 0;                                                               \
    for (const int *f = n->fvector; *f != -1; ++f) { c += (A)[*f + n->y];  }  \
    n->cost =cost_factor_ *(T)c; } while (0)

  if (alpha_float_) {
    ADD_COST(float,  alpha_float_);
  } else {
    ADD_COST(double, alpha_);
  }
#undef ADD_COST
}

void FeatureIndex::calcCost(Path *p) const {
  p->cost = 0.0;

#define ADD_COST(T, A)                                          \
  { T c = 0.0;                                                  \
    for (const int *f = p->fvector; *f != -1; ++f) {            \
      c += (A)[*f + p->lnode->y * y_.size() + p->rnode->y];     \
    }                                                           \
    p->cost =cost_factor_*(T)c; }

  if (alpha_float_) {
    ADD_COST(float,  alpha_float_);
  } else {
    ADD_COST(double, alpha_);
  }
}
#undef ADD_COST
}
