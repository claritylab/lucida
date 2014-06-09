#!/usr/bin/env python

"""
An example for part-of-speech tagging.
Copyright 2010,2011 Naoaki Okazaki.
"""

# Separator of field values.
separator = ' '

# Field names of the input data.
fields = 'w num cap sym p1 p2 p3 p4 s1 s2 s3 s4 y'

# Feature template. This template is identical to the one bundled in CRF++
# distribution, but written in a Python object.
templates = (
    (('num', 0), ),
    (('cap', 0), ),
    (('sym', 0), ),
    (('p1', 0), ),
    (('p2', 0), ),
    (('p3', 0), ),
    (('p4', 0), ),
    (('s1', 0), ),
    (('s2', 0), ),
    (('s3', 0), ),
    (('s4', 0), ),

    (('w',  0), ),
    (('w', -1), ),
    (('w',  1), ),
    (('w', -2), ),
    (('w',  2), ),
    (('w', -2), ('w',  -1)),
    (('w', -1), ('w',  0)),
    (('w',  0), ('w',  1)),
    (('w',  1), ('w',  2)),
    (('w', -2), ('w',  -1), ('w',  0)),
    (('w', -1), ('w',  0), ('w',  1)),
    (('w', 0), ('w',  1), ('w',  2)),
    (('w', -2), ('w',  -1), ('w',  0), ('w',  1)),
    (('w',  -1), ('w',  0), ('w',  1), ('w', 2)),
    (('w', -2), ('w',  -1), ('w',  0), ('w',  1), ('w',  2)),

    (('w',  0), ('w',  -1)),
    (('w',  0), ('w',  -2)),
    (('w',  0), ('w',  -3)),
    (('w',  0), ('w',  -4)),
    (('w',  0), ('w',  -5)),
    (('w',  0), ('w',  -6)),
    (('w',  0), ('w',  -7)),
    (('w',  0), ('w',  -8)),
    (('w',  0), ('w',  -9)),

    (('w',  0), ('w',  1)),
    (('w',  0), ('w',  2)),
    (('w',  0), ('w',  3)),
    (('w',  0), ('w',  4)),
    (('w',  0), ('w',  5)),
    (('w',  0), ('w',  6)),
    (('w',  0), ('w',  7)),
    (('w',  0), ('w',  8)),
    (('w',  0), ('w',  9)),
    )


import crfutils

def feature_extractor(X):
    # Apply feature templates to obtain features (in fact, attributes)
    crfutils.apply_templates(X, templates)
    if X:
	# Append BOS and EOS features manually
        X[0]['F'].append('__BOS__')     # BOS feature
        X[-1]['F'].append('__EOS__')    # EOS feature

if __name__ == '__main__':
    crfutils.main(feature_extractor, fields=fields, sep=separator)
