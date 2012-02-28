#!/usr/bin/perl -w

use IO::Socket::INET;
use Socket qw(IPPROTO_TCP TCP_NODELAY);

sub barf(){
	print "$0 portnumber\n";
	exit();
}

$localport = shift;

if(!$localport){
	barf();
}


# flush after every write
$| = 1;

my ($socket,$client_socket);
my ($peeraddress,$peerport);



# creating object interface of IO::Socket::INET modules which internally does
# socket creation, binding and listening at the specified port address.
$socket = new IO::Socket::INET (
	LocalHost => '10.0.0.1',
	LocalPort => $localport,
	Proto => 'tcp',
	Listen => 5,
	Reuse => 0 
) or die "ERROR in Socket Creation : $!\n";

setsockopt($socket, IPPROTO_TCP, TCP_NODELAY, 1);
 
print "SERVER Waiting for client connection on port $localport\n";


use IO::Select;
$read_set = new IO::Select(); # create handle set for reading
$read_set->add($socket);           # add the main socket to the set

while (1) { # forever
  # get a set of readable handles (blocks until at least one handle is ready)
  my ($rh_set) = IO::Select->select($read_set, undef, undef, 1);
  # take all readable handles in turn
  foreach $rh (@$rh_set) {
     # if it is the main socket then we have an incoming connection and
     # we should accept() it and then add the new socket to the $read_set
     if ($rh == $socket) {
        $ns = $rh->accept();
        $read_set->add($ns);
     }
     # otherwise it is an ordinary socket and we should read and process the request
     else {
        $buf = <$rh>;
        if($buf) { # we get normal input
            print $buf ."\n";
        }
        else { # the client has closed the socket
            # remove the socket from the $read_set and close it
            $read_set->remove($rh);
            close($rh);
        }
     }
  }
}



sleep(1);
$socket->close();


























#my $run = 1;
#while($run) {

	## waiting for new client connection.
	#$client_socket = $socket->accept();

	#my $pid = fork();

	## parent
	#if ($pid) {
		#push(@childs, $pid);
		##waitpid($pid,0);
	#} 
	## child
	#elsif ($pid == 0) {
		## get the host and port number of newly connected client.
		#$peer_address = $client_socket->peerhost();
		#$peer_port = $client_socket->peerport();
		#print "Accepted New Client Connection From : $peer_address, $peer_port\n";

		## read operation on the newly accepted client
		#$data = <$client_socket>;
		## we can also read from socket through recv()  in IO::Socket::INET
		##$client_socket->recv($data,1024);
		#my @split = split(/:/,$data);
		
		#print "Received packet ". $split[1] ." of ". $split[2] ." from Client\n$data\n";
			
		#print “@array\n”;
		#exit(0);
	#}

	#foreach (@childs) {
		#waitpid($_, 0);
	#}





	
	
	
	
	
#}
