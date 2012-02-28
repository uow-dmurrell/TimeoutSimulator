#!/usr/bin/perl -w
#See readline notes.txt for example of what's stored in the hash. 

use strict;
use warnings;
use diagnostics;
use Data::Dumper;
use Time::HiRes qw(gettimeofday tv_interval);
use IO::Uncompress::Gunzip qw(gunzip $GunzipError);

my %handles = ();
my %hashpos = ();
my %seekbuf = ();

sub get_write_handles;
sub get_algos;
sub init_summary;
sub seektodata;
sub process;
sub start_sort;
sub loadline;
sub evenup;
sub finish;
sub print_timerstats;



#autoflush
$| = 1;

my $debug = 0;
my $splitval = 4;
my $printval = 8192;
my $run_limit = 0;

my $location = shift;
my $trace = shift;
my $date = shift;

if(!$trace || !$date || !$location ){
	print "Three arguments needed for program:\n$0 /path/to/sortedoutput tracefilename timestamp\n";
	exit(1);
}

chdir($location);

#counters:
my $total_flows = 0;        

my $summary = {};

#benchmarks
my $timea = [gettimeofday];
my $timeb = [gettimeofday];


#total number of flows processed
$summary->{'general'}{'total_flows'} = 0;

my @filenames = glob("sorted.$trace*$date*.csv*");

if(scalar(@filenames) == 0){
    print "No files found matching pattern: >sorted.".$trace.'*'.$date.'*.csv*'."<\n";
    exit(1);
}

%handles = get_write_handles(glob("sorted."."$trace*$date*.csv*"));
my @algos = get_algos();
my $store = {};
init_summary();

if($debug) { print Dumper(\%handles); }

#number of algorithms read in
my $algocount = scalar(keys(%handles));

seektodata();
my $i = 0;
my $j = 1;
my $minhash = 0;
my $maxhash = 0;
my $refmax = 0;
my $current_flow = "";

#sets up the claffy unlimited flow setter - only one line per flow.
my $cu = "";
foreach my $g (keys %handles){
    if( $g =~ /claffy_unlimited/){
        $cu = $g;
        last;
    }
}
if(!$cu){ print "Need claffy_unlimited tracefile present to read. Exiting.\n";exit(1); }

my $line_cache = "";
sub sparkup{
    my $line = "";
    #for(my $z = 0;$z < 1; $z++){
    my $hash = 0;

    while(1){
        #load from cache first
        if($line_cache ne ""){
            $line = $line_cache;
            $line_cache = "";
        }
        #nothing in cache, run as normal.
        else { 
            $line = readline($handles{$cu}); 
        }
        if($line eq "" || !defined($line)){
            #if($z == 0){ return 0; }
            #else{ last; }
            #last;
            return 0;
        }


        my @v = split(/,/,$line);
        
        #if haven't called evenup or have a hash collision, load other algos.
        if($hash == 0 || $hash == $v[5]){
            loadline($cu,$line);
            evenup($v[5]);
            $hash = $v[5];
        }
        else{
            $line_cache = $line; #put line into a cache for next sparkup.
            return 1;
        }
        
    }
    return 1;
}

