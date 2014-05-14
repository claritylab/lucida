#include "drutils.h"
#include "hash.h"

typedef struct hashEntry *HashEntry;

struct hashTable {
  int memType;
  unsigned long size;
  unsigned long entries;
  float maxAvgBinSize;
  unsigned long (*hashFunc)(void *, unsigned long);
  int (*entryCompare)(void *, void *);
  HashEntry *bin;
};

struct hashEntry {
  void *key;  /* key may be a pointer into the data or not */
  void *data;
  HashEntry next;
};

/* Returns NULL on failure */
HashTable hashTableCreate(unsigned long initialSize, float maxAvgBinSize,
			  unsigned long (*hashFunc)(void *, unsigned long),
			  int (*entryCompare)(void *, void *), int memType) {
  HashTable T = (HashTable) smartCalloc(1, sizeof(struct hashTable), memType);
  T->memType = memType;
  T->size = initialSize;
  T->maxAvgBinSize = maxAvgBinSize;
  T->hashFunc = hashFunc;
  T->entryCompare = entryCompare;
  T->bin = (HashEntry *) smartCalloc(initialSize, sizeof(HashEntry), memType);
  if (!T->bin) {
    smartFree(T, memType);
    return NULL;
  }
  return T;
}

void hashTableClear(HashTable T) {
  unsigned long i;
  HashEntry E, N;
  if (!T) return;
  for (i = 0; i < T->size; i++) {
    for (E = T->bin[i]; E; E = N) {
      N = E->next;
      smartFree(E, T->memType);
    }
    T->bin[i] = NULL;
  }
  T->entries = 0;
}

void hashTableFree(HashTable T) {
  if (!T) return;
  hashTableClear(T);
  smartFree(T->bin, T->memType);
  smartFree(T, T->memType);
}

unsigned long hashVal(HashTable T, void *key) {
  return T->hashFunc(key, T->size);
}

/* Returns NULL on failure */
void *hashTableFastLookup(HashTable T, void *key, unsigned long hashVal) {
  int val = -1;
  HashEntry E;
  if (!T) return NULL;
  hashVal %= T->size;
  for (E = T->bin[hashVal]; E && (val = T->entryCompare(E->key, key)) < 0; 
       E = E->next);
  if (!E) return NULL;
  if (val == 0) return E->data;
  return NULL;
}

void *hashTableLookup(HashTable T, void *key) {
  if (!T) return NULL;
  return hashTableFastLookup(T, key, T->hashFunc(key, T->size));
}

/* Returns 1 if the entry is added (not a duplicate), else 0. */
static char fastInsertEntry(HashTable T, HashEntry E, unsigned long hashVal, 
			    char allowDups) {
  int c = 0;
  HashEntry P, N;
  hashVal %= T->size;
  for (P = NULL, N = T->bin[hashVal]; 
       N && (c = T->entryCompare(N->key, E->key)) < 0; P = N, N = N->next);

  if (!allowDups && N && c == 0) {
    N->data = E->data;
    smartFree(E, T->memType);
    return 0;
  }
  
  if (!P) T->bin[hashVal] = E;
  else P->next = E;
  E->next = N;
  return 1;
}

/* Returns 1 if the entry is added (not a duplicate), else 0. */
static char insertEntry(HashTable T, HashEntry E, char allowDups) {
  return fastInsertEntry(T, E, T->hashFunc(E->key, T->size), allowDups);
}

void hashTableGrow(HashTable T) {
  HashEntry E, N, *oldBin = T->bin;
  unsigned long i, oldSize = T->size;
  T->size *= 2;
  T->bin = (HashEntry *) smartCalloc(T->size, sizeof(HashEntry), T->memType);
  for (i = 0; i < oldSize; i++)
    for (E = oldBin[i]; E; E = N) {
      N = E->next;
      insertEntry(T, E, TRUE);
    }
  smartFree(oldBin, T->memType);
}

