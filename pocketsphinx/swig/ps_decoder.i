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


// TODO: check for multiple values
%extend Decoder {
    /* Following functions have no bindings:
     * ps_mllr_t *ps_update_mllr - requires
     * int ps_decode_senscr
     * int ps_process_cep
     */

    Decoder() {
        Decoder *d = ps_init(cmd_ln_init(NULL, ps_args(), FALSE, NULL));
        return d;
    }

    Decoder(Config *config) {
        Decoder *decoder = ps_init(config);
        return decoder;
    }

    ~Decoder() {
        ps_free($self);
    }

    void reinit(Config *config, int *errcode) {
        *errcode = ps_reinit($self, config);
    }

    void load_dict(
        char const *fdict, char const *ffilter, char const *format, int *errcode) {
        *errcode = ps_load_dict($self, fdict, ffilter, format);
    }

    void save_dict(char const *dictfile, char const *format, int *errcode) {
        *errcode = ps_save_dict($self, dictfile, format);
    }

    void add_word(char const *word, char const *phones, int update, int *errcode) {
        *errcode = ps_add_word($self, word, phones, update);
    }

    Lattice * get_lattice() {
        return ps_lattice_retain(ps_get_lattice($self));
    }

    Config *get_config() {
        return cmd_ln_retain(ps_get_config($self));
    }

    static Config *default_config() {
        return cmd_ln_parse_r(NULL, ps_args(), 0, NULL, FALSE);
    }

    static Config *file_config(char const * path) {
        return cmd_ln_parse_file_r(NULL, ps_args(), path, FALSE);
    }

    void start_utt(char const *uttid, int *errcode) {
        *errcode = ps_start_utt($self, uttid);
    }

    char const *get_uttid() {
        return ps_get_uttid($self);
    }

    void end_utt(int *errcode) {
        *errcode = ps_end_utt($self);
    }

    int
#ifdef SWIGPYTHON
    process_raw(const void *SDATA, size_t NSAMP, bool no_search, bool full_utt,
                int *errcode) {
        NSAMP /= sizeof(int16);
#else
    process_raw(const int16 *SDATA, size_t NSAMP, bool no_search, bool full_utt,
                int *errcode) {
#endif
        return *errcode = ps_process_raw($self, SDATA, NSAMP, no_search, full_utt);
    }

    int decode_raw(FILE *fin, int *errcode) {
        *errcode = ps_decode_raw($self, fin, 0, -1);
        return *errcode;
    }

    %newobject hyp;
    Hypothesis * hyp() {
        char const *hyp, *uttid;
        int32 best_score;
        hyp = ps_get_hyp($self, &best_score, &uttid);
        return hyp ? new_Hypothesis(hyp, uttid, best_score) : NULL;
    }

    %newobject nbest;
    NBest * nbest() {
        return new_NBest(ps_nbest($self, 0, -1, NULL, NULL));
    }

    %newobject seg;
    Segment * seg() {
        int32 best_score;
        return new_Segment(ps_seg_iter($self, &best_score));
    }

    FrontEnd * get_fe() {
        return ps_get_fe($self);
    }

    Feature * get_feat() {
        return ps_get_feat($self);
    }
   
    bool get_vad_state() {
        return ps_get_vad_state($self);
    }

    FsgModel * get_fsg(const char *name) {
        return fsg_model_retain(ps_get_fsg($self, name));
    }

    void set_fsg(const char *name, FsgModel *fsg, int *errcode) {
        *errcode = ps_set_fsg($self, name, fsg);
    }

    void set_jsgf_file(const char *name, const char *path, int *errcode) {
        *errcode = ps_set_jsgf_file($self, name, path);
    }

    const char * get_kws(const char *name) {
        return ps_get_kws($self, name);
    }

    void set_kws(const char *name, const char *keyfile, int *errcode) {
        *errcode = ps_set_kws($self, name, keyfile);
    }

    void set_keyphrase(const char *name, const char *keyphrase, int *errcode) {
        *errcode = ps_set_keyphrase($self, name, keyphrase);
    }

    NGramModel * get_lm(const char *name) {
        return ngram_model_retain(ps_get_lm($self, name));
    }

    void set_lm(const char *name, NGramModel *lm, int *errcode) {
        *errcode = ps_set_lm($self, name, lm);
    }

    void set_lm_file(const char *name, const char *path, int *errcode) {
        *errcode = ps_set_lm_file($self, name, path);
    }

    LogMath * get_logmath() {
        return logmath_retain(ps_get_logmath($self));
    }

    void set_search(const char *search_name, int *errcode) {
      *errcode = ps_set_search($self, search_name);
    }

    const char * get_search() {
        return ps_get_search($self);
    }

    int n_frames() {
        return ps_get_n_frames($self);
    }
}

/* vim: set ts=4 sw=4: */
