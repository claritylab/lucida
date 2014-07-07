#!/bin/bash
# Copyright 2011 RWTH Aachen University. All rights reserved.
#
# Licensed under the RWTH ASR License (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.hltpr.rwth-aachen.de/rwth-asr/rwth-asr-license.html
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# 
# Checks if all requirements of RWTH ASR are fulfilled.
# Verifies availability of tools (gcc, make), header files, and libraries.

GCC_MIN_MAJOR=4
GCC_MAX_MAJOR=4
GCC_MIN_MINOR=0
GCC_MAX_MINOR=7
HEADERS="pthread.h libxml/parser.h zlib.h sndfile.h"

function getTempfile
{
    mktemp -t XXXXXXXX
}

function getSettings()
{
    local result=$1
    cat > $tmpMake <<EOF
default : settings
TOPDIR = ${rootDir}
include \$(TOPDIR)/Makefile.cfg
VARS = CXX CC LD BISON CPPFLAGS CCFLAGS CFLAGS CXXFLAGS LDFLAGS CXX_MAJOR CXX_MINOR
settings :
	@printf "\$(foreach v,\$(VARS),\$(v)=\"\$(\$(v))\"\n)"

.PHONY : settings
EOF
    make -f $tmpMake > $result
}

function readLink()
{
    local abspath
    set +e
    # try readlink -f
    abspath=$(readlink -f "$1" 2> /dev/null)
    # if this does not work (Mac OS X)
    [ -z "$abspath" ] && abspath=$(python -c "import os.path; print os.path.realpath(\"$1\");" 2> /dev/null)
    # backup if python is not available
    [ -z "$abspath" ] && abspath=$1
    echo $abspath
    set -e
}

function getRootDir()
{
    local scriptDir=$(dirname $(readLink $0))
    echo $(readLink "${scriptDir}/..")
}

function setup()
{
    rootDir=$(getRootDir)
    tmpMake=$(getTempfile)
    tmpSource=$(getTempfile)
    mv "$tmpSource" "${tmpSource}.cc"
    tmpSource=${tmpSource}.cc
    tmpResult=$(getTempfile)
    tmpOutput=$(getTempfile)
    settings=$(getTempfile)
    tempFiles="$tmpMake $tmpSource $tmpResult $tmpOutput $settings"
}

function cleanup()
{
    local f
    for f in $tempFiles; do
	[ -f $f ] && rm -f $f
    done
}

function reportError()
{
    echo "ERROR: $*"
    exit 1
}

function reportWarning()
{
    echo "WARNING: $*"
}


function checkProgram
{
    local prog=$1
    local path=$(which $prog)
    [ -n "$path" -a -e "$path" ] || reportError "'$prog' not found"
    [ -x $path ] || reportError "'$path' not executable"
    echo "${prog} = ${path}: OK"
}

function checkCompiler
{
    checkProgram ${CC}
    [ "${CC/ccache/x}" != "${CC}" ] && \
	CC=${CC/ccache /} && \
	checkProgram ${CC}
    checkProgram ${CXX}
    [ "${CXX/ccache/x}" != "${CXX}" ] && \
	CXX=${CXX/ccache /} && \
	checkProgram ${CXX}
    checkProgram ${LD}
    checkProgram ${BISON}
}

function checkGcc()
{
    local ccBase=$(basename $CC)
    if [ "${ccBase:0:3}" != "gcc" ]; then
	reportWarning using compiler other than gcc: $CC
	return
    fi
    if [ -z "${CXX_MAJOR}" -o -z "${CXX_MINOR}" ]; then
	reportWarning cannot determine compiler version
	return
    fi
    if [ "${CXX_MAJOR}" -lt "${GCC_MIN_MAJOR}" -o \
	 "${CXX_MAJOR}" -gt "${GCC_MAX_MAJOR}" -o \
	 "${CXX_MINOR}" -lt "${GCC_MIN_MINOR}" -o \
	 "${CXX_MINOR}" -gt "${GCC_MAX_MINOR}" ]; then
	reportWarning unsupported version of GCC: ${CXX_MAJOR/ /}.${CXX_MINOR}
    else
	echo "GCC version ${CXX_MAJOR/ /}.${CXX_MINOR}: OK"
    fi
}

function checkHeader
{
    local file=$1
    shift 1
    rm -f $tmpSource
    for f in $@; do
        echo "#include <${f}>" >> $tmpSource
    done
    echo "#include <${file}>" >> $tmpSource
    local cmd="${CXX} ${CXXFLAGS} ${CPPFLAGS} -o ${tmpOutput} -c ${tmpSource} &> $tmpResult"
    set +e
    eval $cmd
    local result=$?
    set -e
    if [ $result -ne 0 ]; then
	echo "$cmd"
	cat $tmpResult
	reportError "header '${file}' not found"
    fi
    echo "header ${file}: OK"
}

function needLib()
{
  [ "${LDFLAGS/${1}/}" != "${LDFLAGS}" ]
}

function checkImageHeaders()
{
    needLib jpeg && checkHeader jpeglib.h stdlib.h stdio.h
    needLib png && checkHeader png.h
    needLib netpbm && checkHeader pam.h
}


function checkLibs()
{
    local flags
    local libs
    local p
    local result
    for p in ${LDFLAGS}; do
	if [ "${p:0:2}" = "-l" ]; then
	    libs="$libs $p"
	else
	    flags="$flags $p"
	fi
    done
    libs=$(echo $libs | tr ' ' '\n' | sort -u | tr '\n' ' ')
    echo "int main() { return 0; }" > $tmpSource
    local cmd="${CXX} ${CPPFLAGS} ${CXXFLAGS} ${flags} -o $tmpOutput $tmpSource"
    for p in $libs; do
	set +e
	eval "$cmd $p &> $tmpResult"
	result=$?
	set -e
        if [ $result -ne 0 ]; then
	    echo "$cmd"
	    cat $tmpResult
	    reportError "library ${p/-l/lib} not found"

	else
	    echo "library ${p/-l/lib}: OK"
	fi
    done
}

set -e
trap "cleanup; exit" EXIT
setup

# check for make
checkProgram make

# load settins from Makefiles
getSettings $settings
source $settings

# check for compilers
checkCompiler
checkGcc

# check for header files
for h in $HEADERS; do
    checkHeader $h
done
checkImageHeaders

# check for libraries
checkLibs

