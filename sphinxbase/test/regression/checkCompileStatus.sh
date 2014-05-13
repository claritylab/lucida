#!/bin/bash

module=$1
shift
options=$*

# Set the cpu limit to 2 hours, just in case
ulimit -t 7200

# Define a path, just in case
export PATH="/usr/local/bin:/bin:/usr/bin:/usr/ccs/bin"

# Make sure we at least specify the module to test
if test x$module == x ; then
    echo "FAILED: Usage: $0 <module to test>"
    exit 1
fi

# Try to find gmake, supposedly the GNU make. autogen.sh is too smart
# for our own good though: it defines MAKE internally in the Makefile,
# so beware of how you name this variable

GMAKE=`command -v gmake`
if test z"$GMAKE" == z; then
# If we failed, try make
    GMAKE=`command -v make`
    if test z"$GMAKE" == z; then
# If we failed again, bail out: we cannot make the project!
    echo "FAILED: make not found, tried gmake and make"
# Exit with non zero value
    exit 1;
    fi
fi

# Create root and move there

root=/tmp/regression$$
if mkdir -p $root; then
cd $root

repeat=$root/repeat.sh
# Create our own "try until success" script, just in case we can't
# download a copy from the repository"
cat > $repeat <<-EOF
#!/bin/bash

cmd=\$*;
# start loop
count=0;

while ! \$cmd; do
let count++;
if ((\$count>50)); then
# not successful, and we attempted it too many times. Clean up and leave.
exit \$count;
fi
done

EOF

chmod +x $repeat

# Fresh download of sphinxbase. Some modules need it, some don't. Just
# in case, get it.
$repeat svn co https://cmusphinx.svn.sourceforge.net/svnroot/cmusphinx/trunk/sphinxbase

# Configure it
pushd sphinxbase || echo "FAILED: sphinxbase not found"

if  (./autogen.sh && ./autogen.sh $options); then

# sphinxbase is needed for everything else, so build it in-place, and don't remove it
    if ! ${GMAKE} ; then
        echo "sphinxbase compilation FAILED"
    fi
else
    echo "sphinxbase autogen FAILED"
fi

popd

# Fresh download of pocketsphinx
$repeat svn co https://cmusphinx.svn.sourceforge.net/svnroot/cmusphinx/trunk/$module

# Configure it
pushd $module || echo "FAILED: $module not found"

if test -e ./autogen.sh; then
    ./autogen.sh && ./autogen.sh
    target="distcheck DISTCHECK_CONFIGURE_FLAGS=--with-sphinxbase=$root/sphinxbase"
else
    target="all"
fi

if (./configure $options); then

# Compile and run test, and verify if both were successful
# Note that distcheck builds in a subdirectory so we need to give an
# an (absolute) path to sphinxbase.
    if ! ${GMAKE} $target ; then
        echo "$module compilation or test FAILED"
    fi

else
    echo "$module configure FAILED"
fi

popd

# Make everything writable so it can be deleted later
chmod -R +w .

fi

# Remove what we created
cd /tmp
/bin/rm -rf $root
