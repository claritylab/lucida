#!/bin/perl
# note:      the file should contain lines with items separated
#            by $delimiter characters (default space). The final
#            two items should contain the correct tag and the 
#            guessed tag in that order. Sentences should be
#            separated from each other by empty lines or lines
#            with $boundary fields (default -X-).
# url:       http://lcg-www.uia.ac.be/conll2000/chunking/
# started:   1998-09-25
# version:   2000-04-14
# author:    Erik Tjong Kim Sang <erikt@uia.ua.ac.be>

# modified:  Taku Kudoh <taku-ku@is.aist-nara.ac.jp>
# $Id: conlleval.pl.in,v 1.1.1.1 2001/02/25 18:47:45 taku-ku Exp $;

use strict;

my $false = 0;
my $true = 42;

my $boundary = "-X-";     # sentence boundary
my $correct;              # current corpus chunk tag (I,O,B)
my $correctChunk = 0;     # number of correctly identified chunks
my $correctTags = 0;      # number of correct chunk tags
my $correctType;          # type of current corpus chunk tag (NP,VP,etc.)
my $delimiter = " ";      # field delimiter
my $FB1 = 0.0;            # FB1 score (Van Rijsbergen 1979)
my $firstItem;            # first feature (for sentence boundary checks)
my $foundCorrect = 0;     # number of chunks in corpus
my $foundGuessed = 0;     # number of identified chunks
my $guessed;              # current guessed chunk tag
my $guessedType;          # type of current guessed chunk tag
my $i;                    # miscellaneous counter
my $inCorrect = $false;   # currently processed chunk is correct until now
my $lastCorrect = "O";    # previous chunk tag in corpus
my $lastCorrectType = ""; # type of previously identified chunk tag
my $lastGuessed = "O";    # previously identified chunk tag
my $lastGuessedType = ""; # type of previous chunk tag in corpus
my $lastType;             # temporary storage for detecting duplicates
my $line;                 # line
my $nbrOfFeatures = -1;   # number of features per line
my $precision = 0.0;      # precision score
my $recall = 0.0;         # recall score
my $tokenCounter = 0;     # token counter (ignores sentence breaks)

my %correctChunk = ();    # number of correctly identified chunks per type
my %foundCorrect = ();    # number of chunks in corpus per type
my %foundGuessed = ();    # number of identified chunks per type

my @features;             # features on line
my @sortedTypes;          # sorted list of chunk type names

# sanity check
#if (@ARGV) { die "conlleval: unexpected command line argument\n"; } # if file is not given read from stdin
# (modified by taku kudoh)
# process input
while (<>) {  # read from STDIN and FILEs.. (modified by taku kudoh)
   chomp($line = $_);
   @features = split(/\s/,$line); # split tab or space (modified by taku kudoh)

   # added condition of sentence boundaries, EOS line is accepted.
   if (@features == 0 || (@features == 1 && $features[0] eq "EOS")) { 
      @features = ($boundary,"O","O"); 
   } elsif ($nbrOfFeatures < 0) { 
      $nbrOfFeatures = $#features;
   } elsif ($nbrOfFeatures != $#features and @features != 0) {
      printf STDERR "unexpected number of features: %d (%d)\n",$#features+1,$nbrOfFeatures+1;
      exit(1);
   }
   
   ($guessed,$guessedType) = split(/-/,pop(@features));
   ($correct,$correctType) = split(/-/,pop(@features));
   $guessedType = $guessedType ? $guessedType : "";
   $correctType = $correctType ? $correctType : "";
   $firstItem = shift(@features);

   # 1999-06-26 sentence breaks should always be counted as out of chunk
   if ( $firstItem eq $boundary ) { $guessed = "O"; }

   if ($inCorrect) {
      if ( &endOfChunk($lastCorrect,$correct,$lastCorrectType,$correctType) and
           &endOfChunk($lastGuessed,$guessed,$lastGuessedType,$guessedType) and
           $lastGuessedType eq $lastCorrectType) {
         $inCorrect=$false;
         $correctChunk++;
         $correctChunk{$lastCorrectType} = $correctChunk{$lastCorrectType} ?
             $correctChunk{$lastCorrectType}+1 : 1;
      } elsif ( 
           &endOfChunk($lastCorrect,$correct,$lastCorrectType,$correctType) != 
           &endOfChunk($lastGuessed,$guessed,$lastGuessedType,$guessedType) or
           $guessedType ne $correctType ) {
         $inCorrect=$false; 
      }
   }

   if ( &startOfChunk($lastCorrect,$correct,$lastCorrectType,$correctType) and 
        &startOfChunk($lastGuessed,$guessed,$lastGuessedType,$guessedType) and
        $guessedType eq $correctType) { $inCorrect = $true; }

   if ( &startOfChunk($lastCorrect,$correct,$lastCorrectType,$correctType) ) {
      $foundCorrect++; 
      $foundCorrect{$correctType} = $foundCorrect{$correctType} ?
          $foundCorrect{$correctType}+1 : 1;
   }
   if ( &startOfChunk($lastGuessed,$guessed,$lastGuessedType,$guessedType) ) {
      $foundGuessed++; 
      $foundGuessed{$guessedType} = $foundGuessed{$guessedType} ?
          $foundGuessed{$guessedType}+1 : 1;
   }
   if ( $firstItem ne $boundary ) { 
      if ( $correct eq $guessed and $guessedType eq $correctType ) { 
         $correctTags++; 
      }
      $tokenCounter++; 
   }

   $lastGuessed = $guessed;
   $lastCorrect = $correct;
   $lastGuessedType = $guessedType;
   $lastCorrectType = $correctType;
}
if ($inCorrect) { 
   $correctChunk++;
   $correctChunk{$lastCorrectType} = $correctChunk{$lastCorrectType} ?
       $correctChunk{$lastCorrectType}+1 : 1;
}