#main loop
while(1){
    # foreach my $x (keys %handles){
        # if($debug) { print "Reading line from file $x\n"; }
        # my $line = readline($handles{$x});
        # if($line == '' || $line == undef){
            # last;
        # }
        # chomp $line;
        # if($debug) { print "Read this line: >" . $line ."<\n\n"; }
        # 
        # loadline($x,$line);
    # }

    if(sparkup() == 0){
        finish();
        last;
    }

    #print Dumper($store);
    #exit(1);
    

    
    # if($debug) { print "Store contains this data:\n"; }
    # if($debug) { print Dumper(%{$store}); }
    # if($debug) { print "entries in store: " . scalar(keys(%{$store}) )."\n"; }
     #print ">".  Dumper(
     #   ${$store->{'10.0.248.21:3048->10.0.28.79:80 [tcp]'}{'claffy'}}[0]->{'hash'}
     #   )."<\n" ;
    
    # if($debug) { print "minhash this round: " . $minhash."\n"; }
    # 
    # if($debug) { print "finding entries to write out\n"; }
    # 
    #only going to delete hashes lower than what we've got. also assuming that all hashes are the same.
    foreach my $g (keys(%{$store})){
    
        #my $possible = 0;
        #$possible = ${$store->{$g}{'claffy_unlimited'}}[0]->{'hash'};
        
        # if(!$possible){
            # exit(1);
            # #hash isn't fully loaded into store. (hash collision) 
             # if($debug) {
                # print "Barfing on partially loaded store\n";
                # print Dumper(%{$store}); 
                # print "Deleting extra claffy unlimited entry\n";
            # }
            # #claffy unlimited will always have the minimum number of flows - given that nothing misses packets.
            # delete(${$store->{$g}{'claffy_unlimited'}}[0]);
            # if($debug) { print "Loading $refmax\n"; }
            # evenup($refmax);
            # if($debug) {
                # print "Evenup called with $refmax, store:\n";
                # print Dumper(%{$store}); 
            # }
            # $possible = ${$store->{$g}{'claffy_unlimited'}}[0]->{'hash'};
        # }
        #if($possible < $minhash){
            #possible has to be less than the minimum bumped to by evenup, otherwise we loose data.
            #if($debug) { print "Writing out " . $g ." as $possible is less than $minhash\n"; }
            $current_flow = $g;
            # foreach my $x (keys(%hashpos)){
                # if($hashpos{$x} == $possible){
                    # evenup($possible);
                    # last;
                # }
            # }
            #my $pret = 
            process($store->{$g});
            #if($debug) { print "Deleting $possible from store\n"; }
            #if($pret == 1) { 
            delete($store->{$g}); 
            
            #} #process only returns one if successful.
            
           # if($debug) { print "entries now in store: " . scalar(keys(%{$store}) )."\n"; }
        #}
        # else{
            # if($debug) { print "Tested $g. Not going to delete as $possible >= $minhash.\n"; }
        # }
        
    }
    #if($debug) { print "processing finished. reading new flow in.\n"; }
    

    
    $i++;
    #if($i == $run_limit){ finish(); }
    #$splitval = 32;
    #$printval = 4096;
    #if($i %$splitval == 0){
    if($i %1024 == 0){
        #my ($s, $usec) = gettimeofday();


        if($i %$printval == 0 ) { 
            my $totalall = tv_interval ($timeb, $timea);
            my $elapsed = tv_interval ($timea);
            
            print "summary at $i:  (".scalar(keys(%{$store}) ).") keys in store \n";
            print Dumper($summary); 
            my $lval = int($i/$totalall);
            my $val = int($splitval/$elapsed);
            print "($lval) calling evenup ($maxhash) $splitval/$elapsed:\t (". $val .") (amount done/time to do)\n";
            
        
        }
        #evenup($maxhash);
        $timea = [gettimeofday];
        
        
    }
    #if($i == 30) {
    #    print scalar(keys(%handles)) ."\n";    
    #    exit(1);
    #}
    #else{ if($debug) { print "\n"; } }
    # if($minhash > 56377153){
        # evenup(56377153);
        # finish();
        # exit(1);
    # } 

    # if($minhash == 0 && $maxhash == 0){
        # finish();
        # last;
    # } 
    # else{
        # $minhash = 0;
        # $maxhash = 0;
    # }
}

sub print_timerstats{
    $timea = [gettimeofday];
    my $totalall = tv_interval ($timeb, $timea);
    print "summary at $i:  (".scalar(keys(%{$store}) ).") keys in store \n";
    my $lval = int($i/$totalall);
    print "($lval) = (i: $i / totalall: $totalall ) \n";

}


#returns an array of algorithms in the current set of csv's
sub get_algos{
    my @a;
    foreach my $x (keys %handles){
        my @splitarray = split /_/,$x;
        my $algo = $splitarray[1];
        if($#splitarray == 3){
            $algo = $splitarray[1]."_".$splitarray[2];
        }
        if($debug) { print "algo: >$algo<\n"; }
        push (@a,$algo);
    }
    return @a;
}



#zeros out the main storage hash. (fixing a stupid bug below in process())
sub init_summary {
    $summary->{'general'}{'memory_time'} = 0; 

    foreach my $y (@algos){
        next if ($y =~ /claffy_unlimited/);
        $summary->{$y}{'memory_time'}  = 0; 
        $summary->{$y}{'time_waste'}  = 0; 
        $summary->{$y}{'shortened'}  = 0; 
        $summary->{$y}{'shortflow'}  = 0; 
        $summary->{$y}{'lengthened'} = 0;
    }
}

