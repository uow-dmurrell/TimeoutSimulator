#!/usr/bin/perl -w
use strict;
use warnings;
use diagnostics;

use Data::Dumper;

sub build_store;
sub write_flows_missing_algos;
sub get_write_handles;
sub dump_store;

my $trace = "";
$trace = shift;
my $date = "";
$date = shift;

if(!$trace || !$date ){
	print "$0 tracefilename timestamp\n";
	exit 1;
}

my @filenames = glob("sorted.$trace*$date*.csv*");
#print Dumper(@filenames);

my @algos = ();


my %handles = get_write_handles(glob("sorted."."$trace*$date*.csv*"));

print Dumper(\%handles);



foreach (keys %handles) {
  my $line = $_;
  print $_
}

exit(1);



my $store = {};

#print Dumper($store);

##summarising per flow data: 
#claffy unlimited is our truth for:
#	time_start 
#	time_end
#	packet_in
#	packet_out
#	data_in
#	data_out

#find flow most accurate estimate
#note flows that split
#if split, is it effective?


#$VAR1 = {
          #'110.175.156.36:60591->192.168.1.145:6881 [tcp]' => {
					#'mbet' => [
								#{
								  #'time_start' => '2.469791',
								  #'time_expire' => '75.386131',
								  #'data_in' => '144',
								  #'packet_out' => '0',
								  #'status' => 'Active',
								  #'time_end' => '11.386131',
								  #'packet_in' => '3',
								  #'list' => 'UDP flows',
								  #'data_out' => '0'
								#}
							  #],
					#'claffy' => []



my $flow = "";

foreach my $x (keys %handles){
    print $x."\n";
}
my $filename = shift;


#decompress
if ($filename =~ m|.gz$|){
    `gzip -d $filename`;
    $filename =~ s|.gz$||;		
}
if ($filename =~ m|.bz2$|){
    `bzip2 -d $filename`;
    $filename =~ s|.bz2$||;
}
print "Loading $filename\n";	

open FILE, $filename or die $!;
my @lines = <FILE>;

my $algo;
foreach my $line (@lines) {
    #List,Status,From,To,Proto,Hash,Packets In,Packets Out,Bytes In,Bytes Out,Start Time(n), End Time(n), Expiretime(n), Start Time, End Time, Expire Time
    if ($line =~ /^Trace filepath:/){ 
        if($line =~ /Algorithm:(.*)$/){ 
            $algo = $1; 
            push (@algos,$algo);
        }
        next; 
    }
    if ($line =~ /^List,Status/){ next; }
    if ($line =~ /^Algorithm/)  { last; }
    if ($line =~ /^Flowlevel/)  { next; }
    #chomp $line;
    #print $line."\n";
    
    my @v = split(/,/,$line);
    my $list    = $v[0];
    my $status  = $v[1];
    my $src     = $v[2];
    my $dst     = $v[3];
    my $proto   = $v[4];
    my $hash    = $v[5];
    my $pkt_i   = $v[6];
    my $pkt_o   = $v[7];
    my $data_i  = $v[8];
    my $data_o  = $v[9];
    my $time_st = $v[10];
    my $time_en = $v[11];
    my $time_ex = $v[12];
    
    #for the moment, we're going to ignore icmp. Its messy.
    if($proto == 1) { 
        $proto = "icmp";
        next;
    
    }
    elsif($proto == 6) { $proto = "tcp";}
    elsif($proto == 17){ $proto = "udp";}
    
    
    my %details = (
        time_start  => $time_st, 
        time_end    => $time_en, 
        time_expire => $time_ex,
        packet_in   => $pkt_i,
        packet_out  => $pkt_o,
        data_in     => $data_i,
        data_out    => $data_o,
        list        => $list,
        status      => $status,
    );
    
    my @tmparr = ();
    push @tmparr, { %details };
    
    my $flow = $src."->".$dst." [".$proto."]";

    #print "Pushing $flow\n";	

    push @{$store->{$flow}{$algo}}, @tmparr;
    
    #for my $what (keys %details) {
    #	$store->{$src."->".$dst."_".$proto}{$algo}{$what} = $details{$what};
    #}

    
}





    open (FILE,">$trace"."_missedalgos.txt");
	#$store->{$src."->".$dst."_".$proto}{$algo}{$what} = $details{$what};
	#walk the algorithms of each flow, ensure that all are present. Barf if not. -> is probably a bug. 
	#	$VAR1 = {
	#          '110.175.156.36:60591->192.168.1.145:6881 [tcp]' => {
	#                                                                'mbet' => []
	#                                                                'claffy' => []
	print FILE "\n";
	for my $tuple (keys %{$store}) {
		my %unique = map { $_ => 1 } @algos;
		my $count = 0;
		for my $x (keys %{ $store->{$tuple} } ) {
			delete $unique{$x};
			$count++;
		}
		if ($count < 9){
			print FILE $tuple."\nMissed algos: ";
			for my $x (keys %unique){
				print FILE "$x ";
			}
			print FILE"\n\n";
		}
	}
	print FILE "\n";
    close(FILE);



sub get_write_handles {
  my @file_names = @_;
  my %file_handles;
  foreach (@file_names) {
    open my $fh, $_ or next;
    $file_handles{$_} = $fh;
  }
  return %file_handles;
}

sub dump_store {
    for my $tuple (keys %{$store}) {
        print $tuple."\n";
        for my $algo (keys %{ $store->{$tuple} } ) {
            print "\t$algo\n";
            foreach my $entry (@{$store->{$tuple}->{$algo} } ) {
                
                foreach my $key (keys %{$entry}){
                    print "\t\t$key -> " . $entry->{$key} ."\n"; 
                }
            }
            exit 1;
        }
    }

}
