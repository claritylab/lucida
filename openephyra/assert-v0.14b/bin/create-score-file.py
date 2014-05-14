#!/bin/python

import string
import sys
import re
import os

TRUE  = 1
FALSE = 0
DEBUG = FALSE

if( len(sys.argv) == 1 ):
	print "Usage: create-score-file.py <svm-scores-file>"
	sys.exit(1)

score_file_lines       = open(sys.argv[1]).readlines()

ref_array = []
hyp_array = []

if( len(score_file_lines) == 0 ):
	exit

#--- find out the index of ref and hyp columns and the index of the first after hyp ---#
some_list   = string.split(score_file_lines[0])

i=0
start_score_index = 0
hyp_index         = 0
ref_index         = 0
for i in range(4, len(some_list)):
	if( len(re.findall(r"/",some_list[i])) == 1 ):
		start_score_index = i
		hyp_index         = i-1
		ref_index         = i-2
		break


#print >>sys.stderr, ref_index
#print >>sys.stderr, hyp_index
#print >>sys.stderr, start_score_index

#--- delete all the lines in the score lines that contain both the hypothesis and the reference as null ---#
NON_NULL_FLAG = FALSE
score_file_lines_no_nulls = []
o=0
for o in range(0, len(score_file_lines)):
	if( len(string.strip(score_file_lines[o])) == 0 ):
		if( NON_NULL_FLAG == FALSE ):
			#--- add a dummy one example for this sentence ---#
			score_file_lines_no_nulls.append(some_null_hyp_ref_line)

		#--- then add the blank line ---#
	   	score_file_lines_no_nulls.append(score_file_lines[o])

		NON_NULL_FLAG = FALSE
		continue

	token_list = string.split(score_file_lines[o])

	if( token_list[hyp_index] == "O" and token_list[ref_index] == "O" ):
		some_null_hyp_ref_line = score_file_lines[o]
		continue
	else:
		NON_NULL_FLAG = TRUE
		score_file_lines_no_nulls.append(score_file_lines[o])


if( DEBUG == TRUE ):
	for line in score_file_lines:
		print string.strip(line)

	print "---------------------------------"

	for line in score_file_lines_no_nulls:
		print string.strip(line)

score_file_lines = [] + score_file_lines_no_nulls		

for line in score_file_lines:
	token_list = string.split(line)

	if(len(string.strip(line)) == 0):
		print " %s |%s| |%s|" % (sentence_id, string.join(ref_array, "|"), string.join(hyp_array, "|"))
		ref_array = []
		hyp_array = []
		
	else:
		sentence_id = token_list[0]
		#print >>sys.stderr, sentence_id

		if(token_list[ref_index] == "O"):
		   token_list[ref_index] = ""
		   
		if(token_list[hyp_index] == "O"):
		   token_list[hyp_index] = ""
		   
		ref_array.append(token_list[ref_index])
		hyp_array.append(token_list[hyp_index])


