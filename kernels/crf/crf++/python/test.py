#!/usr/bin/ruby

import CRFPP
import sys

try:
    # -v 3: access deep information like alpha,beta,prob
    # -nN: enable nbest output. N should be >= 2
    tagger = CRFPP.Tagger("-m ../model -v 3 -n2")

    # clear internal context
    tagger.clear()

    # add context
    tagger.add("Confidence NN")
    tagger.add("in IN")
    tagger.add("the DT")
    tagger.add("pound NN")
    tagger.add("is VBZ")
    tagger.add("widely RB")
    tagger.add("expected VBN")
    tagger.add("to TO")
    tagger.add("take VB")
    tagger.add("another DT")
    tagger.add("sharp JJ")
    tagger.add("dive NN")
    tagger.add("if IN")
    tagger.add("trade NN")
    tagger.add("figures NNS")
    tagger.add("for IN")
    tagger.add("September NNP")

    print "column size: " , tagger.xsize()
    print "token size: " , tagger.size()
    print "tag size: " , tagger.ysize()

    print "tagset information:"
    ysize = tagger.ysize()
    for i in range(0, ysize-1):
        print "tag " , i , " " , tagger.yname(i)

    # parse and change internal stated as 'parsed'
    tagger.parse()

    print "conditional prob=" , tagger.prob(), " log(Z)=" , tagger.Z()

    size = tagger.size()
    xsize = tagger.xsize()
    for i in range(0, (size - 1)):
       for j in range(0, (xsize-1)):
          print tagger.x(i, j) , "\t",
       print tagger.y2(i) , "\t",
       print "Details",
       for j in range(0, (ysize-1)):
          print "\t" , tagger.yname(j) , "/prob=" , tagger.prob(i,j),"/alpha=" , tagger.alpha(i, j),"/beta=" , tagger.beta(i, j),
       print "\n",

    print "nbest outputs:"
    for n in range(0, 9):
        if (not tagger.next()):
            continue
        print "nbest n=" , n , "\tconditional prob=" , tagger.prob()
        # you can access any information using tagger.y()...

    print "Done"

except RuntimeError, e:
    print "RuntimeError: ", e,
