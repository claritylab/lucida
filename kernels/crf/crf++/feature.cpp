//
//  CRF++ -- Yet Another CRF toolkit
//
//  $Id: feature.cpp 1595 2007-02-24 10:18:32Z taku $;
//
//  Copyright(C) 2005-2007 Taku Kudo <taku@chasen.org>
//
#include "feature_index.h"
#include "common.h"
#include "node.h"
#include "path.h"
#include "tagger.h"

namespace CRFPP {
const size_t kMaxContextSize = 8;

const char *BOS[kMaxContextSize] = { "_B-1", "_B-2", "_B-3", "_B-4",
                                     "_B-5", "_B-6", "_B-7", "_B-8" };
const char *EOS[kMaxContextSize] = { "_B+1", "_B+2", "_B+3", "_B+4",
                                     "_B+5", "_B+6", "_B+7", "_B+8" };

const char *FeatureIndex::getIndex(const char *&p,
                                   size_t pos,
                                   const TaggerImpl &tagger) const {
  if (*p++ !='[') {
    return 0;
  }

  int col = 0;
  int row = 0;

  int neg = 1;
  if (*p++ == '-') {
    neg = -1;
  } else {
    --p;
  }

  for (; *p; ++p) {
    switch (*p) {
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        row = 10 * row +(*p - '0');
        break;
      case ',':
        ++p;
        goto NEXT1;
      default: return  0;
    }
  }

NEXT1:

  for (; *p; ++p) {
    switch (*p) {
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        col = 10 * col + (*p - '0');
        break;
      case ']': goto NEXT2;
      default: return 0;
    }
  }

NEXT2:

  row *= neg;

  if (row < -static_cast<int>(kMaxContextSize) ||
      row > static_cast<int>(kMaxContextSize) ||
      col < 0 || col >= static_cast<int>(tagger.xsize())) {
    return 0;
  }

  // TODO(taku): very dirty workaround
  if (check_max_xsize_) {
    max_xsize_ = std::max(max_xsize_, static_cast<unsigned int>(col + 1));
  }

  const int idx = pos + row;
  if (idx < 0) {
    return BOS[-idx-1];
  }
  if (idx >= static_cast<int>(tagger.size())) {
    return EOS[idx - tagger.size()];
  }

  return tagger.x(idx, col);
}

bool FeatureIndex::applyRule(string_buffer *os,
                             const char *p,
                             size_t pos,
                             const TaggerImpl& tagger) const {
  os->assign("");  // clear
  const char *r;

  for (; *p; p++) {
    switch (*p) {
      default:
        *os << *p;
        break;
      case '%':
        switch (*++p) {
          case 'x':
            ++p;
            r = getIndex(p, pos, tagger);
            if (!r) {
              return false;
            }
            *os << r;
            break;
          default:
            return false;
        }
        break;
    }
  }

  *os << '\0';

  return true;
}

void FeatureIndex::rebuildFeatures(TaggerImpl *tagger) const {
  size_t fid = tagger->feature_id();
  const size_t thread_id = tagger->thread_id();

  Allocator *allocator = tagger->allocator();
  allocator->clear_freelist(thread_id);
  FeatureCache *feature_cache = allocator->feature_cache();

  for (size_t cur = 0; cur < tagger->size(); ++cur) {
    const int *f = (*feature_cache)[fid++];
    for (size_t i = 0; i < y_.size(); ++i) {
      Node *n = allocator->newNode(thread_id);
      n->clear();
      n->x = cur;
      n->y = i;
      n->fvector = f;
      tagger->set_node(n, cur, i);
    }
  }

  for (size_t cur = 1; cur < tagger->size(); ++cur) {
    const int *f = (*feature_cache)[fid++];
    for (size_t j = 0; j < y_.size(); ++j) {
      for (size_t i = 0; i < y_.size(); ++i) {
        Path *p = allocator->newPath(thread_id);
        p->clear();
        p->add(tagger->node(cur - 1, j),
               tagger->node(cur,     i));
        p->fvector = f;
      }
    }
  }
}

#define ADD { const int id = getID(os.c_str());         \
    if (id != -1) feature.push_back(id); } while (0)

bool FeatureIndex::buildFeatures(TaggerImpl *tagger) const {
  string_buffer os;
  std::vector<int> feature;

  FeatureCache *feature_cache = tagger->allocator()->feature_cache();
  tagger->set_feature_id(feature_cache->size());

  for (size_t cur = 0; cur < tagger->size(); ++cur) {
    for (std::vector<std::string>::const_iterator it
             = unigram_templs_.begin();
         it != unigram_templs_.end(); ++it) {
      if (!applyRule(&os, it->c_str(), cur, *tagger)) {
        return false;
      }
      ADD;
    }
    feature_cache->add(feature);
    feature.clear();
  }

  for (size_t cur = 1; cur < tagger->size(); ++cur) {
    for (std::vector<std::string>::const_iterator
             it = bigram_templs_.begin();
         it != bigram_templs_.end(); ++it) {
      if (!applyRule(&os, it->c_str(), cur, *tagger)) {
        return false;
      }
      ADD;
    }
    feature_cache->add(feature);
    feature.clear();
  }

  return true;
}
#undef ADD
}
