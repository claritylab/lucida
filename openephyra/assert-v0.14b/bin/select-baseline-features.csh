#!/bin/csh

#---------------------------------------------
# using this script:
# 1. copy it to a new file 
# 2. change the variable type
# 3. select the required columns
#
# 1                         2 3 4 5            6  7 8    9 101112 13     14 15     1617 1819 20            21    22 23 24 25 2627282930313233 
# wsj/02/wsj_0200.mrg.0.17 13 0 0 revitalized 23 IN IN-3 U U U NP review NP PP-3-t 0 23 3 PP revitalize.01 FRAME in in IN IN U U U 0 0 0 b IN<-PP->NP->PP->NP->PP->NP->PRN->S->NP->VBN 
#
# 34     35363738          39404142 43 44 45 46              47484950515253545556      57      58          59       60   61  62  63  64  65  66676869  70 71727374  75
# IN<-PP U U U NP->VBN-NNS 0 0 0 In In IN IN verb-cluster-19 a 0 0 0 0 0 0 0 o goodman theatre revitalized classics take NNP NNP VBN NNS VBP z e d zed ed U i n Uin in 
#
# 76 77 78                                  79                              80         81         82          83          84          85           86          87          88
# IN PP NP->PP->NP->PP->NP->PRN->S->NP->VBN I<-P->N->P->N->P->N->P->S->N->V IN<-PP->NP PP->NP->PP >NP->PP->NP >PP->NP->PP >NP->PP->NP >PP->NP->PRN >NP->PRN->S >PRN->S->NP >S->NP->VBN 
#
# 89         90       91     92    93    94    95    96    97       98       99       100       101      102      103   104                            05060708091011121314151617181920 
# >NP->VBN-U >VBN-U-U IN-U-U U-U-U U-U-U U-U-U U-U-U U-U-U NP-PP-NP PP-NP-PP NP-PP-NP PP-NP-PRN NP-PRN-S PRN-S-NP <-->S *<-*->*->*->*->*->*->*->S->*->* 1 U U U 0 0 0 U U U U U U 0 0 0 
#
# 21222324   25  26 27   28   29  30 31   32   33  34 35  36   37  38 39  4041
# U U U </s> <s> an oct. </s> <s> an oct. </s> <s> DT NNP </s> <s> DT NNP 0 IN-in<-PP-in->NP-review->PP-of->NP-misanthrope->PP-at->NP-theatre->PRN-take->S-take->NP-classics->VBN-revitalized 
#
# 42                                          43                                                                                                                              07
# IN<-PP->NP->PP->NP->PP->NP->PRN->S->NP->VBN 1 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 1 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
#
# 08091011121314151617 18 19 20 21 22 23 24 25 26 27 282930
# U U U U U U U U U IN PP NP PP NP PP NP PRN S NP VBN U 0 O
#
#
#  5 dominating-verb
#- 6 tree distance of phrase from target
#- 7 phrase type
#- 8 concatination phrase ordinal tree distance
#- 9 left sibling phrase
#-10 left sibling head word
#-11 left sibling head word pos
#-12 right sibling phrase
#-13 right sibling head word
#-14 right sibling head word pos
# 15 parent ordinal distance concat
# 16 parent start
# 17 parent end
# 18 phrase ordinal tree distance from target
#-19 parent-phrase-type
#-20 target
# 21 frame
#-22 head word
#-23 parent-head-word
#-24 head word pos
#-25 parent-head-word-pos
#-26 light-verb
#-27 being-verb
#-28 verb
#-29 is-light-verb
#-30 is-being-verb
#-31 is-verb
#-32 binary position
#-33 broken path
#-42
#-43 path
# 44 half-path
#-45 path-to-light-verb
#-46 path-to-being-verb
#-47 path-to-any-verb
#-48 verb subcategorization
# 49 genetive
# 50 genetive-phrase
# 51 plural
#-52 first word in constituent
#-53 last word in constituent
#-54 first pos in constituent
# 55 last pos in constituent
# 56 noun cluster class
#-57 voice
# 58 named entity flags (7)
# 59 ..
# ..
# 150 TMP binary
# 243 class
#---------------------------------------------#

#--- change this to select the required fields ---#
set file = $1
awk '{if(NF==0) print; else {if($16=="a") $16="after"; if($16=="b") $16="before"; if($31=="a") $31="ACT"; if($31=="p") $31=PASS; print $1" "$2" "$3" "$4" "$5" "$6" "$8" "$16" "$17" "$22" "$30" "$31" "$NF}}'  $file

/bin/rm -rf head-map-file head-tagged-fn-file sentences
