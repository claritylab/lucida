#!/bin/csh

sed -f $ASSERT/util/bin/clean_TREC_sed_pattern_file_3.NEW $1 > $1.fourthpass
$CHARNIAK_PARSER/bin/charniak-client.pl localhost 15000 < $1.fourthpass | sed 's/)(/) (/g' > $1.CHARNIAK-PARSED

sed -f $ASSERT/util/bin/remove-punctuations-from-collins.1.sed $1.CHARNIAK-PARSED > $1.CHARNIAK-PARSED-NO-PUNC
/bin/rm -f $1.fourthpass
