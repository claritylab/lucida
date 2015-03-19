/*
 * $Id$
 */

#include "crf.h"
#include <cmath>
#include <cstdio>
#include <cfloat>
#include <set>

#include "../../utils/memoryman.h"

using namespace std;

extern bool USE_EDGE_TRIGRAMS;

const string BOS_LABEL = "!BOS!";
const string EOS_LABEL = "!EOS!";
const bool USE_BOS_EOS = false;

const bool OUTPUT_MARGINAL_PROB = true;

extern int push_stop_watch();

static CRF_Model* pointer_to_working_object =
    NULL;  // this is not a good solution...

CRF_Model::CRF_Model() {
  _nheldout = 0;
  _early_stopping_n = 0;
  _line_counter = 0;

  if (USE_EDGE_TRIGRAMS) {
    p_edge_feature_id3 =
        (int*)sirius_malloc(sizeof(int) * MAX_LABEL_TYPES * MAX_LABEL_TYPES *
                            MAX_LABEL_TYPES * MAX_LABEL_TYPES);
    p_edge_weight3 = (double*)sirius_malloc(sizeof(double) * MAX_LABEL_TYPES *
                                            MAX_LABEL_TYPES * MAX_LABEL_TYPES *
                                            MAX_LABEL_TYPES);
  }
  p_edge_feature_id2 = (int*)sirius_malloc(sizeof(int) * MAX_LABEL_TYPES *
                                           MAX_LABEL_TYPES * MAX_LABEL_TYPES);
  p_edge_feature_id =
      (int*)sirius_malloc(sizeof(int) * MAX_LABEL_TYPES * MAX_LABEL_TYPES);
  p_state_weight =
      (double*)sirius_malloc(sizeof(double) * MAX_LEN * MAX_LABEL_TYPES);
  p_edge_weight = (double*)sirius_malloc(sizeof(double) * MAX_LABEL_TYPES *
                                         MAX_LABEL_TYPES);
  p_edge_weight2 = (double*)sirius_malloc(sizeof(double) * MAX_LABEL_TYPES *
                                          MAX_LABEL_TYPES * MAX_LABEL_TYPES);
  p_forward_cache =
      (double*)sirius_malloc(sizeof(double) * MAX_LEN * MAX_LABEL_TYPES);
  p_backward_cache =
      (double*)sirius_malloc(sizeof(double) * MAX_LEN * MAX_LABEL_TYPES);
  p_backward_pointer =
      (int*)sirius_malloc(sizeof(int) * MAX_LEN * MAX_LABEL_TYPES);
}

CRF_Model::~CRF_Model() {
  if (USE_EDGE_TRIGRAMS) {
    sirius_free(p_edge_feature_id3);
    sirius_free(p_edge_weight3);
  }
  sirius_free(p_edge_feature_id2);
  sirius_free(p_edge_feature_id);
  sirius_free(p_state_weight);
  sirius_free(p_edge_weight2);
  sirius_free(p_edge_weight);
  sirius_free(p_forward_cache);
  sirius_free(p_backward_cache);
  sirius_free(p_backward_pointer);
}

double CRF_Model::FunctionGradient(const vector<double>& x,
                                   vector<double>& grad) {
  assert(_fb.Size() == x.size());
  for (size_t i = 0; i < x.size(); i++) {
    _vl[i] = x[i];
    if (_vl[i] < -50) _vl[i] = -50;
    if (_vl[i] > 50) _vl[i] = 50;
  }

  double score = update_model_expectation();

  if (_sigma == 0) {
    for (size_t i = 0; i < x.size(); i++) {
      grad[i] = -(_vee[i] - _vme[i]);
    }
  } else {
    const double c = 1 / (_sigma * _sigma);
    for (size_t i = 0; i < x.size(); i++) {
      grad[i] = -(_vee[i] - _vme[i] - c * _vl[i]);
    }
  }

  return -score;
}

double CRF_Model::FunctionGradientWrapper(const vector<double>& x,
                                          vector<double>& grad) {
  return pointer_to_working_object->FunctionGradient(x, grad);
}

int CRF_Model::perform_BFGS() { return 0; }

double CRF_Model::forward_prob(const int len) {
  for (int x = 0; x < len; x++) {
    double total = 0;
    for (int i = 0; i < _num_classes; i++) {
      double sum;
      if (x == 0) {
        sum = edge_weight(_num_classes, i);  // BOS
      } else {
        sum = 0;
        for (int j = 0; j < _num_classes; j++) {
          sum += edge_weight(j, i) * forward_cache(x - 1, j);
        }
      }
      sum *= state_weight(x, i);
      forward_cache(x, i) = sum;
      total += sum;
    }
    for (int i = 0; i < _num_classes; i++) {
      forward_cache(x, i) /= total;
      state_weight(x, i) /= total;
    }
  }
  double total = 0;
  for (int i = 0; i < _num_classes; i++) {
    total +=
        forward_cache(len - 1, i) * edge_weight(i, _num_classes + 1);  // EOS
  }

  return total;
}

