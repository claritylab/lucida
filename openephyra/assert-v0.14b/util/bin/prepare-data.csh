#!/bin/csh

setenv LANG en_US.iso8859-1

if( $# != 1 ) then
	echo "Usage: prepare-data.csh <filestem.ext>"
	exit
endif

set filename = $1
set filestem = $1:r

cp $1 $filestem.fn-train
$ASSERT/bin/classify_passives.py $filestem.fn-train $filestem.passives $filestem.tmp $filestem.corpus

$ASSERT/bin/print-constit-details-svm.pl -soft < $filestem.fn-train > $filestem.data
/bin/cp $filestem.data $filestem.data.orig
$ASSERT/bin/correct-passives.py $filestem.passives $filestem.data.orig > $filestem.data

/bin/rm sentences $filestem.data.orig $filestem.fn-train $filestem.passives 
#$filestem.sentences* $filestem.svm* 

