#include <libtrace.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "helpers.h"

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

using namespace std;



/************** * IP Helpers * **************/

/* Determines if an IP address is an RFC1918 address.
 *
 * Parameters:
 *  ip_addr - the IP address to check
 *
 * Returns:
 *  true if the address is an RFC1918 address, false otherwise
 */
bool rfc1918_ip_addr ( uint32_t ip_addr ) {

    /* Check if 10.0.0.0/8 */
    if ( ( ip_addr & 0x000000FF ) == 0x0000000A )
        return true;
    /* Check if 192.168.0.0/16 */
    if ( ( ip_addr & 0x0000FFFF ) == 0x0000A8C0 )
        return true;
    /* Check if 172.16.0.0/12 */
    if ( ( ip_addr & 0x0000FFFF ) == 0x000010AC )
        return true;

    /* Otherwise, we're not RFC 1918 */
    return false;
}

/* Determines if either of the addresses in an IP header are RFC 1918.
 *
 * Parameters:
 *  ip - a pointer to the IP header to be checked
 *
 * Returns:
 *  true if either of the source or destination IP address are RFC 1918,
 *  false otherwise
 */
bool rfc1918_ip ( libtrace_ip_t *ip ) {
    /* Check source address */
    if ( rfc1918_ip_addr ( ip->ip_src.s_addr ) )
        return true;
    /* Check dest address */
    if ( rfc1918_ip_addr ( ip->ip_dst.s_addr ) )
        return true;
    return false;
}

/* NOTE: ip_a and port_a must be from the same endpoint, likewise with ip_b and port_b. */

/* Extracts the VLAN Id from a libtrace packet.
 *
 * This is a rather simplistic implementation with plenty of scope for
 * improvement.
 *
 * Parameters:
 *  packet - the libtrace packet to extract the VLAN id from
 *
 * Returns:
 *  the value of the Id field in the VLAN header, if present. Otherwise,
 *  returns 0.
 */
uint16_t extract_vlan_id ( libtrace_packet_t *packet ) {

    void *ethernet = NULL;
    void *payload = NULL;
    uint16_t ethertype;
    libtrace_linktype_t linktype;
    uint32_t remaining;
    libtrace_8021q_t *vlan;
    uint16_t tag;

    /* First, find the ethernet header */
    ethernet = trace_get_layer2 ( packet, &linktype, &remaining );

    /* We only support VLANs over Ethernet for the moment */
    if ( linktype != TRACE_TYPE_ETH )
        return 0;

    /* XXX I am assuming the next header will be a VLAN header */
    payload = trace_get_payload_from_layer2 ( ethernet, linktype,
              &ethertype, &remaining );

    if ( payload == NULL || remaining == 0 )
        return 0;

    /* XXX Only gets the topmost label */
    if ( ethertype != 0x8100 )
        return 0;

    vlan = ( libtrace_8021q_t * ) payload;
    if ( remaining < 4 )
        return 0;

    /* VLAN tags are actually 12 bits in size */
    tag = * ( uint16_t * ) vlan;
    tag = ntohs ( tag );
    tag = tag & 0x0fff;
    return tag;

}

/*** flow helpers ***/
