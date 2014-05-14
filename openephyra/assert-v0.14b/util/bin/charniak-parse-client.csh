#!/bin/csh

set THEPATH=`pwd`

#echo "processing $1..."

cp $1 $1.thirdpass.RAW-MG-READY
#
# Generate or use existing fourth pass
#
if (-e "$1.fourthpass" && -e "$1.fourthpass.FOLDED-RAW-MG-READY") then
	#echo "Using existing $1.fourthpass and moving ahead..."
else
	#echo "Fourth pass..."
	sed -f $ASSERT/util/bin/clean_TREC_sed_pattern_file_3.NEW $1.thirdpass.RAW-MG-READY > $1.fourthpass
endif

$CHARNIAK_PARSER/charniak_parser_client.pl localhost 15000 < $1.fourthpass > $1.CHARNIAK-PARSED

#sed -f $ASSERT/util/bin/remove-punctuations-from-collins.1.sed $1.CHARNIAK-PARSED | sed -f $ASSERT/util/bin/remove-punctuations-from-collins.2.sed | sed 's/)(/) (/g'> $1.CHARNIAK-PARSED-NO-PUNC
sed -f $ASSERT/util/bin/remove-punctuations-from-collins.1.sed $1.CHARNIAK-PARSED > $1.CHARNIAK-PARSED-NO-PUNC

/bin/rm -f $1.thirdpass.RAW-MG-READY $1.fourthpass.FOLDED-RAW-MG-READY $1.eighthpass $1.fourthpass
