#!/usr/bin/perl -w

my $trace = "";
$trace = shift;

if (!$trace){
  print "Usage: $0 tracefile\n";
  exit 1;
}

@algos = ('claffy','claffy_tcp','claffy_unlimited','rfc','netramet','netramet_short','mbet','dots','pgat');

my $me = `whoami`;
chomp $me;

if($trace =~ m|(\d\d\d\d.*)|){
  $tfile = $1;
}


my $date = `date +%Y%m%d-%H%M%S`;
my $year = `date +%Y`;

chomp $date; chomp $year;

my $rundir = $tfile."_".$date;
my $home = `ls -d ~`; #"/home/dtm7/";
chomp($home);

my $runpath = $home."/School/".$rundir;

print "moving to $home\n";
chdir($home);
print "making $runpath\n";
mkdir $runpath;
print "moving to $runpath\n";
chdir($runpath);

my $example  = "$home/School/591/libflowmanager-2.0.0/example/example";
my $sorter   = "$home/School/591/tools/sorter.pl"; 
my $readline = "$home/School/591/tools/readlines.pl";


for $i (@algos){
  print "$example -e $i $trace"  ."\n";
  `$example -e $i $trace`;
  print "$sorter $tfile"."_".$i."_20*"	."\n";
  `$sorter $tfile"."_".$i."_20*`;
}
