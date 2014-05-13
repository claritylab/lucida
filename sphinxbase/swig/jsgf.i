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


%extend Jsgf {
#if SWIGJAVA
  %rename(name) getName;
#endif

  Jsgf(const char *path) {
    return jsgf_parse_file(path, NULL);
  }

  ~Jsgf() {
    jsgf_grammar_free($self);
  }

  const char * name() {
    return jsgf_grammar_name($self);
  }

  JsgfRule * get_rule(const char *name) {
    return jsgf_get_rule($self, name);
  }

  FsgModel * build_fsg(JsgfRule *rule, LogMath *logmath, float lw) {
    return jsgf_build_fsg($self, rule, logmath, lw);
  }
}

%extend JsgfRule {
#if SWIGJAVA
  %rename(getName) name;
  %rename(isPublic) public;

  %javamethodmodifiers JsgfRule "private";
#endif

  JsgfRule() {
    return NULL;
  }

  ~JsgfRule() {
  }

  const char * name() {
    return jsgf_rule_name($self);
  }

  bool public() {
    return jsgf_rule_public($self);
  }
}

%runtime %{
jsgf_rule_t * next_JsgfIterator(jsgf_rule_iter_t *iter)
{
    return jsgf_rule_iter_rule(iter);
}
%}

/* vim: set ts=4 sw=4: */