double CRF_Model::backward_prob(const int len) {
  for (int x = len - 1; x >= 0; x--) {
    for (int i = 0; i < _num_classes; i++) {
      double sum;
      if (x == len - 1) {
        sum = edge_weight(i, _num_classes + 1);  // EOS
      } else {
        sum = 0;
        for (int j = 0; j < _num_classes; j++) {
          sum += edge_weight(i, j) * backward_cache(x + 1, j);
        }
      }
      sum *= state_weight(x, i);
      backward_cache(x, i) = sum;
    }
  }
  double total = 0;
  for (int i = 0; i < _num_classes; i++) {
    total += backward_cache(0, i) * edge_weight(_num_classes, i);  // BOS
  }

  return total;
}

void CRF_Model::initialize_edge_weights() {
  for (int i = 0; i < _label_bag.Size(); i++) {
    for (int j = 0; j < _label_bag.Size(); j++) {
      const int id = edge_feature_id(i, j);
      assert(id >= 0);
      const double ew = _vl[id];
      edge_weight(i, j) = exp(ew);
    }
  }
}

void CRF_Model::initialize_state_weights(const Sequence& seq) {
  vector<double> powv(_num_classes);
  for (size_t i = 0; i < seq.vs.size(); i++) {
    powv.assign(_num_classes, 0.0);
    const Sample& s = seq.vs[i];
    for (vector<int>::const_iterator j = s.positive_features.begin();
         j != s.positive_features.end(); ++j) {
      for (vector<int>::const_iterator k = _feature2mef[*j].begin();
           k != _feature2mef[*j].end(); ++k) {
        const double w = _vl[*k];
        powv[_fb.Feature(*k).label()] += w;
      }
    }

    for (int j = 0; j < _num_classes; j++) {
      state_weight(i, j) = exp(powv[j]);
    }
  }
}

double CRF_Model::forward_backward(const Sequence& seq) {
  initialize_state_weights(seq);

  const double fp = forward_prob(seq.vs.size());
  const double bp = backward_prob(seq.vs.size());
  assert(abs(fp - 1) < 0.01);
  assert(abs(bp - 1) < 0.01);

  if (!(fp > 0 && fp < DBL_MAX && bp > 0 && bp < DBL_MAX)) {
    cerr
        << endl << "error: line:" << _line_counter
        << " floating overflow. a different value of Gaussian prior might work."
        << endl;
    return 1.f;
  }

  assert(fp > 0 && fp < DBL_MAX);
  assert(bp > 0 && bp < DBL_MAX);

  return fp;
}

double CRF_Model::viterbi(const Sequence& seq, vector<int>& best_seq) {
  initialize_state_weights(seq);

  const int len = seq.vs.size();

  for (int x = 0; x < len; x++) {
    double total = 0;
    for (int i = 0; i < _num_classes; i++) {
      double m = -DBL_MAX;
      if (x == 0) {
        m = edge_weight(_num_classes, i);  // BOS
      } else {
        for (int j = 0; j < _num_classes; j++) {
          double score = edge_weight(j, i) * forward_cache(x - 1, j);
          if (score > m) {
            m = score;
            backward_pointer(x, i) = j;
          }
        }
      }
      m *= state_weight(x, i);
      forward_cache(x, i) = m;
      total += m;
    }
    for (int i = 0; i < _num_classes; i++) {
      forward_cache(x, i) /= total;
    }
  }

  double m = -DBL_MAX;
  for (int i = 0; i < _num_classes; i++) {
    double score =
        forward_cache(len - 1, i) * edge_weight(i, _num_classes + 1);  // EOS
    if (score > m) {
      m = score;
      best_seq[len - 1] = i;
    }
  }
  for (int x = len - 2; x >= 0; x--) {
    best_seq[x] = backward_pointer(x + 1, best_seq[x + 1]);
  }

  return 0;
}

void CRF_Model::nbest_search(const double lb, const int len, const int x,
                             const int y, const double rhs_score,
                             vector<Path>& vp) {
  if (x < len && forward_cache(x, y) * rhs_score < lb) return;

  nbest_search_path[x] = y;

  // root node
  if (x == len) {
    for (int i = 0; i < _num_classes; i++) {
      nbest_search(lb, len, x - 1, i, rhs_score, vp);
    }
    return;
  }

  const double sw = state_weight(x, y);

  // leaf nodes
  if (x == 0) {
    const double path_score = rhs_score * sw;
    Path p(path_score,
           vector<int>(&(nbest_search_path[0]), &(nbest_search_path[len])));
    vp.push_back(p);
    return;
  }

  for (int i = 0; i < _num_classes; i++) {
    const double ew = edge_weight(i, y);
    nbest_search(lb, len, x - 1, i, rhs_score * sw * ew, vp);
  }
}

