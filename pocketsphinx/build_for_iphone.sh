#!/bin/bash
#
# This script drives ./configure for cross-platform (ie simulator and device) building on
# MacOS X.
#
################################################################################
#
# Copyright (c) 2008-2009 Christopher J. Stawarz
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation files
# (the "Software"), to deal in the Software without restriction,
# including without limitation the rights to use, copy, modify, merge,
# publish, distribute, sublicense, and/or sell copies of the Software,
# and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
################################################################################



# Disallow undefined variables
set -u

default_gcc_version=4.2
default_iphoneos_version=3.1.2
default_macos_version=10.5

GCC_VERSION="${GCC_VERSION:-$default_gcc_version}"
export IPHONEOS_DEPLOYMENT_TARGET="${IPHONEOS_DEPLOYMENT_TARGET:-$default_iphoneos_version}"
export MACOSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET:-$default_macos_version}"


usage ()
{
    cat >&2 << EOF
Usage: ${0##*/} [-ht] [-p prefix] target [configure_args]
	-h	Print help message
	-p	Installation prefix (default: \$HOME/Developer/Platforms/...)
	-t	Use 16-bit Thumb instruction set (instead of 32-bit ARM)

The target must be "device" or "simulator".  Any additional arguments
are passed to configure.

The following environment variables affect the build process:

	GCC_VERSION			(default: $default_gcc_version)
	IPHONEOS_DEPLOYMENT_TARGET	(default: $default_iphoneos_version)
	MACOSX_DEPLOYMENT_TARGET	(default: $default_macos_version)

EOF
}


while getopts ":hp:t" opt; do
    case $opt in
	h  ) usage ; exit 0 ;;
	p  ) prefix="$OPTARG" ;;
	t  ) thumb_opt=thumb ;;
	\? ) usage ; exit 2 ;;
    esac
done
shift $(( $OPTIND - 1 ))

if (( $# < 1 )); then
    usage
    exit 2
fi

target=$1
shift

case $target in

    device )
	arch=armv6
	platform=iPhoneOS
	extra_cflags="-m${thumb_opt:-no-thumb} -mthumb-interwork"
	;;

    simulator )
	arch=i386
	platform=iPhoneSimulator
	extra_cflags="-D__IPHONE_OS_VERSION_MIN_REQUIRED=${IPHONEOS_DEPLOYMENT_TARGET%%.*}0000"
	;;

    * )
	usage
	exit 2

esac


platform_dir="/Developer/Platforms/${platform}.platform/Developer"
platform_bin_dir="${platform_dir}/usr/bin"
platform_sdk_dir="${platform_dir}/SDKs/${platform}${IPHONEOS_DEPLOYMENT_TARGET}.sdk"
prefix="${prefix:-${HOME}${platform_sdk_dir}}"

export CC="${platform_bin_dir}/gcc-${GCC_VERSION}"
export CFLAGS="-arch ${arch} -pipe -Os -gdwarf-2 -isysroot ${platform_sdk_dir} ${extra_cflags}"
export LDFLAGS="-arch ${arch} -isysroot ${platform_sdk_dir}"
export CXX="${platform_bin_dir}/g++-${GCC_VERSION}"
export CXXFLAGS="${CFLAGS}"
export CPP="/Developer/usr/bin/cpp-${GCC_VERSION}"
export CXXCPP="${CPP}"


./configure \
    --prefix="${prefix}" \
    --host="${arch}-apple-darwin" \
    --disable-shared \
    --enable-static \
    "$@" || exit

make install || exit

cat >&2 << EOF

Build succeeded!  Files were installed in

  $prefix

EOF
