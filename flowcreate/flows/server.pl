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

my @childs = ();
my $run = 1;
while($run) {

	# waiting for new client connection.
	$client_socket = $socket->accept();

	my $pid = fork();

	# parent
	if ($pid) {
		push(@childs, $pid);
		#waitpid($pid,0);
	} 
	# child
	elsif ($pid == 0) {
		# get the host and port number of newly connected client.
		$peer_address = $client_socket->peerhost();
		$peer_port = $client_socket->peerport();
		print "Accepted New Client Connection From : $peer_address, $peer_port\n";

		# read operation on the newly accepted client
		$data = <$client_socket>;
		# we can also read from socket through recv()  in IO::Socket::INET
		#$client_socket->recv($data,1024);
		my @split = split(/:/,$data);
		
		print "Received packet ". $split[1] ." of ". $split[2] ." from Client\n$data\n";
			
		print “@array\n”;
		exit(0);
	}

	foreach (@childs) {
		waitpid($_, 0);
	}





	
	
	
	
	
}
sleep(1);
$socket->close();


