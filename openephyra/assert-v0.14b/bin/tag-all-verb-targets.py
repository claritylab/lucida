#!/bin/python
#-----------------------------------------------------------------#
# Some assumptions:
# We will currently assume that one target appears only once in
# a given sentence... this might not be true anymore
#-----------------------------------------------------------------#

import string
import re
import sys

if( len(sys.argv) != 6):
	print "Usage: tag-all-verb-targets.py <pos_tagged_file> <stop-verbs-file> <morph-file> <output_file> <line_index_list>"
	sys.exit()

input_file = open(sys.argv[1])
input_file_lines = input_file.readlines()

stop_verb_file = open(sys.argv[2])
stop_verb_file_lines = stop_verb_file.readlines()

morph_file_lines = open(sys.argv[3]).readlines()

output_file = open(sys.argv[4], "w")
output_file_lines = []

line_index_list = []
line_index_file = open(sys.argv[5], "w")

target_matcher = re.compile(r" (?P<word>[^_][^_]*)_(?P<tag>V[^ ][^ ]*) ")


stop_verb_hash = {}

for verb in stop_verb_file_lines:
	stop_verb_hash[string.strip(verb)] = 1

morph_hash = {}
for line in morph_file_lines:
	list = string.split(line)
	if( len(list) == 2 ):
		(verb, morph_verb) = (list[0], list[1])
		morph_hash[verb] = morph_verb

target_list = []

i = 0
for line in input_file_lines:
	start_index = 0
	end_index = len(line)
	some_match = target_matcher.search(line,start_index,end_index)
	while (some_match != None):
		some_string = "%s <C TARGET=\"y\"> %s/%s </C> %s" % (line[0:some_match.span('word')[0]], line[some_match.span('word')[0]:some_match.span('word')[1]], line[some_match.span('tag')[0]:some_match.span('tag')[1]], line[some_match.span('tag')[1]:end_index])
		if( not stop_verb_hash.has_key(line[some_match.span('word')[0]:some_match.span('word')[1]]) ):
			output_file_lines.append(some_string)
			line_index_list.append("%d\n" % i)
			target_list.append(line[some_match.span('word')[0]:some_match.span('word')[1]])
		start_index = some_match.span('tag')[1]
		some_match = target_matcher.search(line,start_index,end_index)
	i = i+1


i=0
for i in range(0, len(output_file_lines)):
	output_file_lines[i] = re.sub("_", "/", output_file_lines[i])

	#--- replace the target with the morphed version in the target list, if there is one ---#
	if( morph_hash.has_key(target_list[i]) ):
		target_list[i] = morph_hash[target_list[i]]
		
	output_file_lines[i] = "DOMAIN/FRAME/%s.v.ar:<S TPOS=\"%s\"> %s </S>\n" % (target_list[i], string.zfill(i+1000, 14), string.strip(output_file_lines[i]))

	#--- replace slash and underscore back ---#
	output_file_lines[i] = re.sub(r"-SLASH-", r"/", output_file_lines[i])
	output_file_lines[i] = re.sub(r"-UNDERSCORE-", r"_", output_file_lines[i])
	
output_file.writelines(output_file_lines)
line_index_file.writelines(line_index_list)