/* Returns 1 on failure */
char hashTableInsert(HashTable T, void *key, void *data, char allowDups) {
  HashEntry E;
  if (!T) return 1;
  if (T->entries >= (T->maxAvgBinSize * T->size))
    hashTableGrow(T);
  E = (HashEntry) smartMalloc(sizeof(struct hashEntry), T->memType);
  E->key  = key;
  E->data = data;
  T->entries += insertEntry(T, E, allowDups);
  return 0;
}

/* Returns 1 on failure */
char hashTableFastInsert(HashTable T, void *key, void *data, 
			 unsigned long hashVal, char allowDups) {
  HashEntry E;
  if (!T) return 1;
  if (T->entries >= (T->maxAvgBinSize * T->size)) {
    hashTableGrow(T);
    hashVal = T->hashFunc(key, T->size);
  }
  E = (HashEntry) smartMalloc(sizeof(struct hashEntry), T->memType);
  E->key  = key;
  E->data = data;
  T->entries += fastInsertEntry(T, E, hashVal, allowDups);
  return 0;
}

/* Returns the data entry from the table that matches the key. */
void *hashTableRemove(HashTable T, void *key) {
  int val = -1;
  HashEntry P, E;
  void *data;
  unsigned long hashVal;
  if (!T) return NULL;
  hashVal = T->hashFunc(key, T->size);
  for (P = NULL, E = T->bin[hashVal]; 
       E && (val = T->entryCompare(E->key, key)) < 0; P = E, E = E->next);
  if (E && val == 0) {
    if (P) P->next = E->next;
    else T->bin[hashVal] = E->next;
    data = E->data;
    smartFree(E, T->memType);
    T->entries--;
    return data;
  } else return NULL;
}

/* If the function returns non-zero, the node will be removed */
/* This may fail if E or N are removed by func */
void hashTableForEach(HashTable T, 
		      char (*func)(HashTable, void *, void *, void *), 
		      void *userData) {
  unsigned long i;
  HashEntry P, E, N;
  if (!T) return;
  for (i = 0; i < T->size; i++)
    for (E = T->bin[i], P = NULL; E; E = N) {
      N = E->next;
      if (func(T, E->key, E->data, userData)) {
	if (!P) T->bin[i] = N;
	else P->next = N;
	smartFree(E, T->memType);
	T->entries--;
      } else P = E;
    }
}

void hashTableForEachMatch(HashTable T, void *key,
			   char (*func)(HashTable, void *, void *, void *),
			   void *userData) {
  int val;
  HashEntry P, E, N;
  unsigned long hashVal;
  if (!T) return;
  hashVal = T->hashFunc(key, T->size);
  for (E = T->bin[hashVal], P = NULL; 
       E && (val = T->entryCompare(E->key, key)) <= 0; E = N) {
    N = E->next;
    if (val == 0 && func(T, E->key, E->data, userData)) {
      if (!P) T->bin[hashVal] = N;
      else P->next = N;
      smartFree(E, T->memType);
      T->entries--;
    } else P = E;
  }
}

unsigned long hashTableNumEntries(HashTable T) {
  if (!T) return 0;
  return T->entries;
}

unsigned long hashInt(void *v, unsigned long n) {
  return abs(((int) v) % n);
}

int compareInts(void *a, void *b) {
  return (int) a - (int) b;
}

unsigned long hashString(void *v, unsigned long n) {
  unsigned long val;
  char *s = (char *) v;
  for (val = 0; *s; s++)
    val = *s + 31 * val;
  return val % n;
}

int compareStrings(void *a, void *b) {
  return strcmp((char *) a, (char *) b);
}

real hashTableAvgBinSize(HashTable T) {
  unsigned long i, n, cost = 0;
  HashEntry E;
  if (!T) return 0.0;
  for (i = 0; i < T->size; i++) {
    for (E = T->bin[i], n = 0; E; E = E->next) n++;
    cost += n * n;
  }
  return SQRT((real) cost / T->entries);
}
