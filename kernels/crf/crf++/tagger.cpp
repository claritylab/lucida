//
//  CRF++ -- Yet Another CRF toolkit
//
//  $Id: tagger.cpp 1601 2007-03-31 09:47:18Z taku $;
//
//  Copyright(C) 2005-2007 Taku Kudo <taku@chasen.org>
//
#include <iostream>
#include <vector>
#include <iterator>
#include <cmath>
#include <string>
#include <sstream>
#include "stream_wrapper.h"
#include "common.h"
#include "tagger.h"

namespace {
const char kUnknownError[] = "Unknown Error";
const size_t kErrorBufferSize = 256;
}  // namespace

#if defined(_WIN32) && !defined(__CYGWIN__)
namespace {
DWORD g_tls_index = TLS_OUT_OF_INDEXES;

const char *getGlobalError() {
  LPVOID data = ::TlsGetValue(g_tls_index);
  return data == NULL ? kUnknownError : reinterpret_cast<const char *>(data);
}

void setGlobalError(const char *str) {
  char *data = reinterpret_cast<char *>(::TlsGetValue(g_tls_index));
  if (data == NULL) {
    return;
  }
  strncpy(data, str, kErrorBufferSize - 1);
  data[kErrorBufferSize - 1] = '\0';
}
}  // namespace
HINSTANCE DllInstance = 0;

extern "C" {
  BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID) {
    LPVOID data = 0;
    if (!DllInstance) {
      DllInstance = hinst;
    }
    switch (dwReason) {
      case DLL_PROCESS_ATTACH:
        if ((g_tls_index = ::TlsAlloc()) == TLS_OUT_OF_INDEXES) {
          return FALSE;
        }
        // Not break in order to initialize the TLS.
      case DLL_THREAD_ATTACH:
        data = (LPVOID)::LocalAlloc(LPTR, kErrorBufferSize);
        if (data) {
          ::TlsSetValue(g_tls_index, data);
        }
        break;
      case DLL_THREAD_DETACH:
        data = ::TlsGetValue(g_tls_index);
        if (data) {
          ::LocalFree((HLOCAL)data);
        }
        break;
      case DLL_PROCESS_DETACH:
        data = ::TlsGetValue(g_tls_index);
        if (data) {
          ::LocalFree((HLOCAL)data);
        }
        ::TlsFree(g_tls_index);
        g_tls_index = TLS_OUT_OF_INDEXES;
        break;
      default:
        break;
    }
    return TRUE;
  }
}
#else  // _WIN32

namespace {
#ifdef HAVE_TLS_KEYWORD
__thread char kErrorBuffer[kErrorBufferSize];
#else
char kErrorBuffer[kErrorBufferSize];
#endif
}

namespace {
const char *getGlobalError() {
  return kErrorBuffer;
}

void setGlobalError(const char *str) {
  strncpy(kErrorBuffer, str, kErrorBufferSize - 1);
  kErrorBuffer[kErrorBufferSize - 1] = '\0';
}
}  // namespace
#endif

namespace {
static const CRFPP::Option long_options[] = {
  {"model",  'm',  0,       "FILE",  "set FILE for model file"},
  {"nbest",  'n', "0",      "INT",   "output n-best results"},
  {"verbose" , 'v', "0",    "INT",   "set INT for verbose level"},
  {"cost-factor", 'c', "1.0", "FLOAT", "set cost factor"},
  {"output",         'o',  0,       "FILE",  "use FILE as output file"},
  {"version",        'v',  0,        0,       "show the version and exit" },
  {"help",   'h',  0,        0,       "show this help and exit" },
  {0, 0, 0, 0, 0}
};
}  // namespace

