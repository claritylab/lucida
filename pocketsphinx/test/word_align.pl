#!/usr/bin/perl -w

# word_align.pl - Calculate word error and accuracy for a recognition
# hypothesis file vs. a reference transcription
#
# Written by David Huggins-Daines <dhuggins@cs.cmu.edu> for Speech
# Recognition and Understanding 11-751, Carnegie Mellon University,
# October 2004.

use strict;
use Getopt::Long;
use Pod::Usage;
use vars qw($Verbose $CER $IgnoreUttID);
use encoding 'utf8';

my ($help,%hyphash);
GetOptions(
	   'help|?' => \$help,
	   'verbose|v' => \$Verbose,
	   'cer|c' => \$CER,
	   'ignore-uttid|i' => \$IgnoreUttID,
	  ) or pod2usage(1);
pod2usage(1) if $help;

pod2usage(2) unless @ARGV == 2;
my ($ref, $hyp) = @ARGV;

open HYP, "<$hyp" or die "Failed to open $hyp: $!";
while (defined(my $hyp_utt=<HYP>)){
    my $hyp_uttid;
    ($hyp_utt, $hyp_uttid) = s3_magic_norm($hyp_utt);
    $hyphash{$hyp_uttid} = "$hyp_utt ($hyp_uttid)";
}
close HYP;

open REF, "<$ref" or die "Failed to open $ref: $!";
open HYP, "<$hyp" or die "Failed to open $hyp: $!";

use constant INS => 1;
use constant DEL => 2;
use constant MATCH => 3;
use constant SUBST => 4;
use constant BIG_NUMBER => 1e50;

my ($total_words, $total_match, $total_cost, $total_hyp);
my ($total_ins, $total_del, $total_subst);
while (defined(my $ref_utt = <REF>)) {
    my $hyp_utt;
    my $ref_uttid;
    my $hyp_uttid;

    last unless defined $ref_utt;

    ($ref_utt,$ref_uttid)=s3_magic_norm($ref_utt);

    if(defined $IgnoreUttID){
	$hyp_utt = <HYP>;
	die "UttID is ignored but file size mismatch between $ref and $hyp" unless defined($hyp_utt);
    }else{
	$hyp_utt=$hyphash{$ref_uttid};
        die "UttID is not ignored but it could not found in any entries of the hypothesis file on line3 $. UTTID: $ref_uttid\n" unless defined($hyp_utt);
    }

    ($hyp_utt,$hyp_uttid)=s3_magic_norm($hyp_utt);

    if(! defined $IgnoreUttID){
	die "Utterance ID mismatch on line $.: $ref_uttid != $hyp_uttid"
	    unless $ref_uttid eq $hyp_uttid;
    }

    # Split the text into an array of words
    my @ref_words = split ' ', $ref_utt;
    my @hyp_words = split ' ', $hyp_utt;
    if ($CER) {
	# Split the text into an array of characters
	@ref_words = map { split "" } @ref_words;
	@hyp_words = map { split "" } @hyp_words;
    }

    my (@align_matrix, @backtrace_matrix);

    # Initialize the alignment and backtrace matrices
    initialize(\@ref_words, \@hyp_words, \@align_matrix, \@backtrace_matrix);
    # Do DP alignment maintaining backtrace pointers
    my $cost = align(\@ref_words, \@hyp_words, \@align_matrix, \@backtrace_matrix);
    # Find the backtrace
    my ($alignment, $ins, $del, $subst, $match) = backtrace(\@ref_words, \@hyp_words,
							    \@align_matrix, \@backtrace_matrix);

    # Format the alignment nicely
    my ($ref_align, $hyp_align) = ("", "");
    foreach (@$alignment) {
	my ($ref, $hyp) = @$_;
	my $width = 0;

	if (defined($ref) and defined($hyp)) {
	    if ($CER or
		($ref =~ /\p{InCJKUnifiedIdeographs}/ or
		 $ref =~ /\p{Han}/ or
		 $hyp =~ /\p{Han}/)) {
		# Assume this is Chinese, no capitalization so put ** around errors
		if ($ref ne $hyp) {
		    $ref = "*$ref*";
	    	    $hyp = "*$hyp*";
		}
	    } elsif ($ref eq $hyp) {
		# Capitalize errors (they already are...), lowercase matches
		$ref = lc $ref;
		$hyp = lc $hyp;
	    }
	}

	# Replace deletions with ***
	foreach ($ref, $hyp) { $_ = "***" unless defined $_ };

	# Find the width of this column
	foreach ($ref, $hyp) { $width = length if length > $width };
	$width = 3 if $width < 3; # Make it long enough for ***

	# Space out the words and concatenate them to the output
	$ref_align .= sprintf("%-*s ", $width, $ref);
	$hyp_align .= sprintf("%-*s ", $width, $hyp);
    }
    $ref_uttid = "" unless defined $ref_uttid; # avoid warnings
    print "$ref_align ($ref_uttid)\n$hyp_align ($hyp_uttid)\n";

    # Print out the word error and accuracy rates
    my $error = @ref_words == 0 ? 1 : $cost/@ref_words;
    my $acc = @ref_words == 0 ? 0 : $match/@ref_words;
    printf("Words: %d Correct: %d Errors: %d Percent correct = %.2f%% Error = %.2f%% Accuracy = %.2f%%\n",
	   scalar(@ref_words), $match, $cost, $acc*100, $error*100, 100-$error*100);
    print "Insertions: $ins Deletions: $del Substitutions: $subst\n";

    $total_cost += $cost;
    $total_match += $match;
    $total_words += @ref_words;
    $total_hyp += @hyp_words;
    $total_ins += $ins;
    $total_del += $del;
    $total_subst += $subst;
}
# Print out the total word error and accuracy rates
my ($error, $acc);
if ($total_words == 0) {
    $error = $total_cost/$total_hyp;
    $acc = $total_match/$total_hyp;
}
else {
    $error = $total_cost/$total_words;
    $acc = $total_match/$total_words;
}
printf("TOTAL Words: %d Correct: %d Errors: %d\nTOTAL Percent correct = %.2f%% Error = %.2f%% Accuracy = %.2f%%\n",
       $total_words, $total_match, $total_cost, $acc*100, $error*100, 100-$error*100);
