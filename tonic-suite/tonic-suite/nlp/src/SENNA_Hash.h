#ifndef SENNA_HASH_H
#define SENNA_HASH_H

typedef struct SENNA_Hash_ {
  char *is_admissible_key;
  char **keys;
  int size;
  char is_sorted;

} SENNA_Hash;

SENNA_Hash *SENNA_Hash_new(const char *path, const char *filename);
SENNA_Hash *SENNA_Hash_new_with_admissible_keys(
    const char *path, const char *filename,
    const char *admissible_keys_filename);

int SENNA_Hash_index(SENNA_Hash *hash, const char *key);
const char *SENNA_Hash_key(SENNA_Hash *hash, int idx);
void SENNA_Hash_convert_IOBES_to_brackets(SENNA_Hash *hash);
void SENNA_Hash_convert_IOBES_to_IOB(SENNA_Hash *hash);
int SENNA_Hash_size(SENNA_Hash *hash);
char SENNA_Hash_is_admissible_index(SENNA_Hash *hash, int idx);

void SENNA_Hash_free(SENNA_Hash *hash);

#endif
