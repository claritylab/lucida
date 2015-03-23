#ifndef STRDIC_H
#define STRDIC_H

#include <vector>
#include <string>
#include <iostream>
//
// This is an implementation of the string hashing method described
// in the following paper.
//
// N. Askitis and J. Zobel, Cache-conscious Collision Resolution in
// String Hash Tables, Proc. of String Processing and Information
// Retrieval Symp., Springer-Verlag, pp. 92--104, 2006.

class StrDic {
 public:
  enum { LOAD_FACTOR = 2 };

  explicit StrDic(const int n = 1) { Clear(n); }
  ~StrDic() {
    for (size_t i = 0; i < _v.size(); i++) free(_v[i]);
  }
  int Put(const std::string &s) {
    assert(s.size() > 0 && s.size() < 256);
    return Insert(s, _num_terms);
  }
  int Id(const std::string &s) const {
    assert(s.size() > 0 && s.size() < 256);
    const size_t idx = hash_func(s) % _v.size();
    const unsigned char *base = _v[idx];
    if (base != NULL) {
      base = ScanStr(base, s);
      if (*base) return GetInt(base);
    }
    return -1;
  }
  void Clear(const int n = 1) {
    for (size_t i = 0; i < _v.size(); i++) free(_v[i]);
    _v.resize(n);
    for (size_t i = 0; i < _v.size(); i++) _v[i] = NULL;
    _num_terms = 0;
    _min_idx = _v.size();
    _max_idx = 0;
  }
  size_t Size() const { return _num_terms; }

  struct const_Iterator {
    const StrDic *obj;
    int idx;
    const unsigned char *base;
    const_Iterator(const StrDic *o, int i, const unsigned char *b)
        : obj(o), idx(i), base(b) {}
    const_Iterator &operator++(int) {
      base += *base + EXTRA_DATA_SIZE;
      if (*base) return *this;
      for (int i = idx + 1; i <= (int)(obj->_max_idx); i++) {
        if (obj->_v[i]) {
          idx = i;
          base = obj->_v[i];
          return *this;
        }
      }
      idx = -1;
      base = NULL;
      return *this;
    }
    std::string getStr() const {
      return std::string((char *)(base + 1), (*base));
    }
    int getId() const { return obj->GetInt(base); }
  };
  const_Iterator begin() const {
    return const_Iterator(this, _min_idx, _v[_min_idx]);
  }
  const_Iterator end() const { return const_Iterator(this, -1, NULL); }

 private:
  size_t _num_terms;
  size_t _min_idx;
  size_t _max_idx;
  std::vector<unsigned char *> _v;

  enum {
    EXTRA_DATA_SIZE = 5
  };  // length of the string (1 byte) + int (4 bytes)

  size_t hash_func(const std::string &s) const {
    unsigned int h = 5;
    for (size_t i = 0; i < s.size(); i++) {
      h ^= ((h << 5) + (h >> 2) + s[i]);
    }
    return h;
  }
  void StoreStrInt(const std::string &s, const unsigned int val,
                   unsigned char *base) {
    *(base++) = (unsigned char)s.size();
    for (size_t i = 0; i < s.size(); i++) *(base++) = s[i];
    *(base++) = (unsigned char)(val);
    *(base++) = (unsigned char)(val >> 8);
    *(base++) = (unsigned char)(val >> 16);
    *(base++) = (unsigned char)(val >> 24);
    *base = 0;
  }
  int GetInt(const unsigned char *base) const {
    assert(base != NULL);
    base += *base + 4;
    unsigned int val = 0;
    val |= *(base--);
    val = val << 8;
    val |= *(base--);
    val = val << 8;
    val |= *(base--);
    val = val << 8;
    val |= *base;
    return val;
  }
  const unsigned char *ScanStr(const unsigned char *base,
                               const std::string &s) const {
    assert(base != NULL);
    while (*base != 0) {
      bool is_same = false;
      if (*base == s.size()) {
        is_same = true;
        for (size_t i = 0; i < s.size(); i++) {
          if (base[i + 1] != s[i]) {
            is_same = false;
            break;
          }
        }
      }
      if (is_same) return base;
      base += *base + EXTRA_DATA_SIZE;
    }
    return base;
  }
  int Insert(const std::string &s, const int val) {
    assert(s.size() < 256);
    const size_t idx = hash_func(s) % _v.size();
    const unsigned char *base = _v[idx];
    unsigned int current_size = 0;
    if (base != NULL) {
      base = ScanStr(base, s);
      if (*base) return GetInt(base);  /////////// doesn't store the value
      current_size = base - _v[idx];
    }
    const size_t new_size = current_size + s.size() + EXTRA_DATA_SIZE + 1;
    unsigned char *p = (unsigned char *)realloc(_v[idx], new_size);
    if (p == NULL) {
      std::cerr << "error: realloc() failed." << std::endl;
      exit(1);
    }
    _v[idx] = p;
    StoreStrInt(s, val, p + current_size);
    _num_terms++;
    _min_idx = std::min(_min_idx, idx);
    _max_idx = std::max(_max_idx, idx);

    if (_num_terms > _v.size() * LOAD_FACTOR) this->Expand();

    return _num_terms - 1;
  }
  void Expand() {
    StrDic c(_v.size() * 2);

    for (size_t i = _min_idx; i <= _max_idx; i++) {
      if (_v[i] == NULL) continue;
      const unsigned char *base = _v[i];
      while (*base) {
        c.Insert(std::string((char *)(base + 1), (*base)), GetInt(base));
        base += *base + EXTRA_DATA_SIZE;
      }
      free(_v[i]);
    }

    _v = c._v;
    _min_idx = c._min_idx;
    _max_idx = c._max_idx;

    c._v.resize(0);
  }
};

inline bool operator!=(const StrDic::const_Iterator &x,
                       const StrDic::const_Iterator &y) {
  if (x.idx == y.idx && x.base == y.base) return false;
  return true;
}

#endif
