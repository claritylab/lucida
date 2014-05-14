#!/bin/python

import string
import sys
import re
import os
import commands
import popen2

if(len(sys.argv) == 1):
	print "Usage: classify_passives.py <fn.txt> <passives-file> <tmp-file> <corpus-file>"
	sys.exit(1)

infile = open(sys.argv[1])
passives = open(sys.argv[2], "w")

fn_formatted_line = infile.readline()
parse = infile.readline()
blank_line = infile.readline()

sys.stderr.write("classifying passive targets.")
count = 0
while ( fn_formatted_line != "" and parse != "" ):
	try:
		#print fn_formatted_line
		target_lemma = re.findall(r"^DOMAIN/FRAME/(.*?)\.v\.ar", fn_formatted_line)[0]
		target = string.strip(re.findall(r"<C TARGET=\"y\">(.*?)\/.*?</C>", fn_formatted_line)[0])
		tpos   = re.findall(r"TPOS=\"(.*?)\"", fn_formatted_line)[0]

		parse = re.sub(target, "TARGET", parse)

		temp_file = open(sys.argv[3], "w")
		temp_file.write(parse)
		temp_file.close()

		#print os.getenv("ASSERT")
		#print "%s/packages/Tgrep2/tgrep2 -p %s %s" % (os.getenv("ASSERT"), sys.argv[3], sys.argv[4])
		child = popen2.Popen4("%s/packages/Tgrep2/tgrep2 -p %s %s" % (os.getenv("ASSERT"), sys.argv[3], sys.argv[4]), 1)
		data = child.fromchild.read()
		err  = child.wait()

		#print "data:", data
		#print "error:", err

		#print "%s/bin/passivestarget.pl %s" % (os.getenv("ASSERT"), sys.argv[4])
		child = popen2.Popen4("%s/bin/passivestarget.pl %s" % (os.getenv("ASSERT"), sys.argv[4]), 1)
		output = child.fromchild.read()
		err    = child.wait()

		#print "output:", output
		#print "error:", err

		if(len(re.findall(">>-->", output)) > 0):
			passives.write("TPOS=%s\n" % (tpos))
			sys.stderr.write(".")
			count = count + 1

		fn_formatted_line = infile.readline()
		parse = infile.readline()
		blank_line = infile.readline()
	except:
		print "\n\nERROR: Check input file format! Exiting ...\n\n"
		sys.exit(1)
sys.stderr.write("\n")

os.remove(sys.argv[3])
os.remove(sys.argv[4])
