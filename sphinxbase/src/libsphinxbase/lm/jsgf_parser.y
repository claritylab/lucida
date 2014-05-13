/* -*- c-basic-offset:4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2007 Carnegie Mellon University.  All rights
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
%{
#define YYERROR_VERBOSE

#include <stdio.h>
#include <string.h>

#include <sphinxbase/hash_table.h>
#include <sphinxbase/ckd_alloc.h>
#include <sphinxbase/err.h>

#include "jsgf_internal.h"
#include "jsgf_parser.h"
#include "jsgf_scanner.h"

/* Suppress warnings from generated code */
#if defined _MSC_VER
#pragma warning(disable: 4273)
#endif

void yyerror(yyscan_t lex, jsgf_t *jsgf, const char *s);

%}

%pure-parser
%lex-param { void* yyscanner }
%parse-param { void* yyscanner }
%parse-param { jsgf_t *jsgf }

%union {
       char *name;
       float weight;
       jsgf_rule_t *rule;
       jsgf_rhs_t *rhs;
       jsgf_atom_t *atom;
}

%token           HEADER GRAMMAR IMPORT PUBLIC
%token <name>    TOKEN RULENAME TAG
%token <weight>  WEIGHT
%type  <atom>    rule_atom rule_item tagged_rule_item
%type  <rhs>     rule_expansion alternate_list
%type  <name>    grammar_header
%type  <rule>    rule_group rule_optional
%%

grammar: header
	| header rule_list
	| header import_header rule_list
	;

header: jsgf_header grammar_header { jsgf->name = $2; }
	;

jsgf_header: HEADER ';'
	| HEADER TOKEN ';' { jsgf->version = $2; }
	| HEADER TOKEN TOKEN ';' { jsgf->version = $2; jsgf->charset = $3; }
	| HEADER TOKEN TOKEN TOKEN ';' { jsgf->version = $2; jsgf->charset = $3;
					 jsgf->locale = $4; }
	;

grammar_header: GRAMMAR TOKEN ';' { $$ = $2; }
	;

import_header: import_statement
	| import_header import_statement
	;

import_statement: IMPORT RULENAME ';' { jsgf_import_rule(jsgf, $2); ckd_free($2); }
	;

rule_list: rule
	| rule_list rule
	;

rule: RULENAME '=' alternate_list ';' { jsgf_define_rule(jsgf, $1, $3, 0); ckd_free($1); }
| PUBLIC RULENAME '=' alternate_list ';'  { jsgf_define_rule(jsgf, $2, $4, 1); ckd_free($2); }
	;

alternate_list: rule_expansion { $$ = $1; $$->atoms = glist_reverse($$->atoms); }
	| alternate_list '|' rule_expansion { $$ = $3;
                                              $$->atoms = glist_reverse($$->atoms);
                                              $$->alt = $1; }
	;

rule_expansion: tagged_rule_item { $$ = ckd_calloc(1, sizeof(*$$));
				   $$->atoms = glist_add_ptr($$->atoms, $1); }
	| rule_expansion tagged_rule_item { $$ = $1;
					    $$->atoms = glist_add_ptr($$->atoms, $2); }
	;

tagged_rule_item: rule_item
	| tagged_rule_item TAG { $$ = $1;
				 $$->tags = glist_add_ptr($$->tags, $2); }
	;

rule_item: rule_atom
	| WEIGHT rule_atom { $$ = $2; $$->weight = $1; }
	;

rule_group: '(' alternate_list ')' { $$ = jsgf_define_rule(jsgf, NULL, $2, 0); }
	;

rule_optional: '[' alternate_list ']' { $$ = jsgf_optional_new(jsgf, $2); }
	;

rule_atom: TOKEN { $$ = jsgf_atom_new($1, 1.0); ckd_free($1); }
	| RULENAME { $$ = jsgf_atom_new($1, 1.0); ckd_free($1); }
	| rule_group { $$ = jsgf_atom_new($1->name, 1.0); }
	| rule_optional { $$ = jsgf_atom_new($1->name, 1.0); }
	| rule_atom '*' { $$ = jsgf_kleene_new(jsgf, $1, 0); }
	| rule_atom '+' { $$ = jsgf_kleene_new(jsgf, $1, 1); }
	;

%%

void
yyerror(yyscan_t lex, jsgf_t *jsgf, const char *s)
{
    E_ERROR("%s at line %d current token '%s'\n", s, yyget_lineno(lex), yyget_text(lex));
}