print "TOTAL Insertions: $total_ins Deletions: $total_del Substitutions: $total_subst\n";

# This function normalizes a line of a match file. 
sub s3_magic_norm{
    my ($word)=@_;
    my $uttid;

    # Remove line endings
    $word =~ s/[\n\r]+$//;  # the agnostic way...

    # This computes the uttid and remove it from a line.
    $word =~ s/\(([^) ]+)[^)]*\)$// ;
    $uttid = $1;

    # Normalize case
    $word = uc $word;   
    # Remove filler words and context cues
    $word =~ s/<[^>]+>//g;
    $word =~ s/\+\+[^+]+\+\+//g;
    $word =~ s/\+[^+]+\+//g;

    # Remove alternative pronunciations
    $word =~ s/\([1-9]\)//g;

    # Remove class tags
    $word =~ s/:\S+//g;

    # Split apart compound words and acronyms
    $word =~ tr/-_./  /;

    return ($word,$uttid);
}

sub initialize {
    my ($ref_words, $hyp_words, $align_matrix, $backtrace_matrix) = @_;

    # All initial costs along the j axis are insertions
    for (my $j = 0; $j <= @$hyp_words; ++$j) {
	$$align_matrix[0][$j] = $j;
    }
    for (my $j = 0; $j <= @$hyp_words; ++$j) {
	$$backtrace_matrix[0][$j] = INS;
    }
    # All initial costs along the i axis are deletions
    for (my $i = 0; $i <= @$ref_words; ++$i) {
	$$align_matrix[$i][0] = $i;
    }
    for (my $i = 0; $i <= @$ref_words; ++$i) {
	$$backtrace_matrix[$i][0] = DEL;
    }
}

