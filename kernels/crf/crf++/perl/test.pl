#!/usr/bin/perl

use lib "../src/.libs";
use lib $ENV{PWD} . "/blib/lib";
use lib $ENV{PWD} . "/blib/arch";
use CRFPP;

print $CRFPP::VERSION, "\n";

# -v 3: access deep information like alpha,beta,prob
# -nN: enable nbest output. N should be >= 2
my $tagger = new CRFPP::Tagger("-m ../model -v 3 -n2");

# clear internal context
$tagger->clear();

# add context
$tagger->add("Confidence NN");
$tagger->add("in IN");
$tagger->add("the DT");
$tagger->add("pound NN");
$tagger->add("is VBZ");
$tagger->add("widely RB");
$tagger->add("expected VBN");
$tagger->add("to TO");
$tagger->add("take VB");
$tagger->add("another DT");
$tagger->add("sharp JJ");
$tagger->add("dive NN");
$tagger->add("if IN");
$tagger->add("trade NN");
$tagger->add("figures NNS");
$tagger->add("for IN");
$tagger->add("September NNP");

print "column size: " , $tagger->xsize() , "\n";
print "token size: " , $tagger->size() , "\n";
print "tag size: " , $tagger->ysize() , "\n";

print "tagset information:" , "\n";
for (my $i = 0; $i < $tagger->ysize(); ++$i) {
    print "tag " , $i , " " , $tagger->yname($i) , "\n";
}

# parse and change internal stated as 'parsed'
die if (!$tagger->parse());

print "conditional prob=" , $tagger->prob(), " log(Z)=" , $tagger->Z() , "\n";

for (my $i = 0; $i < $tagger->size(); ++$i) {
    for (my $j = 0; $j < $tagger->xsize(); ++$j) {
        print $tagger->x($i, $j) , "\t";
    }
    print $tagger->y2($i) , "\t";
    print "\n";
     print "Details";
     for (my $j = 0; $j < $tagger->ysize(); ++$j) {
         print "\t" , $tagger->yname($j) , "/prob=" , $tagger->prob($i,$j)
             ,"/alpha=" , $tagger->alpha($i, $j)
             ,"/beta=" , $tagger->beta($i, $j);
     }
    print "\n";
}

print "nbest outputs:" , "\n";
for (my $n = 0; $n < 10; ++$n) {
    last if (! $tagger->next());
    print "nbest n=" , $n , "\tconditional prob=" , $tagger->prob() , "\n";
    # you can access any information using $tagger->y()...
}

print "Done" , "\n";
