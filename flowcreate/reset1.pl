#!/usr/bin/perl -wT
use strict;
use warnings;

use Socket;

use constant IPPROTO_RAW => 255;
use constant DUMMY_ADDR => scalar( sockaddr_in( 0, inet_aton('127.0.0.1') ) );

my $ETH;
socket $ETH, PF_INET, SOCK_RAW, IPPROTO_RAW or die "$!";

my $tcp = new_tcp(
    {
        src_ip => 0x0A00_0001,
        src_p  => 50000,
        dst_ip => 0x0A00_0002,
        dst_p  => shift,
        seq    => shift,
    }
);

my $text = "Now is the time for all good men to come to the aid of the party";

for ( 1 .. 4 ) {
    my $pkt = tcp_packet( $tcp, $text );

# Because $ETH is not connected, Perl requires a valid looking sockaddr to send to.
# However, the address is ignored on sockets opened SOCK_RAW, IPPROTO_RAW !

    send $ETH, $pkt, 0, DUMMY_ADDR or die "$!";

    $text .= "!";
}

# Construct new TCP "state".
#
#   src_ip   => source IP address       (numeric) -- required
#   src_p    => source port             (numeric) -- required
#   dst_ip   => destination IP address  (numeric) -- required
#   dst_p    => destination port        (numeric) -- required
#   seq      => initial sequence number (numeric) -- default = random setting
#   a_seq    => acknowledgement number  (numeric) -- default = none
#   window   => window size             (numeric) -- default = 10000
#
#   ttl      => TTL to use              (numeric) -- default = 100
#   tos      => TIS to use              (numeric) -- default = 0

sub new_tcp {
    my ($tcp) = @_;

    die "new_tcp: requires 'src_ip'" if !exists( $tcp->{src_ip} );
    die "new_tcp: requires 'src_p'"  if !exists( $tcp->{src_p} );
    die "new_tcp: requires 'dst_ip'" if !exists( $tcp->{dst_ip} );
    die "new_tcp: requires 'dst_p'"  if !exists( $tcp->{dst_p} );

    $tcp->{seq}    = int( rand(0xFFFF_FFFF) ) + 1 if !exists( $tcp->{seq} );
    $tcp->{a_seq}  = undef                        if !exists( $tcp->{a_seq} );
    $tcp->{window} = 10000                        if !exists( $tcp->{window} );

    $tcp->{ttl} = 100 if !exists( $tcp->{ttl} );
    $tcp->{tos} = 0   if !exists( $tcp->{tos} );

    $tcp->{proto} = 6;

    return $tcp;
}

# Construct TCP packet (complete with IP header)
#
# Takes: $tcp     -- ref:Hash, tcp state
#                        src_p    -- numeric -- required
#                        dst_p    -- numeric -- required
#                        seq      -- numeric -- required
#                        window   -- numeric -- required
#
#                        src_ip   -- numeric -- required       (as per IP)
#                        dst_ip   -- numeric -- required       (as per IP)
#                        proto    -- numeric -- required       (as per IP)
#                        ttl      -- numeric -- default = 100  (as per IP)
#                        tos      -- numeric -- default = 0    (as per IP)
#        $data    -- data to go
#        $r_st    -- ref:Hash, tcp state to update (eg { window => 10 } );
#
# NB: given tcp state may include:
#
#        urg => size of urgent data.  If not zero, sets URG and PSH flags
#        SYN => true                  Sets SYN flag
#        FIN => true                  Sets FIN flag
#        RST => true                  Sets RST flag
#
#     This state is cleared when packet is returned.

