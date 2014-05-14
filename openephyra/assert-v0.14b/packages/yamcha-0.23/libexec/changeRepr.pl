#!/bin/perl
#             there are seven formats:
#             - iob1:   standard RM95
#             - iob2:   RM95 plus B tag at every chunk start
#             - ioe1:   replaces RM95 IB sequences by EI
#             - ioe2:   as ioe1 but with all end of chunks marked with E 
#             - io:     as iob1 but with all B's replaced by I's
#             - openb:  mark chunk-initial word with [, rest with .
#             - closeb: mark chunk-final word with ], rest with .
# usage:     changeRepr [iob1|iob2|ioe1|ioe2|io|openb|closeb] < file
# 981125 erikt@uia.ua.ac.be
# version date 20000730

$false = 0;
$sep = " ";
$true = 1;
$usage = "usage: changeRepr [iob1|iob2|ioe1|ioe2|io|openb|closeb] < file";

sub endOfChunk {
   local($prevTag) = shift(@_);
   local($tag) = shift(@_);
   local($prevType) = shift(@_);
   local($type) = shift(@_);
   local($chunkEnd) = $false;

   if ( $prevTag eq "B" && $tag eq "B" ) { $chunkEnd = $true; }
   if ( $prevTag eq "B" && $tag eq "O" ) { $chunkEnd = $true; }
   if ( $prevTag eq "I" && $tag eq "B" ) { $chunkEnd = $true; }
   if ( $prevTag eq "I" && $tag eq "O" ) { $chunkEnd = $true; }

   if ( $prevTag eq "E" && $tag eq "E" ) { $chunkEnd = $true; }
   if ( $prevTag eq "E" && $tag eq "I" ) { $chunkEnd = $true; }
   if ( $prevTag eq "E" && $tag eq "O" ) { $chunkEnd = $true; }
   if ( $prevTag eq "I" && $tag eq "O" ) { $chunkEnd = $true; }

   if ($prevTag ne "O" && $prevType ne $type) { $chunkEnd = $true; }

   $chunkEnd;   
}

sub startOfChunk {
   local($prevTag) = shift(@_);
   local($tag) = shift(@_);
   local($prevType) = shift(@_);
   local($type) = shift(@_);
   local($chunkStart) = $false;

   if ( $prevTag eq "B" && $tag eq "B" ) { $chunkStart = $true; }
   if ( $prevTag eq "I" && $tag eq "B" ) { $chunkStart = $true; }
   if ( $prevTag eq "O" && $tag eq "B" ) { $chunkStart = $true; }
   if ( $prevTag eq "O" && $tag eq "I" ) { $chunkStart = $true; }

   if ( $prevTag eq "E" && $tag eq "E" ) { $chunkStart = $true; }
   if ( $prevTag eq "E" && $tag eq "I" ) { $chunkStart = $true; }
   if ( $prevTag eq "O" && $tag eq "E" ) { $chunkStart = $true; }
   if ( $prevTag eq "O" && $tag eq "I" ) { $chunkStart = $true; }

   if ($tag ne "O" && $prevType ne $type) { $chunkStart = $true; }

   $chunkStart;   
}

sub iob1 {
   local($prevTag) = shift(@_);
   local($tag) = shift(@_);
   local($prevType) = shift(@_);
   local($type) = shift(@_);
   local($newTag) = $tag;

   if ( &startOfChunk($prevTag,$tag,$prevType,$type) && 
        &endOfChunk($prevTag,$tag,$prevType,$type) &&
        $prevType eq $type) {
      $newTag = "B";
   } elsif ( $tag ne "O" ) {
      $newTag = "I";
   }   
   $newTag;
}

sub iob2 {
   local($prevTag) = shift(@_);
   local($tag) = shift(@_);
   local($prevType) = shift(@_);
   local($type) = shift(@_);
   local($newTag) = $tag;

   if ( &startOfChunk($prevTag,$tag,$prevType,$type) ) {
      $newTag = "B";
   } elsif ( $tag ne "O" ) {
      $newTag = "I";
   }   
   $newTag;
}

sub ioe1 {
   local($prevTag) = shift(@_);
   local($tag) = shift(@_);
   local($prevType) = shift(@_);
   local($type) = shift(@_);
   local($newTag) = $prevTag;

   if ( &startOfChunk($prevTag,$tag,$prevType,$type) && 
        &endOfChunk($prevTag,$tag,$prevType,$type) &&
        $prevType eq $type) {
      $newTag = "E";
   } elsif ( $prevTag ne "O" ) {
      $newTag = "I";
   }   
   $newTag;
}

sub ioe2 {
   local($prevTag) = shift(@_);
   local($tag) = shift(@_);
   local($prevType) = shift(@_);
   local($type) = shift(@_);
   local($newTag) = $prevTag;

   if ( &endOfChunk($prevTag,$tag,$prevType,$type) ) {
      $newTag = "E";
   } elsif ( $prevTag ne "O" ) {
      $newTag = "I";
   }   
   $newTag;
}

