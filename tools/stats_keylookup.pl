#!/usr/bin/perl -w
use File::Glob qw(:globally :nocase);
use Data::Dumper;

%store = ();

sub dodir {
  my @files = <stats*>;

  #extracts the stats part out of the original file, saving a bunch of space.
  foreach my $infile (@files){
    open (FILE, $infile) || die "cant open $infile\n";
    my $date = "";
    if($infile =~ /(\d+)\./){
      $date = $1;
    }
    print "Reading: $infile  $date \n";
 
    my @splitarray = split /_/,$infile;
    my $algo = $splitarray[1];
   
    if($#splitarray == 2){
        $algo = $splitarray[1]."_".$splitarray[2];
    }

    print Dumper(\@splitarray);
    print $algo."\n";
    
    while(<FILE>){
      my $line = $_;
      if($line =~ /highwater/){
        chomp $line;
        $line =~ s/Flow highwater mark: //;
        $store{$date}{$algo} = $line;
        print $algo.",".$line."\n";
        last;
      }
    }
  }
}


my @dirs = <200312*>;
my $i = 1;
#print Dumper(\@dirs);
#print `pwd`;

foreach my $x (@dirs){
  #print "\n\nmoving into $x\n";
  #print `pwd`;
  chdir $x;
  #print `pwd`;
  dodir();
  chdir "..";
  
  #if($i == 2){ exit(1); }
  
  $i++;
  
}


foreach my $j (sort keys(%store)){
   #print $j ."\n";
   my $num = scalar(keys(%{$store{$j}}));
   if($num != 9){
      print "Deleting $j as $num != 9\n";
#      delete $store{$j};
   }
  
}
print Dumper(\%store);


my @dates = sort keys(%store);
my @algos = sort keys(%{$store{$dates[0]}});


foreach my $d (@dates){ 
  print ",".$d;
}
print "\n";

foreach my $a (@algos){
  print $a;
  foreach my $d (@dates){ 
    if(defined($store{$d}{$a})){   
    	print ",".$store{$d}{$a};
    }
    else{
 	print ",";
    }
    
    
  }
  print "\n";

}