sub align {
    my ($ref_words, $hyp_words, $align_matrix, $backtrace_matrix) = @_;

    for (my $i = 1; $i <= @$ref_words; ++$i) {
	for (my $j = 1; $j <= @$hyp_words; ++$j) {
	    # Find insertion, deletion, substitution scores
	    my ($ins, $del, $subst);

	    # Cost of a substitution (0 if they are equal)
	    my $cost = $$ref_words[$i-1] ne $$hyp_words[$j-1];

	    # Find insertion, deletion, substitution costs
	    $ins = $$align_matrix[$i][$j-1] + 1;
	    $del = $$align_matrix[$i-1][$j] + 1;
	    $subst = $$align_matrix[$i-1][$j-1] + $cost;
	    print "Costs at $i $j: INS $ins DEL $del SUBST $subst\n" if $Verbose;

	    # Get the minimum one
	    my $min = BIG_NUMBER;
	    foreach ($ins, $del, $subst) {
		if ($_ < $min) {
		    $min = $_;
		}
	    }
	    $$align_matrix[$i][$j] = $min;

	    # If the costs are equal, prefer match or substitution
	    # (keep the path diagonal).
	    if ($min == $subst) {
		print(($cost ? "SUBSTITUTION" : "MATCH"),
		      "($$ref_words[$i-1] <=> $$hyp_words[$j-1])\n") if $Verbose;
		$$backtrace_matrix[$i][$j] = MATCH+$cost;
	    }
	    elsif ($min == $ins) {
		print "INSERTION (0 => $$hyp_words[$j-1])\n" if $Verbose;
		$$backtrace_matrix[$i][$j] = INS;
	    }
	    elsif ($min == $del) {
		print "DELETION ($$ref_words[$i-1] => 0)\n" if $Verbose;
		$$backtrace_matrix[$i][$j] = DEL;
	    }
	}
    }
    return $$align_matrix[@$ref_words][@$hyp_words];
}

sub backtrace {
    my ($ref_words, $hyp_words, $align_matrix, $backtrace_matrix) = @_;

    # Backtrace to find number of ins/del/subst
    my @alignment;
    my $i = @$ref_words;
    my $j = @$hyp_words;
    my ($inspen, $delpen, $substpen, $match) = (0,0,0,0);
    while (!($i == 0 and $j == 0)) {
	my $pointer = $$backtrace_matrix[$i][$j];
	print "Cost at $i $j: $$align_matrix[$i][$j]\n"
	    if $Verbose;
	if ($pointer == INS) {
	    print "INSERTION (0 => $$hyp_words[$j-1])" if $Verbose;
	    # Append the pair 0:hyp[j] to the front of the alignment
	    unshift @alignment, [undef, $$hyp_words[$j-1]];
	    ++$inspen;
	    --$j;
	    print " - moving to $i $j\n" if $Verbose;
	}
	elsif ($pointer == DEL) {
	    print "DELETION ($$ref_words[$i-1] => 0)" if $Verbose;
	    # Append the pair ref[i]:0 to the front of the alignment
	    unshift @alignment, [$$ref_words[$i-1], undef];
	    ++$delpen;
	    --$i;
	    print " - moving to $i $j\n" if $Verbose;
	}
	elsif ($pointer == MATCH) {
	    print "MATCH ($$ref_words[$i-1] <=> $$hyp_words[$j-1])" if $Verbose;
	    # Append the pair ref[i]:hyp[j] to the front of the alignment
	    unshift @alignment, [$$ref_words[$i-1], $$hyp_words[$j-1]];
	    ++$match;
	    --$j;
	    --$i;
	    print " - moving to $i $j\n" if $Verbose;
	}
	elsif ($pointer == SUBST) {
	    print "SUBSTITUTION ($$ref_words[$i-1] <=> $$hyp_words[$j-1])" if $Verbose;
	    # Append the pair ref[i]:hyp[j] to the front of the alignment
	    unshift @alignment, [$$ref_words[$i-1], $$hyp_words[$j-1]];
	    ++$substpen;
	    --$j;
	    --$i;
	    print " - moving to $i $j\n" if $Verbose;
	}
	else {
	    last;
	}
    }

    return (\@alignment, $inspen, $delpen, $substpen, $match);
}

__END__

=head1 NAME

calculate_wer - Calculate Word Error Rate from a reference and hypothesis file

=head1 SYNOPSIS

 calculate_wer [options] reference_file hypothesis_file

=head1 OPTIONS

=over 8

=item B<--help>, B<-?>

Print a brief help message and exit.

=item B<--verbose>, B<-v>

Print out messages tracing the alignment algorithm.

=cut
