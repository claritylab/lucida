#!/bin/csh

while(1)
	set charniak_prob = `ps -def| grep "charniak-server" | sed '/grep/d' | wc -l`
	if( $charniak_prob == 0 ) then
		set dt = `date +%b_%d_%Y_%H:%M`
		echo "charniak server restarted at $dt" >> stay-alive.log
		$CHARNIAK_PARSER/bin/charniak-server.pl >>& assert-log
	endif
	sleep 10
end
