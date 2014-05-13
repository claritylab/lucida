#!/usr/bin/env perl

use strict;

my $nstates;
while (<>) {
    chomp;
    s/#.*$//;
    next if /^$/;

    if (/FSG_BEGIN (\S+)/) {
	print "digraph $1 {\n\trankdir=LR;\n\t";
    }
    elsif (/NUM_STATES (\d+)/) {
	$nstates = $1;
    }
    elsif (/START_STATE (\d+)/) {
    }
    elsif (/FINAL_STATE (\d+)/) {
	my $end = $1;

	print "\tnode [shape=circle];";
	for (my $i = 0; $i < $nstates; ++$i) {
	    print " $i" unless $i == $end;
	}
	print ";\n\tnode [shape=doublecircle]; $end;\n\n";
    }
    elsif (/TRANSITION/) {
	my (undef, $from, $to, $weight, $word) = split;

	my $label;
	if ($weight != 1.0 and defined($word)) {
	    $label = sprintf "%s/%.2f", $word, $weight;
	}
	elsif ($weight != 1.0) {
	    $label = sprintf "%.2f", $weight;
	}
	elsif ($word) {
	    $label = $word;
	}
	print "\t$from -> $to [label=\"$label\"];\n";
    }
}
print "}\n";
