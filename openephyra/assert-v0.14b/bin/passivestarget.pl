#!/bin/perl

$corpus_filename = $ARGV[0];
print "corpus file:", $corpus_filename, "\n";

$BE_GET = "is|are|was|were|be|am|been|get|gets|got|gotten|getting|being";

$Passive_1 = "(/^VP/<(VBN<TARGET)!>NP%(VB<<$BE_GET))";
$Passive_2 = "(/^VP/<(/^VP/<(VBN<TARGET))!>NP%(VB<<$BE_GET))";
$Passive_3 = "(ADJP<(VBN<TARGET)!>NP%(VB<<$BE_GET))";
$Passive_4 = "(/^VP/<(VBN<TARGET)!>NP%(AUX<<$BE_GET))";
$Passive_5 = "(/^VP/<(/^VP/<(VBN<TARGET))!>NP%(AUX<<$BE_GET))";
$Passive_6 = "(ADJP<(VBN<TARGET)!>NP%(AUX<<$BE_GET))";
$Passive_7 = "(/^VP/<(VBN<TARGET)!>NP%(/VB/<$BE_GET))";
$Passive_8 = "(/^VP/<(/^VP/<(VBN<TARGET))!>NP%(/VB/<$BE_GET))";
$Passive_9 = "(ADJP<(VBN<TARGET)!>NP%(/VB/<$BE_GET))";
$Passive_10 = "(/^VP/<(VBN<TARGET)!>NP<(/^VP/%(/VB/<$BE_GET)))";
$Passive_11 = "(/^VP/<(/^VP/<(VBN<TARGET))!>NP<(/^VP/%(/VB/<$BE_GET)))";
$Passive_12 = "(/^VP/<(VBN<TARGET)!>NP%(/^VP/<(/VB/<$BE_GET)))";
$Passive_13 = "(/^VP/<(/^VP/<(VBN<TARGET))!>NP%(/^VP/<(/VB/<$BE_GET)))";
$Passive_14 = "(/^VP/<(VBN<TARGET)>NP%(NP))";

#--- Ashley's addition to the rules ---#
#$Passive_15 = "(/^VP/<(/^VP/<(VBN<TARGET))>NP%(NP))";


system("$ENV{\"ASSERT\"}/packages/Tgrep2/tgrep2","-c",$corpus_filename,"$Passive_1","-s >>--> rule 1: ");
system("$ENV{\"ASSERT\"}/packages/Tgrep2/tgrep2","-c",$corpus_filename,"$Passive_2","-s >>--> rule 2: ");
system("$ENV{\"ASSERT\"}/packages/Tgrep2/tgrep2","-c",$corpus_filename,"$Passive_3","-s >>--> rule 3: ");
system("$ENV{\"ASSERT\"}/packages/Tgrep2/tgrep2","-c",$corpus_filename,"$Passive_4","-s >>--> rule 4: ");
system("$ENV{\"ASSERT\"}/packages/Tgrep2/tgrep2","-c",$corpus_filename,"$Passive_5","-s >>--> rule 5: ");
system("$ENV{\"ASSERT\"}/packages/Tgrep2/tgrep2","-c",$corpus_filename,"$Passive_6","-s >>--> rule 6: ");
system("$ENV{\"ASSERT\"}/packages/Tgrep2/tgrep2","-c",$corpus_filename,"$Passive_7","-s >>--> rule 7: ");
system("$ENV{\"ASSERT\"}/packages/Tgrep2/tgrep2","-c",$corpus_filename,"$Passive_8","-s >>--> rule 8: ");
system("$ENV{\"ASSERT\"}/packages/Tgrep2/tgrep2","-c",$corpus_filename,"$Passive_9","-s >>--> rule 9: ");
system("$ENV{\"ASSERT\"}/packages/Tgrep2/tgrep2","-c",$corpus_filename,"$Passive_10","-s >>--> rule 10: ");
system("$ENV{\"ASSERT\"}/packages/Tgrep2/tgrep2","-c",$corpus_filename,"$Passive_11","-s >>--> rule 11: ");
system("$ENV{\"ASSERT\"}/packages/Tgrep2/tgrep2","-c",$corpus_filename,"$Passive_12","-s >>--> rule 12: ");
system("$ENV{\"ASSERT\"}/packages/Tgrep2/tgrep2","-c",$corpus_filename,"$Passive_13","-s >>--> rule 13: ");
system("$ENV{\"ASSERT\"}/packages/Tgrep2/tgrep2","-c",$corpus_filename,"$Passive_14","-s >>--> rule 14: ");
#system("$ENV{\"ASSERT\"}/packages/Tgrep2/tgrep2","-c",$corpus_filename,"$Passive_15","-s >>--> rule 15: ");

