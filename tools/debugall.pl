#!/usr/bin/perl -w


$trace = shift;
$debug = shift;

@algos = ('claffy','claffy_tcp','claffy_unlimited','rfc','netramet','netramet_short','mbet','dots','pgat');

for $i (@algos){
  `./example/.libs/example -d $debug -e $i $trace`;
}