double CRF_Model::nbest(const Sequence& seq, vector<Path>& vp,
                        const int max_num, const double min_prob) {
  initialize_state_weights(seq);
  const double fp = forward_prob(seq.vs.size());
  assert(abs(fp - 1) < 0.01);

  const int len = seq.vs.size();

  for (int x = 0; x < len; x++) {
    for (int i = 0; i < _num_classes; i++) {
      double m = -DBL_MAX;
      if (x == 0) {
        m = edge_weight(_num_classes, i);  // BOS
      } else {
        for (int j = 0; j < _num_classes; j++) {
          double score = edge_weight(j, i) * forward_cache(x - 1, j);
          if (score > m) {
            m = score;
            backward_pointer(x, i) = j;
          }
        }
      }
      m *= state_weight(x, i);
      forward_cache(x, i) = m;
    }
  }

  double m = -DBL_MAX;
  for (int i = 0; i < _num_classes; i++) {
    double score =
        forward_cache(len - 1, i) * edge_weight(i, _num_classes + 1);  // EOS
    if (score > m) {
      m = score;
    }
  }

  // n-best
  int iter = 0;
  for (double lb = 0.3 * m;; lb *= 0.3) {
    vp.clear();
    nbest_search(max(lb, min_prob), len, len, 0, 1.0, vp);
    if (iter++ > 1000) break;
    if ((int)vp.size() >= max_num) break;
    if (lb <= min_prob) break;
  }
  sort(vp.begin(), vp.end());

  return vp.size();
}

double CRF_Model::calc_loglikelihood(const Sequence& seq) {
  const int len = seq.vs.size();

  double logp = 0;
  for (int i = 0; i < len; i++) {
    logp += log(state_weight(i, seq.vs[i].label));
  }
  for (int i = 0; i < len - 1; i++) {
    logp += log(edge_weight(seq.vs[i].label, seq.vs[i + 1].label));
  }
  logp += log(edge_weight(_num_classes, seq.vs[0].label));            // BOS
  logp += log(edge_weight(seq.vs[len - 1].label, _num_classes + 1));  // EOS

  return logp;
}

int CRF_Model::make_feature_bag(const int cutoff) {
  typedef std::map<mefeature_type, int> map_type;

  map_type count;
  if (cutoff > 0) {
    for (vector<Sequence>::const_iterator k = _vs.begin(); k != _vs.end();
         ++k) {
      for (vector<Sample>::const_iterator i = k->vs.begin(); i != k->vs.end();
           ++i) {
        for (vector<int>::const_iterator j = i->positive_features.begin();
             j != i->positive_features.end(); ++j) {
          count[ME_Feature(i->label, *j).body()]++;
        }
      }
    }
  }

  for (vector<Sequence>::const_iterator k = _vs.begin(); k != _vs.end(); ++k) {
    for (vector<Sample>::const_iterator i = k->vs.begin(); i != k->vs.end();
         ++i) {
      for (vector<int>::const_iterator j = i->positive_features.begin();
           j != i->positive_features.end(); ++j) {
        const ME_Feature feature(i->label, *j);
        if (cutoff > 0 && count[feature.body()] <= cutoff) continue;
        _fb.Put(feature);
      }
    }
  }

  init_feature2mef();

  return 0;
}

double CRF_Model::heldout_likelihood() {
  double logl = 0;
  int ncorrect = 0, total_len = 0;

  initialize_edge_weights();
  for (std::vector<Sequence>::const_iterator i = _heldout.begin();
       i != _heldout.end(); ++i) {
    total_len += i->vs.size();
    double fp = forward_backward(*i);
    logl += calc_loglikelihood(*i);

    for (size_t j = 0; j < i->vs.size(); j++) {
      const Sample& s = i->vs[j];
      vector<double> wsum = calc_state_weight(j);
      if (s.label == max_element(wsum.begin(), wsum.end()) - wsum.begin())
        ncorrect++;
    }
  }
  _heldout_error = 1 - (double)ncorrect / total_len;

  return logl /= _heldout.size();
}

vector<double> CRF_Model::calc_state_weight(const int i) const {
  vector<double> wsum(_num_classes);
  for (int j = 0; j < _num_classes; j++) {
    wsum[j] = forward_cache(i, j) / state_weight(i, j) * backward_cache(i, j);
  }

  return wsum;
}

void CRF_Model::add_sample_empirical_expectation(const Sequence& seq,
                                                 vector<double>& vee) {
  for (size_t i = 0; i < seq.vs.size(); i++) {
    for (vector<int>::const_iterator j = seq.vs[i].positive_features.begin();
         j != seq.vs[i].positive_features.end(); ++j) {
      for (vector<int>::const_iterator k = _feature2mef[*j].begin();
           k != _feature2mef[*j].end(); ++k) {
        if (_fb.Feature(*k).label() == seq.vs[i].label) {
          assert(*k >= 0 && *k < _vee.size());
          _vee[*k] += 1.0;
        }
      }
    }
  }

  for (int i = 0; i < (int)seq.vs.size() - 1; i++) {
    const int c0 = seq.vs[i].label;
    const int c1 = seq.vs[i + 1].label;
    _vee[edge_feature_id(c0, c1)] += 1.0;
  }
  if (USE_BOS_EOS) {
    _vee[edge_feature_id(_num_classes, seq.vs[0].label)] += 1.0;
    _vee[edge_feature_id(seq.vs[seq.vs.size() - 1].label, _num_classes + 1)] +=
        1.0;
  }
}

