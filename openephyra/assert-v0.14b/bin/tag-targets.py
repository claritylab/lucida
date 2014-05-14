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
	print "Usage: tag-all-verb-targets.py <pos_tagged_file> <target-list-file> <stop-verbs-file> <morph-file> <output_file> <line_index_list>"
	sys.exit()

input_file = open(sys.argv[1])
input_file_lines = input_file.readlines()

target_list_file_lines = open(sys.argv[2]).readlines()

stop_verb_file = open(sys.argv[3])
stop_verb_file_lines = stop_verb_file.readlines()

morph_file_lines = open(sys.argv[4]).readlines()

output_file = open(sys.argv[5], "w")
output_file_lines = []

line_index_list = []
line_index_file = open(sys.argv[6], "w")

target_matcher = re.compile(r" (?P<word>[^_][^_]*)_(?P<tag>V[^ ][^ ]*) ")

target_list_hash = {}

stop_verb_hash = {}

for verb in stop_verb_file_lines:
	stop_verb_hash[string.strip(verb)] = 1

morph_hash = {}
for line in morph_file_lines:
	#print line
	list = string.split(line)
	if( len(list) == 2 ):
		(verb, morph_verb) = (list[0], list[1])
		morph_hash[verb] = morph_verb

for verb in target_list_file_lines:
	verb = string.strip(verb)

	if( morph_hash.has_key(verb) ):
		target_list_hash[morph_hash[verb]] = 1
	else:
		target_list_hash[verb] = 1
					
target_list = []

i = 0
for line in input_file_lines:
	#print line
	#print target_matcher.pattern
	start_index = 0
	end_index = len(line)
	some_match = target_matcher.search(line,start_index,end_index)
	while (some_match != None):
		target = line[some_match.span('word')[0]:some_match.span('word')[1]]
		tag = line[some_match.span('tag')[0]:some_match.span('tag')[1]]
		
		some_string = "%s <C TARGET=\"y\"> %s/%s </C> %s" % (line[0:some_match.span('word')[0]], target, tag, line[some_match.span('tag')[1]:end_index])
		
	    #--- replace the target with the morphed version in the target list, if there is one ---#
		morph_target = ""
		if( morph_hash.has_key(target) ):
			morph_target = morph_has[target]
		else:
			morph_target = target
			
		if( not stop_verb_hash.has_key(target) or target_list_hash.has_key(morph_target)):
			output_file_lines.append(some_string)
			line_index_list.append("%d\n" % i)
			target_list.append(morph_target)
		start_index = some_match.span('tag')[1]
		some_match = target_matcher.search(line,start_index,end_index)
	i = i+1


i=0
for i in range(0, len(output_file_lines)):
	output_file_lines[i] = re.sub("_", "/", output_file_lines[i])

	output_file_lines[i] = "DOMAIN/FRAME/%s.v.ar:<S TPOS=\"%s\"> %s </S>\n" % (target_list[i], string.zfill(i+1000, 14), string.strip(output_file_lines[i]))

	#--- replace slash and underscore back ---#
	output_file_lines[i] = re.sub(r"-SLASH-", r"/", output_file_lines[i])
	output_file_lines[i] = re.sub(r"-UNDERSCORE-", r"_", output_file_lines[i])
	
output_file.writelines(output_file_lines)
line_index_file.writelines(line_index_list)
