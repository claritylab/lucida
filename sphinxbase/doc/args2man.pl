#!/usr/bin/env perl
use strict;
use Pod::Usage;

my $program = shift;
pod2usage(2) unless defined($program);

open ARGTEXT, "$program 2>&1 |" or die "Failed to run $program: $!";
my $inargs = 0;
my @args;
while (<ARGTEXT>) {
    chomp;
    if (/^\[NAME/) {
	$inargs = 1;
	next;
    }
    next unless $inargs;
    last if /^\s*$/;
    my ($name, $deflt, $descr) = /^(\S+)\s+(\S+)\s+(.*)$/;
    push @args, [$name, $deflt, $descr];
}
die "No arguments found!" unless @args;

while (<>) {
    if (/\.\\\" ### ARGUMENTS ###/) {
	foreach (@args) {
	    my ($name, $deflt, $descr) = @$_;
	    $name =~ s/-/\\-/g;
	    $descr =~ s/ (-\S+)/ \\fB\\$1\\fR/g;
	    print <<"EOA";
.TP
.B $name
$descr
EOA
	}
    }
    else {
	print;
    }
}

__END__

=head1 NAME

sphinx_args2man - Generate manual pages from the output of Sphinx programs

=head1 SYNOPSIS

B<sphinx_args2man> I<PROGRAM> E<lt> I<TEMPLATE> E<gt> I<OUTPUT>

=head1 DESCRIPTION

This program runs a Sphinx program I<PROGRAM>, reads a template file
from standard input, and writes a manual page in L<man(7)> format to
standard output.

The template file is a manual page in L<man(7)> format, containing a
comment line of the form:

 .\" ### ARGUMENTS ###

Which will be replaced in the output with the arguments and their
descriptions from I<PROGRAM>.

=head1 AUTHOR

David Huggins-Daines <dhuggins@cs.cmu.edu>

=head1 COPYRIGHT

Copyright (c) 2007 Carnegie Mellon University.  You may copy and
distribute this file under the same conditions as the rest of
PocketSphinx.  See the file COPYING for more information.

=cut
