#!/bin/bash

function call_test {

module=$1

if ./regression/checkCompileStatus.sh $module > $outdir/$module.log 2>&1; then
    if grep FAILED $outdir/$module.log > /dev/null; then 
	echo "$module FAILED" >> $mail_file;
	    return 1;
    else echo "$module PASSED" >> $mail_file;
	return 0;
    fi;
else echo "$module FAILED" >> $mail_file;
    return 1;
fi

}


ulimit -t 7200

# Define a path, just in case
export PATH="/usr/local/bin:/bin:/usr/bin:/usr/ccs/bin"

# Try to find an executable that can send mail
# Default to sendmail
MAILX=sendmail

# Since we're running under bash, don't bother using "which", which
# doesn't work on Solaris anyway.  Use "command" instead which is a
# handy builtin.

# Try to find mhmail
TMPMAIL=`command -v mhmail`
if test z"$TMPMAIL" = z; then
# If we failed, try mailx
    TMPMAIL=`command -v mailx`
    if test z"$TMPMAIL" = z; then
# If we failed again, try mail
	TMPMAIL=`command -v mail`
    fi
fi

# If we found one of the above, use it. Otherwise, keep sendmail
if test z${TMPMAIL} != z; then MAILX=${TMPMAIL};fi

# Define the mailing list
MAILLIST='cmusphinx-results@lists.sourceforge.net'

# Define SF_ROOT in the shell initialization script (~/.profile or
# ~/.login, depending on whether you're using sh or csh), as in:
# export SF_ROOT=${HOME}/project/SourceForge/svn
if test x$SF_ROOT == x ; then
    echo "Please define SF_ROOT pointing to the top of your repository working copy:"
    echo "\$SF_ROOT/sphinxbase, for example, should contains sphinxbase"
    exit 1
fi

# Directory where to put output files
outdir=$SF_ROOT/regression/`hostname`
if ! (test -e $outdir) ; then
    mkdir -p $outdir
fi
cd $outdir

success=0
mail_file=$outdir/mail.txt
echo "Results in $outdir" > $mail_file
echo "Summary:" >> $mail_file

# Fresh download of sphinxbase
if svn co https://cmusphinx.svn.sourceforge.net/svnroot/cmusphinx/trunk/sphinxbase/test/regression; then

    success=1;

    if ! call_test sphinxbase; then success=0; fi
    if ! call_test pocketsphinx; then success=0; fi
    if ! call_test sphinx2; then success=0; fi
    if ! call_test sphinx3; then success=0; fi
    if ! call_test SphinxTrain; then success=0; fi

fi

# Send the message, finally
if [ $success == 1 ] ; then
    ${MAILX} -s "All tests passed in `hostname`" ${MAILLIST} < $mail_file;
else 
    ${MAILX} -s "Test FAILED in `hostname`" ${MAILLIST} < $mail_file;
fi

# Remove what we created
/bin/rm -rf $outdir/regression

