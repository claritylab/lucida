#!/bin/python

import string
import sys
import re
import os

if( len(sys.argv) == 1 ):
	print "Usage: sub-np-head-in-pp.py <old-features-file> <phrase-type-column> <head-column> <head-pos-column>"
	print "outputs new features file to stdout"
	sys.exit(1)

TRUE  = 1
FALSE = 0	

infile = open(sys.argv[1])
phrase_column = int(sys.argv[2])
head_column     = int(sys.argv[3])
head_pos_column = int(sys.argv[4])

outfile_lines = []
example_feature_list = []
modified_examples_list = []
example_line = infile.readline()

while( example_line != "" ):
	if( len(string.strip(example_line)) == 0 ):
		#--- do the processing for replacing the head of a pp with that of the np inside it ---#
		i=0
		j=0
		for i in range(0, len(example_feature_list)):
			if( example_feature_list[i][phrase_column] == "PP" ):
				pp_start = int(example_feature_list[i][2])
				pp_end   = int(example_feature_list[i][3])

				NP_FOUND_FLAG = FALSE
				
				j=i+1
				while(j < len(example_feature_list)):
					if( (example_feature_list[j][phrase_column] == "NP") and
						(int(example_feature_list[j][2]) >= pp_start) and
						(int(example_feature_list[j][3]) <= pp_end)
					  ):
						NP_FOUND_FLAG = TRUE
						example_feature_list[i][phrase_column] = example_feature_list[i][phrase_column] + "-" + example_feature_list[i][head_column]
						example_feature_list[i][head_column] = example_feature_list[j][head_column]
						example_feature_list[i][head_pos_column] = example_feature_list[j][head_pos_column]
						break
					j=j+1

				#if( NP_FOUND_FLAG == FALSE ):
				#	print >>sys.stderr, "Did not find NP in PP for example: " + example_feature_list[i][0]
				#else:
				#	print >>sys.stderr, "Found NP in PP for example: " + example_feature_list[i][0]
			print string.join(example_feature_list[i])
		print
		#if(h>2000):
		#	sys.exit(1)
		example_feature_list = []
	else:
		example_feature_list = example_feature_list + [string.split(example_line)]
	example_line = infile.readline()
