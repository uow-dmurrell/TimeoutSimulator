#!/usr/bin/perl -w
use strict;
use warnings;
use diagnostics;

my $trace = "";
$trace = shift;
my $date = "";
$date = shift;

if(!$trace || !$date ){
	print "$0 tracefilename timestamp\n";
	exit 1;
}

my @filenames = glob("$trace*$date*.csv");

my $algo = "";

`rm *.input`;

foreach my $file (@filenames) {
	#cat output.pcap_netramet-short_20111030-004707.csv |grep level|sed 's|Flowlevel,||g'|sed 's|,| |g' >> netramet.input
	if($file =~ /$trace\_(.*)\_$date/){
		$algo = $1;
	}
	else{
		exit("no algo found for $file, exiting!\n");
	}

	`cat $file |grep level|sed 's|Flowlevel,||g'|sed 's|,| |g' >> $algo.input`;
}

my @algonames = glob("*.input");

my $type = 'set terminal png';
my $outp = 'set output "foo.png"';
my $cmdl = "plot ";


for(my $i = 0; $i <= $#algonames; $i++){
	my $algorithm = $algonames[$i];
	$algorithm =~ s/\.input//g;

	$cmdl .= ' "./'.$algonames[$i].'" using 2:1 with lines title "'.$algorithm.'"';
	if($i != $#algonames){
		$cmdl .= ", ";
	}
}

print $cmdl."\n";
#`rm *.input`;

#`gnuplot $type; $outp; $cmdl;`;
open (MYFILE, '>gnuplot.cfg');
print MYFILE "$type\n";
print MYFILE "$outp\n";
print MYFILE "$cmdl\n";
close (MYFILE); 

`gnuplot gnuplot.cfg`;