#dumps out the store hash to csv and summary file
sub finish {
       # $debug = 1;
        print "Final processing:\n";
        print_timerstats();
        print "Entries still in store: " . scalar(keys(%{$store})) .". Now processing.\n";
        
        print Dumper($store);
        
        #need to process all remaining flows in the hash.
        foreach my $g (keys(%{$store})){
            my $lhash = ${$store->{$g}{'claffy_unlimited'}}[0]->{'hash'};
            my $records = scalar(keys(%{$store->{$g}}));
            print "Processing hash: ". $lhash ."(". $records . ")\n";
            if ( $records < $algocount ){
                print "calling evenup $lhash\n";
                #$debug =0;
                evenup($lhash);
                #$debug =1;
            }
            
            #print Dumper($store->{$g}); 
            
            my $ret = process($store->{$g});
            if($ret == 0){ process($store->{$g}); }
            delete($store->{$g});
        }    
        
        open(RESULTS,">$location/runstats") || warn("can't open $location/runstats: $!");
        print RESULTS "Number remaining: " . scalar(keys(%{$store})) ."\n";;
        print RESULTS Dumper(%{$store}); 
        print RESULTS "summary:\n";
        print RESULTS Dumper($summary); 
        print RESULTS "\n";
        close(RESULTS);
        
        open(RESULTS,">$location/runresults.csv") || warn("can't open $location/runresults.csv: $!");
        print RESULTS "name,time_waste,lengthened,shortflow,shortened,memory_time\n";
        
        foreach my $x (keys(%{$summary})){
            my $body   = "";
            
            if($x =~ /general/){
                $body .= "general,";
                $body .= "0,0,";
                $body .= $summary->{$x}{"total_flows"}.",";
                $body .= "0,";
                $body .= $summary->{$x}{"memory_time"}."\n";
            }
            else{
                $body .= $x.",";    #name
                $body .= $summary->{$x}{"time_waste"}  .",";   #timewaste
                $body .= $summary->{$x}{"lengthened"}  .",";   #lengthened
                $body .= $summary->{$x}{"shortflow"}   .",";   #shortflow
                $body .= $summary->{$x}{"shortened"}   .",";   #shortened
                $body .= $summary->{$x}{"memory_time"} ."\n";   #memory time
            }
            print RESULTS $body;
        }        
        close(RESULTS);
}




#returns a hash of the available csv files to be processed
sub get_write_handles {
  my @file_names = @_;
  my %file_handles;
  foreach (@file_names) {
    my $fh = new IO::Uncompress::Gunzip $_;
    #open my $fh, $_ or next;
    $file_handles{$_} = $fh;
  }
  if($debug) { print Dumper(\%file_handles); }

  return %file_handles;
}

#moves forward past the comment section of the hash 
sub seektodata {
    foreach my $x (keys %handles){
        my $run = 1;
        while($run == 1){
            my $line = readline($handles{$x});
            if($line =~ /List/){
                $run = 0;
            }
            if($debug) { print "Skipping: \t".$line; }
        }
    }
    if($debug) { print "Finished skipping headers.\n\n"; }
}



#brings the file pointers up to the same hash by reading into the to-process store
#doesn't actually do any real processing
sub evenup {
    my $max = shift;
    my $hold = 1;
    #if($debug){ print "Loading up evener. Max is $max\n"; }
    foreach my $x (keys %handles){
        while(1){
            #if($debug) { print "Evenup Reading line from file $x\n"; }
            my $line = "";
            if($seekbuf{$x}){ 
              $line = $seekbuf{$x}; 
              delete($seekbuf{$x});
            }
            else{
              $line = readline($handles{$x});
            }
            if( $line eq '' || !defined($line) ){
               # if($debug) { print "lasting...\n"; }
                last;
            }
                        
            my @v = split(/,/,$line);
            if($v[5] > $max){
                $refmax = $max;
                #if($debug){ print "reached ".$v[5]." for $max on\n"; }
                #print "Seeking ". -length($line) ." characters in the file...(should be -ve)\n";
                #seek($handles{$x}, -length($line), 1); #go back one, so we don't start skipping things.
                #that doesn't work with gzip. Buffering it is.
                $seekbuf{$x} = $line;
                last;
            }
            
            loadline($x,$line);
            
            #chomp $line;
            #if($debug) { print "Read this line: >" . $line ."<\n\n"; }
           

        }
    }
    
}


