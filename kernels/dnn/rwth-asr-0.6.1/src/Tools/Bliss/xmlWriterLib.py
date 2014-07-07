# -*- coding: iso-8859-1 -*-

# $Id: xmlWriterLib.py 5201 2005-09-14 08:53:48Z hoffmeister $

import string
import codecs

from miscLib import uopen, uclose


def openXml(filename, encoding = 'ascii'):
    xml = XmlWriter(uopen(filename, encoding, 'w'), encoding)
    xml.begin()
    return xml

def closeXml(xml):
    xml.end()
    uclose(xml.fd)


class XmlWriter:
    def __init__(self, fd, encoding):
	self.fd = fd
	self.encoding = encoding
	self.path = []
	self.indentStr = '  '
	self.margin = 78

    def write(self, data):
	self.fd.write(data)

    def begin(self):
	self.write('<?xml version="1.0" encoding="' + self.encoding + '"?>\n')

    def end(self):
	assert len(self.path) == 0
	pass

    def setMargin(self, margin):
	self.margin = margin

    def setIndent_str(self, indent_str):
	self.indentStr = indent_str

    def indent_str(self):
	return self.indentStr * len(self.path)

    def formTag(self, element, attr=[]):
	return self.escape(string.join([element] + map(lambda kv: '%s="%s"' % kv, attr)))

    def open(self, element, args = {}, **args2):
	if args == {}: args = args2
	self.write(self.indent_str() + '<' + self.formTag(element, args.items()) + '>\n')
	self.path.append(element)

    def empty(self, element, args = {}, **args2):
	if args == {}: args = args2
	self.write(self.indent_str() + '<' + self.formTag(element, args.items()) + '/>\n')

    def close(self, element):
	assert element == self.path[-1]
	del self.path[-1]
	self.write(self.indent_str() + '</' + element + '>\n')

    def openComment(self):
	self.write('<!--\n')
	self.path.append('<!--')

    def closeComment(self):
	assert self.path[-1] == '<!--'
	del self.path[-1]
	self.write('-->\n')

    def cdata(self, w):
	if '<!--' in self.path:
	    w = string.replace(w, '--', '=') # comment must not contain double-hyphens
	indent_str = self.indent_str()
	ll = []
	l = [] ; n = len(indent_str)
	for a in string.split(w):
	    if n + len(a) < self.margin:
		n = n + len(a) + 1
		l.append(a)
	    else:
		ll.append(indent_str + string.join(l))
		l = [a] ; n = len(indent_str) + len(a)
	if len(l) > 0:
	    ll.append(indent_str + string.join(l))
	self.write(string.join(ll, '\n') + '\n')

    def formatted_cdata(self, s):
	for w in string.split(s, '\n'):
	    self.cdata(w)

    def comment(self, comment):
	comment = string.replace(comment, '--', '=') # comment must not contain double-hyphens
	self.cdata('<!-- ' + comment + ' -->')

    def element(self, element, cdata=None, **args):
	if cdata is None:
	    apply(self.empty, (element,), args)
	else:
	    s = self.indent_str() \
		+ '<' + self.formTag(element, args.items()) + '>' \
		+ cdata \
		+ '</' + element + '>'
	    if len(s) <= self.margin:
		self.write(s + '\n')
	    else:
		apply(self.open, (element,), args)
		self.cdata(cdata)
		self.close(element)
    def escape(self, data):
	return data.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')
