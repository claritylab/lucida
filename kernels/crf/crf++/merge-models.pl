#!/usr/bin/perl -w

use strict;
use warnings;
my %model_u;
my %model_b;
my $xsize = -1;
my $version = -1;
my $cost_factor = -1;
my $template = "";
my @class = ();
my $model_size = 0;

die "merge-models.pl files ... > new_text_file\n" if (scalar(@ARGV) == 0);

for my $file (@ARGV) {
    print STDERR "reading $file ..\n";

    # header
    open(F, $file) || die "$file\n";
    while (<F>) {
	chomp;
	last if (/^$/);
	if (/xsize: (\S+)/) {
	    die "xsize $xsize != $1\n" if ($xsize != -1 && $xsize != $1);
	    $xsize = $1;
	} elsif (/version: (\S+)/) {
	    die "version $version != $1\n" if ($version != -1 && $version != $1);
	    $version = $1;
	} elsif (/cost-factor: (\S+)/) {
	    die "cost-factor $cost_factor != $1\n" if ($cost_factor != -1 && $cost_factor != $1);
	    $cost_factor = $1;
	}
    }

    # class
    my @tmp = ();
    while (<F>) {
	chomp;
	last if (/^$/);
	push @tmp, $_;
    }

    die "@tmp != @class\n" if (@class && join(" ", @tmp) ne join(" ", @class));
    @class = @tmp;

    # template
    my $templ = "";
    while (<F>) {
	last if (/^$/);
	$templ .= $_;
    }

    die "$templ != $template\n" if ($template ne "" && $template ne $templ);
    $template = $templ;

    # dic
    my %u;
    my %b;
    while (<F>) {
	chomp;
	last if (/^$/);
	my ($id, $v) = split;
	if ($v =~ /^U/) {
	    $u{$v} = $id;
	} elsif ($v =~ /^B/) {
	    $b{$v} = $id;
	}
    }

    # weights
    my @w;
    while (<F>) {
	chomp;
	push @w, $_;
    }
    close(F);

    ## merge
    {
	my $size = scalar(@class);
	for my $v (keys %u) {
	    my $id = $u{$v};
	    for (my $i = 0; $i < $size; ++$i) {
		$model_u{$v}->[$i] += $w[$id + $i];
	    }
	}
    }

    {
	my $size = scalar(@class) * scalar(@class);
	for my $v (keys %b) {
	    my $id = $b{$v};
	    for (my $i = 0; $i < $size; ++$i) {
		$model_b{$v}->[$i] += $w[$id + $i];
	    }
	}
    }

    ++$model_size;
}

my $size = scalar(@class);
my $maxid = scalar(keys %model_u) * $size + scalar(keys %model_b) * $size * $size;

# output
print "version: $version\n";
print "cost-factor: 1\n";
print "maxid: $maxid\n";
print "xsize: $xsize\n";
print "\n";
print (join "\n", @class);
print "\n\n";
print $template;
print "\n";

my $id = 0;
my @w;
for my $v (sort keys %model_b) {
    my $size = scalar(@class) * scalar(@class);
    for (my $i = 0; $i < $size; ++$i) {
	push @w, 1.0 * $model_b{$v}->[$i] / $model_size;
    }
    print "$id $v\n";
    $id += $size;
}

for my $v (sort keys %model_u) {
    my $size = scalar(@class);
    for (my $i = 0; $i < $size; ++$i) {
	push @w, 1.0 * $model_u{$v}->[$i] / $model_size;
    }
    print "$id $v\n";
    $id += $size;
}

print "\n";

for (@w) {
    print "$_\n";
}
