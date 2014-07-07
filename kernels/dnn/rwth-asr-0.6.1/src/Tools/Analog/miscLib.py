import codecs
import gzip
import sys


# ******************************************************************************
# automatic detection for
# compressed file types : *.gz
# stdio                 : -, stdin, stdout, stderr
# /dev/null             : null, nil
#
# ATTENTION:
# When dealing with text files use uopen instead of zopen
# in order to support easy encoding transformations.
# Use zopen in combination with an xml-Parser or to process
# files containing binary data.

def zopen(fileName, mode = 'r'):
    if fileName == '-':
	if mode == 'w' or mode == 'a':
	    return sys.stdout
	elif mode == 'r':
	    return sys.stdin
	else:
	    raise IOError('stdio does not support mode "' + mode + '"')
    elif fileName == 'stdin':
	if mode == 'r':
	    return sys.stdin
	else:
	    raise IOError('stdin does not support mode "' + mode + '"')
    elif fileName == 'stdout':
	if mode == 'w' or mode == 'a':
	    return sys.stdout
	else:
	    raise IOError('stdout does not support mode "' + mode + '"')
    elif fileName == 'stderr':
	if mode == 'w' or mode == 'a':
	    return sys.stderr
	else:
	    raise IOError('stderr does not support mode "' + mode + '"')
    elif fileName == 'null' or fileName == 'nil':
	return open('/dev/null', mode)
    elif fileName.endswith('.gz'):
	return gzip.open(fileName, mode)
    else:
	return open(fileName, mode)

def zclose(fd):
    if fd != sys.stdin and fd != sys.stdout and fd != sys.stderr:
	fd.close()


# universal unicode open:
# files are opened such that characters are encoded as unicode
# using the specified encoding and unicode is expected when
# characters are written (and decoded using the specified encoding);
# file type detection is supported (see zopen)
def uopen(fileName, encoding = 'ascii', mode = 'r'):
    encoder, decoder, streamReader, streamWriter = codecs.lookup(encoding)

    fd = zopen(fileName, mode)

    if mode == 'w' or mode == 'a':
	return streamWriter(fd)
    elif mode == 'r':
	return streamReader(fd)
    else:
	return codecs.StreamReaderWriter(fd, streamReader, streamWriter)


def uread(fileName, encoding = 'ascii'):
    return uopen(fileName, encoding, 'r')


def uappend(fileName, encoding = 'ascii'):
    return uopen(fileName, encoding, 'a')


def uwrite(fileName, encoding = 'ascii'):
    return uopen(fileName, encoding, 'w')

def uclose(fd):
    zclose(fd.stream)
