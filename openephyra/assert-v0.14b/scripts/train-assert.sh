#!/bin/bash

source ~/.bashrc

usage()
{
    cat <<EOF

Usage: train-assert.sh [--opinion] <filename.fn> <1|2>

EOF
    exit $1
}

if test $# = 0; then
	usage 0
	exit 0
fi

opinion="false"
while test $# -gt 2; do
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

if test ! -e $ASSERT/models/accompanying-model-files; then
	mkdir $ASSERT/models/accompanying-model-files
fi

if test ! -e log; then
	mkdir log
fi

if test $opinion = "true"; then
	$ASSERT/bin/fn2data.sh --opinion $1
else
	$ASSERT/bin/fn2data.sh $1
fi

filename=$1
filestem=`echo $1 | awk -F/ '{print $NF}' | sed 's/\.[^.][^.]*$//g'`

model="$ASSERT/models/$filestem.e-0.001"
classification_mode=$2

echo "training model: $model.model"
(make -f $ASSERT/data/Makefile ASSERT=$ASSERT CORPUS=$filestem.data MULTI_CLASS=$2 MODEL=$model train > log/$filestem.log; mv $ASSERT/models/$filestem.e-0.001.[^m]* $ASSERT/models/accompanying-model-files)&
