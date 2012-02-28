#!/usr/bin/perl
#tcpclient.pl

use IO::Socket::INET;
use Time::HiRes qw( usleep gettimeofday );

sub barf(){
	print "$0 count sleeptime(ms) [port 35000 default]\n";
	exit();
}

my $starttime = Time::HiRes::gettimeofday( );

my $count = shift;
my $time = shift;
my $remoteport = shift;
my $localport = shift;

if(!$count || !$time){
	barf();
}

if(!$remoteport){
	$remoteport = 35000;
}

$time *= 1000;

print $count . " " . $time ."\n";


# flush after every write
$| = 1;

my ($socket,$client_socket);

# creating object interface of IO::Socket::INET modules which internally creates
# socket, binds and connects to the TCP server running on the specific port.
$socket = new IO::Socket::INET (
	PeerHost => '10.0.0.1',
	PeerPort => $remoteport,
	LocalHost => '10.0.0.2',
	LocalPort => $localport,
	Proto => 'tcp',
) or die "ERROR in Socket Creation : $!\n";

setsockopt($socket, IPPROTO_TCP, TCP_NODELAY, 1);

#print "TCP Connection Success.\n";

# read the socket data sent by server.
#$data = <$socket>;
# we can also read from socket through recv()  in IO::Socket::INET
# $socket->recv($data,1024);
#print "Received from Server : $data\n";

# write on the socket to server.
# we can also send the data through IO::Socket::INET module,
#$socket->send($data);


 

for(my $x=1; $x < $count+1;$x++){
	
	$data = "ClientData:$x:$count";
	print $socket "$data";
	print $data ."\n";
	usleep($time);
}

my $endtime = Time::HiRes::gettimeofday( );

#number of packets = syn/synack/ack fin/fin/ack + 2 x packetcount
$npkt = 3 + 3 + (2*$count);

my $fulltime = ($endtime-$starttime);
$fulltime *= 1000;

#stream length = stream first time - stream last time
$length = $time * $count;
$length /= 1000;

#interval = stream length / num packets
$ipkt = ($length / $npkt);



print "Expected stats for system\n";
#print "\tnumber of flows   = $count\n";
print "\tnumber of packets = $npkt\n";
printf "\tinterpacket time = %.2f (ms)\n",$ipkt;
print "\texpected time    = $length (ms)\n";
printf "\tactual time      = %.2f (ms)\n",$fulltime;
$socket->close();