double CRF_Model::add_sample_model_expectation(const Sequence& seq,
                                               vector<double>& vme,
                                               int& ncorrect) {
  const double fp = forward_backward(seq);
  assert(abs(fp - 1.0) < 0.01);
  const double logl = calc_loglikelihood(seq);

  for (size_t i = 0; i < seq.vs.size(); i++) {
    const Sample& s = seq.vs[i];

    // model expectation (state)
    vector<double> wsum = calc_state_weight(i);
    for (vector<int>::const_iterator j = s.positive_features.begin();
         j != s.positive_features.end(); ++j) {
      for (vector<int>::const_iterator k = _feature2mef[*j].begin();
           k != _feature2mef[*j].end(); ++k) {
        vme[*k] += wsum[_fb.Feature(*k).label()];
      }
    }
    if (s.label == max_element(wsum.begin(), wsum.end()) - wsum.begin())
      ncorrect++;

    // model expectation (edge)
    if (i == seq.vs.size() - 1) continue;
    for (int j = 0; j < _num_classes; j++) {
      const double lhs = forward_cache(i, j);
      for (int k = 0; k < _num_classes; k++) {
        const double rhs = backward_cache(i + 1, k);
        assert(lhs != DBL_MAX && rhs != DBL_MAX);
        const double w = lhs * edge_weight(j, k) * rhs;
        vme[edge_feature_id(j, k)] += w;
      }
    }
  }
  if (USE_BOS_EOS) {
    // model expectation (BOS -> *)
    for (int j = 0; j < _num_classes; j++) {
      const double rhs = backward_cache(0, j);
      const double w = edge_weight(_num_classes, j) * rhs;
      vme[edge_feature_id(_num_classes, j)] += w;
    }
    // model expectation (* -> EOS)
    const int len = seq.vs.size();
    for (int j = 0; j < _num_classes; j++) {
      const double lhs = forward_cache(len - 1, j);
      const double w = edge_weight(j, _num_classes + 1) * lhs;
      vme[edge_feature_id(j, _num_classes + 1)] += w;
    }
  }

  return logl;
}

double CRF_Model::update_model_expectation() {
  double logl = 0;
  int ncorrect = 0, total_len = 0;

  _vme.resize(_fb.Size());
  for (int i = 0; i < _fb.Size(); i++) _vme[i] = 0;

  initialize_edge_weights();
  for (vector<Sequence>::const_iterator n = _vs.begin(); n != _vs.end(); ++n) {
    const Sequence& seq = *n;
    total_len += seq.vs.size();
    logl += add_sample_model_expectation(seq, _vme, ncorrect);
  }

  for (int i = 0; i < _fb.Size(); i++) {
    _vme[i] /= _vs.size();
  }

  _train_error = 1 - (double)ncorrect / total_len;

  logl /= _vs.size();

  if (_sigma > 0) {
    const double c = 1 / (2 * _sigma * _sigma);
    for (int i = 0; i < _fb.Size(); i++) {
      logl -= _vl[i] * _vl[i] * c;
    }
  }

  return logl;
}

inline bool contain_space(const string& s) {
  for (int i = 0; i < s.size(); i++) {
    if (isspace(s[i])) return true;
  }
  return false;
}

void CRF_Model::add_training_sample(const CRF_Sequence& seq) {
  if (seq.vs.size() >= MAX_LEN) {
    cerr << "error: sequence is too long.";
    exit(1);
  }
  if (seq.vs.size() == 0) {
    cerr << "warning: empty sentence" << endl;
    return;
  }
  assert(seq.vs.size() > 0);

  Sequence s1;
  for (vector<CRF_State>::const_iterator i = seq.vs.begin(); i != seq.vs.end();
       ++i) {
    if (i->label == BOS_LABEL || i->label == EOS_LABEL) {
      cerr << "error: the label name \"" << i->label
           << "\" is reserved. Use a different name.";
      exit(1);
    }
    if (contain_space(i->label)) {
      cerr << "error: the name of a label must not contain any space." << endl;
      exit(1);
    }
    Sample s;
    s.label = _label_bag.Put(i->label);
    if (s.label >= MAX_LABEL_TYPES - 2) {
      cerr << "error: too many types of labels." << endl;
      exit(1);
    }
    assert(s.label >= 0 && s.label < MAX_LABEL_TYPES);
    for (vector<string>::const_iterator j = i->features.begin();
         j != i->features.end(); ++j) {
      if (contain_space(*j)) {
        cerr << "error: the name of a feature must not contain any space."
             << endl;
        exit(1);
      }
      s.positive_features.push_back(_featurename_bag.Put(*j));
    }
    s1.vs.push_back(s);
  }
  _vs.push_back(s1);
}

