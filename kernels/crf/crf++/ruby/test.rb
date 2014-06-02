#!/usr/bin/ruby

require 'CRFPP'

begin
    # -v 3: access deep information like alpha,beta,prob
    # -nN: enable nbest output. N should be >= 2
    tagger = CRFPP::Tagger.new("-m ../model -v 3 -n2")

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

    print "column size: " , tagger.xsize() , "\n"
    print "token size: " , tagger.size() , "\n"
    print "tag size: " , tagger.ysize() , "\n"

    print "tagset information:" , "\n"
    ysize = tagger.ysize()
    for i in 0..(ysize-1)
        print "tag " , i , " " , tagger.yname(i) , "\n"
    end

    # parse and change internal stated as 'parsed'
    die if !tagger.parse()

    print "conditional prob=" , tagger.prob(), " log(Z)=" , tagger.Z() , "\n"

    size = tagger.size()
    xsize = tagger.xsize()
    for i in 0..(size - 1)
       for j in 0..(xsize-1)
          print tagger.x(i, j) , "\t"
       end
       print tagger.y2(i) , "\t"
       print "\n"
       print "Details"
       for j in 0..(ysize-1)
          print "\t" , tagger.yname(j) , "/prob=" , tagger.prob(i,j),"/alpha=" , tagger.alpha(i, j),"/beta=" , tagger.beta(i, j)
        print "\n"
       end
    end


    print "nbest outputs:" , "\n"
    for n in 0..9
       last if !tagger.next()
       print "nbest n=" , n , "\tconditional prob=" , tagger.prob() , "\n"
       # you can access any information using tagger.y()...
    end

print "Done" , "\n"

rescue
    print "RuntimeError: ", $!, "\n"
end
