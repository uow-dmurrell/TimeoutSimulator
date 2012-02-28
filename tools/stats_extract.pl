#!/usr/bin/perl -w
use File::Glob qw(:globally :nocase);
use Data::Dumper;

sub makefiles {
  my @files = <200*\.csv>;

  #extracts the stats part out of the original file, saving a bunch of space.
  foreach my $infile (@files){
    print $infile . "\n";

    my $outfile = 'stats.';

    my @splitarray = split /_/,$infile;

    $outfile .= $splitarray[0]."_".$splitarray[1];

    if($#splitarray == 3){
      $outfile .= "_".$splitarray[2];
    }


    #cat the stats of the file to another file.
    $exe = "head -n1 $infile > $outfile";
    #print $exe."\n";
    `$exe`;
    $exe = "tail -n12 $infile >> $outfile";
    #print $exe."\n";
    `$exe`;
  }
}

my @dirs = <200706*>;
my $i = 1;
print Dumper(\@dirs);
print `pwd`;

foreach my $x (@dirs){
  if($x =~ /2007060/){ next; }
  print "\n\nmoving into $x\n";
  print `pwd`;
  chdir $x;
  print `pwd`;
  makefiles();

  chdir "..";
  
  #if($i == 2){ exit(1); }
  
  $i++;
  
}