sub tcp_packet {
    my ( $tcp, $data, $r_st ) = @_;

    if ($r_st) { @{$tcp}{ keys %$r_st } = values %$r_st; }

    my $options = '';    # No option handling, pro tem

    if ( my $o = length($options) & 3 ) { $options .= "\0" x ( 4 - $o ); }

    die "tcp_packet: requires 'src_p'"  if !defined( $tcp->{src_p} );
    die "tcp_packet: requires 'dst_p'"  if !defined( $tcp->{dst_p} );
    die "tcp_packet: requires 'seq'"    if !defined( $tcp->{seq} );
    die "tcp_packet: requires 'window'" if !defined( $tcp->{window} );

    my $hlen = 20 + length($options);
    my $dlen = length($data);

    my $pseudo = pack(
        'N N CCn',
        $tcp->{src_ip},    #  N: Source IP
        $tcp->{dst_ip},    #  N: Destination IP
        0,                 #  C: padding
        $tcp->{proto},     #  C: protocol
        $hlen + $dlen      #  n: TCP Header Length + TCP Data Length
    );

    my $urg = $tcp->{urg} || 0;
    my $flags = $hlen << 10;    # == ($hlen / 4) << 12 !

    $flags |= 0x28 if $urg;
    $flags |= 0x10 if defined( $tcp->{a_seq} );
    $flags |= 0x04 if $tcp->{RST};
    $flags |= 0x02 if $tcp->{SYN};
    $flags |= 0x01 if $tcp->{FIN};

    my $csum = 0;
    my $dsum =
      unpack( "%32n*", $pseudo . $data . ( length($data) & 1 ? "\0" : '' ) );

    my $header;

    for ( 1 .. 2 ) {
        $header = pack(
            'nn N N nn nn A*',
            $tcp->{src_p},    #  n: Source Port
            $tcp->{dst_p},    #  n: Destination Port
            $tcp->{seq},      #  N: Sequence Number
            $tcp->{a_seq} || 0,    #  N: Acknowledgement Number
            $flags,                #  n: Header Length & TCP Flags
            $tcp->{window},        #  n: Window Size
            $csum,                 #  n: Header Checksum
            $urg,                  #  n: Urgent Pointer
            $options               # A*: Options
        );

        #   my @h = unpack("N*", $header) ;
        #   foreach (@h) { printf "%08X\n", $_ ; } ;

        $csum = unpack( "%32n*", $header ) + $dsum;
        $csum = ( ~( ( $csum >> 16 ) + $csum ) ) & 0xFFFF;
    }

    if ( $csum != 0 ) { die "$csum"; }

    delete $tcp->{urg};
    delete $tcp->{syn};
    delete $tcp->{fin};

    return ipv4_packet( $tcp, $header . $data );
}

# Construct an IPv4 Packet
#
# Requires: $ipv4   -- ref:Hash, containing:
#                        src_ip   -- numeric -- required
#                        dst_ip   -- numeric -- required
#                        proto    -- numeric -- required
#                        ttl      -- numeric -- default = 100
#                        tos      -- numeric -- default = 0
#           $data   -- the data !

sub ipv4_packet {
    my ( $ipv4, $data ) = @_;

    die "ipv4_packet: requires 'src_ip'" if !defined( $ipv4->{src_ip} );
    die "ipv4_packet: requires 'dst_ip'" if !defined( $ipv4->{dst_ip} );
    die "ipv4_packet: requires 'proto'"  if !defined( $ipv4->{proto} );

    my $ttl = $ipv4->{ttl} || 100;
    my $tos = $ipv4->{tos} || 0;

    my $options = '';    # No option handling, pro tem

    if ( my $o = length($options) & 3 ) { $options .= "\0" x ( 4 - $o ); }

    my $csum = 0;
    my $hlen = 20 + length($options);
    my $dlen = length($data);

    my $header;

    for ( 1 .. 2 ) {
        $header = pack(
            'CCn nn CCn N N A*',
            0x40 + ( $hlen / 4 ),    #  C: Version & Header Length in "words"
            0,                       #  C: TOS
            $hlen + $dlen,           #  n: total packet length
            0,                       #  n: expect stack to set identification
            0,                       #  n: no fragmentation
            $ttl,                    #  C: TTL
            $ipv4->{proto},          #  C: Protocol
            $csum,                   #  n: Header Checksum
            $ipv4->{src_ip},         #  N: Source IP
            $ipv4->{dst_ip},         #  N: Destination IP
            $options                 # A*: Options
        );

        $csum = unpack( "%32n*", $header );
        $csum = ( ~( ( $csum >> 16 ) + $csum ) ) & 0xFFFF;
    }

    if ( $csum != 0 ) { die "$csum"; }

    return $header . $data;
}
