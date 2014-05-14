#!/bin/python

import string
import sys
import re
import os


TRUE = 1
FALSE = 0

if( len(sys.argv) == 1 ):
	print "Usage: add-parent-phrase-and-head-as-feature.py <data-file> <phrase-index> <parent-phrase-concat-ordinal-index> <parent-phrase-start-index> <parent-phrase-end-index> <head-word-index> <head-word-pos-index>"
	print "FOR THIS SCRIPT TO WORK, THE NODES MUST BE ARRANGED IN ASCENDING ORDER OF NODE START-END INDICES"
	sys.exit(1)

infile = open(sys.argv[1])
phrase_index = int(sys.argv[2])
parent_phrase_concat_ordinal_index = int(sys.argv[3])
parent_phrase_start_index = int(sys.argv[4])
parent_phrase_end_index = int(sys.argv[5])
head_word_index = int(sys.argv[6])
head_word_pos_index = int(sys.argv[7])

feature_vector_list = []
modified_feature_vector_list = []

phrase_counter_hash = {}

a_feature_vector = infile.readline()
left_sibling_index = 0

while ( a_feature_vector != "" ):
	if( a_feature_vector == "\012" ):
		modified_feature_vector_list = []

		#--- the entire examples is read in the feature_vector_list.  let's process it ---#
		for i in range(0, len(feature_vector_list)):
			found_left_sibling_flag = FALSE
			found_right_sibling_flag = FALSE

			for j in range(i-1, -1, -1):
				if( (feature_vector_list[i][parent_phrase_concat_ordinal_index] == feature_vector_list[j][parent_phrase_concat_ordinal_index])
#					and
#					(int(feature_vector_list[j][parent_phrase_start_index]) <= int(feature_vector_list[i][2])) and
#					(int(feature_vector_list[j][parent_phrase_end_index]) >= int(feature_vector_list[i][3]))
					):
					found_left_sibling_flag = TRUE
					left_sibling_index = j
					break

			if( found_left_sibling_flag == FALSE ):
				left_sibling_phrase = "U"
				left_sibling_head_word = "U"
				left_sibling_head_word_pos = "U"
			else:
				left_sibling_phrase = feature_vector_list[left_sibling_index][phrase_index]
				left_sibling_head_word = feature_vector_list[left_sibling_index][head_word_index]
				left_sibling_head_word_pos = feature_vector_list[left_sibling_index][head_word_pos_index]


			for j in range(i+1, len(feature_vector_list)):
				if( (feature_vector_list[i][parent_phrase_concat_ordinal_index] == feature_vector_list[j][parent_phrase_concat_ordinal_index])
#					and
#					(int(feature_vector_list[j][parent_phrase_start_index]) <= int(feature_vector_list[i][2])) and
#					(int(feature_vector_list[j][parent_phrase_end_index]) >= int(feature_vector_list[i][3]))
					):
					found_right_sibling_flag = TRUE
					right_sibling_index = j
					break

			if( found_right_sibling_flag == FALSE ):
				right_sibling_phrase = "U"
				right_sibling_head_word = "U"
				right_sibling_head_word_pos = "U"
			else:
				right_sibling_phrase = feature_vector_list[right_sibling_index][phrase_index]
				right_sibling_head_word = feature_vector_list[right_sibling_index][head_word_index]
				right_sibling_head_word_pos = feature_vector_list[right_sibling_index][head_word_pos_index]


			#--- a copy of the feature vector list is necessary ---#
			some_list = [] + feature_vector_list[i]
			modified_feature_vector_list = modified_feature_vector_list + [some_list]


			#--- the indices become 1, 2, 3, etc. because the length of the list dynamically increases ---#
			modified_feature_vector_list[-1][phrase_index+1:phrase_index+1] = [left_sibling_phrase]
			modified_feature_vector_list[-1][phrase_index+2:phrase_index+2] = [left_sibling_head_word]
			modified_feature_vector_list[-1][phrase_index+3:phrase_index+3] = [left_sibling_head_word_pos]

			modified_feature_vector_list[-1][phrase_index+4:phrase_index+4] = [right_sibling_phrase]
			modified_feature_vector_list[-1][phrase_index+5:phrase_index+5] = [right_sibling_head_word]
			modified_feature_vector_list[-1][phrase_index+6:phrase_index+6] = [right_sibling_head_word_pos]
			
		#--- after processing, print the list of feature vectors to std output---#
		for list in modified_feature_vector_list:
			print string.join(list)
		print

		#--- empty the feature vector list ---#
		feature_vector_list = []
		modified_feature_vector_list = []
		
	else:
		#--- if it is a legitimate feature vector (no blank line or end of file), then add it to the list of feature vectors ---#
		if( a_feature_vector != "" and a_feature_vector != "\012" ):
			a_feature_vector_list = string.split(a_feature_vector)
			feature_vector_list.append(a_feature_vector_list)

	a_feature_vector = infile.readline()

		
# a_feature_vector = infile.readline()

# while ( a_feature_vector != "" ):
# 	if( a_feature_vector == "\012" ):
# 		modified_feature_vector_list = []

# 		#--- the entire examples is read in the feature_vector_list.  let's process it ---#
# 		for i in range(0, len(feature_vector_list)):



# 			#--- a copy of the feature vector list is necessary ---#
# 			some_list = [] + feature_vector_list[j]
# 			modified_feature_vector_list = modified_feature_vector_list + [some_list]


# 			#--- the indices become 1, 2, 3, etc. because the length of the list dynamically increases ---#
# 			modified_feature_vector_list[-1][phrase_index:phrase_index] = [str(counter)]

# 		#--- after processing, print the list of feature vectors to std output---#
# 		for list in modified_feature_vector_list:
# 			print string.join(list)
# 		print

# 		sys.exit(1)
# 		#--- empty the feature vector list ---#
# 		feature_vector_list = []
# 		modified_feature_vector_list = []
		
# 	else:
# 		#--- if it is a legitimate feature vector (no blank line or end of file), then add it to the list of feature vectors ---#
# 		if( a_feature_vector != "" and a_feature_vector != "\012" ):
# 			a_feature_vector_list = string.split(a_feature_vector)
# 			feature_vector_list.append(a_feature_vector_list)

# 	a_feature_vector = infile.readline()

		
