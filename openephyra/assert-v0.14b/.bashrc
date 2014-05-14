
ulimit -s unlimited
ulimit -f unlimited

export ASSERT=/home/gpuser/umvoice/openephyra/assert-v0.14b

export YAMCHA=$ASSERT/packages/yamcha-0.23
export TINY_SVM=$ASSERT/packages/TinySVM-0.09
export CHARNIAK_PARSER=$ASSERT/packages/CharniakParser
export MORPH=$ASSERT/packages/morph-1.5
export TGREP2=$ASSERT/packages/Tgrep2

#--- maybe this can be kept under $ASSERT/lib ---#
export PERL5LIB=$ASSERT/bin  
export PYTHONPATH=$ASSERT/packages:$ASSERT/lib/python-lib

export LD_LIBRARY_PATH='.:$YAMCHA/lib:/usr/local/lib:/usr/local/include:/usr/include'
export TK_LIBRARY='/home/CU/packages/tk8.2.2/library'
export TCL_LIBRARY='/home/CU/packages/tcl8.2.2/library'

export PATH='.:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/bin/X11:/usr/X11R6/bin:/bin:$ASSERT/util/bin:$ASSERT/bin:$ASSERT/scripts/batch:$ASSERT/scripts/client:$ASSERT/scripts/remote:$TGREP2:$CHARNIAK_PARSER/bin:$YAMCHA/bin:$TINY_SVM/bin'

