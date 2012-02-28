#!/usr/bin/perl -w

my $trace = "";

$trace = shift;


if (!$trace){
  print "Usage: $0 type:/fullpath/tracefile\n";
  exit 1;
}
if($trace !~ m|:/| || $trace !~ m|/home|){
  print "Usage: $0 type:/fullpath/tracefile\n";
  exit 1;
}

@algos = ('claffy','claffy_tcp','claffy_unlimited','rfc','netramet','netramet_short','mbet','dots','pgat');

my $tfile = "";

if($trace =~ m|(\d\d\d\d.*)|){
  $tfile = $1;
}


my $date = `date +%Y%m%d-%H%M%S`;
my $year = `date +%Y`;

chomp $date; chomp $year;

my $rundir = $tfile."_".$date;
my $home = `ls -d ~`; #"/home/dtm7/";
chomp($home);
$home = $home . "/";
my $runpath = $home.$rundir;


print "moving to $home\n";
chdir($home);
print "making $runpath\n";
mkdir $runpath;
print "moving to $runpath\n";
chdir($runpath);

my $example  = $home."School/591/libflowmanager-2.0.0/example/example";
my $sorter   = $home."School/591/tools/sorter.pl"; 
my $readline = $home."School/591/tools/readlines.pl";

my $range = 1000000;
my $j = int(rand($range));


my @queue = ();

for $i (@algos){

  my $scratchloc = "/scratch/$j";

  my $file = $home."runfile/start_$j";

  open (FILE,">$file");
  
  print FILE '#!/bin/bash -x
  
  #make some space in the scratch dir
  mkdir '.$scratchloc.'
  cd '.$scratchloc.'

  
  #setup our trace run
  '.$example.' -e '.$i.' '.$trace.'

  
  #copy our output to the scratch space
  #cp '.$tfile.'_'.$i.'_20* '.$scratchloc.'

  #read in output, and run sorter on it
  '.$sorter.' '.$tfile.'_'.$i.'_20*

  #cat the stats of the file to another file.
  head -n1 '.$tfile.'_'.$i.'_20* > stats.'.$tfile.'_'.$i.'
  tail -n11 '.$tfile.'_'.$i.'_20* >> stats.'.$tfile.'_'.$i.'
  
  #delete the output.
  rm 20*
  
  #gzip the remaning files
  gzip *.csv

  #copy everything back
  cp * '.$runpath.'
  #cp dtm7* '.$runpath.'
  
  

  #dump out the server vars to a file
  echo "PBS_O_HOST: $PBS_O_HOST" >> serverstatus.txt
  echo "PBS_SERVER: $PBS_SERVER" >> serverstatus.txt
  echo "PBS_O_QUEUE: $PBS_O_QUEUE" >> serverstatus.txt
  echo "PBS_ARRAYID: $PBS_ARRAYID" >> serverstatus.txt
  echo "PBS_ENVIRONMENT: $PBS_ENVIRONMENT" >> serverstatus.txt
  echo "PBS_JOBID: $PBS_JOBID" >> serverstatus.txt
  echo "PBS_JOBNAME: $PBS_JOBNAME" >> serverstatus.txt
  echo "PBS_NODEFILE: $PBS_NODEFILE" >> serverstatus.txt
  echo "PBS_QUEUE: $PBS_QUEUE" >> serverstatus.txt
  rm '.$file.'
  rm -rf '.$scratchloc.'
  ';

  #print "Writing: to $file:\n$str\n";
  close(FILE);
  
  print "Scheduling this:\tqsub -N dtm7_$i $file"	."\n";
  my $scriptval = "";
  #-q expert -l walltime=3:00:00,pmem=1500m 
  $scriptval = `qsub -o $runpath  -N dtm7_$i $file`;
  chomp($scriptval);
  $scriptval =~ s/.symphony.cs.waikato.ac.nz//;
  push @queue,$scriptval;
  $j++;
}

my $file = $home."runfile/start_$j";

my $depends = "";

foreach my $x (@queue){ 
  $depends .= ":".$x;
}

open (FILE,">$file");
print FILE '#!/bin/bash'."\n";
print FILE "cd $runpath\n";
print FILE "$readline $runpath $tfile $year\n";
close (FILE);
#-q expert -l walltime=5:00:00,pmem=4000m 
my $runner = "qsub -W depend=afterok$depends $file -N dtm7_summarizer";
print "Scheduling this:\t$runner"	."\n";

`$runner`;





