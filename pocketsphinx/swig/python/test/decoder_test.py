# -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
# ====================================================================
# Copyright (c) 2013 Carnegie Mellon University.  All rights
# reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer. 
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
#
# This work was supported in part by funding from the Defense Advanced 
# Research Projects Agency and the National Science Foundation of the 
# United States of America, and the CMU Sphinx Speech Consortium.
#
# THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND 
# ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
# NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# ====================================================================


from os import environ, path
from itertools import izip

from pocketsphinx import *
from sphinxbase import *

MODELDIR = "../../../model"
DATADIR = "../../../test/data"

# Create a decoder with certain model
config = Decoder.default_config()
config.set_string('-hmm', path.join(MODELDIR, 'hmm/en_US/hub4wsj_sc_8k'))
config.set_string('-lm', path.join(MODELDIR, 'lm/en_US/hub4.5000.DMP'))
config.set_string('-dict', path.join(MODELDIR, 'lm/en_US/hub4.5000.dic'))
decoder = Decoder(config)

# Decode static file.
decoder.decode_raw(open(path.join(DATADIR, 'goforward.raw'), 'rb'))

# Retrieve hypothesis.
hypothesis = decoder.hyp()
print 'Best hypothesis: ', hypothesis.best_score, hypothesis.hypstr
print 'Best hypothesis segments: ', [seg.word for seg in decoder.seg()]

# Access N best decodings.
print 'Best 10 hypothesis: '
for best, i in izip(decoder.nbest(), range(10)):
	print best.hyp().best_score, best.hyp().hypstr

# Decode streaming data.
decoder = Decoder(config)
decoder.start_utt('goforward')
stream = open(path.join(DATADIR, 'goforward.raw'), 'rb')
while True:
  buf = stream.read(1024)
  if buf:
    decoder.process_raw(buf, False, False)
  else:
    break
decoder.end_utt()
print 'Stream decoding result:', decoder.hyp().hypstr
