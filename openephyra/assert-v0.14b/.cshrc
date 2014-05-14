
limit stacksize unlimited
limit filesize unlimited

setenv ASSERT /home/gpuser/umvoice/openephyra/assert-v0.14b

setenv YAMCHA          $ASSERT/packages/yamcha-0.23
setenv TINY_SVM        $ASSERT/packages/TinySVM-0.09
setenv CHARNIAK_PARSER $ASSERT/packages/CharniakParser
setenv MORPH           $ASSERT/packages/morph-1.5
setenv TGREP2          $ASSERT/packages/Tgrep2

#--- maybe this can be kept under $ASSERT/lib ---#
setenv PERL5LIB     $ASSERT/bin  
setenv PYTHONPATH   $ASSERT/packages:$ASSERT/lib/python-lib

setenv LD_LIBRARY_PATH .:$YAMCHA/lib:/usr/local/lib:/usr/local/include:/usr/include
setenv TK_LIBRARY        /home/CU/packages/tk8.2.2/library
setenv TCL_LIBRARY       /home/CU/packages/tcl8.2.2/library

set path = ( . \
		/sbin \
		/usr/bin \
		/usr/sbin \
		/usr/local/bin \
		/usr/bin/X11 \
		/usr/X11R6/bin \
		/bin \
		$ASSERT/util/bin \
		$ASSERT/bin \
		$ASSERT/scripts/batch \
		$ASSERT/scripts/client \
		$ASSERT/scripts/remote \
		$TGREP2 \
		$CHARNIAK_PARSER/bin \
		$YAMCHA/bin \
		$TINY_SVM/bin )


alias em 'emacs -nw'
alias dir 'ls -l --color=always'
alias delete 'rm -rf *~ #*'
alias del '/bin/mv \!:* ~/Desktop/Trash/'
