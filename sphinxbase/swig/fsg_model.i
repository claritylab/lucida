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


// TODO: search for functions returning error code
%extend FsgModel {
    FsgModel(fsg_model_t *ptr) {
        return ptr;
    }

    FsgModel(char const *path, LogMath *logmath, float w) {
        return fsg_model_readfile(path, logmath, w);
    }

    ~FsgModel() {
        fsg_model_free($self);
    }

    int word_id(const char *word) {
        return fsg_model_word_id($self, word);
    }

    int word_add(const char *word) {
        return fsg_model_word_add($self, word);
    }

    void trans_add(int32 src, int32 dst, int32 logp, int32 wid) {
        fsg_model_trans_add($self, src, dst, logp, wid);
    }

    int32 null_trans_add(int32 src, int32 dst, int32 logp) {
        return fsg_model_null_trans_add($self, src, dst, logp);
    }

    int32 tag_trans_add(int32 src, int32 dst, int32 logp, int32 wid) {
        return fsg_model_tag_trans_add($self, src, dst, logp, wid);
    }

    int add_silence(const char *silword, int state, int32 silprob) {
        return fsg_model_add_silence($self, silword, state, silprob); 
    }

    int add_alt(const char *baseword, const char *altword) {
        return fsg_model_add_alt($self, baseword, altword);
    }

    void write(FILE *file) {
        fsg_model_write($self, file);
    }

    void writefile(const char *path) {
        fsg_model_writefile($self, path);
    }
}