int CRF_Model::train(const OptimizationMethod method, const int cutoff,
                     const double sigma, const double widthfactor) {
  if (sigma > 0 && widthfactor > 0) {
    cerr << "error: Gausian prior and inequality modeling cannot be used "
            "together." << endl;
    return 0;
  }
  if (_vs.size() == 0) {
    cerr << "error: no training data." << endl;
    return 0;
  }
  if (_nheldout >= (int)_vs.size()) {
    cerr << "error: too much heldout data. no training data is available."
         << endl;
    return 0;
  }

  _label_bag.Put(BOS_LABEL);
  _label_bag.Put(EOS_LABEL);
  _num_classes = _label_bag.Size() - 2;

  for (int i = 0; i < _nheldout; i++) {
    _heldout.push_back(_vs.back());
    _vs.pop_back();
  }

  int total_len = 0;
  for (size_t i = 0; i < _vs.size(); i++) total_len += _vs[i].vs.size();

  _sigma = sigma;
  _inequality_width = widthfactor / _vs.size();

  if (cutoff > 0) cerr << "cutoff threshold = " << cutoff << endl;
  if (widthfactor > 0) cerr << "widthfactor = " << widthfactor << endl;
  cerr << "preparing for estimation...";
  make_feature_bag(cutoff);
  cerr << "done" << endl;
  cerr << "number of state types = " << _num_classes << endl;
  cerr << "number of samples = " << _vs.size() << endl;
  cerr << "number of features = " << _fb.Size() << endl;

  cerr << "calculating empirical expectation...";
  _vee.resize(_fb.Size());
  _vee.assign(_vee.size(), 0.0);

  int count = 0;
  for (vector<Sequence>::const_iterator n = _vs.begin(); n != _vs.end();
       ++n, count++) {
    add_sample_empirical_expectation(*n, _vee);
  }

  for (int i = 0; i < _vee.size(); i++) _vee[i] /= _vs.size();

  _vl.resize(_fb.Size());
  _vl.assign(_vl.size(), 0.0);

  perform_LookaheadTraining();

  if (_inequality_width > 0) {
    int sum = 0;
    for (int i = 0; i < _fb.Size(); i++) {
      if (_vl[i] != 0) sum++;
    }
    cerr << "number of active features = " << sum << endl;
  }

  return 0;
}

void CRF_Model::get_features(list<pair<pair<string, string>, double> >& fl) {
  fl.clear();
  for (StrDic::const_Iterator i = _featurename_bag.begin();
       i != _featurename_bag.end(); i++) {
    for (int j = 0; j < _label_bag.Size(); j++) {
      const string label = _label_bag.Str(j);
      const string history = i.getStr();
      int id = _fb.Id(ME_Feature(j, i.getId()));
      if (id < 0) continue;
      fl.push_back(make_pair(make_pair(label, history), _vl[id]));
    }
  }
}

bool CRF_Model::load_from_file(const string& filename, bool verbose) {
  FILE* fp = fopen(filename.c_str(), "r");
  if (!fp) {
    cerr << "error: cannot open " << filename << "!" << endl;
    return false;
  }

  _vl.clear();
  _label_bag.Clear();
  _featurename_bag.Clear();
  _fb.Clear();
  char buf[1024];
  while (fgets(buf, 1024, fp)) {
    string line(buf);
    string::size_type t1 = line.find_first_of('\t');
    string::size_type t2 = line.find_last_of('\t');
    string classname = line.substr(0, t1);
    string featurename = line.substr(t1 + 1, t2 - (t1 + 1));
    float lambda;
    string w = line.substr(t2 + 1);
    sscanf(w.c_str(), "%f", &lambda);

    const int label = _label_bag.Put(classname);
    const int feature = _featurename_bag.Put(featurename);
    _fb.Put(ME_Feature(label, feature));
    _vl.push_back(lambda);
  }

  // for zero-wight edges
  _label_bag.Put(BOS_LABEL);
  _label_bag.Put(EOS_LABEL);
  for (int i = 0; i < _label_bag.Size(); i++) {
    for (int j = 0; j < _label_bag.Size(); j++) {
      const string& label1 = _label_bag.Str(j);
      const int l1 = _featurename_bag.Put("->\t" + label1);
      const int id = _fb.Id(ME_Feature(i, l1));
      if (id < 0) {
        _fb.Put(ME_Feature(i, l1));
        _vl.push_back(0);
      }
    }
  }
  for (int i = 0; i < _label_bag.Size(); i++) {
    for (int j = 0; j < _label_bag.Size(); j++) {
      for (int k = 0; k < _label_bag.Size(); k++) {
        const string& label1 = _label_bag.Str(j);
        const string& label2 = _label_bag.Str(k);
        const int l1 =
            _featurename_bag.Put("->\t" + label1 + "\t->\t" + label2);
        const int id = _fb.Id(ME_Feature(i, l1));
        if (id < 0) {
          _fb.Put(ME_Feature(i, l1));
          _vl.push_back(0);
        }
      }
    }
  }
  if (USE_EDGE_TRIGRAMS) {
    for (int i = 0; i < _label_bag.Size(); i++) {
      for (int j = 0; j < _label_bag.Size(); j++) {
        for (int k = 0; k < _label_bag.Size(); k++) {
          for (int l = 0; l < _label_bag.Size(); l++) {
            const string& label1 = _label_bag.Str(j);
            const string& label2 = _label_bag.Str(k);
            const string& label3 = _label_bag.Str(l);
            const int l1 = _featurename_bag.Put("->\t" + label1 + "\t->\t" +
                                                label2 + "\t->\t" + label3);
            const int id = _fb.Id(ME_Feature(i, l1));
            if (id < 0) {
              _fb.Put(ME_Feature(i, l1));
              _vl.push_back(0);
            }
          }
        }
      }
    }
  }

  _num_classes = _label_bag.Size() - 2;

  init_feature2mef();
  initialize_edge_weights();

  fclose(fp);

  return true;
}

