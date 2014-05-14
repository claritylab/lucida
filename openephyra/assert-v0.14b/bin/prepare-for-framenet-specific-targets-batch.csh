#!/bin/csh

if ($#argv != 3 ) then
    echo "Usage: prepare-for-framenet-specific-targets.csh <plain_sentence_filename> <target_file> <framenet_test_filename>"
    exit
endif

#--- just in case the tagging is done in dos, lets convert it to unix format ---#
#dos2unix $1

#--- charniak parse the sentences ---#
$CHARNIAK_PARSER/bin/charniak-batch.csh $1

#---- POS tag sentences ----#
$CHARNIAK_PARSER/bin/postag-from-charniak.py $1.CHARNIAK-PARSED-NO-PUNC > $1.POS

#--- convert the sentences to target-tagged SGML sentences ---#
$ASSERT/bin/tag-specific-targets.py $1.POS $2 $MORPH/data/morph_english_verbs.flat $1.SGML $1.line-index-file

#--- sync the parses ---#
$ASSERT/bin/extract-lines.py $1.CHARNIAK-PARSED-NO-PUNC $1.line-index-file > $1.SYNCD-CHARNIAK-PARSES

sed -f $ASSERT/util/bin/extract-sentence-from-charniak.sed $1.SYNCD-CHARNIAK-PARSES > $1.charniak
sed -f $ASSERT/util/bin/extract-words-from-framenet-tagged-sentences.sed $1.SGML > $1.framenet

#--- create the syncd SGML and charniak sentences together ---#
$ASSERT/util/bin/extract-compatible-lines-from-files.py $1.SYNCD-CHARNIAK-PARSES $1.SGML $1.line-index-file $1.charniak $1.framenet $1.CHARNIAK $1.FRAMENET $1.LINES

#---- and generate the file ready for feature extraction ---#
$ASSERT/bin/create-test.py  $1.FRAMENET $1.CHARNIAK > $3

