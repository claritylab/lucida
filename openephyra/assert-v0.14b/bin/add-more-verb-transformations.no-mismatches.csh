#!/bin/csh

if( $# == 0 ) then
	echo "Usage: add-transformations.csh <file.txt>"
	exit
endif

set file = $1
echo "extracting basic features..."
$ASSERT/bin/print-constit-details-svm+all-verb+noun.pl -with-ne -head-pos -null-and-roles -salient-words $ASSERT/data/salient-words.TMP < $file > $file.1

echo "adding parent phrase and head as a feature..."
$ASSERT/bin/add-parent-phrase-and-head-as-feature.py $file.1 4 7 8 > $file.2

echo "adding tree distance as a feature...."
$ASSERT/bin/add-tree-distance-from-target.py $file.2 4 8 > $file.3

echo "adding parent phrase concatenated position as feature..."
$ASSERT/bin/add-parent-phrase-concat-position-as-feature.py $file.3 6 7 21 > $file.4

echo "adding left and right sibling phrase and head and head pos as features..."
$ASSERT/bin/add-left+right-sibling-phrase-head-hpos.py $file.4 6 7 8 9 14 16 > $file.5

awk '{if(NF==0) print; else {$7 = $7" "$7"-"$17; print}}' $file.5 > $file.6

echo "removing nodes crossing target..."
$ASSERT/bin/remove-nodes-crossing-target.py 6 > $file.7

echo "adding np-head-pp..."
$ASSERT/bin/sub-np-head-in-pp-eff.py  $file.7 6 21 23 > $file.8

awk '{if(NF==0 ||NF==142) print}' $file.8 > $file.9

sed 's/  */ /g' $file.9 > $file.10


