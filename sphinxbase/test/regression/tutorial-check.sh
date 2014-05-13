#!/bin/bash -x

# Check if we're running under PBS and print information useful for debugging
if [ -n "$PBS_ENVIRONMENT" ]; then
    echo ""
    echo "This job was submitted by user: $PBS_O_LOGNAME"
    echo "This job was submitted to host: $PBS_O_HOST"
    echo "This job was submitted to queue: $PBS_O_QUEUE"
    echo "PBS working directory: $PBS_O_WORKDIR"
    echo "PBS job id: $PBS_JOBID"
    echo "PBS job name: $PBS_JOBNAME"
    echo "PBS environment: $PBS_ENVIRONMENT"
    echo ""
    echo "This script is running on `hostname` "
    echo ""
fi

# make sure that /usr/ucb is here, towards the beginning, to make sure
# that the 'ps' command in Solaris is compatible with the gnu one.
export PATH=/usr/ucb:/usr/local/bin:${PATH}

loopUntilSuccess () {
    cmd=$@
    # start loop to download code
    count=0;

    while ! $cmd; do
	count=`expr $count + 1`
	if [ $count -gt 50 ]; then
	    # not successful, and we attempted it too many times. Clean up and leave.
	    return $count
	fi
    done
}

# Check that we have all executables
if ! WGET=`command -v wget 2>&1`; then exit 1; fi
if ! SVN=`command -v svn 2>&1`; then exit 1; fi
if ! PERL=`command -v perl 2>&1`; then exit 1; fi
if ! MAKE=`command -v gmake 2>&1`; then 
  if ! MAKE=`command -v make 2>&1`; then exit 1; fi
fi
if ! TAR=`command -v gtar 2>&1`; then
  if ! TAR=`command -v tar 2>&1`; then exit 1; fi
fi
if ! MAIL=`command -v mhmail 2>&1`; then
  if ! MAIL=`command -v mailx 2>&1`; then
    if ! MAIL=`command -v sendmail 2>&1`; then
      if ! MAIL=`command -v mutt 2>&1`; then exit 1; fi
    fi
  fi
fi

# Create temp directory
temp_dir=/tmp/temp$$
mkdir $temp_dir
pushd $temp_dir > /dev/null

LOG=$temp_dir/log.txt
echo > $LOG
MAILLIST=cmusphinx-results@lists.sourceforge.net

# Get the data
${WGET} -q http://www.speech.cs.cmu.edu/databases/an4/an4_sphere.tar.gz
${TAR} -xzf an4_sphere.tar.gz
/bin/rm an4_sphere.tar.gz

# Get sphinxbase
if (loopUntilSuccess ${SVN} co https://cmusphinx.svn.sourceforge.net/svnroot/cmusphinx/trunk/sphinxbase > /dev/null &&
cd sphinxbase &&
./autogen.sh &&
./autogen.sh CFLAGS="-O2 -Wall" --prefix=`(cd ..; pwd)`/build &&
${MAKE} all install) >> $LOG 2>&1 ; then

# Get the trainer
if (loopUntilSuccess ${SVN} co https://cmusphinx.svn.sourceforge.net/svnroot/cmusphinx/trunk/SphinxTrain > /dev/null &&
cd SphinxTrain &&
./configure CFLAGS="-O2 -Wall" --with-sphinxbase=$temp_dir/sphinxbase && 
${MAKE} && 
${PERL} scripts_pl/setup_tutorial.pl an4) >> $LOG 2>&1 ; then

# Get the decoder
if (loopUntilSuccess ${SVN} co https://cmusphinx.svn.sourceforge.net/svnroot/cmusphinx/trunk/sphinx3 > /dev/null &&
cd sphinx3 &&
./autogen.sh &&
./autogen.sh  CFLAGS="-O2 -Wall" --prefix=`(cd ..; pwd)`/build --with-sphinxbase=$temp_dir/sphinxbase && 
${MAKE} all install && 
${PERL} scripts/setup_tutorial.pl an4) >> $LOG 2>&1 ; then

# Run it
cd an4 &&
perl scripts_pl/make_feats.pl -ctl etc/an4_train.fileids >> $LOG 2>&1 &&
perl scripts_pl/make_feats.pl -ctl etc/an4_test.fileids >> $LOG 2>&1 &&
perl scripts_pl/RunAll.pl >> $LOG 2>&1 &&
perl scripts_pl/decode/slave.pl >> $LOG 2>&1

# end of if (sphinx3)
fi
# end of if (SphinxTrain)
fi
# end of sphinxbase
fi

# Check whether the Word Error Rate is reasonable, hardwired for now
if SER=`grep 'SENTENCE ERROR' $LOG 2>&1`; then
SUCCESS=`echo $SER | awk '{d = $(3) - 57};{if (d < 0) d = -d};{if (d < 2) print "1"}'`;
fi;

# Send mail if we failed
if test x${SUCCESS} == x ; then
cat `find $temp_dir/an4/logdir/ -type f -print | tail -1` >> ${LOG}
$MAIL -s "Tutorial failed" ${MAILLIST} < ${LOG}
else
echo $SER | $MAIL -s "Tutorial succeded" ${MAILLIST}
fi

# Remove what we created
popd > /dev/null
/bin/rm -rf $temp_dir