# compute overall precision, recall and FB1 (default values are 0.0)
$precision = 100*$correctChunk/$foundGuessed if ($foundGuessed > 0);
$recall = 100*$correctChunk/$foundCorrect if ($foundCorrect > 0);
$FB1 = 2*$precision*$recall/($precision+$recall)
   if ($precision+$recall > 0);

# print overall performance
printf "processed $tokenCounter tokens with $foundCorrect phrases; ";
printf "found: $foundGuessed phrases; correct: $correctChunk.\n";
if ($tokenCounter>0) {
   printf "accuracy: %6.2f%%; ",100*$correctTags/$tokenCounter;
   printf "precision: %6.2f%%; ",$precision;
   printf "recall: %6.2f%%; ",$recall;
   printf "FB1: %6.2f\n",$FB1;
}

# sort chunk type names
undef($lastType);
@sortedTypes = ();
foreach $i (sort (keys %foundCorrect,keys %foundGuessed)) {
   if (not($lastType) or $lastType ne $i) { 
      push(@sortedTypes,($i));
   }
   $lastType = $i;
}
# print performance per chunk type
for $i (@sortedTypes) {
   next if ($i eq ""); # ignore empty type (taku kudoh)
   $correctChunk{$i} = $correctChunk{$i} ? $correctChunk{$i} : 0;
   if (not($foundGuessed{$i})) { $precision = 0.0; }
   else { $precision = 100*$correctChunk{$i}/$foundGuessed{$i}; }
   if (not($foundCorrect{$i})) { $recall = 0.0; }
   else { $recall = 100*$correctChunk{$i}/$foundCorrect{$i}; }
   if ($precision+$recall == 0.0) { $FB1 = 0.0; }
   else { $FB1 = 2*$precision*$recall/($precision+$recall); }
   printf "%17s: ",$i;
   printf "precision: %6.2f%%; ",$precision;
   printf "recall: %6.2f%%; ",$recall;
   printf "FB1: %6.2f\n",$FB1;
}

exit 0;

# endOfChunk: checks if a chunk ended between the previous and current word
# arguments:  previous and current chunk tags, previous and current types
# note:       this code is capable of handling other chunk representations
#             than the default CoNLL-2000 ones, see EACL'99 paper of Tjong
#             Kim Sang and Veenstra http://xxx.lanl.gov/abs/cs.CL/9907006

sub endOfChunk {
   my $prevTag = shift(@_);
   my $tag = shift(@_);
   my $prevType = shift(@_);
   my $type = shift(@_);
   my $chunkEnd = $false;

   if ( $prevTag eq "B" and $tag eq "B" ) { $chunkEnd = $true; }
   if ( $prevTag eq "B" and $tag eq "O" ) { $chunkEnd = $true; }
   if ( $prevTag eq "I" and $tag eq "B" ) { $chunkEnd = $true; }
   if ( $prevTag eq "I" and $tag eq "O" ) { $chunkEnd = $true; }

   if ( $prevTag eq "E" and $tag eq "E" ) { $chunkEnd = $true; }
   if ( $prevTag eq "E" and $tag eq "I" ) { $chunkEnd = $true; }
   if ( $prevTag eq "E" and $tag eq "O" ) { $chunkEnd = $true; }
   if ( $prevTag eq "I" and $tag eq "O" ) { $chunkEnd = $true; }

   if ($prevTag ne "O" and $prevTag ne "." and $prevType ne $type) { 
      $chunkEnd = $true; 
   }

   # corrected 1998-12-22: these chunks are assumed to have length 1
   if ( $prevTag eq "]" ) { $chunkEnd = $true; }
   if ( $prevTag eq "[" ) { $chunkEnd = $true; }

   return($chunkEnd);   
}

# startOfChunk: checks if a chunk started between the previous and current word
# arguments:    previous and current chunk tags, previous and current types
# note:         this code is capable of handling other chunk representations
#               than the default CoNLL-2000 ones, see EACL'99 paper of Tjong
#               Kim Sang and Veenstra http://xxx.lanl.gov/abs/cs.CL/9907006

sub startOfChunk {
   my $prevTag = shift(@_);
   my $tag = shift(@_);
   my $prevType = shift(@_);
   my $type = shift(@_);
   my $chunkStart = $false;

   if ( $prevTag eq "B" and $tag eq "B" ) { $chunkStart = $true; }
   if ( $prevTag eq "I" and $tag eq "B" ) { $chunkStart = $true; }
   if ( $prevTag eq "O" and $tag eq "B" ) { $chunkStart = $true; }
   if ( $prevTag eq "O" and $tag eq "I" ) { $chunkStart = $true; }

   if ( $prevTag eq "E" and $tag eq "E" ) { $chunkStart = $true; }
   if ( $prevTag eq "E" and $tag eq "I" ) { $chunkStart = $true; }
   if ( $prevTag eq "O" and $tag eq "E" ) { $chunkStart = $true; }
   if ( $prevTag eq "O" and $tag eq "I" ) { $chunkStart = $true; }

   if ($tag ne "O" and $tag ne "." and $prevType ne $type) { 
      $chunkStart = $true; 
   }

   # corrected 1998-12-22: these chunks are assumed to have length 1
   if ( $tag eq "[" ) { $chunkStart = $true; }
   if ( $tag eq "]" ) { $chunkStart = $true; }

   return($chunkStart);   
}