#loads a line into the to-process store. 
sub loadline {
        my $x = shift;
        my $line = shift;
        
        my @splitarray = split /_/,$x;
        my $algo = $splitarray[1];
        if($#splitarray == 3){
            $algo = $splitarray[1]."_".$splitarray[2];
        }
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
        my $time_st = $v[10];   #relative time since start of trace
        my $time_en = $v[11];   
        my $time_ex = $v[12];
        #my $atime_st = $v[13]; #^a = actual time in sec since 1970
        #my $atime_en = $v[14];
        #my $atime_ex = $v[15];
        my $tcplong = $v[16];    
        
           
        chomp($tcplong);
        
        $minhash = $hash if ( $minhash == 0 );
        if ( $hash < $minhash ){
            #if($debug) { print "Found non matching flow\n"; }
            $minhash = $hash 
        }
        $maxhash = $hash if ( $maxhash == 0 );
        if ( $hash > $maxhash ){
            $maxhash = $hash;
        }
        if($proto == 1) { next; }
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
            hash        => $hash,
            lengthened  => $tcplong
        );
    
        my @tmparr = ();
        push @tmparr, { %details };
        
        my $flow = $src."->".$dst." [".$proto."]";

        #print "Pushing $flow\n";	
        
        $hashpos{$algo} = $hash;

        push @{$store->{$flow}{$algo}}, @tmparr;
        #$store->{$flow}{$algo} = %details;
        #if($debug) { print "\tHash for this flow entry: $hash\n"; }
        
        #how to start dumping at the error point, probably bringing in all important information
        #if($hash == 4294959793){ $debug = 1; }
}



#sort the shortened entries by start time. 
sub start_sort {
   return $a->{'time_start'} <=> $b->{'time_start'};
}


