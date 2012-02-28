#!/usr/bin/perl -w
use Data::Dumper;
use IO::Uncompress::Gunzip qw(gunzip $GunzipError);


my %handles = ();






#returns a hash of the available csv files to be processed
sub get_write_handles {
  my @file_names = @_;
  my %file_handles;
  foreach (@file_names) {
    #open my $fh, $_ or next;
    my $fh = new IO::Uncompress::Gunzip $_;
    $file_handles{$_} = $fh;
    

  }
  return %file_handles;
}







%handles = get_write_handles(glob("sorted*.csv*"));

print Dumper(\%handles);


foreach my $x (keys %handles){

  my $line = readline($handles{$x});
  print $line."\n";
  
  
}

foreach my $x (keys %handles){

  my $line = readline($handles{$x});
  print $line."\n";
  
  
}