//
//  CRF++ -- Yet Another CRF toolkit
//
//  $Id: tagger.h 1588 2007-02-12 09:03:39Z taku $;
//
//  Copyright(C) 2005-2007 Taku Kudo <taku@chasen.org>
//
#ifndef CRFPP_TAGGER_H_
#define CRFPP_TAGGER_H_

#include <iostream>
#include <vector>
#include <queue>
#include "param.h"
#include "crfpp.h"
#include "scoped_ptr.h"
#include "feature_index.h"

namespace CRFPP {

static inline double toprob(Node *n, double Z) {
  return std::exp(n->alpha + n->beta - n->cost - Z);
}

class Allocator;

class ModelImpl : public Model {
 public:
  ModelImpl() : nbest_(0), vlevel_(0) {}
  virtual ~ModelImpl() {}
  bool open(int argc,  char** argv);
  bool open(const char* arg);
  bool openFromArray(int argc,  char** argv,
                     const char *buf, size_t size);
  bool openFromArray(const char* arg,
                     const char *buf, size_t size);
  Tagger *createTagger() const;
  const char* what() { return what_.str(); }

  unsigned int nbest() const { return nbest_; }
  unsigned int vlevel() const { return vlevel_; }
  FeatureIndex *feature_index() const { return feature_index_.get(); }
  const char *getTemplate() const;

 private:
  bool open(const Param &param);
  bool openFromArray(const Param &param,
                     const char *buf, size_t size);

  whatlog       what_;
  unsigned int nbest_;
  unsigned int vlevel_;
  scoped_ptr<DecoderFeatureIndex> feature_index_;
};

class TaggerImpl : public Tagger {
 public:
  explicit TaggerImpl() : mode_(TEST), vlevel_(0), nbest_(0),
                          ysize_(0), Z_(0), feature_id_(0),
                          thread_id_(0), feature_index_(0),
                          allocator_(0) {}
  virtual ~TaggerImpl() { close(); }

  Allocator *allocator() const {
    return allocator_;
  }

  void   set_feature_id(size_t id) { feature_id_  = id; }
  size_t feature_id() const { return feature_id_; }
  void   set_thread_id(unsigned short id) { thread_id_ = id; }
  unsigned short thread_id() const { return thread_id_; }
  Node  *node(size_t i, size_t j) const { return node_[i][j]; }
  void   set_node(Node *n, size_t i, size_t j) { node_[i][j] = n; }

  // for LEARN mode
  bool         open(FeatureIndex *feature_index, Allocator *allocator);

  // for TEST mode, but feature_index is shared.
  bool         open(FeatureIndex *feature_index,
                    unsigned int nvest, unsigned velvel);

  // for TEST mode
  bool         open(const Param &param);
  bool         open(const char *argv);
  bool         open(int argc, char **argv);

  bool         set_model(const Model &model);


  int          eval();
  double       gradient(double *);
  double       collins(double *);
  bool         shrink();
  bool         parse_stream(std::istream *is, std::ostream *os);
  bool         read(std::istream *is);
  void         close();
  bool         add(size_t size, const char **line);
  bool         add(const char*);
  size_t       size() const { return x_.size(); }
  size_t       xsize() const { return feature_index_->xsize(); }
  size_t       dsize() const { return feature_index_->size(); }
  const float *weight_vector() const { return feature_index_->alpha_float(); }
  bool         empty() const { return x_.empty(); }
  size_t ysize() const { return ysize_; }
  double cost() const { return cost_; }
  double Z() const { return Z_; }
  double       prob() const { return std::exp(- cost_ - Z_); }
  double       prob(size_t i, size_t j) const {
    return toprob(node_[i][j], Z_);
  }
  double       prob(size_t i) const {
    return toprob(node_[i][result_[i]], Z_);
  }
  void set_penalty(size_t i, size_t j, double penalty);
  double penalty(size_t i, size_t j) const;
  double alpha(size_t i, size_t j) const { return node_[i][j]->alpha; }
  double beta(size_t i, size_t j) const { return node_[i][j]->beta; }
  double emission_cost(size_t i, size_t j) const { return node_[i][j]->cost; }
  double next_transition_cost(size_t i, size_t j, size_t k) const {
    return node_[i][j]->rpath[k]->cost;
  }
  double prev_transition_cost(size_t i, size_t j, size_t k) const {
    return node_[i][j]->lpath[k]->cost;
  }
  double best_cost(size_t i, size_t j) const {
    return node_[i][j]->bestCost;
  }
  const int *emission_vector(size_t i, size_t j) const {
    return const_cast<int *>(node_[i][j]->fvector);
  }
  const int* next_transition_vector(size_t i, size_t j, size_t k) const {
    return node_[i][j]->rpath[k]->fvector;
  }
  const int* prev_transition_vector(size_t i, size_t j, size_t k) const {
    return node_[i][j]->lpath[k]->fvector;
  }
  size_t answer(size_t i) const { return answer_[i]; }
  size_t result(size_t i) const { return result_[i]; }
  size_t y(size_t i)  const    { return result_[i]; }
  const char* yname(size_t i) const    { return feature_index_->y(i); }
  const char* y2(size_t i) const      { return yname(result_[i]); }
  const char*  x(size_t i, size_t j) const { return x_[i][j]; }
  const char** x(size_t i) const {
    return const_cast<const char **>(&x_[i][0]);
  }
  const char* toString();
  const char* toString(char *, size_t);
  const char* parse(const char*);
  const char* parse(const char*, size_t);
  const char* parse(const char*, size_t, char*, size_t);
  bool parse();
  bool clear();
  bool next();

  unsigned int vlevel() const { return vlevel_; }

  float cost_factor() const {
    return feature_index_ ? feature_index_->cost_factor() : 0.0;
  }

  size_t nbest() const { return nbest_; }

  void set_vlevel(unsigned int vlevel) {
    vlevel_ = vlevel;
  }

  void set_cost_factor(float cost_factor) {
    if (cost_factor > 0 && feature_index_) {
      feature_index_->set_cost_factor(cost_factor);
    }
  }

  void set_nbest(size_t nbest) {
    nbest_ = nbest;
  }

  const char* what() { return what_.str(); }

 private:
  void forwardbackward();
  void viterbi();
  void buildLattice();
  bool initNbest();
  bool add2(size_t, const char **, bool);

  struct QueueElement {
    Node *node;
    QueueElement *next;
    double fx;
    double gx;
  };

  class QueueElementComp {
   public:
    const bool operator()(QueueElement *q1,
                          QueueElement *q2)
    { return(q1->fx > q2->fx); }
  };

  enum { TEST, TEST_SHARED, LEARN };
  unsigned int    mode_ ;
  unsigned int    vlevel_;
  unsigned int    nbest_;
  size_t          ysize_;
  double          cost_;
  double          Z_;
  size_t          feature_id_;
  unsigned short  thread_id_;
  FeatureIndex   *feature_index_;
  Allocator      *allocator_;
  std::vector<std::vector <const char *> > x_;
  std::vector<std::vector <Node *> > node_;
  std::vector<std::vector<double> > penalty_;
  std::vector<unsigned short int>  answer_;
  std::vector<unsigned short int>  result_;
  whatlog       what_;
  string_buffer os_;

  scoped_ptr<std::priority_queue <QueueElement*, std::vector <QueueElement *>,
                                  QueueElementComp> > agenda_;
  scoped_ptr<FreeList <QueueElement> > nbest_freelist_;
};
}
#endif