namespace CRFPP {

Tagger *ModelImpl::createTagger() const {
  if (!feature_index_.get()) {
    return 0;
  }
  TaggerImpl *tagger = new TaggerImpl;
  tagger->open(feature_index_.get(), nbest_, vlevel_);
  return tagger;
}

bool TaggerImpl::open(FeatureIndex *feature_index,
                      Allocator *allocator) {
  close();
  mode_ = LEARN;
  feature_index_ = feature_index;
  allocator_ = allocator;
  ysize_ = feature_index_->ysize();
  return true;
}

bool TaggerImpl::open(FeatureIndex *feature_index,
                      unsigned int nbest,
                      unsigned int vlevel) {
  close();
  mode_ = TEST_SHARED;
  feature_index_ = feature_index;
  nbest_ = nbest;
  vlevel_ = vlevel;
  allocator_ = new Allocator;
  ysize_ = feature_index_->ysize();
  return true;
}

bool ModelImpl::openFromArray(const Param &param,
                              const char *buf,
                              size_t size) {
  nbest_ = param.get<int>("nbest");
  vlevel_ = param.get<int>("verbose");
  feature_index_.reset(new DecoderFeatureIndex);
  if (!feature_index_->openFromArray(buf, size)) {
    WHAT << feature_index_->what();
    feature_index_.reset(0);
    return false;
  }
  const double c = param.get<double>("cost-factor");
  feature_index_->set_cost_factor(c);
  return true;
}

bool ModelImpl::open(const Param &param) {
  nbest_ = param.get<int>("nbest");
  vlevel_ = param.get<int>("verbose");
  const std::string model = param.get<std::string>("model");
  feature_index_.reset(new DecoderFeatureIndex);
  if (!feature_index_->open(model.c_str())) {
    WHAT << feature_index_->what();
    feature_index_.reset(0);
    return false;
  }
  const double c = param.get<double>("cost-factor");
  feature_index_->set_cost_factor(c);
  return true;
}

bool ModelImpl::open(int argc,  char** argv) {
  Param param;
  CHECK_FALSE(param.open(argc, argv, long_options))
      << param.what();
  return open(param);
}

bool ModelImpl::open(const char* arg) {
  Param param;
  CHECK_FALSE(param.open(arg, long_options)) << param.what();
  return open(param);
}

bool ModelImpl::openFromArray(int argc,  char** argv,
                              const char *buf, size_t size) {
  Param param;
  CHECK_FALSE(param.open(argc, argv, long_options))
      << param.what();
  return openFromArray(param, buf, size);
}

bool ModelImpl::openFromArray(const char* arg,
                              const char *buf, size_t size) {
  Param param;
  CHECK_FALSE(param.open(arg, long_options)) << param.what();
  return openFromArray(param, buf, size);
}

const char *ModelImpl::getTemplate() const {
  if (feature_index_.get()) {
    return feature_index_->getTemplate();
  }
  return 0;
}

bool TaggerImpl::open(const Param &param) {
  close();

  if (!param.help_version()) {
    close();
    return false;
  }

  nbest_ = param.get<int>("nbest");
  vlevel_ = param.get<int>("verbose");

  std::string model = param.get<std::string>("model");

  DecoderFeatureIndex *decoder_feature_index = new DecoderFeatureIndex;
  feature_index_ = decoder_feature_index;
  allocator_ = new Allocator;

  if (!decoder_feature_index->open(model.c_str())) {
    WHAT << feature_index_->what();
    close();
    return false;
  }

  const double c = param.get<double>("cost-factor");

  if (c <= 0.0) {
    WHAT << "cost factor must be positive";
    close();
    return false;
  }

  feature_index_->set_cost_factor(c);
  ysize_ = feature_index_->ysize();

  return true;
}

bool TaggerImpl::open(int argc, char **argv) {
  Param param;
  CHECK_FALSE(param.open(argc, argv, long_options))
      << param.what();
  return open(param);
}

bool TaggerImpl::open(const char *arg) {
  Param param;
  CHECK_FALSE(param.open(arg, long_options)) << param.what();
  return open(param);
}

void TaggerImpl::close() {
  if (mode_ == TEST) {
    delete feature_index_;
    delete allocator_;
    feature_index_ = 0;
    allocator_ = 0;
  } else if (mode_ == TEST_SHARED) {
    delete allocator_;
    allocator_ = 0;
  }
}

bool TaggerImpl::set_model(const Model &model) {
  if (mode_ == TEST) {
    // feature_index_ => took the owner
    // allocator_ => reuse
    delete feature_index_;
  } else if (mode_ == LEARN) {
    // feature_index_ => did not take the owner
    // allocator_ => did not take the owner.
    allocator_ = new Allocator;
  } else if (mode_ == TEST_SHARED) {
    // feature_index_ => did not take the owner
    // allocator_ => reuse
  }
  mode_ = TEST_SHARED;
  const ModelImpl *model_impl = static_cast<const ModelImpl *>(&model);
  feature_index_ = model_impl->feature_index();
  nbest_ = model_impl->nbest();
  vlevel_ = model_impl->vlevel();
  ysize_ = feature_index_->ysize();
  return true;
}

bool TaggerImpl::add2(size_t size, const char **column, bool copy) {
  const size_t xsize = feature_index_->xsize();

  if ((mode_ == LEARN && size < xsize + 1) ||
      ((mode_ == TEST || mode_ == TEST_SHARED)  && size < xsize)) {
    CHECK_FALSE(false) << "# x is small: size="
                       << size << " xsize=" << xsize;
  }

  size_t s = x_.size() + 1;
  x_.resize(s);
  node_.resize(s);
  answer_.resize(s);
  result_.resize(s);
  s = x_.size() - 1;

  if (copy) {
    for (size_t k = 0; k < size; ++k) {
      x_[s].push_back(allocator_->strdup(column[k]));
    }
  } else {
    for (size_t k = 0; k < size; ++k) {
      x_[s].push_back(column[k]);
    }
  }

  result_[s] = answer_[s] = 0;  // dummy
  if (mode_ == LEARN) {
    size_t r = ysize_;
    for (size_t k = 0; k < ysize_; ++k) {
      if (std::strcmp(yname(k), column[xsize]) == 0) {
        r = k;
      }
    }

    CHECK_FALSE(r != ysize_) << "cannot find answer: " << column[xsize];
    answer_[s] = r;
  }

  node_[s].resize(ysize_);

  return true;
}

bool TaggerImpl::add(size_t size, const char **column) {
  return add2(size, column, true);
}

bool TaggerImpl::add(const char* line) {
  char *p = allocator_->strdup(line);
  scoped_fixed_array<const char *, 8192> column;
  const size_t size = tokenize2(p, "\t ", column.get(), column.size());
  if (!add2(size, column.get(), false)) {
    return false;
  }
  return true;
}

bool TaggerImpl::read(std::istream *is) {
  scoped_fixed_array<char, 8192> line;
  clear();

  for (;;) {
    if (!is->getline(line.get(), line.size())) {
      is->clear(std::ios::eofbit|std::ios::badbit);
      return true;
    }
    if (line[0] == '\0' || line[0] == ' ' || line[0] == '\t') {
      break;
    }
    if (!add(line.get())) {
      return false;
    }
  }

  return true;
}

void TaggerImpl::set_penalty(size_t i, size_t j, double penalty) {
  if (penalty_.empty()) {
    penalty_.resize(node_.size());
    for (size_t s = 0; s < penalty_.size(); ++s) {
      penalty_[s].resize(ysize_);
    }
  }
  penalty_[i][j] = penalty;
}

double TaggerImpl::penalty(size_t i, size_t j) const {
  return penalty_.empty() ? 0.0 : penalty_[i][j];
}

bool TaggerImpl::shrink() {
  CHECK_FALSE(feature_index_->buildFeatures(this))
      << feature_index_->what();
  std::vector<std::vector<const char *> >(x_).swap(x_);
  std::vector<std::vector<Node *> >(node_).swap(node_);
  std::vector<unsigned short int>(answer_).swap(answer_);
  std::vector<unsigned short int>(result_).swap(result_);

  return true;
}

bool TaggerImpl::initNbest() {
  if (!agenda_.get()) {
    agenda_.reset(new std::priority_queue <QueueElement*,
                  std::vector<QueueElement *>, QueueElementComp>);
    nbest_freelist_.reset(new FreeList <QueueElement>(128));
  }

  nbest_freelist_->free();
  while (!agenda_->empty()) {
    agenda_->pop();   // make empty
  }

  const size_t k = x_.size()-1;
  for (size_t i = 0; i < ysize_; ++i) {
    QueueElement *eos = nbest_freelist_->alloc();
    eos->node = node_[k][i];
    eos->fx = -node_[k][i]->bestCost;
    eos->gx = -node_[k][i]->cost;
    eos->next = 0;
    agenda_->push(eos);
  }

  return true;
}

bool TaggerImpl::next() {
  while (!agenda_->empty()) {
    QueueElement *top = agenda_->top();
    agenda_->pop();
    Node *rnode = top->node;

    if (rnode->x == 0) {
      for (QueueElement *n = top; n; n = n->next) {
        result_[n->node->x] = n->node->y;
      }
      cost_ = top->gx;
      return true;
    }

    for (const_Path_iterator it = rnode->lpath.begin();
         it != rnode->lpath.end(); ++it) {
      QueueElement *n =nbest_freelist_->alloc();
      n->node = (*it)->lnode;
      n->gx   = -(*it)->lnode->cost     -(*it)->cost +  top->gx;
      n->fx   = -(*it)->lnode->bestCost -(*it)->cost +  top->gx;
      //          |              h(x)                 |  |  g(x)  |
      n->next = top;
      agenda_->push(n);
    }
  }

  return 0;
}

int TaggerImpl::eval() {
  int err = 0;
  for (size_t i = 0; i < x_.size(); ++i) {
    if (answer_[i] != result_[i]) {
      ++err;
    }
  }
  return err;
}

bool TaggerImpl::clear() {
  if (mode_ == TEST || mode_ == TEST_SHARED) {
    allocator_->clear();
  }
  x_.clear();
  node_.clear();
  answer_.clear();
  result_.clear();
  Z_ = cost_ = 0.0;
  return true;
}

void TaggerImpl::buildLattice() {
  if (x_.empty()) {
    return;
  }

  feature_index_->rebuildFeatures(this);

  for (size_t i = 0; i < x_.size(); ++i) {
    for (size_t j = 0; j < ysize_; ++j) {
      feature_index_->calcCost(node_[i][j]);
      const std::vector<Path *> &lpath = node_[i][j]->lpath;
      for (const_Path_iterator it = lpath.begin(); it != lpath.end(); ++it) {
        feature_index_->calcCost(*it);
      }
    }
  }

  // Add penalty for Dual decomposition.
  if (!penalty_.empty()) {
    for (size_t i = 0; i < x_.size(); ++i) {
      for (size_t j = 0; j < ysize_; ++j) {
        node_[i][j]->cost += penalty_[i][j];
      }
    }
  }
}

void TaggerImpl::forwardbackward() {
  if (x_.empty()) {
    return;
  }

  for (int i = 0; i < static_cast<int>(x_.size()); ++i) {
    for (size_t j = 0; j < ysize_; ++j) {
      node_[i][j]->calcAlpha();
    }
  }

  for (int i = static_cast<int>(x_.size() - 1); i >= 0;  --i) {
    for (size_t j = 0; j < ysize_; ++j) {
      node_[i][j]->calcBeta();
    }
  }

  Z_ = 0.0;
  for (size_t j = 0; j < ysize_; ++j) {
    Z_ = logsumexp(Z_, node_[0][j]->beta, j == 0);
  }

  return;
}

void TaggerImpl::viterbi() {
  for (size_t i = 0;   i < x_.size(); ++i) {
    for (size_t j = 0; j < ysize_; ++j) {
      double bestc = -1e37;
      Node *best = 0;
      const std::vector<Path *> &lpath = node_[i][j]->lpath;
      for (const_Path_iterator it = lpath.begin(); it != lpath.end(); ++it) {
        double cost = (*it)->lnode->bestCost +(*it)->cost +
            node_[i][j]->cost;
        if (cost > bestc) {
          bestc = cost;
          best  = (*it)->lnode;
        }
      }
      node_[i][j]->prev     = best;
      node_[i][j]->bestCost = best ? bestc : node_[i][j]->cost;
    }
  }

  double bestc = -1e37;
  Node *best = 0;
  size_t s = x_.size()-1;
  for (size_t j = 0; j < ysize_; ++j) {
    if (bestc < node_[s][j]->bestCost) {
      best  = node_[s][j];
      bestc = node_[s][j]->bestCost;
    }
  }

  for (Node *n = best; n; n = n->prev) {
    result_[n->x] = n->y;
  }

  cost_ = -node_[x_.size()-1][result_[x_.size()-1]]->bestCost;
}

double TaggerImpl::gradient(double *expected) {
  if (x_.empty()) return 0.0;

  buildLattice();
  forwardbackward();
  double s = 0.0;

  for (size_t i = 0;   i < x_.size(); ++i) {
    for (size_t j = 0; j < ysize_; ++j) {
      node_[i][j]->calcExpectation(expected, Z_, ysize_);
    }
  }

  for (size_t i = 0;   i < x_.size(); ++i) {
    for (const int *f = node_[i][answer_[i]]->fvector; *f != -1; ++f) {
      --expected[*f + answer_[i]];
    }
    s += node_[i][answer_[i]]->cost;  // UNIGRAM cost
    const std::vector<Path *> &lpath = node_[i][answer_[i]]->lpath;
    for (const_Path_iterator it = lpath.begin(); it != lpath.end(); ++it) {
      if ((*it)->lnode->y == answer_[(*it)->lnode->x]) {
        for (const int *f = (*it)->fvector; *f != -1; ++f) {
          --expected[*f +(*it)->lnode->y * ysize_ +(*it)->rnode->y];
        }
        s += (*it)->cost;  // BIGRAM COST
        break;
      }
    }
  }

  viterbi();  // call for eval()

  return Z_ - s ;
}

double TaggerImpl::collins(double *collins) {
  if (x_.empty()) {
    return 0.0;
  }

  buildLattice();
  viterbi();  // call for finding argmax y*
  double s = 0.0;

  // if correct parse, do not run forward + backward
  {
    size_t num = 0;
    for (size_t i = 0; i < x_.size(); ++i) {
      if (answer_[i] == result_[i]) {
        ++num;
      }
    }

    if (num == x_.size()) return 0.0;
  }

  for (size_t i = 0; i < x_.size(); ++i) {
    // answer
    {
      s += node_[i][answer_[i]]->cost;
      for (const int *f = node_[i][answer_[i]]->fvector; *f != -1; ++f) {
        ++collins[*f + answer_[i]];
      }

      const std::vector<Path *> &lpath = node_[i][answer_[i]]->lpath;
      for (const_Path_iterator it = lpath.begin(); it != lpath.end(); ++it) {
        if ((*it)->lnode->y == answer_[(*it)->lnode->x]) {
          for (const int *f = (*it)->fvector; *f != -1; ++f) {
            ++collins[*f +(*it)->lnode->y * ysize_ +(*it)->rnode->y];
          }
          s += (*it)->cost;
          break;
        }
      }
    }

    // result
    {
      s -= node_[i][result_[i]]->cost;
      for (const int *f = node_[i][result_[i]]->fvector; *f != -1; ++f) {
        --collins[*f + result_[i]];
      }

      const std::vector<Path *> &lpath = node_[i][result_[i]]->lpath;
      for (const_Path_iterator it = lpath.begin(); it != lpath.end(); ++it) {
        if ((*it)->lnode->y == result_[(*it)->lnode->x]) {
          for (const int *f = (*it)->fvector; *f != -1; ++f) {
            --collins[*f +(*it)->lnode->y * ysize_ +(*it)->rnode->y];
          }
          s -= (*it)->cost;
          break;
        }
      }
    }
  }

  return -s;
}

bool TaggerImpl::parse() {
  CHECK_FALSE(feature_index_->buildFeatures(this))
      << feature_index_->what();

  if (x_.empty()) {
    return true;
  }
  buildLattice();
  if (nbest_ || vlevel_ >= 1) {
    forwardbackward();
  }
  viterbi();
  if (nbest_) {
    initNbest();
  }

  return true;
}

const char* TaggerImpl::parse(const char* input) {
  return parse(input, std::strlen(input));
}

const char* TaggerImpl::parse(const char* input, size_t length) {
  std::istringstream is(std::string(input, length));
  if (!read(&is) || !parse()) {
    return 0;
  }
  toString();
  return os_.c_str();
}

const char* TaggerImpl::parse(const char*input, size_t len1,
                              char *output, size_t len2) {
  std::istringstream is(std::string(input, len1));
  if (x_.empty()) {
    return 0;
  }
  toString();
  if ((os_.size() + 1) < len2) {
    memcpy(output, os_.data(), os_.size());
    output[os_.size()] = '\0';
    return output;
  } else {
    return 0;
  }
}

bool TaggerImpl::parse_stream(std::istream *is,
                              std::ostream *os) {
  if (!read(is) || !parse()) {
    return false;
  }
  if (x_.empty()) {
    return true;
  }
  toString();
  os->write(os_.data(), os_.size());
  return true;
}

const char* TaggerImpl::toString(char *output,
                                 size_t len) {
  const char* p = toString();
  const size_t l = std::min(std::strlen(p), len);
  std::strncpy(output, p, l);
  return output;
}

const char* TaggerImpl::toString() {
  os_.assign("");

#define PRINT                                                   \
  for (size_t i = 0; i < x_.size(); ++i) {                      \
    for (std::vector<const char*>::iterator it = x_[i].begin(); \
         it != x_[i].end(); ++it)                               \
      os_ << *it << '\t';                                       \
    os_ << yname(y(i));                                         \
    if (vlevel_ >= 1) os_ << '/' << prob(i);                    \
    if (vlevel_ >= 2) {                                         \
      for (size_t j = 0; j < ysize_; ++j)                       \
        os_ << '\t' << yname(j) << '/' << prob(i, j);           \
    }                                                           \
    os_ << '\n';                                                \
  }                                                             \
  os_ << '\n';

  if (nbest_ >= 1) {
    for (size_t n = 0; n < nbest_; ++n) {
      if (!next()) {
        break;
      }
      os_ << "# " << n << " " << prob() << '\n';
      PRINT;
    }
  } else {
    if (vlevel_ >= 1) {
      os_ << "# " << prob() << '\n';
    }
    PRINT;
  }

  return const_cast<const char*>(os_.c_str());

#undef PRINT
}

Tagger *createTagger(int argc, char **argv) {
  TaggerImpl *tagger = new TaggerImpl();
  if (!tagger->open(argc, argv)) {
    setGlobalError(tagger->what());
    delete tagger;
    return 0;
  }
  return tagger;
}

Tagger *createTagger(const char *argv) {
  TaggerImpl *tagger = new TaggerImpl();
  if (!tagger->open(argv)) {
    setGlobalError(tagger->what());
    delete tagger;
    return 0;
  }
  return tagger;
}

Model *createModel(int argc, char **argv) {
  ModelImpl *model = new ModelImpl();
  if (!model->open(argc, argv)) {
    setGlobalError(model->what());
    delete model;
    return 0;
  }
  return model;
}

Model *createModelFromArray(int argc, char **argv,
                            const char *buf, size_t size) {
  ModelImpl *model = new ModelImpl();
  if (!model->openFromArray(argc, argv, buf, size)) {
    setGlobalError(model->what());
    delete model;
    return 0;
  }
  return model;
}

Model *createModel(const char *argv) {
  ModelImpl *model = new ModelImpl();
  if (!model->open(argv)) {
    setGlobalError(model->what());
    delete model;
    return 0;
  }
  return model;
}

Model *createModelFromArray(const char *arg,
                           const char *buf, size_t size) {
  ModelImpl *model = new ModelImpl();
  if (!model->openFromArray(arg, buf, size)) {
    setGlobalError(model->what());
    delete model;
    return 0;
  }
  return model;
}

const char *getTaggerError() {
  return getGlobalError();
}

const char *getLastError() {
  return getGlobalError();
}

namespace {
int crfpp_test(const Param &param) {
  if (param.get<bool>("version")) {
    std::cout <<  param.version();
    return -1;
  }

  if (param.get<bool>("help")) {
    std::cout <<  param.help();
    return -1;
  }

  CRFPP::TaggerImpl tagger;
  if (!tagger.open(param)) {
    std::cerr << tagger.what() << std::endl;
    return -1;
  }

  std::string output = param.get<std::string>("output");
  if (output.empty()) {
    output = "-";
  }

  CRFPP::ostream_wrapper os(output.c_str());
  if (!*os) {
    std::cerr << "no such file or directory: " << output << std::endl;
    return -1;
  }

  const std::vector<std::string>& rest_ = param.rest_args();
  std::vector<std::string> rest = rest_;  // trivial copy
  if (rest.empty()) {
    rest.push_back("-");
  }

  for (size_t i = 0; i < rest.size(); ++i) {
    CRFPP::istream_wrapper is(rest[i].c_str());
    if (!*is) {
      std::cerr << "no such file or directory: " << rest[i] << std::endl;
      return -1;
    }
    while (*is) {
      tagger.parse_stream(is.get(), os.get());
    }
  }

  return 0;
}
}  // namepace
}  // namespace CRFPP

int crfpp_test(int argc, char **argv) {
  CRFPP::Param param;
  param.open(argc, argv, long_options);
  return CRFPP::crfpp_test(param);
}

int crfpp_test2(const char *arg) {
  CRFPP::Param param;
  param.open(arg, long_options);
  return CRFPP::crfpp_test(param);
}