void CRF_Model::init_feature2mef() {
  _feature2mef.clear();
  for (int i = 0; i < _featurename_bag.Size(); i++) {
    vector<int> vi;
    for (int k = 0; k < _num_classes; k++) {
      int id = _fb.Id(ME_Feature(k, i));
      if (id >= 0) vi.push_back(id);
    }
    _feature2mef.push_back(vi);
  }

  for (int i = 0; i < _label_bag.Size(); i++) {
    for (int j = 0; j < _label_bag.Size(); j++) {
      const string& label1 = _label_bag.Str(j);
      const int l1 = _featurename_bag.Put("->\t" + label1);
      const int id = _fb.Put(ME_Feature(i, l1));
      edge_feature_id(i, j) = id;
    }
  }

  for (int i = 0; i < _label_bag.Size(); i++) {
    for (int j = 0; j < _label_bag.Size(); j++) {
      for (int k = 0; k < _label_bag.Size(); k++) {
        const string& label1 = _label_bag.Str(j);
        const string& label2 = _label_bag.Str(k);
        const int l1 =
            _featurename_bag.Put("->\t" + label1 + "\t->\t" + label2);
        const int id = _fb.Put(ME_Feature(i, l1));
        edge_feature_id2(i, j, k) = id;
      }
    }
  }
  if (USE_EDGE_TRIGRAMS) {
    for (int i = 0; i < _label_bag.Size(); i++) {
      for (int j = 0; j < _label_bag.Size(); j++) {
        for (int k = 0; k < _label_bag.Size(); k++) {
          for (int l = 0; l < _label_bag.Size(); l++) {
            const string& label1 = _label_bag.Str(j);
            const string& label2 = _label_bag.Str(k);
            const string& label3 = _label_bag.Str(l);
            const int l1 = _featurename_bag.Put("->\t" + label1 + "\t->\t" +
                                                label2 + "\t->\t" + label3);
            const int id = _fb.Put(ME_Feature(i, l1));
            edge_feature_id3(i, j, k, l) = id;
          }
        }
      }
    }
  }
}

bool CRF_Model::save_to_file(const string& filename, const double th) const {
  FILE* fp = fopen(filename.c_str(), "w");
  if (!fp) {
    cerr << "error: cannot open " << filename << "!" << endl;
    return false;
  }
  cerr << "saving " << filename << "...";

  for (StrDic::const_Iterator i = _featurename_bag.begin();
       i != _featurename_bag.end(); i++) {
    for (int j = 0; j < _label_bag.Size(); j++) {
      string label = _label_bag.Str(j);
      string history = i.getStr();
      int id = _fb.Id(ME_Feature(j, i.getId()));
      if (id < 0) continue;
      if (_vl[id] == 0) continue;       // ignore zero-weight features
      if (abs(_vl[id]) < th) continue;  // cut off low-weight features
      fprintf(fp, "%s\t%s\t%f\n", label.c_str(), history.c_str(), _vl[id]);
    }
  }

  fclose(fp);

  return true;
}

void CRF_Model::decode_forward_backward(CRF_Sequence& s0,
                                        vector<map<string, double> >& tagp) {
  if (s0.vs.size() >= MAX_LEN) {
    cerr << "error: sequence is too long." << endl;
    return;
  }

  Sequence seq;

  for (vector<CRF_State>::const_iterator i = s0.vs.begin(); i != s0.vs.end();
       ++i) {
    Sample s;
    for (vector<string>::const_iterator j = i->features.begin();
         j != i->features.end(); ++j) {
      const int id = _featurename_bag.Id(*j);
      if (id >= 0) s.positive_features.push_back(id);
    }
    seq.vs.push_back(s);
  }

  tagp.clear();
  forward_backward(seq);
  for (size_t i = 0; i < seq.vs.size(); i++) {
    vector<double> wsum = calc_state_weight(i);
    map<string, double> tp;
    if (OUTPUT_MARGINAL_PROB) {
      double sum = 0;
      for (vector<double>::const_iterator j = wsum.begin(); j != wsum.end();
           ++j)
        sum += *j;
      s0.vs[i].label = "";
      assert(abs(sum - 1) < 0.01);
      double maxp = -1;
      string maxtag;
      for (size_t j = 0; j < wsum.size(); j++) {
        double p = wsum[j] / sum;
        if (p <= 0.001) continue;
        tp[_label_bag.Str(j).c_str()] = p;
        if (p > maxp) {
          maxp = p;
          maxtag = _label_bag.Str(j).c_str();
        }
      }
      tagp.push_back(tp);
      s0.vs[i].label = maxtag;
    } else {
      const int l = max_element(wsum.begin(), wsum.end()) - wsum.begin();
      s0.vs[i].label = _label_bag.Str(l);
    }
  }
}

