#!/usr/bin/perl -w
use strict;
use warnings;
use diagnostics;
use Data::Dumper;
use Time::HiRes qw(gettimeofday tv_interval);
use Net::FTP;

sub getday;

#use to merge
my $merge = 1;
my $actual = 1;

    #$ftp->cwd("/auckland/8") or die "Cannot change working directory ", $ftp->message;
    #ftp://wits.cs.waikato.ac.nz/waikato/5/20070606-000000-0.gz
    #                          >----path--|--day------ --------<
    
#waikato
#my $path = "/waikato/5";
#my $lpath = "/waikato5";

#auckland
my $path = "/auckland/8";
my $lpath = "/auckland8";


for(my $x = 20031206;$x <= 20031206;$x++){
    #print "Sleeping for 1 hr\n";
    #sleep 3600;

    #20070606-000000-0.gz
    #$x .= "-000000-0.gz";
    getday ($x);
}



sub getday {


    my $day = shift;

    if(!$day){
        print "$0 requires an argument.\n";
        exit(1);
    }
    my $ftp = Net::FTP->new("wits.cs.waikato.ac.nz", Debug => 0) or die "Cannot connect to wits.cs.waikato.ac.nz $@";
    $ftp->login("anonymous",'-anonymous@') or die "Cannot login ", $ftp->message;
    $ftp->cwd($path) or die "Cannot change working directory ", $ftp->message;
    print Dumper($ftp->pwd);
    $ftp->binary;

    #my $day = 1201;

    my @array = $ftp->ls;

    my $file = "";

    foreach my $f (@array){
    if ($f =~ /$day/){
        print "Getting $f\n";
        $file = $f;
        my $get = "wget ftp://wits.cs.waikato.ac.nz".$ftp->pwd."/$f";
        print "$get\n";
        if($actual){`$get`;}
        #doesn't do ipv4, and wits is much faster on ipv6
        #$ftp->get($f) or die "get failed ", $ftp->message;
        }
    }
    
    $ftp->quit;

    #print Dumper(\@array);

    my $merge_trace = "";

    if($merge){
        my $string = "";
        foreach my $f (@array){	
            if ($f =~ /$day/){
                $string .= "erf:./".$f." ";
            }	
        }
        my $bare_name = "";
        my $name = "";
         foreach my $f (@array){
            if($f =~ /$day/ && $f =~ /^(\d\d\d\d\d\d\d\d)/){
                $name = "erf:./".$f;
                $bare_name = $1.".gz";
                last;
            }
        }
        $merge_trace = $bare_name;

        my $run = "/home/dtm7/libtrace-3.0.13/tools/tracemerge/tracemerge -z 3 -Z gzip erf:./$bare_name $string";

        print "name: $name\n";
        print "run:  $run\n";
        if($actual){`$run`;}

        foreach my $f (@array){
            if ($f =~ /$day/){
                print "unlink($f)\n";
                if($actual){unlink($f);}
            }
        }
        
    }
    
    my $x = "";
    if($merge){
        $x = "/home/dtm7/School/591/tools/runcluster.pl erf:/home/dtm7/wits/$lpath/$merge_trace";
    }else{    
        $x = "/home/dtm7/School/591/tools/runcluster.pl erf:/home/dtm7/wits/$lpath/$file";
    }
    print $x."\n";
    if($actual){`$x`;}

}



