# -*- coding: iso-8859-1 -*-

import os.path, sys
from ioLib import *

def addPrefix(path, prefix):
    """
    adds prefix to the filename given in path
    """
    dirname, filename = os.path.split(path)
    assert filename
    return os.path.join(dirname, prefix + filename)

def swapSuffix(path, suffix, keepCompressionSuffix = False):
    """
    1) swaps the filename's suffix, if filename has suffix
    2) addd suffix, if filename has no suffix
    3) ignores known suffixes indicating compression (see zopen) and performs 1) or 2)
    """
    dirname, filename = os.path.split(path)
    assert filename
    if filename.endswith('.gz'):
	filename = filename[:-3]
	compressionSuffix = '.gz'
    else:
	compressionSuffix = ''
    i = filename.rfind('.')
    if i == -1:
	filename += '.' + suffix
    else:
	filename = filename[:i+1] + suffix
    if keepCompressionSuffix:
	filename += compressionSuffix
    return os.path.join(dirname, filename)

# ******************************************************************************
class ToAsciiConverter:
    def encode(self, s):
	a = []
	for c in s:
	    o = ord(c)
	    if o >= 128:
		c = hex(o)
	    a.append(c)
	return ''.join(a)

# ******************************************************************************

def loadList(path, encoding = 'ascii'):
    fd = uopen(path, encoding, 'r')
    l = [r.strip() for r in fd if r.strip()]
    uclose(fd)
    return l

# ******************************************************************************
