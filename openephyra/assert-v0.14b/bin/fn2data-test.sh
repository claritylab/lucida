#!/bin/bash

LANG=en_US.iso8859-1
export LANG

usage()
{
    cat <<EOF

Usage: fn2data.sh [--opinion] <filename.ext>

EOF
    exit $1
}

if test $# = 0; then
	usage 0
	exit 0
fi

opinion="false"
while test $# -gt 1; do
    case "$1" in
    -*=*) optarg=`echo "$1" | sed 's/[-_a-zA-Z0-9]*=//'` 
			;;
    *) optarg="" 
			;;
    esac

    case "$1" in
		--opinion)
			opinion="true"
			;;

		--help)
			usage 0
			exit 0
			;;
		*)
			usage 0
			exit 1
			;;
    esac
    shift
done

filename=$1
filestem=`echo "$filename" | awk -F/ '{print $NF}' | sed 's/\.[^.][^.]*$//g'`

if test ! -e $filestem.fn; then
    cp $1 $filestem.fn
fi

$ASSERT/bin/classify_passives_train.py $filestem.fn $filestem.passives $filestem.tmp $filestem.corpus

if test "$?" -ne "0"; then
	exit
fi

$ASSERT/bin/print-constit-details-svm.pl $filestem.fn > $filestem.data
/bin/cp $filestem.data $filestem.data.orig
$ASSERT/bin/correct-passives.py $filestem.passives $filestem.data.orig 30 p a> $filestem.data

if test "$opinion" = "true"; then
$ASSERT/bin/PrintWordFeatureData.py --word-score-sum=$ASSERT/data/words-hong/words-threshold-2.5.scores --phrase-type=ADJP $filestem.fn $filestem.data > $filestem.data.1
/bin/mv $filestem.data.1 $filestem.data
fi

/bin/rm sentences head-map-file head-tagged-fn-file $filestem.data.orig $filestem.passives
