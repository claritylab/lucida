#ifndef DR_HASH_H
#define DR_HASH_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hashTable *HashTable;

extern HashTable hashTableCreate(unsigned long initialSize, 
				 float maxAvgBinSize,
				 unsigned long (*hashFunc)(void *, 
							   unsigned long),
				 int (*entryCompare)(void *, void *),
				 int memType);
extern void hashTableClear(HashTable T);
extern void hashTableFree(HashTable T);
extern unsigned long hashVal(HashTable T, void *key);
extern char hashTableInsert(HashTable T, void *key, void *data, 
			    char allowDups);
extern char hashTableFastInsert(HashTable T, void *key, void *data, 
				unsigned long hashVal, char allowDups);
extern void *hashTableLookup(HashTable T, void *key);
extern void *hashTableFastLookup(HashTable T, void *key, 
				 unsigned long hashVal);
extern void *hashTableRemove(HashTable T, void *key);
extern void hashTableForEach(HashTable T, 
			     char (*func)(HashTable, void *, void *, void *), 
			     void *userData);
extern void hashTableForEachMatch(HashTable T, void *key, 
				  char (*func)(HashTable, void *, void *,
					       void *), 
				  void *userData);

extern unsigned long hashTableNumEntries(HashTable T);

extern unsigned long hashInt(void *v, unsigned long n);
extern int compareInts(void *a, void *b);
extern unsigned long hashString(void *s, unsigned long n);
extern int compareStrings(void *a, void *b);

extern real hashTableAvgBinSize(HashTable T);

#ifdef __cplusplus
}
#endif

#endif /* DR_HASH_H */