sub openb {
   local($prevTag) = shift(@_);
   local($tag) = shift(@_);
   local($prevType) = shift(@_);
   local($type) = shift(@_);
   local($newTag) = ".";

   if ( &startOfChunk($prevTag,$tag,$prevType,$type) ) {
      $newTag = "[";
   }
   $newTag;
}

sub closeb {
   local($prevTag) = shift(@_);
   local($tag) = shift(@_);
   local($prevType) = shift(@_);
   local($type) = shift(@_);
   local($newTag) = ".";

   if ( &endOfChunk($prevTag,$tag,$prevType,$type) ) {
      $newTag = "]";
   }
   $newTag;
}

sub io {
   local($prevTag) = shift(@_);
   local($tag) = shift(@_);
   local($newTag) = ".";

   if ( $tag eq "O" ) {
      $newTag = "O";
   } else { $newTag = "I"; }
   $newTag;
}

if (! defined($ARGV[0])) { print STDERR "$usage\n"; exit(1); }
$arg0 = shift(@ARGV);
$prevLastFeature = "O";
$newPrevLastFeature = "!";
$emptyLastLine = $true;
$type = "";
$prevType = "";
while (<>) {
   $line = "$_";
   chop($line);
   $emptyLine = $false;
   if ($line !~ /^$sep*$/) {
      @features = split(/$sep/,$line);
      ($lastFeature,$type) = split(/-/,pop(@features));
      $type = $type ? $type : "";
      $line = join($sep,@features);
   } else {
      $lastFeature = "O";
      $type = "";
      $line = "";
      $emptyLine = $true;
   }
   if ($lastFeature !~ /^[IOBE]$/) {
      print STDERR "cannot handle IOBE tag with value \"$lastFeature\"\n";
      exit(1);
   }
   if ( $arg0 eq "iob1" ) { 
      $newLastFeature = 
         &iob1($prevLastFeature,$lastFeature,$prevType,$type);
   } elsif ( $arg0 eq "iob2" ) { 
      $newLastFeature = 
         &iob2($prevLastFeature,$lastFeature,$prevType,$type);
   } elsif ( $arg0 eq "ioe1" ) { 
      $newPrevLastFeature = 
         &ioe1($prevLastFeature,$lastFeature,$prevType,$type);
   } elsif ( $arg0 eq "ioe2" ) { 
      $newPrevLastFeature = 
         &ioe2($prevLastFeature,$lastFeature,$prevType,$type);
   } elsif ( $arg0 eq "openb" ) { 
      $newLastFeature = 
         &openb($prevLastFeature,$lastFeature,$prevType,$type);
   } elsif ( $arg0 eq "closeb" ) { 
      $newPrevLastFeature = 
         &closeb($prevLastFeature,$lastFeature,$prevType,$type);
   } elsif ( $arg0 eq "io" ) { 
      $newLastFeature = 
         &io($prevLastFeature,$lastFeature,$prevType,$type);
   } else {
      print STDERR "$usage\n";
      exit(1);
   }
   if ( defined($prevLine) ) { 
      if ($emptyLastLine) { print "\n"; }
      else {
         if ($prevLine eq "") { print "$newPrevLastFeature"; }
         else { print "$prevLine$sep$newPrevLastFeature"; }
         if ($prevType ne "" 
             && $newPrevLastFeature ne "O"
             && $newPrevLastFeature ne ".") { print "-$prevType"; }
         print "\n";        
      }
   }
   $prevType = $type;
   $prevLine = $line;
   $prevLastFeature = $lastFeature;   
   $newPrevLastFeature = $newLastFeature;
   $emptyLastLine = $emptyLine;   
}
$lastFeature = "O";
$type = "";
if ( defined($prevLine) ) {
   if ( $arg0 eq "ioe1" ) { 
      $newPrevLastFeature = 
         &ioe1($prevLastFeature,$lastFeature,$prevType,$type);
   } elsif ( $arg0 eq "ioe2" ) { 
      $newPrevLastFeature = 
         &ioe2($prevLastFeature,$lastFeature,$prevType,$type);
   } elsif ( $arg0 eq "closeb" ) { 
      $newPrevLastFeature = 
         &closeb($prevLastFeature,$lastFeature,$prevType,$type);
   } 
   if ($emptyLastLine) { print "\n"; }
   else {
      if ($prevLine eq "") { print "$newPrevLastFeature"; }
      else { print "$prevLine$sep$newPrevLastFeature"; }
      if ($prevType ne "" 
          && $newPrevLastFeature ne "O"
          && $newPrevLastFeature ne ".") { print "-$prevType"; }
      print "\n";        
   }
} 

exit(0);

