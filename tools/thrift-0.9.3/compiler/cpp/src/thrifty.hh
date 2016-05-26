/* A Bison parser, made by GNU Bison 3.0.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2013 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef YY_YY_SRC_THRIFTY_HH_INCLUDED
# define YY_YY_SRC_THRIFTY_HH_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    tok_identifier = 258,
    tok_literal = 259,
    tok_doctext = 260,
    tok_st_identifier = 261,
    tok_int_constant = 262,
    tok_dub_constant = 263,
    tok_include = 264,
    tok_namespace = 265,
    tok_cpp_namespace = 266,
    tok_cpp_include = 267,
    tok_cpp_type = 268,
    tok_php_namespace = 269,
    tok_py_module = 270,
    tok_perl_package = 271,
    tok_java_package = 272,
    tok_xsd_all = 273,
    tok_xsd_optional = 274,
    tok_xsd_nillable = 275,
    tok_xsd_namespace = 276,
    tok_xsd_attrs = 277,
    tok_ruby_namespace = 278,
    tok_smalltalk_category = 279,
    tok_smalltalk_prefix = 280,
    tok_cocoa_prefix = 281,
    tok_csharp_namespace = 282,
    tok_delphi_namespace = 283,
    tok_void = 284,
    tok_bool = 285,
    tok_byte = 286,
    tok_string = 287,
    tok_binary = 288,
    tok_slist = 289,
    tok_senum = 290,
    tok_i16 = 291,
    tok_i32 = 292,
    tok_i64 = 293,
    tok_double = 294,
    tok_map = 295,
    tok_list = 296,
    tok_set = 297,
    tok_oneway = 298,
    tok_typedef = 299,
    tok_struct = 300,
    tok_xception = 301,
    tok_throws = 302,
    tok_extends = 303,
    tok_service = 304,
    tok_enum = 305,
    tok_const = 306,
    tok_required = 307,
    tok_optional = 308,
    tok_union = 309,
    tok_reference = 310
  };
#endif
/* Tokens.  */
#define tok_identifier 258
#define tok_literal 259
#define tok_doctext 260
#define tok_st_identifier 261
#define tok_int_constant 262
#define tok_dub_constant 263
#define tok_include 264
#define tok_namespace 265
#define tok_cpp_namespace 266
#define tok_cpp_include 267
#define tok_cpp_type 268
#define tok_php_namespace 269
#define tok_py_module 270
#define tok_perl_package 271
#define tok_java_package 272
#define tok_xsd_all 273
#define tok_xsd_optional 274
#define tok_xsd_nillable 275
#define tok_xsd_namespace 276
#define tok_xsd_attrs 277
#define tok_ruby_namespace 278
#define tok_smalltalk_category 279
#define tok_smalltalk_prefix 280
#define tok_cocoa_prefix 281
#define tok_csharp_namespace 282
#define tok_delphi_namespace 283
#define tok_void 284
#define tok_bool 285
#define tok_byte 286
#define tok_string 287
#define tok_binary 288
#define tok_slist 289
#define tok_senum 290
#define tok_i16 291
#define tok_i32 292
#define tok_i64 293
#define tok_double 294
#define tok_map 295
#define tok_list 296
#define tok_set 297
#define tok_oneway 298
#define tok_typedef 299
#define tok_struct 300
#define tok_xception 301
#define tok_throws 302
#define tok_extends 303
#define tok_service 304
#define tok_enum 305
#define tok_const 306
#define tok_required 307
#define tok_optional 308
#define tok_union 309
#define tok_reference 310

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE YYSTYPE;
union YYSTYPE
{
#line 73 "src/thrifty.yy" /* yacc.c:1909  */

  char*          id;
  int64_t        iconst;
  double         dconst;
  bool           tbool;
  t_doc*         tdoc;
  t_type*        ttype;
  t_base_type*   tbase;
  t_typedef*     ttypedef;
  t_enum*        tenum;
  t_enum_value*  tenumv;
  t_const*       tconst;
  t_const_value* tconstv;
  t_struct*      tstruct;
  t_service*     tservice;
  t_function*    tfunction;
  t_field*       tfield;
  char*          dtext;
  t_field::e_req ereq;
  t_annotation*  tannot;
  t_field_id     tfieldid;

#line 187 "src/thrifty.hh" /* yacc.c:1909  */
};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_SRC_THRIFTY_HH_INCLUDED  */
