/*
 * This file is part of libflowmanager
 *
 * Copyright (c) 2009 The University of Waikato, Hamilton, New Zealand.
 * Author: Shane Alcock
 *
 * All rights reserved.
 *
 * This code has been developed by the University of Waikato WAND
 * research group. For further information please see http://www.wand.net.nz/
 *
 * libflowmanager is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libflowmanager is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libflowmanager; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: libflowmanager.cc 43 2010-11-17 21:52:00Z yww4 $
 *
 */

#include "libflowmanager.h"

/* LRU for unestablished TCP flows */
ExpireList expire_tcp_syn;

/* LRU for established TCP flows */
ExpireList expire_tcp_estab;

/* LRU for UDP flows - a bit of a misnomer, all non-TCP flows end up in here */
ExpireList expire_udp;

/* LRU for short-lived UDP flows (only used if the short_udp config option is set) */
ExpireList expire_udpshort;

/* LRU for claffy timeout */
ExpireList expire_claffy;

/* LRU for all flows that met an instant expiry condition, e.g. TCP RST */
/* LRU for all flows that met an instant expiry condition, e.g. TCP RST (Ordered)*/
ExpireList expired_flows;
OrderedExpireList expired_ordered_flows;

/* LRU for netramet udp */
/* LRU for netramet udp (Ordered) */
ExpireList expire_netramet_tcp;
OrderedExpireList expire_netramet_ordered_tcp;

/* LRU for netramet udp */
/* LRU for netramet udp (Ordered)*/
ExpireList expire_netramet_udp;
OrderedExpireList expire_netramet_ordered_udp;


/* LRU Generic Ordered Expire List */
OrderedExpireList ordered_flows;


/* Map containing all flows that are present in one of the LRUs */
FlowMap active_flows;
OrderedFlowMap ordered_active_flows;

// StreamMap active_streams;


/* Each flow has a unique ID number - it is set to the value of this variable
* when created and next_conn_id is incremented */
//extern int next_conn_id;

/*  init flow numbers. */
int next_conn_id = 0;

lfm_config_opts config;

bool packet_debug 		= false;
bool timeoutbump_debug 	= false;
bool packetmatch_debug 	= false;
bool flowexpire_debug 	= false;
bool debug_netramet 	= false;
bool debug_optarg		= false;
bool debug_activeflow	= false;
bool csv_header_printed = false;

/* Sets a libflowmanager configuration option.
 *
 * Note that config options should be set BEFORE any packets are passed in
 * to other libflowmanager functions.
 *
 * Parameters:
 *  opt - the config option that is being changed
 *  value - the value to set the option to
 *
 * Returns:
 *  1 if the option is set successfully, 0 otherwise
 */
int lfm_set_config_option ( lfm_config_t opt, void *value ) {
    switch ( opt ) {
            case LFM_CONFIG_IGNORE_RFC1918:
                config.ignore_rfc1918 = * ( bool * ) value;
                return 1;
            case LFM_CONFIG_TCP_TIMEWAIT:
                config.tcp_timewait = * ( bool * ) value;
                return 1;
            case LFM_CONFIG_SHORT_UDP:
                config.short_udp = * ( bool * ) value;
                return 1;
            case LFM_CONFIG_VLAN:
                config.key_vlan = * ( bool * ) value;
                return 1;
            case LFM_CONFIG_IGNORE_ICMP_ERROR:
                config.ignore_icmp_errors = * ( bool * ) value;
                return 1;

            case LFM_CONFIG_DISABLE_IPV4:
                config.disable_ipv4 = * ( bool * ) value;
                return 1;
            case LFM_CONFIG_DISABLE_IPV6:
                config.disable_ipv6 = * ( bool * ) value;
                return 1;
        }
    return 0;
}

void lfm_init () {

    config.disable_ipv4 = false;
    config.disable_ipv6 = false;
    config.ignore_icmp_errors  = false;
    config.ignore_rfc1918 = false;
    config.key_vlan = false;
    config.short_udp = false;
    config.tcp_timewait = false;
	config.timeout_type = CLAFFY;
	config.rfc_strict_tcp = false;
}






/* Constructors and Destructors */
Flow::Flow ( const FlowId conn_id ) {
    id = conn_id;
    expire_list = NULL;
    expire_time = 0.0;
    saw_rst = false;
    saw_outbound = false;
    flow_state = FLOW_STATE_NONE;
    expired = false;
    extension = NULL;
    packets_in = 0;
    packets_out = 0;
    bytes_in = 0;
    bytes_out = 0;
    start_time = 0;
    last_time = 0;
	col = -1;
	tcp_lengthened = 0;
}

/* 'less-than' operator for comparing expiry times. - given that we want the biggest value at the top of the list, this is backwards.*/
bool Flow::operator< (  const Flow& b ) const {
    return false;

}


bool comp::operator()(const Flow* a, const Flow* b) const
{
  return a->expire_time < b->expire_time;
}




/************* * Direction * *************/

DirectionInfo::DirectionInfo() {
    saw_fin = false;
    saw_syn = false;
	seq = 0;
    first_pkt_ts = 0.0;
}

