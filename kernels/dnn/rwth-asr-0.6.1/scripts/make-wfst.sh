#!/bin/bash

REP_URL=http://www-i6/svn/sprint/branches/RASR_WFST
ADDONS="src/Search/Wfst src/OpenFst src/Tools/SpeechRecognizer/FsaSearchBuilder.cc config/os-linux.make"

options=
for a in $ADDONS; do
    options="$options -a ${a}|${REP_URL}/${a}"
done

$(dirname $(readlink -f $0))/make-rwth-asr.sh \
    "$@" $options -e MODULE_SEARCH_WFST -e MODULE_OPENFST
