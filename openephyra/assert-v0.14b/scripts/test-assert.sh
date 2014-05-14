#!/bin/bash

#echo $ASSERT
#source /home/users/assert/assert-v0.12b/.bashrc

usage()
{
    cat <<EOF

Usage: test-assert.sh [--opinion] <test-filename.fn> <model-name>"

<model-file>: one of the models under $ASSERT/models directory

EOF
    exit $1
}

if test $# = 0; then
	usage 0
	exit 0
fi

model=$2
n_best=1

opinion="false"
while test $# -gt 4; do
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

if test $opinion = "true"; then
	$ASSERT/bin/fn2data-test.sh --opinion $1
else
	$ASSERT/bin/fn2data-test.sh $1
fi

filename=`echo $1 | awk -F/ '{print $NF}'`
filestem=`echo $1 | awk -F/ '{print $NF}' | sed 's/\.[^.][^.]*$//g'`

echo "WARNING: Currently only works for N-best=1!"

sed -n '/U[[:space:]][[:space:]]*U[[:space:]][[:space:]]*U[[:space:]][[:space:]]*U[[:space:]][[:space:]]*U[[:space:]][[:space:]]*U[[:space:]][[:space:]]*U[[:space:]][[:space:]]*U[[:space:]][[:space:]]*U[[:space:]][[:space:]]*U[[:space:]][[:space:]]*U[[:space:]][[:space:]]*U[[:space:]][[:space:]]*U[[:space:]][[:space:]]*U[[:space:]][[:space:]]*U[[:space:]][[:space:]]*U/p' $filestem.data | awk '{print $1" "$2" "$3" "$4" "$NF}' > $filestem.missing-spans

cp $filestem.data $filestem.data.orig
sed '/U[[:space:]][[:space:]]*U[[:space:]][[:space:]]*U[[:space:]][[:space:]]*U[[:space:]][[:space:]]*U[[:space:]][[:space:]]*U[[:space:]][[:space:]]*U[[:space:]][[:space:]]*U[[:space:]][[:space:]]*U[[:space:]][[:space:]]*U[[:space:]][[:space:]]*U[[:space:]][[:space:]]*U[[:space:]][[:space:]]*U[[:space:]][[:space:]]*U[[:space:]][[:space:]]*U[[:space:]][[:space:]]*U/d' $filestem.data.orig > $filestem.data
rm -rf $filestem.data.orig

missed=`wc -l $filestem.missing-spans|awk '{print $1}'`


echo $ASSERT/packages/yamcha-0.23/bin/yamcha
$ASSERT/packages/yamcha-0.23/bin/yamcha -V -m $model < $filestem.data > $filestem.svm-scores-with-overlap
$ASSERT/bin/remove-overlaps.py $filestem.svm-scores-with-overlap > $filestem.svm-scores-no-overlap

i=1
while test "$i" -le "$n_best"; do
	$ASSERT/bin/create-score-file.py $filestem.svm-scores-no-overlap > $filestem.${i}_best.ready-for-evaluation-no-overlap
	echo "non-overlapping constituent scores:" >> $filestem.${i}_best.results
	$ASSERT/bin/score-seq.pl -missed $missed $filestem.${i}_best.ready-for-evaluation-no-overlap >> $filestem.${i}_best.results
	i=$(($i + 1))
done

if test ! -e $filestem; then
	mkdir $filestem
fi

/bin/mv -f $filestem* $filestem >& /dev/null
