#!/bin/python
import httplib
import urllib
import urlparse
import sys
import string
import getopt


def usage():
	print """
Usage: assert-remote.py [--url=url] [--format=plain|html] [--roles=argument|theta|opinion] [--help] file
The first value in the list of possible values for options is the default
"""

def main():
	try:
		opts, args = getopt.getopt(sys.argv[1:], "h", ["help", "url=", "format=", "roles="])
	except getopt.GetoptError:
		usage()
		sys.exit(1)

	#--- initiate to default values ---#
	URL = "http://localhost/cgi-bin/assert/assert-cgi.py"
	roles = "propbank-args"
	tagging = "PropBank Arguments"
	format = "plain"

	for o, a in opts:
		if o in ("-h", "--help"):
			usage()
			sys.exit(1)

		if o in ("--url"):
			URL = "http://%s/cgi-bin/assert/assert-cgi.py" % (a)

		if o in ("--format"):
			format = a
			if( format == "html" ):
				format = "HTML"

		if o in ("--roles"):
			roles = a
			if( roles == "argument" ):
				tagging = "PropBank Arguments"
			elif( roles == "theta" ):
				tagging = "Thematic Roles"
			elif( roles == "opinion" ):
				tagging = "Opinion and Opinion Holder"

	#print args
	#sys.exit(0)

	if( len(args) > 0 ):
		sentences = string.join(open(args[0]).readlines())
		param = urllib.urlencode({"sentence":"%s" % (sentences), "tagging":"%s" % (tagging), "format":"%s" % (format)})
		f = urllib.urlopen(URL, param)
		print f.read()

	else:
		while 1:
			sentence = sys.stdin.readline()
			
			if( string.strip(sentence) == ""):
				break
			if not sentence:
				break

			param = urllib.urlencode({"sentence":"%s" % (sentence), "tagging":"%s" % (tagging), "format":"%s" % (format)})
			f = urllib.urlopen(URL, param)
			print f.read()

if __name__ == "__main__":
	main()

