/*
 TinySVM -- Yet Another Tiny SVM Package

 $Id: cache.h,v 1.22 2002/08/20 06:31:16 taku-ku Exp $;

 Copyright (C) 2001-2002  Taku Kudo <taku-ku@is.aist-nara.ac.jp>
 All rights reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License as published by the Free Software Foundation; either
 version 2 of the License, or (at your option) any later verjsion.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public
 License along with this library; if not, write to the
 Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 Boston, MA 02111-1307, USA.
*/


#ifndef _CACHE_H
#define _CACHE_H
#include "common.h"

// $Id: cache.h,v 1.22 2002/08/20 06:31:16 taku-ku Exp $;

// Kernel Cache

namespace TinySVM {

template <class T = double> class Cache
{
private:
  int    l;
  int    free_mem_size;
  double cache_mem_size;

  struct head_t
  {
    head_t *prev, *next;    // a cicular list
    int    index;
    T*     data;
  };

  head_t*  lru_head;
  head_t** index2head;

  inline int get_cache_size(const int _l, const double mem_size)
  {
    return _min (_l, _max(2, (int)(1024 * 1024 * mem_size/(sizeof(T) * _l))));
  }
 
  // delete h from current postion
  inline void delete_lru(head_t *h)
  {
    h->prev->next = h->next;
    h->next->prev = h->prev;
  }

  // insert h to lru - 1, position
  inline void insert_lru(head_t *h)
  {
    h->next = lru_head;
    h->prev = lru_head->prev;
    h->prev->next = h;
    h->next->prev = h;
  }

public:
  int size;

  Cache(int, double);
  ~Cache();

  void update(const int);

  void delete_index(const int index)
  {
    head_t *h = index2head[index];
    if (h) {
      if (lru_head != h) {
	delete_lru (h);
	insert_lru (h);
      }
      lru_head = h;
    }
  }

  inline int getData(const int index, T **data)
  {
    head_t *h = index2head[index];
    if (h) {
      if(lru_head == h) {
	lru_head = lru_head->next;
      } else {
	delete_lru(h);
	insert_lru(h);
      }

      *data = h->data;
      return 1;
    } else {
      h = lru_head;
      lru_head = lru_head->next;
      if (h->index!=-1) index2head[h->index] = 0;
      h->index = index;
      index2head[index] = h;
      *data = h->data;
      return 0;
    }
  }

  void swap_index(const int i, const int j)
  {
    _swap (index2head[i], index2head[j]);
    for (head_t *h = lru_head;;h = h->next) {
      if (h->index == i)  h->index = j;
      else if (h->index == j) h->index = i; // fixed, 2001-12-23, fatal bug ..
      _swap(h->data[i], h->data[j]);
      if (h == lru_head->prev) break;
    }
  }
};

template <class T> 
Cache<T>::Cache(int _l, double _cache_mem_size):
  l(_l),
  cache_mem_size(_cache_mem_size)
{
  try {
    size = get_cache_size(l, cache_mem_size);

    free_mem_size = 0;

    head_t *start = new head_t;
    head_t *prev  = start;
    start->index = -1;
    start->data = new T [l];

    int tmp_size = 1;

    for(int i = 1; i < size; i++) {
      tmp_size++;
      head_t *head = new head_t;
      head->prev = prev;
      prev->next = head;
      head->index = -1;
      head->data = new T [l];
      prev = head;
    }

    prev->next = start;
    start->prev = prev;
    lru_head = start;

    index2head = new head_t *[l];
    for(int i = 0; i < l; i++) index2head[i] = 0;
  }

  catch (...) {
    fprintf(stderr,"Cache::Cache(): Out of memory\n");
    exit(1);
  }  
}

template <class T>
Cache<T>::~Cache()
{
  delete [] index2head;

  int flag = 1;
  head_t *end = lru_head->prev;
  head_t *h = lru_head;

  while (flag) {
    delete [] h->data;
    head_t *tmp = h->next;
    if (h == end) flag = 0;
    delete h;
    h = tmp;
  }
}

// change the size of each elements to be _l
// if _l < l, allocate new erea for cache
template <class T> 
void Cache<T>::update(const int _l)
{
  int new_size = get_cache_size(_l, cache_mem_size);

  if (100 * new_size/size >= 110 && new_size < _l) {
    // realloc
    for (head_t *h = lru_head;;h = h->next) {
      T *new_data;
	_clone(new_data, h->data, _l);
	delete [] h->data;
	h->data = new_data; 
	if (h == lru_head->prev) break; 
      }
    
    // new node
    for (int i = 0; i < (new_size - size); i++) {
      head_t *h = new head_t;
      h->data  = new T [_l];
      h->index = -1;
      insert_lru(h);
      lru_head = h;
    }
    
    size = new_size;
  }

  l = _l;
}

};
#endif

