#ifndef SENNA_TOKENIZER_H
#define SENNA_TOKENIZER_H

#include "SENNA_Hash.h"

typedef struct SENNA_Tokens_ {
  char **words;
  int *start_offset;
  int *end_offset;
  int *word_idx;
  int *caps_idx;
  int *suff_idx;
  int *gazl_idx;
  int *gazm_idx;
  int *gazo_idx;
  int *gazp_idx;
  int n;

} SENNA_Tokens;

typedef struct SENNA_Tokenizer_ {
  /* true if text already tokenized */
  int is_tokenized;

  /* hashes */
  SENNA_Hash *word_hash;
  SENNA_Hash *caps_hash;
  SENNA_Hash *suff_hash;
  SENNA_Hash *gazt_hash;
  SENNA_Hash *gazl_hash;
  SENNA_Hash *gazm_hash;
  SENNA_Hash *gazo_hash;
  SENNA_Hash *gazp_hash;

  /* buffers */
  SENNA_Tokens *tokens;
  int *words_sizes;
  int max_tokens;

  char *sentence0n;
  int *offset0n2raw;
  int max_sentence0n_size;

  char *entity;
  int max_entity_size;

  /* common indices */
  int word_hash_unknown_idx;
  int suff_hash_nosuffix_idx;
  int caps_hash_allcaps_idx;
  int caps_hash_hascap_idx;
  int caps_hash_initcap_idx;
  int caps_hash_nocaps_idx;
  int gazt_hash_no_entity_idx;
  int gazt_hash_is_entity_idx;

} SENNA_Tokenizer;

SENNA_Tokenizer *SENNA_Tokenizer_new(
    SENNA_Hash *word_hash, SENNA_Hash *caps_hash, SENNA_Hash *suff_hash,
    SENNA_Hash *gazt_hash, SENNA_Hash *gazl_hash, SENNA_Hash *gazm_hash,
    SENNA_Hash *gazo_hash, SENNA_Hash *gazp_hash, int is_tokenized);

void SENNA_Tokenizer_free(SENNA_Tokenizer *tokenizer);

SENNA_Tokens *SENNA_Tokenizer_tokenize(SENNA_Tokenizer *tokenizer,
                                       const char *sentence);

/* cool functions */
void SENNA_tokenize_untilspace(int *size_, const char *sentence);
void SENNA_tokenize_alphanumeric(int *size_, const char *sentence);
void SENNA_tokenize_dictionarymatch(int *size_, int *idxhash_, SENNA_Hash *hash,
                                    const char *sentence);
void SENNA_tokenize_number(int *size_, const char *sentence);

#endif
