/* MANAGED-BY-SYSTEM-BUILDER                                    */

/*
** User heap source file generated on Feb 22, 2008 at 17:31:53.
**
** Copyright (C) 2000-2006 Analog Devices Inc., All Rights Reserved.
**
** This file is generated automatically based upon the options selected
** in the LDF Wizard. Changes to the LDF configuration should be made by 
** changing the appropriate options rather than editing this file. 
**
** Configuration:-
**     crt_doj:                                .\Debug\bf561sphinx_basiccrt.doj
**     processor:                              ADSP-BF561
**     si_revision:                            automatic
**     cplb_init_cplb_ctrl:                    81
**     using_cplusplus:                        true
**     mem_init:                               false
**     use_vdk:                                false
**     use_eh:                                 true
**     use_argv:                               false
**     running_from_internal_memory:           true
**     user_heap_src_file:                     C:\work\pocketsphinx\visualdsp\pocketsphinx_continuous\bf561sphinx_heaptab.c
**     libraries_use_stdlib:                   true
**     libraries_use_fileio_libs:              true
**     libraries_use_ieeefp_emulation_libs:    false
**     libraries_use_eh_enabled_libs:          false
**     system_heap:                            L3
**     system_heap_size:                       16M
**     system_stack:                           L2
**     system_stack_size:                      31K
**     use_sdram:                              true
**     use_sdram_size:                         64M
**     use_sdram_partitioned:                  custom
**     num_user_heaps:                         1
**     user_heap0:                             L3
**     user_heap0_size:                        16M
**     user_heap0_heap_name:                   MyHeap
**     use_multicores:                         2
**     use_multicores_use_core:                multi_core
**
*/


extern "asm" int ldf_heap_space;
extern "asm" int ldf_heap_length;
extern "asm" int MyHeap_space;
extern "asm" int MyHeap_length;


struct heap_table_t
{
  void          *base;
  unsigned long  length;
  long int       userid;
};

#pragma file_attr("libData=HeapTable")
#pragma section("constdata")
struct heap_table_t heap_table[3] =
{


  { &ldf_heap_space, (int) &ldf_heap_length, 0 },
  { &MyHeap_space, (int) &MyHeap_length, 1 },


  { 0, 0, 0 }
};


