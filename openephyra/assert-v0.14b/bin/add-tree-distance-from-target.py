#!/bin/python

import string
import sys
import re
import os


TRUE = 1
FALSE = 0

if( len(sys.argv) == 1 ):
	print "Usage: add-parent-phrase-and-head-as-feature.py <data-file> <phrase-index> <head-index>"
	print "FOR THIS SCRIPT TO WORK, THE NODES MUST BE ARRANGED IN ASCENDING ORDER OF NODE START-END INDICES"
	print "ALSO, NOW, THE TARGET NODE HAS TO BE PRESENT IN THE LIST OF NODES"
	sys.exit(1)

infile = open(sys.argv[1])
phrase_index = int(sys.argv[2])
head_index = int(sys.argv[3])

feature_vector_list = []
modified_feature_vector_list = []

phrase_counter_hash = {}

a_feature_vector = infile.readline()

while ( a_feature_vector != "" ):
	if( a_feature_vector == "\012" ):
		modified_feature_vector_list = []
		#--- the entire examples is read in the feature_vector_list.  let's process it ---#
		for i in range(0, len(feature_vector_list)):
			if( int(feature_vector_list[i][2]) < int(feature_vector_list[i][1]) ):
				continue
			else:
				break

		target_phrase_index = i

		dominating_verb = "U"
		for k in range(target_phrase_index, 0, -1):
			if( feature_vector_list[k][phrase_index] == "AUX" or feature_vector_list[k][phrase_index][0:2] == "VB" ):
				dominating_verb = feature_vector_list[k][head_index]
				break

		phrase_counter_hash = {}
		counter = 0
		for j in range(target_phrase_index, len(feature_vector_list)):
			#--- a copy of the feature vector list is necessary ---#
			some_list = [] + feature_vector_list[j]
			modified_feature_vector_list = modified_feature_vector_list + [some_list]


			#--- check the phrase counter hash and get the phrase index ---#
			if( phrase_counter_hash.has_key(feature_vector_list[j][phrase_index]) ):
				phrase_counter_hash[feature_vector_list[j][phrase_index]] = phrase_counter_hash[feature_vector_list[j][phrase_index]] + 1
			else:
				phrase_counter_hash[feature_vector_list[j][phrase_index]] = 1
			
			
			#--- the indices become 1, 2, 3, etc. because the length of the list dynamically increases ---#
			modified_feature_vector_list[-1][phrase_index:phrase_index] = [str(counter)]
			counter = counter + 1

			modified_feature_vector_list[-1][phrase_index+2:phrase_index+2] = [str(phrase_counter_hash[feature_vector_list[j][phrase_index]])]
			modified_feature_vector_list[-1][phrase_index:phrase_index] = [dominating_verb]

		phrase_counter_hash = {}
		counter = 1
		for j in range(target_phrase_index-1, -1, -1):
			#--- a copy of the feature vector list is necessary ---#
			some_list = [] + feature_vector_list[j]
			modified_feature_vector_list = [some_list] + modified_feature_vector_list

			#--- check the phrase counter hash and get the phrase index ---#
			if( phrase_counter_hash.has_key(feature_vector_list[j][phrase_index]) ):
				phrase_counter_hash[feature_vector_list[j][phrase_index]] = phrase_counter_hash[feature_vector_list[j][phrase_index]] + 1
			else:
				phrase_counter_hash[feature_vector_list[j][phrase_index]] = 1

			#--- the indices become 1, 2, 3, etc. because the length of the list dynamically increases ---#
			modified_feature_vector_list[0][phrase_index:phrase_index] = [str(counter)]
			counter = counter + 1

			modified_feature_vector_list[0][phrase_index+2:phrase_index+2] = [str(phrase_counter_hash[feature_vector_list[j][phrase_index]])]
			modified_feature_vector_list[0][phrase_index:phrase_index] = [dominating_verb]

		#--- after processing, print the list of feature vectors to std output---#
		for list in modified_feature_vector_list:
			print string.join(list)
		print

#		#--- after processing, print the list of feature vectors to std output---#
#		for list in feature_vector_list:
#			print string.join(list)
#		print

#		sys.exit(1)
		#--- empty the feature vector list ---#
		feature_vector_list = []
		modified_feature_vector_list = []
		
	else:
		#--- if it is a legitimate feature vector (no blank line or end of file), then add it to the list of feature vectors ---#
		if( a_feature_vector != "" and a_feature_vector != "\012" ):
			a_feature_vector_list = string.split(a_feature_vector)
			feature_vector_list.append(a_feature_vector_list)

	a_feature_vector = infile.readline()

		
