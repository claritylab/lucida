#include "SENNA_utils.h"
#include "SENNA_Hash.h"

#define MAX_KEY_SIZE 256

SENNA_Hash *SENNA_Hash_new(const char *path, const char *filename) {
  FILE *f;
  SENNA_Hash *hash;
  char **keys = NULL;
  int n_keys;
  char key[MAX_KEY_SIZE];
  int i;

  SENNA_message("loading hash: %s%s", (path ? path : ""),
                (filename ? filename : ""));

  f = SENNA_fopen(path, filename, "rt"); /* the t is to comply with Windows */
  n_keys = 0;
  while (fgets(key, MAX_KEY_SIZE, f)) n_keys++;
  SENNA_fclose(f);
  keys = SENNA_malloc(n_keys, sizeof(char *));

  f = SENNA_fopen(path, filename, "rt"); /* the t is to comply with Windows */
  n_keys = 0;
  while (fgets(key, MAX_KEY_SIZE, f)) {
    int key_size = strlen(key);
    key[key_size - 1] = '\0'; /* discard the newline */
    keys[n_keys] = SENNA_malloc(key_size, sizeof(char));
    strcpy(keys[n_keys], key);
    n_keys++;
  }
  SENNA_fclose(f);

  hash = SENNA_malloc(sizeof(SENNA_Hash), 1);
  hash->keys = keys;
  hash->size = n_keys;
  hash->is_admissible_key = NULL;

  /* sorted or unsorted hash ? */
  /* (unsorted cannot return an index for a key) */
  hash->is_sorted = 1;
  for (i = 0; i < n_keys - 1; i++) {
    if (strcmp(keys[i], keys[i + 1]) >= 0) {
      hash->is_sorted = 0;
      break;
    }
  }

  return hash;
}

SENNA_Hash *SENNA_Hash_new_with_admissible_keys(
    const char *path, const char *filename,
    const char *admissible_keys_filename) {
  SENNA_Hash *hash = SENNA_Hash_new(path, filename);
  FILE *f;
  int admissiblekeyssize = 0;

  f = SENNA_fopen(path, admissible_keys_filename, "rb");
  SENNA_fseek(f, 0, SEEK_END);
  admissiblekeyssize = SENNA_ftell(f);

  if (admissiblekeyssize != hash->size)
    SENNA_error("inconsistent hash and admissible key files");

  SENNA_fseek(f, 0, SEEK_SET);
  hash->is_admissible_key = SENNA_malloc(sizeof(char), admissiblekeyssize);
  SENNA_fread(hash->is_admissible_key, 1, admissiblekeyssize, f);
  SENNA_fclose(f);

  return hash;
}

void SENNA_Hash_convert_IOBES_to_IOB(SENNA_Hash *hash) {
  int i;

  for (i = 0; i < hash->size; i++) {
    char *key = hash->keys[i];
    if (strlen(key) < 3) continue;

    if ((key[0] == 'E') && (key[1] == '-')) key[0] = 'I';

    if ((key[0] == 'S') && (key[1] == '-')) key[0] = 'B';
  }
}

void SENNA_Hash_convert_IOBES_to_brackets(SENNA_Hash *hash) {
  int i, j;

  for (i = 0; i < hash->size; i++) {
    char *key = hash->keys[i];
    int key_size = strlen(key);

    if (!strcmp(key, "O")) key[0] = '*';

    if (key_size < 3) continue;

    if ((key[0] == 'B') && (key[1] == '-')) {
      key[0] = '(';
      for (j = 1; j < key_size - 1; j++) key[j] = key[j + 1];
      key[key_size - 1] = '*';
    }

    if ((key[0] == 'I') && (key[1] == '-')) {
      key[0] = '*';
      key[1] = '\0';
    }

    if ((key[0] == 'E') && (key[1] == '-')) {
      key[0] = '*';
      key[1] = ')';
      key[2] = '\0';
    }

    if ((key[0] == 'S') && (key[1] == '-')) {
      key = SENNA_realloc(key, key_size + 2, sizeof(char));

      key[0] = '(';
      for (j = 1; j < key_size - 1; j++) key[j] = key[j + 1];
      key[key_size - 1] = '*';
      key[key_size] = ')';
      key[key_size + 1] = '\0';

      hash->keys[i] = key;
    }
  }
}

void SENNA_Hash_free(SENNA_Hash *hash) {
  int i;

  for (i = 0; i < hash->size; i++) SENNA_free(hash->keys[i]);
  SENNA_free(hash->keys);
  if (hash->is_admissible_key) SENNA_free(hash->is_admissible_key);
  SENNA_free(hash);
}

int SENNA_Hash_index(SENNA_Hash *hash, const char *key) {
  char **keys = hash->keys;
  int idxinf = 0;
  int idxsup = hash->size - 1;

  if (!hash->is_sorted)
    SENNA_error("cannot search a key into an unsorted hash");

  if (strcmp(key, keys[idxinf]) < 0 || strcmp(key, keys[idxsup]) > 0) return -1;

  while (idxinf <= idxsup) {
    int idxmiddle = (idxsup + idxinf) / 2;
    int status = strcmp(key, keys[idxmiddle]);
    /*    printf("%d %d %d [%d]\n", idxinf, idxmiddle, idxsup, status); */
    if (status < 0)
      idxsup = idxmiddle - 1;
    else if (status > 0)
      idxinf = idxmiddle + 1;
    else
      return idxmiddle;
  }

  return -1;
}

const char *SENNA_Hash_key(SENNA_Hash *hash, int idx) {
  if ((idx < 0) || (idx >= hash->size)) SENNA_error("hash index out of bounds");

  return hash->keys[idx];
}

int SENNA_Hash_size(SENNA_Hash *hash) { return hash->size; }

char SENNA_Hash_is_admissible_index(SENNA_Hash *hash, int idx) {
  if (!hash->is_admissible_key)
    SENNA_error("hash does not handle admissible keys");

  if ((idx < 0) || (idx >= hash->size)) SENNA_error("hash index out of bounds");

  return hash->is_admissible_key[idx];
}
