#!/bin/csh

setenv LANG en_US.8859-1

set dt = `date +%b_%d_%Y_%H:%M`

if( -e assert-log ) then 
	mv assert-log assert-log_$dt
endif

#--- 15000 ---#
$CHARNIAK_PARSER/bin/charniak-server.pl >>& assert-log &



