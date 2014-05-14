#!/bin/csh

sed -f $ASSERT/util/bin/clean_TREC_sed_pattern_file_3.NEW $1 | sed 's/^/<s> /g;s/$/ <\/s>/g' > $1.fourthpass
$CHARNIAK_PARSER/bin/parseStdin -l399 $CHARNIAK_PARSER/DATA/ < $1.fourthpass | sed 's/)(/) (/g' > $1.CHARNIAK-PARSED

#--- this is a hack, so in the long term it would be 
if( `tail -n 1 $1.CHARNIAK-PARSED | sed '/^ *$/d' | wc -l | awk '{print $1}'` == 0 ) then
	set num_lines = `wc -l $1.CHARNIAK-PARSED | awk '{print $1}'`
	@ num_lines = $num_lines - 1
	head -n $num_lines $1.CHARNIAK-PARSED > $1.CHARNIAK-PARSED.1
	/bin/mv $1.CHARNIAK-PARSED.1 $1.CHARNIAK-PARSED
endif
	
sed -f $ASSERT/util/bin/remove-punctuations-from-collins.1.sed $1.CHARNIAK-PARSED > $1.CHARNIAK-PARSED-NO-PUNC
/bin/rm -f $1.fourthpass