void CRF_Model::decode_viterbi(CRF_Sequence& s0) {
  if (s0.vs.size() >= MAX_LEN) {
    cerr << "error: sequence is too long." << endl;
    return;
  }

  Sequence seq;

  for (vector<CRF_State>::const_iterator i = s0.vs.begin(); i != s0.vs.end();
       ++i) {
    Sample s;
    for (vector<string>::const_iterator j = i->features.begin();
         j != i->features.end(); ++j) {
      const int id = _featurename_bag.Id(*j);
      if (id >= 0) s.positive_features.push_back(id);
    }
    seq.vs.push_back(s);
  }

  vector<int> vs(seq.vs.size());
  viterbi(seq, vs);
  for (size_t i = 0; i < seq.vs.size(); i++) {
    s0.vs[i].label = _label_bag.Str(vs[i]);
  }
}

void CRF_Model::decode_nbest(CRF_Sequence& s0,
                             vector<pair<double, vector<string> > >& sequences,
                             const int num, const double min_prob) {
  if (s0.vs.size() >= MAX_LEN) {
    cerr << "error: sequence is too long." << endl;
    return;
  }

  Sequence seq;

  for (vector<CRF_State>::const_iterator i = s0.vs.begin(); i != s0.vs.end();
       ++i) {
    Sample s;
    for (vector<string>::const_iterator j = i->features.begin();
         j != i->features.end(); ++j) {
      const int id = _featurename_bag.Id(*j);
      if (id >= 0) s.positive_features.push_back(id);
    }
    seq.vs.push_back(s);
  }

  vector<int> vs(seq.vs.size());
  vector<Path> nb;
  nbest(seq, nb, num, min_prob);

  sequences.clear();
  if (nb.size() == 0) return;

  for (size_t i = 0; i < seq.vs.size(); i++) {
    s0.vs[i].label = _label_bag.Str(nb[0].vs[i]);
  }
  for (vector<Path>::const_iterator i = nb.begin(); i != nb.end(); ++i) {
    if (i->score < min_prob) break;
    vector<string> vstr(i->vs.size());
    for (size_t j = 0; j < vstr.size(); j++) {
      vstr[j] = _label_bag.Str((i->vs)[j]);
    }
    sequences.push_back(pair<double, vector<string> >(i->score, vstr));
    if ((int)sequences.size() >= num) break;
  }
}

int CRF_Model::perform_AveragedPerceptron() {
  const int dim = _fb.Size();

  vector<double> wsum(dim, 0);

  initialize_edge_weights();

  int iter = 0;
  while (iter < 5) {
    for (int i = 0; i < dim; i++) wsum[i] += _vl[i] * _vs.size();
    iter++;

    int error_num = 0;
    int rest = _vs.size();
    for (vector<Sequence>::const_iterator n = _vs.begin(); n != _vs.end();
         ++n, rest--) {
      const Sequence& seq = *n;
      vector<int> vs(seq.vs.size());
      viterbi(seq, vs);

      double Loss = 0;
      for (size_t i = 0; i < vs.size(); i++) {
        if (vs[i] != seq.vs[i].label) Loss += 1.0;
      }
      if (Loss == 0) continue;

      if (Loss > 0) error_num++;

      // single best MIRA (just to compute the coefficient)
      map<int, double> X;
      // state
      for (size_t i = 0; i < seq.vs.size(); i++) {
        const Sample& s = seq.vs[i];
        if (s.label == vs[i]) continue;
        for (vector<int>::const_iterator j = s.positive_features.begin();
             j != s.positive_features.end(); ++j) {
          for (vector<int>::const_iterator k = _feature2mef[*j].begin();
               k != _feature2mef[*j].end(); ++k) {
            const int label = _fb.Feature(*k).label();
            if (label == s.label) X[*k] += 1.0;
            if (label == vs[i]) X[*k] -= 1.0;
          }
        }
      }
      // edge
      for (int i = 0; i < seq.vs.size() - 1; i++) {
        const int eid0 = edge_feature_id(seq.vs[i].label, seq.vs[i + 1].label);
        const int eid1 = edge_feature_id(vs[i], vs[i + 1]);
        if (eid0 == eid1) continue;
        X[eid0] += 1.0;
        X[eid1] -= 1.0;
      }

      double wX = 0, X2 = 0;
      for (map<int, double>::const_iterator i = X.begin(); i != X.end(); ++i) {
        wX += _vl[i->first] * i->second;
        X2 += i->second * i->second;
      }
      const double a = max(0.0, (Loss - wX) / X2);

      // state
      for (size_t i = 0; i < seq.vs.size(); i++) {
        const Sample& s = seq.vs[i];
        if (s.label == vs[i]) continue;

        for (vector<int>::const_iterator j = s.positive_features.begin();
             j != s.positive_features.end(); ++j) {
          for (vector<int>::const_iterator k = _feature2mef[*j].begin();
               k != _feature2mef[*j].end(); ++k) {
            const int label = _fb.Feature(*k).label();
            if (label == s.label) {
              _vl[*k] += a;
              wsum[*k] += a * rest;
            }
            if (label == vs[i]) {
              _vl[*k] -= a;
              wsum[*k] -= a * rest;
            }
          }
        }
      }

      // edge
      for (int i = 0; i < int(seq.vs.size()) - 1; i++) {
        const int eid0 = edge_feature_id(seq.vs[i].label, seq.vs[i + 1].label);
        double& w0 = _vl[eid0];
        w0 += a;
        wsum[eid0] += a * rest;
        edge_weight(seq.vs[i].label, seq.vs[i + 1].label) = exp(w0);

        const int eid1 = edge_feature_id(vs[i], vs[i + 1]);
        double& w1 = _vl[eid1];
        w1 -= a;
        wsum[eid1] -= a * rest;
        edge_weight(vs[i], vs[i + 1]) = exp(w1);
      }
    }
    cerr << "iter = " << iter << " error_num = " << error_num << endl;
    if (error_num == 0) break;
  }

  for (int i = 0; i < dim; i++) {
    _vl[i] = wsum[i] / (iter * _vs.size());
  }

  return 0;
}

