#!/usr/bin/perl -w

my $filename = shift;
if(!$filename){
	print "$0 filename\nSorts expired flows by hash\n";
}

$header = 'head -n2 '.$filename.' > sorted.'.$filename;

$a = 	"/bin/egrep -v '^Flowlevel|^Trace|^List|^Flow|^Final|^Global|^Algorithm|^Claffy' ". $filename .
		" |sort  --general-numeric-sort --field-separator=, --key=6 >> " .'sorted.'.$filename;

`$header`;
`$a`;
		
