#!/bin/csh

foreach pid (`ps -def | grep "framenet-print-constit\|charniak-server\|stay-alive-charniak" | grep "perl\|csh" | awk '{print $2}'`)
	kill -9 $pid
end