inline int sign(double x) {
  if (x > 0) return 1;
  if (x < 0) return -1;
  return 0;
}

static vector<double> pseudo_gradient(const vector<double>& x,
                                      const vector<double>& grad0,
                                      const double C) {
  vector<double> grad = grad0;
  for (size_t i = 0; i < x.size(); i++) {
    if (x[i] != 0) {
      grad[i] += C * sign(x[i]);
      continue;
    }
    const double gm = grad0[i] - C;
    if (gm > 0) {
      grad[i] = gm;
      continue;
    }
    const double gp = grad0[i] + C;
    if (gp < 0) {
      grad[i] = gp;
      continue;
    }
    grad[i] = 0;
  }

  return grad;
}

static void l1ball_projection(vector<double>& v, const double z) {
  vector<double> v1 = v;
  for (int i = 0; i < v1.size(); i++) v1[i] = abs(v1[i]);

  sort(v1.begin(), v1.end());

  int ro = 1;
  double sum = 0, rosum = 0;
  for (int j = 1; j <= v1.size(); j++) {
    const double u = v1[v1.size() - j];
    sum += u;
    if (u - (sum - z) / j > 0) {
      ro = j;
      rosum = sum;
    }
  }

  const double theta = (rosum - z) / ro;

  for (int i = 0; i < v.size(); i++) {
    if (v[i] > 0) {
      v[i] = max(0.0, v[i] - theta);
    } else if (v[i] < 0) {
      v[i] = min(0.0, v[i] + theta);
    }
  }
}

int CRF_Model::perform_StochasticGradientDescent() {
  const double L1_PSEUDO_GRADIENT = 0;
  const int d = _fb.Size();

  vector<int> ri(_vs.size());
  for (int i = 0; i < ri.size(); i++) ri[i] = i;

  const int batch_size = 20;

  _vee.assign(_vee.size(), 0);

  int iter = 0, k = 0;
  const double eta0 = 1.0;
  const double tau = 5.0 * _vs.size() / batch_size;
  for (int iter = 0; iter < 20; iter++) {
    vector<double> vme(d, 0);
    initialize_edge_weights();
    random_shuffle(ri.begin(), ri.end());

    int n = 0, ncorrect = 0, ntotal = 0;
    double logl = 0;
    for (int i = 0;; i++) {
      const Sequence& seq = _vs[ri[i]];
      ntotal += seq.vs.size();
      logl += add_sample_model_expectation(seq, vme, ncorrect);
      add_sample_empirical_expectation(seq, _vee);

      n++;
      if (n == batch_size || i == ri.size() - 1) {
        // update the weights of the features
        for (size_t j = 0; j < d; j++) {
          _vee[j] /= n;
          vme[j] /= n;
        }

        vector<double> grad(d);
        for (size_t j = 0; j < d; j++) {
          grad[j] = vme[j] - _vee[j];
        }

        const double eta = eta0 * tau / (tau + k);
        if (L1_PSEUDO_GRADIENT == 0) {
          for (size_t j = 0; j < d; j++) {
            _vl[j] -= eta * grad[j];
          }
        } else {
          grad = pseudo_gradient(_vl, grad, L1_PSEUDO_GRADIENT);
          for (size_t j = 0; j < d; j++) {
            const double prev = _vl[j];
            _vl[j] -= eta * grad[j];
            if (prev * _vl[j] < 0) _vl[j] = 0;
          }
        }

        // reset
        k++;
        n = 0;
        vme.assign(d, 0);
        _vee.assign(_vee.size(), 0);
        initialize_edge_weights();
        if (i == ri.size() - 1) break;
      }
    }
    logl /= _vs.size();

    cerr << "iter = " << iter << " logl = " << logl
         << " acc = " << (double)ncorrect / ntotal << endl;
  }

  return 0;
}
