#!/bin/bash

REP_URL=http://www-i6/svn/sprint/branches/Teaching
ADDONS="src/Teaching"

options=
for a in $ADDONS; do
    options="$options -a ${a}|${REP_URL}/${a}"
done

$(dirname $(readlink -f $0))/make-rwth-asr.sh \
    "$@" $options -e MODULE_TEACHING
