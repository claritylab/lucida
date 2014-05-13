/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2013 Carnegie Mellon University.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * This work was supported in part by funding from the Defense Advanced 
 * Research Projects Agency and the National Science Foundation of the 
 * United States of America, and the CMU Sphinx Speech Consortium.
 *
 * THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND 
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
 * NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ====================================================================
 *
 */


%extend NGramModel {
    NGramModel(const char*path) {
        logmath_t *lmath = logmath_init(1.0001, 0, 0);
        ngram_model_t * model = ngram_model_read(NULL, path, NGRAM_AUTO, lmath);
        logmath_free(lmath);
        return model;
    }

    NGramModel(Config *config, LogMath *logmath, const char *path) {
        return ngram_model_read(config, path, NGRAM_AUTO, logmath);
    }

    ~NGramModel() {
        ngram_model_free($self);
    }

    void write(const char *path, ngram_file_type_t ftype, int *errcode) {
        *errcode = ngram_model_write($self, path, ftype);
    }

    ngram_file_type_t str_to_type(const char *str) {
        return ngram_str_to_type(str);
    }

    const char * type_to_str(int type) {
        return ngram_type_to_str(type);
    }

    void recode(const char *src, const char *dst, int *errcode) {
        *errcode = ngram_model_recode($self, src, dst);
    }

    void casefold(int kase, int *errcode) {
        *errcode = ngram_model_casefold($self, kase);
    }

    int32 size() {
        return ngram_model_get_size($self);
    }

    int32 add_word(const char *word, float32 weight) {
        return ngram_model_add_word($self, word, weight);
    }

    int32 add_class(const char *c, float32 w, size_t n, char **ptr,
                    const float32 *weights)
    {
        return ngram_model_add_class($self, c, w, ptr, weights, n);
    }

    int32 prob(size_t n, const char * const *ptr) {
        return ngram_prob($self, ptr, n);
    }
}

// TODO: shares ptr type with NGramModel, docstrings are not generated
%extend NGramModelSet {
    NGramModelSet(Config *config, LogMath *logmath, const char *path) {
        return ngram_model_set_read(config, path, logmath);
    }

    ~NGramModelSet() {
        ngram_model_free($self);
    }

    int32 count() {
        return ngram_model_set_count($self);
    }

    NGramModel * add(
        NGramModel *model, const char *name, float weight, bool reuse_widmap) {
        return ngram_model_set_add($self, model, name, weight, reuse_widmap);
    }

    NGramModel * select(const char *name) {
        return ngram_model_set_select($self, name);
    }

    NGramModel * lookup(const char *name) {
        return ngram_model_set_lookup($self, name);
    }

    const char * current() {
        return ngram_model_set_current($self);
    }
}

%runtime %{
ngram_model_t * next_NGramModelSetIterator(ngram_model_set_iter_t *iter)
{
    const char *name;
    return ngram_model_set_iter_model(iter, &name);
}
%}

/* vim: set ts=4 sw=4: */
