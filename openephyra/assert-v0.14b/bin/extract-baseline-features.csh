#!/bin/csh

if( $# == 0 ) then
	echo "Usage: add-baseline-features.csh <file.txt> <passives-file>"
	exit
endif

set file = $1
set filestem = $1:r

#echo "adding named entity information"
#-- add the named entity line (currently pseudo, but to be changed to actual named entities soon) ---#
$ASSERT/bin/add-pseudo-ne.py $file > ${filestem}-ne.fn-test
/bin/mv ${filestem}-ne.fn-test ${filestem}.fn-test  #--- just to avoid book-keeping different filenames... 

#echo "extracting basic features..."
$ASSERT/bin/print-constit-details-svm.pl -with-ne -head-pos -passives-file $2 -old-feature-value-style -null-and-roles -salient-words $ASSERT/data/salient-words.TMP < ${filestem}.fn-test > $filestem.1

#echo "removing nodes crossing target..."
$ASSERT/bin/remove-nodes-crossing-target.py $filestem.1 > $filestem.2

$ASSERT/bin/select-baseline-features.csh $filestem.2 > $filestem.data

/bin/rm -rf $filestem.?
	
