#!/bin/python
import string
import sys
import re

if( len(sys.argv) == 1 or len(sys.argv) != 4 ):
	print "Usage: print-parse.py <svm-scores-file> <sentence-file> <lines-file>"
	sys.exit(1)

score_file_lines = open(sys.argv[1]).readlines()

if( len(score_file_lines) == 0 ):
	print "ERROR: Found an empty input file... exiting."
	sys.exit(1)

list_of_start_arrays = []
list_of_end_arrays   = []
list_of_role_arrays  = []
list_of_target_indices = []
list_of_word_lists = []
start_array = []
end_array   = []
role_array  = []

word_list   = []

#--- find out the index of ref and hyp columns and the index of the first after hyp ---#
some_list   = string.split(score_file_lines[0])

i= len(some_list) - 1
start_score_index = len(some_list) - 1
hyp_index         = len(some_list) - 1
ref_index         = len(some_list) - 1

while( i > 0  ):
	if( len(re.findall(r"/",some_list[i])) == 1 ):
		hyp_index         = hyp_index - 1
		start_score_index = hyp_index + 1
		ref_index         = hyp_index - 1
	else:
		break
	i = i - 1


#print >>sys.stderr, hyp_index
#print >>sys.stderr, start_score_index
#print >>sys.stderr, ref_index

i=0
for i in range(0, len(score_file_lines)):
	if(len(string.strip(score_file_lines[i])) == 0):
		list_of_start_arrays.append(start_array)
		list_of_end_arrays.append(end_array)
		list_of_role_arrays.append(role_array)
		list_of_target_indices.append(target_index)
		start_array = []
		end_array   = []
		role_array  = []
		continue

	list = string.split(score_file_lines[i])

	start_array.append(string.atoi(list[2]))
	end_array.append(string.atoi(list[3]))
	role_array.append(list[hyp_index])
	target_index = string.atoi(list[1])

list_of_word_lists = open(sys.argv[2]).readlines()
list_of_lines      = open(sys.argv[3]).readlines()

kk=0
for kk in range(0, len(list_of_word_lists)):
	list_of_word_lists[kk] = string.split(list_of_word_lists[kk])

jj=0
for jj in range(0, len(list_of_start_arrays)):
	word_list_copy  = list_of_word_lists[jj]
	
	word_list_copy[list_of_target_indices[jj]] = "[TARGET %s " % (word_list_copy[list_of_target_indices[jj]])
	word_list_copy[list_of_target_indices[jj]]   = "%s]" % (word_list_copy[list_of_target_indices[jj]])

	j=0
	for j in range(0, len(list_of_start_arrays[jj])):
		if( list_of_role_arrays[jj][j] == "O" ):
			continue

		try:
			word_list_copy[list_of_start_arrays[jj][j]] = "[%s %s" % (string.upper(list_of_role_arrays[jj][j]), word_list_copy[list_of_start_arrays[jj][j]])
			word_list_copy[list_of_end_arrays[jj][j]]   = "%s]" % (word_list_copy[list_of_end_arrays[jj][j]])
		except:
			print >>sys.stderr, "got some-problem skipping"

	parse = string.join(word_list_copy)
	#--- replace the left, right, angle, curly and square brackets with the original versions ---#
	parse = re.sub(r"-LAB-", r"<", parse)
	parse = re.sub(r"-RAB-", r">", parse)
	parse = re.sub(r"-LCB-", r"{", parse)
	parse = re.sub(r"-RCB-", r"}", parse)
	parse = re.sub(r"-LRB-", r"(", parse)
	parse = re.sub(r"-RRB-", r")", parse)

	print "%s: %s" % (string.strip(list_of_lines[jj]), parse)
