#!/bin/csh

if( $# == 0 ) then
	echo "Usage: extract-propbank-features.csh <file.txt> <passives-file>"  
	exit
endif

set file = $1
set filestem = $1:r

#echo "adding named entity information"
#-- add the named entity line (currently pseudo, but to be changed to actual named entities soon) ---#
$ASSERT/bin/add-pseudo-ne.py $file > ${filestem}-ne.fn-test
/bin/mv ${filestem}-ne.fn-test ${filestem}.fn-test  #--- just to avoid book-keeping different filenames... 

#echo "extracting basic features..."
$ASSERT/bin/print-constit-details-svm.pl -with-ne -head-pos -passives-file $2 -null-and-roles -salient-words $ASSERT/data/salient-words.TMP < ${filestem}.fn-test > $filestem.1

#echo "adding parent phrase and head as a feature..."
$ASSERT/bin/add-parent-phrase-and-head-as-feature.py $filestem.1 4 7 8 > $filestem.2

#echo "adding tree distance as a feature...."
$ASSERT/bin/add-tree-distance-from-target.py $filestem.2 4 8 > $filestem.3

#echo "adding parent phrase concatenated position as feature..."
$ASSERT/bin/add-parent-phrase-concat-position-as-feature.py $filestem.3 6 7 21 > $filestem.4

#echo "adding left and right sibling phrase and head and head pos as features..."
$ASSERT/bin/add-left+right-sibling-phrase-head-hpos.py $filestem.4 6 7 8 9 14 16 > $filestem.5

awk '{if(NF==0) print; else {$7 = $7" "$7"-"$17; print}}' $filestem.5 > $filestem.6

#echo "removing nodes crossing target..."
$ASSERT/bin/remove-nodes-crossing-target.py $filestem.6 > $filestem.7

#echo "adding np-head-pp..."
$ASSERT/bin/sub-np-head-in-pp-eff.py  $filestem.7 6 21 23 > $filestem.8

awk '{if(NF==0 ||NF==126) print}' $filestem.8 > $filestem.9

sed 's/  */ /g' $filestem.9 > $filestem.10

$ASSERT/bin/select-propbank-features.csh $filestem.10 > $filestem.data
/bin/rm -rf $filestem.? $filestem.??
	
