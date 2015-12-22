#ifndef PORTER_H
#define PORTER_H
struct stemmer;

struct stemmer *create_stemmer(void);
void free_stemmer(struct stemmer *z);

extern int stem(struct stemmer *z, char *b, int k);
int stem2(struct stemmer *z);

#define TRUE 1
#define FALSE 0

/* stemmer is a structure for a few local bits of data,
*/

struct stemmer {
  char *b; /* buffer for word to be stemmed */
  int k;   /* offset to the end of the string */
  int j;   /* a general offset into the string */
};

#endif /* PORTER_H*/