#processes lines into the summary hash. Does the real work of the program.
sub process {
    #if($debug) { print "\nIn processing>\n\n"; }

    #increment processed flow counter  (moved to bottom)
    #$summary->{'general'}{'total_flows'}++;
    
    #local summary, for debugging. is same as summary, just local to this function.
    my $lsumary = {};

    my $h = $_[0];
    #what the above translates into:
    #my %ref = @_;
    #   ${$store->{'10.0.248.21:3048->10.0.28.79:80 [tcp]'}{'claffy'}}[0]->{'hash'}
    #   my $possible = ${$store->{$g}{'claffy'}}[0]->{'hash'};

    #if($debug) { print "Hash passed in:\n"; }
    #if($debug) { print Dumper(${$h->{'claffy_unlimited'}}[0]->{'hash'}); }

    #number of algorithms processed for this flow:
    my $records = scalar(keys(%{$h}));
    
    if($records != $algocount){
        my $key = "";
        foreach my $x (keys %{$h}){
            $key = $x;
            last;
        }
        print "records != algocount ($records != $algocount) evenup this hash:". ${$h->{$key}}[0]->{'hash'} ."\n"; 
        print "state:\n";
        print Dumper (\$h);
        print Dumper (\$store);
        
        #we don't expect any data loss, so we haven't read a line in from something, 
        #getting evenup to read some in, and try again.
        evenup(${$h->{$key}}[0]->{'hash'});
        #return 0;

        #print "state after hail mary evenup:\n";
        #print Dumper ($h);
        #print Dumper ($store);
        $summary->{'general'}{'broken flows'}++;
         
        return 0;
    }
    if(scalar(@{$h->{'claffy_unlimited'}}) != 1){
        print "Shortening for claffy_unlimited has occured. Should never ever happen. Exiting.\n"; 
        print Dumper($h);
        print Dumper ($store);
        exit(1);
    }

    my $flow_start  = ${$h->{'claffy_unlimited'}}[0]->{'time_start'};
    my $flow_end    = ${$h->{'claffy_unlimited'}}[0]->{'time_end'};
    my $data_in     = ${$h->{'claffy_unlimited'}}[0]->{'data_in'};
    my $data_out    = ${$h->{'claffy_unlimited'}}[0]->{'data_out'};
    my $packet_in   = ${$h->{'claffy_unlimited'}}[0]->{'packet_in'};
    my $packet_out  = ${$h->{'claffy_unlimited'}}[0]->{'packet_out'};
    
    my $flow_length = $flow_end - $flow_start;
   # if($debug) { print "flow_length: $flow_length\n"; }
    
    
    #each x is an algorithm, which will have returned results for the same flow.
    foreach my $x (keys %{$h}){
        next if ($x =~ /claffy_unlimited/);
       # if($debug) { print "\nReading in algo: $x\n"; }
       # if($debug) { print "\tShortened if > 1: " . scalar(@{$h->{$x}})."\n"; }
        my $short = scalar(@{$h->{$x}});
        
        
        if($debug) { print "\tStart time(s):\n"; }
        
        #local data and packet counts for this algo.
        my $lpi = 0;    #packet in
        my $lpo = 0;    #packet out
        my $ldi = 0;    #data in
        my $ldo = 0;    #data out
            
        #shortened flows. have to be handled a bit different to non short. 
        if($short > 1){
            
           # print Dumper(@{$h->{$x}});
            
            #total time spent in memory for packets. netramet short should be 
            #very low (but with lots of shortening).
            
            #first is top of list
            my @sortd = sort start_sort(@{$h->{$x}});
            if($debug) { print Dumper(@sortd); }
            
            #print Dumper($sortd[0]->{'time_start'});
            #print Dumper($sortd[$#sortd]->{'time_start'});
            
            #$a_start_time = $sortd[0]->{'time_start'};
            #$a_end_time = $sortd[0]->{'time_start'};
            
            
            foreach my $y (@sortd){
                #my $memorytime = $y->{'time_expire'} - $y->{'time_start'};
                #print "noshort memory time: $memorytime\n";
                
                #$summary->{$x}{'memory_time'} += $y->{'time_expire'} - $y->{'time_start'}; #moved to end
                $lsumary->{$x}{'memory_time'} += $y->{'time_expire'} - $y->{'time_start'};
                $lsumary->{$x}{'lengthened'} += $y->{'lengthened'};
                
                $lpi += $y->{'packet_in'};
                $lpo += $y->{'packet_out'};
                $ldi += $y->{'data_in'};
                $ldo += $y->{'data_out'};
            }
            
            #calculate time past last flow 
            #$summary->{$x}{'time_waste'} += $sortd[$#sortd]->{'time_expire'} - $flow_end;
            $lsumary->{$x}{'time_waste'} += $sortd[$#sortd]->{'time_expire'} - $flow_end;
            
            ##write out if its a shortened flow. 
           # $summary->{$x}{'shortened'} += ($short);
            #$summary->{$x}{'shortflow'} ++;
            $lsumary->{$x}{'shortened'} += $short;
            $lsumary->{$x}{'shortflow'} ++;

            #only incremented on shortening events.
            $j++;
        }
        #not a shortend flow.
        else{
          #  if($debug) { print Dumper($h->{$x}); }
            
            #my $memorytime = ${$h->{$x}}[0]->{'time_expire'} - ${$h->{$x}}[0]->{'time_start'};
            #print "noshort memory time: $memorytime\n";
            #$summary->{$x}{'memory_time'} += ${$h->{$x}}[0]->{'time_expire'} - ${$h->{$x}}[0]->{'time_start'};
            $lsumary->{$x}{'memory_time'} += ${$h->{$x}}[0]->{'time_expire'} - ${$h->{$x}}[0]->{'time_start'};
            $lsumary->{$x}{'lengthened'} += ${$h->{$x}}[0]->{'lengthened'};
            
            $lpi = ${$h->{$x}}[0]->{'packet_in'};
            $lpo = ${$h->{$x}}[0]->{'packet_out'};
            $ldi = ${$h->{$x}}[0]->{'data_in'};
            $ldo = ${$h->{$x}}[0]->{'data_out'};

        }
        
        #checking that the data adds up (after accounting for shortening)
        if( $lpi != $packet_in  ||  $lpo != $packet_out || $ldi != $data_in   ||   $ldo != $data_out   ){
            print "lpi:$lpi may not match packet_in:$packet_in\n";
            print "lpo:$lpo may not match packet_in:$packet_out\n";
            print "ldi:$ldi may not match data_in:$data_in\n"; 
            print "ldo:$ldo may not match data_out:$data_out\n"; 

            
            print "$current_flow\n";
            print Dumper(\%{$lsumary});
            print Dumper(\%{$h});
            print Dumper(\%{$summary});
            $summary->{'general'}{'uneven flows'}++;

            
            #have missed a line, asking evenup to read some more and return to try again
            #evenup(${$h->{'claffy_unlimited'}}[0]->{'hash'});
            #exit(1);
            return 0;
        }
    }
    
    #write out all global counters on function exit, when we're happy that the function isn't going to return in error.
    
    #increment processed flow counter 
    $summary->{'general'}{'total_flows'}++;

    #increment global flow timer.
    $summary->{'general'}{'memory_time'} += $flow_length;


    #deal with varying types of definedness in a rather flexible way.
    
    #x is algorithm -ie claffy/netramet
    foreach my $x (keys %{$h}){
        next if ($x =~ /claffy_unlimited/);
        
        # y is the key, so is something like memory_time or time_waste
        foreach my $y (keys %{$lsumary->{$x}}){
           
            unless(!defined($lsumary->{$x}{$y})){
                $summary->{$x}{$y} += $lsumary->{$x}{$y};   
            }
            
        }
    }
    #print "Global Summary:\n".Dumper($summary);
    #print "Local Summary:\n" .Dumper($lsumary);
    
    #exit(1);
    

    # if($debug) { print "local summary\n"; }
    # if($debug) { print Dumper($lsumary); }
    # if($debug) { print "general summary\n"; }
    # if($debug) { print Dumper($summary); }

    return 1;
}



