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



/********* * Flows * *********/

/* Search the active flows map for a flow matching the given 5-tuple.
 *
 * Primarily intended for matching ICMP error packets back to the original
 * flows that they are in response to. This function can also be used to
 * perform look-ups in the flow map without creating a new flow if no match
 * is found.
 *
 * Parameters:
 *  ip_a - the IP address of the first endpoint
 *  ip_b - the IP address of the second endpoint
 *  port_a - the port number used by the first endpoint
 *  port_b - the port number used by the second endpoint
 *  proto - the transport protocol
 *
 * NOTE: ip_a and port_a MUST be from the same endpoint - likewise for ip_b
 * and port_b.
 *
 * Returns:
 *  a pointer to the flow matching the provided 5-tuple, or NULL if no
 *  matching flow is found in the active flows map.
 *
 * Bugs:
 *  Does not support VLAN ids as part of a flow key.
 *
 */
Flow *lfm_find_managed_flow ( uint32_t ip_a, uint32_t ip_b, uint16_t port_a, uint16_t port_b, uint8_t proto ) {

    FlowId flow_id;
    /* If we're ignoring RFC1918 addresses, there's no point
     * looking for a flow with an RFC1918 address */
    if ( config.ignore_rfc1918 && rfc1918_ip_addr ( ip_a ) )
        return NULL;
    if ( config.ignore_rfc1918 && rfc1918_ip_addr ( ip_b ) )
        return NULL;

    /* XXX: We always are going to use a vlan id of zero. At some
     * point this function should accept a vlan ID as a parameter */

    if ( trace_get_server_port ( proto, port_a, port_b ) == USE_SOURCE ) {
        flow_id = FlowId ( ip_a, ip_b, port_a, port_b, 0, proto, 0 );
    } else {
        flow_id = FlowId ( ip_b, ip_a, port_b, port_a, 0, proto, 0 );
    }

    if (config.timeout_type == NETRAMET||config.timeout_type == NETRAMET_SHORT) {
        OrderedFlowMap::iterator i = ordered_active_flows.find ( flow_id );

        if ( i == ordered_active_flows.end() ) {
            /* Not in the map */
            if ( packetmatch_debug ) cout << "ordered not in the ordered map of ipv4 flows\n";
            return NULL;
        } else {
            if ( packetmatch_debug ) cout << "ordered number of active4_flows:\t" << ordered_active_flows.size() << endl;
            return * ( ( *i ).second );
        }

    }
    else {
        FlowMap::iterator i = active_flows.find ( flow_id );

        if ( i == active_flows.end() ) {
            /* Not in the map */
            if ( packetmatch_debug ) cout << "not in the map4\n";
            return NULL;
        } else {
            if ( packetmatch_debug ) cout << "number of active4_flows:\t" << active_flows.size() << endl;
            return * ( ( *i ).second );
        }
    }
}

Flow *lfm_find_managed_flow6 ( uint8_t ip_a[16], uint8_t ip_b[16], uint16_t port_a, uint16_t port_b, uint8_t proto ) {

    FlowId flow_id;

    /* XXX: We always are going to use a vlan id of zero. At some
     * point this function should accept a vlan ID as a parameter */

    if ( trace_get_server_port ( proto, port_a, port_b ) == USE_SOURCE ) {
        flow_id = FlowId ( ip_a, ip_b, port_a, port_b, 0, proto, 0 );
    } else {
        flow_id = FlowId ( ip_b, ip_a, port_b, port_a, 0, proto, 0 );
    }

    //ordered list
    if (config.timeout_type == NETRAMET||config.timeout_type == NETRAMET_SHORT) {
        OrderedFlowMap::iterator i = ordered_active_flows.find ( flow_id );

        if ( i == ordered_active_flows.end() ) {
            /* Not in the map */
            if ( packetmatch_debug ) cout << "ordered not in the ordered map of ipv4 flows\n";
            return NULL;
        } else {
            if ( packetmatch_debug ) cout << "ordered number of active4_flows:\t" << ordered_active_flows.size() << endl;
            return * ( ( *i ).second );
        }

    }
    else {
        FlowMap::iterator i = active_flows.find ( flow_id );

        if ( i == active_flows.end() ) {
            /* Not in the map */
            cout << "not in the map6\n";
            return NULL;
        } else {
            cout << "number of active6_flows:\t" << active_flows.size() << endl;
            return * ( ( *i ).second );
        }
    }
}

/* Parses an ICMP error message to find the flow that originally triggered the error.
 *
 * Parameters:
 *  icmp_hdr - a pointer to the ICMP header from the error message
 *  rem - the number of bytes remaining in the captured packet (including
 *        the ICMP header)
 *
 * Returns:
 *  a pointer to the flow that caused the ICMP message, or NULL if the
 *  flow cannot be found in the active flows map
 */
Flow *icmp_find_original_flow ( libtrace_icmp_t *icmp_hdr, uint32_t rem ) {
    libtrace_ip_t *orig_ip;
    libtrace_ip6_t *orig_ip6 = NULL;
    uint16_t src_port, dst_port;
    uint32_t src_ip, dst_ip;
    uint8_t src_ip6[16], dst_ip6[16];
    uint8_t proto;
    Flow *orig_flow;
    void *post_ip;

    /* ICMP error message packets include the IP header + 8 bytes of the
    * original packet that triggered the error in the first place.
    *
    * Recent WAND captures tend to keep that post-ICMP payload, so we
    * can do match ICMP errors back to the flows that caused them */

    /* First step, see if we can access the post-ICMP payload */
    orig_ip = ( libtrace_ip_t * ) trace_get_payload_from_icmp ( icmp_hdr, &rem );
    if ( orig_ip == NULL ) {
            if ( packetmatch_debug ) cout << "orig_ip ==null " << endl;
            return NULL;
        }
    if ( orig_ip->ip_v == 6 ) {
            if ( rem < sizeof ( libtrace_ip6_t ) ) {
                    if ( packetmatch_debug ) cout << "not enough bytes to find data " << endl;
                    return NULL;
                }
            orig_ip6 = ( libtrace_ip6_t* ) orig_ip;
        } else {
            if ( rem < sizeof ( libtrace_ip_t ) ) {
                    if ( packetmatch_debug ) cout << "packet size wrong: rem < sizeof(libtrace_ip_t)" << rem << " <> " << sizeof ( libtrace_ip_t ) << endl;
                    return NULL;
                }
        }

    /* Get the IP addresses and transport protocol */
    if ( orig_ip6 ) {
            memcpy ( src_ip6, orig_ip6->ip_src.s6_addr, sizeof ( src_ip6 ) );
            memcpy ( dst_ip6, orig_ip6->ip_dst.s6_addr, sizeof ( dst_ip6 ) );
            //rem -= sizeof(libtrace_icmp_t);
            post_ip = trace_get_payload_from_ip6 ( orig_ip6, &proto, &rem );
        } else {
            src_ip = orig_ip->ip_src.s_addr;
            dst_ip = orig_ip->ip_dst.s_addr;
            proto = orig_ip->ip_p;
            rem -= ( orig_ip->ip_hl * 4 );
            post_ip = ( char * ) orig_ip + ( orig_ip->ip_hl * 4 );
        }

    /* Now try to get port numbers out of any remaining payload */
    if ( proto == 6 ) {
            /* TCP */
            if ( rem < 8 ) {
                    if ( packetmatch_debug ) cout << "tcp rem less than 8, rem = " << rem << endl;
                    return NULL;
                }
            libtrace_tcp_t *orig_tcp = ( libtrace_tcp_t * ) post_ip;
            src_port = orig_tcp->source;
            dst_port = orig_tcp->dest;
        } else if ( proto == 17 ) {
            /* UDP */
            if ( rem < 8 ) {
                    if ( packetmatch_debug ) cout << "udp rem less than 8, rem = " << rem << endl;
                    return NULL;
                }
            libtrace_udp_t *orig_udp = ( libtrace_udp_t * ) post_ip;
            src_port = orig_udp->source;
            dst_port = orig_udp->dest;
        } else {
            /* Unknown protocol */
            src_port = 0;
            dst_port = 0;
        }

    /* We have the 5-tuple, can we find the flow? */
    if ( orig_ip6 )
        orig_flow = lfm_find_managed_flow6 ( src_ip6, dst_ip6, src_port,
                                             dst_port, proto );
    else
        orig_flow = lfm_find_managed_flow ( src_ip, dst_ip, src_port, dst_port,
                                            proto );

    /* Couldn't find the original flow! */
    if ( orig_flow == NULL ) {
            if ( packetmatch_debug ) cout << "Can't find the original flow" << endl;
            return NULL;
        }

    if ( packetmatch_debug ) cout << "Returning flow matched by ICMP packet " << endl;
    return orig_flow;
}

/* Process an ICMP error message, in particular find and expire the original flow that caused the error
 *
 * Parameters:
 *  icmp_hdr - a pointer to the ICMP header of the error packet
 *  rem - the number of bytes remaining in the captured packet (including
 *        the ICMP header)
 *
 */
void icmp_error ( libtrace_icmp_t *icmp_hdr, uint32_t rem ) {
    Flow *orig_flow;

    if ( config.ignore_icmp_errors )
        return;

    orig_flow = icmp_find_original_flow ( icmp_hdr, rem );
    if ( orig_flow == NULL )
        return;

    /* Expire the original flow immediately */
    orig_flow->flow_state = FLOW_STATE_ICMPERROR;
}

/* Updates the UDP state, based on whether we're currently looking at an outbound packet or not
 *
 * Parameters:
 *  f - the UDP flow to be updated
 *  dir - the direction of the current packet
 */
void update_udp_state ( Flow *f, uint8_t dir ) {

    /* If the packet is inbound, UDP state cannot be changed */
    if ( dir == 1 )
        return;

    if ( !f->saw_outbound ) {
            /* First outbound packet has been observed */
            f->saw_outbound = true;
        } else {
            /* This must be at least the second outbound packet,
             * ensure we are using standard UDP expiry rules */
            f->flow_state = FLOW_STATE_UDPLONG;
        }

}

/* Examines the TCP flags in a packet and updates the flow state accordingly
 *- only called on algos that do tcp expiry.
 * Parameters:
 *  flow - the flow that the TCP packet belongs to
 *  tcp - a pointer to the TCP header in the packet
 *  dir - the direction of the packet (0=outgoing, 1=incoming)
 *  ts - the timestamp from the packet
 */
void lfm_check_tcp_flags ( Flow *flow, libtrace_tcp_t *tcp, uint8_t dir, double ts ) {
    assert ( tcp );
    assert ( flow );

    if ( dir >= 2 )
        return;

	/* closing tcp sessions: - wait for final ack before closing */
    if ( tcp->ack && flow->dir_info[0].saw_fin && flow->dir_info[1].saw_fin ) {
        flow->flow_state = FLOW_STATE_CLOSE;
    }

	/* if seen a single fin, then half close it */
    if ( tcp->fin ) {
		if ( flow->dir_info[0].saw_fin || flow->dir_info[1].saw_fin )
            flow->flow_state = FLOW_STATE_HALFCLOSE;
    }

	/* opening sessions */
    if ( tcp->syn ) {
		/* SYNs in both directions will put us in the TCP Established
		* state (note that we do not wait for the final ACK in the
		* 3-way handshake). */
		if ( flow->dir_info[0].saw_syn && flow->dir_info[1].saw_syn && tcp->ack ) {
			/* Make sure this is a SYN ACK before shifting state */
			flow->flow_state = FLOW_STATE_ESTAB;
		}

		/* A SYN in only one direction puts us in the TCP Connection
		* Establishment state, i.e. the handshake */
		else if ( flow->dir_info[0].saw_syn || flow->dir_info[1].saw_syn )
			flow->flow_state = FLOW_STATE_CONN;

		/* We can never have a flow exist without observing an initial
		* SYN, so we should never reach a state where neither
		* direction has observed a SYN */
		else
			assert ( 0 );

	}


	if ( tcp->rst ) {
        /* RST = instant expiry */
        flow->flow_state = FLOW_STATE_RESET;
    }

}



void lfm_sync_tcp_flags ( Flow *flow, libtrace_tcp_t *tcp, uint8_t dir, double ts ) {
    assert ( tcp );
    assert ( flow );

    if ( dir >= 2 ) return;

    if ( tcp->fin ) flow->dir_info[dir].saw_fin = true;

    if ( tcp->syn ) {
        if (!flow->dir_info[dir].saw_syn) {
            flow->dir_info[dir].saw_syn = true;

            if (flow->dir_info[dir].seq == 0) {
                if (packet_debug)
                    cout  << "setting flow->dir_info[dir ("<< dir << ")].seq to "
                          << tcp->seq << " (ack num is) " << tcp->ack_seq << endl;
                flow->dir_info[dir].seq = tcp->seq;
            }
        }

        /* If this is the first packet observed for this direction, 
		 * record the timestamp */
        if ( flow->dir_info[dir].first_pkt_ts == 0.0 ) 
		  flow->dir_info[dir].first_pkt_ts = ts;
    }


    if ( tcp->rst ) {
        flow->saw_rst = true;
    }

}

void check_lengthening ( Flow *flow, libtrace_tcp_t *tcp, uint8_t dir, double ts )  {
    int p = 0;
    int q = 1;
    if (dir == 0) {
        p = 1;
        q = 0;
    }

    //checking for a syn in the middle of a flow.
    if (
		tcp->syn 										&& 
        flow->dir_info[0].saw_syn 						&&
        flow->dir_info[1].saw_syn 						&&
        //flow->flow_state == FLOW_STATE_ESTAB 			&&
        ( ( tcp->seq - flow->dir_info[q].seq ) > 1 ) 	&&
        flow->packets_in > 5 							&&
        flow->packets_out > 5

    ) {
        if (debug_activeflow) {
            cout << "tcp->ack_seq:" << tcp->ack_seq - (flow->dir_info[p].seq +16777216)
                 << "(" << tcp->ack_seq << ")"
                 << " tcp->seq:" << tcp->seq - flow->dir_info[q].seq
                 << "(" << tcp->seq << ")"
                 << endl;
        }
        if (debug_activeflow) printdebugflow(flow,"s");
        flow->tcp_lengthened++;
		//cout << "lengthened!\n";
    }
   
   if(debug_activeflow){
	  cout << "a>" <<  flow->dir_info[0].saw_syn << endl;
      cout << "b>" <<  flow->dir_info[1].saw_syn  << endl;
	  cout << "c>" <<  flow->flow_state  << endl;
	  cout << "cx>" << FLOW_STATE_ESTAB << endl;
	  cout << "d>" <<  ( ( tcp->seq - flow->dir_info[q].seq ) > 1 ) 	 << endl;
	  cout << "e>" <<  flow->packets_in  << endl;
	  cout << "f>" <<  flow->packets_out  << endl;

	 
   }
    
}


/* Returns a pointer to the Flow that matches the packet provided. If no such Flow exists, a new Flow is created and added to the flow map before being returned.
 *
 * Flow matching is typically done based on a standard 5-tuple, although I
 * have recently added a config option for also using the VLAN id.
 *
 * Parameters:
 * 	packet - the packet that is to be matched with a flow
 * 	dir - the direction of the packet. 0 indicates outgoing, 1 indicates
 * 	      incoming.
 * 	is_new_flow - a boolean flag that is set to true by this function if
 * 	              a new flow was created for this packet. It is set to
 * 	              false if the returned flow already existed in the flow
 * 	              map.
 *
 * Returns:
 * 	a pointer to the entry in the active flow map that matches the packet
 * 	provided, or NULL if the packet cannot be matched to a flow
 *
 */
Flow *lfm_consolidated_match_packet_to_flow(libtrace_packet_t *packet, uint8_t dir, bool *is_new_flow) {
    uint16_t src_port, dst_port;
    uint8_t trans_proto = 0;
    libtrace_ip_t *ip;
    libtrace_ip6_t *ip6 = NULL;
    FlowId pkt_id;
    Flow *new_conn;
    ExpireList *exp_list;
	OrderedExpireList *ord_exp_list;
    uint16_t l3_type;
    uint32_t pkt_left = 0;
    uint32_t rem;
    uint16_t vlan_id = 0;

    ip = (libtrace_ip_t *)trace_get_layer3(packet, &l3_type, &pkt_left);
    if (ip == NULL) {
        if(packetmatch_debug) cout << "ip == null\n";
        return NULL;
    }
    if (ip->ip_v == 6) {
        if (config.disable_ipv6) {
            if(packetmatch_debug) cout << "ipv6 disabled\n";
            return NULL;
        }
        ip6 = (libtrace_ip6_t*)ip;
    } else {
        if (config.disable_ipv4) {
            if(packetmatch_debug) cout << "conf disable ipv4\n";
            return NULL;
        }
    }

    trace_get_transport(packet, &trans_proto, &rem);

    /* For now, deal with IPv4 only */
    if (l3_type != 0x0800 && l3_type != 0x86DD) {
        if(packetmatch_debug) cout << "ipv4 only (this isn't ipv4)\n";
        return NULL;
    }

    /* If the VLAN key option is set, we'll need the VLAN id */
    if (config.key_vlan) vlan_id = extract_vlan_id(packet);

    /* Get port numbers for our 5-tuple */
    src_port = dst_port = 0;
    src_port = trace_get_source_port(packet);
    dst_port = trace_get_destination_port(packet);
	
	if(packetmatch_debug) cout << "src/dest ports: " << src_port << " / " << dst_port << endl;

    /* Ignore any RFC1918 addresses, if requested */
    if (l3_type == 0x0800 && config.ignore_rfc1918 && rfc1918_ip(ip)) {
        if(packetmatch_debug) cout << "ignoring rfc1918\n";
        return NULL;
    }

    /* Fragmented TCP and UDP packets will have port numbers of zero. We
     * don't do fragment reassembly, so we will want to ignore them.
     */
    if (src_port == 0 && dst_port == 0 && (trans_proto == 6 || trans_proto == 17)) {
        if(packetmatch_debug) cout << "ignoring fragmented packet segments\n";
        return NULL;
    }
    /* Generate the flow key for this packet */

    /* Force ICMP flows to have port numbers of zero, rather than
     * whatever random values trace_get_X_port might give us */
    if (trans_proto == 1 && dir == 0) {
        if (ip6) pkt_id = FlowId(ip6->ip_src.s6_addr, ip6->ip_dst.s6_addr,0, 0, trans_proto, vlan_id, next_conn_id);
        else     pkt_id = FlowId(ip->ip_src.s_addr, ip->ip_dst.s_addr,0, 0, ip->ip_p, vlan_id, next_conn_id);
    } 
    //otherwise, create the flow. 
    else if (ip->ip_p == 1) {
        if (ip6) pkt_id = FlowId(ip6->ip_dst.s6_addr, ip6->ip_src.s6_addr,0, 0, trans_proto, vlan_id, next_conn_id);
        else     pkt_id = FlowId(ip->ip_dst.s_addr, ip->ip_src.s_addr,0, 0, ip->ip_p, vlan_id, next_conn_id);
    }
    else if (
        (src_port == dst_port && dir == 1) ||
        (src_port != dst_port && trace_get_server_port(ip->ip_p, src_port, dst_port) == USE_SOURCE)
    ) {
        /* Server port = source port */
        if (ip6) pkt_id = FlowId(ip6->ip_src.s6_addr,ip6->ip_dst.s6_addr,src_port, dst_port, trans_proto,vlan_id, next_conn_id);
        else     pkt_id = FlowId(ip->ip_src.s_addr,ip->ip_dst.s_addr,src_port, dst_port, ip->ip_p,vlan_id, next_conn_id);
    } else {
        /* Server port = dest port */
        if (ip6) pkt_id = FlowId(ip6->ip_dst.s6_addr,ip6->ip_src.s6_addr,dst_port, src_port, trans_proto,vlan_id, next_conn_id);
        else     pkt_id = FlowId(ip->ip_dst.s_addr,ip->ip_src.s_addr,dst_port, src_port, ip->ip_p,vlan_id, next_conn_id);
    }
    
    if (config.timeout_type == NETRAMET || 
		config.timeout_type == NETRAMET_SHORT ||
		config.timeout_type == PGAT ||
		config.timeout_type == MBET ||
		config.timeout_type == DOTS
	   ) {
        if (packetmatch_debug) cout << "number of ordered_active_flows:\t" << ordered_active_flows.size() << "\n";
    }
    else {
        if (packetmatch_debug) cout << "number of active_flows:\t" << active_flows.size() << "\n";
    }


    /* Try to find the flow key in our active flows map */

    if (config.timeout_type == NETRAMET || 
	    config.timeout_type == NETRAMET_SHORT ||
	    config.timeout_type == PGAT ||
	    config.timeout_type == MBET ||
	    config.timeout_type == DOTS
	   ) {
	  	OrderedFlowMap::iterator j = ordered_active_flows.find(pkt_id);
        if (j != ordered_active_flows.end()) {
            /* Found the flow in the map! */
            Flow *pkt_conn = *((*j).second);

	  
            /* Update UDP "state" */
            if (trans_proto == 17) update_udp_state(pkt_conn, dir);

            *is_new_flow = false;
            if (packetmatch_debug) {
			  cout << "\t packet matched flow (ordered):\n";
			  printflow(pkt_conn,"ab","ab");
			  cout << pkt_conn->id.get_id_num() << "\n";
			}
			if (trans_proto == 6) {
    
// 				libtrace_tcp_t *tcp = trace_get_tcp(packet);
// 				cout << "d: " << dir << endl;
// 				//check for syns in the middle of flows
// 				if(tcp->syn && pkt_conn->dir_info[dir].saw_syn == true){
// 					
// 					assert(false);
// 				}else{
// 					pkt_conn->dir_info[dir].saw_syn = true;
// 				}
			}
            return pkt_conn;
        }
    }
    else {
	  	FlowMap::iterator i = active_flows.find(pkt_id);

        if (i != active_flows.end()) {
            /* Found the flow in the map! */
            Flow *pkt_conn = *((*i).second);

            /* Update UDP "state" */
            if (trans_proto == 17) update_udp_state(pkt_conn, dir);

            *is_new_flow = false;
            if (packetmatch_debug) cout << "\t packet matched flow: " << pkt_conn->id.get_id_num() << "\n";
            return pkt_conn;
        }
    }

    /* If we reach this point, we must be dealing with a new flow */

	/** TCP */
    if (trans_proto == 6) {
    
        libtrace_tcp_t *tcp = trace_get_tcp(packet);
		
		if (!tcp) {
		  if(packetmatch_debug) cout << "tcp flow must actually be tcp\n";
		  return NULL;
		}
		

		if (config.timeout_type == NETRAMET || 
		    config.timeout_type == NETRAMET_SHORT || 
		    config.timeout_type == CLAFFY || 
		    config.timeout_type == CLAFFY_TCP ||
		    config.timeout_type == CLAFFY_UNLIMITED  ||
		    config.timeout_type == DOTS ||
		    config.timeout_type == MBET ||
		    config.timeout_type == PGAT
		) {
		  /*no tcp state for these, but set syn flag for direction*/
		  //first packet

// 		  cout << "new dir: " << dir << endl;
// 		  if(tcp->syn && !tcp->ack){  
// 			new_conn->dir_info[dir].saw_syn = true;
// 		  }
// 		  //second packet (synack) - assume syn in opposite is dir has been seen
// 		  else if(tcp->syn && tcp->ack){
// 			new_conn->dir_info[dir].saw_syn = true;
// 		  }
// 		  //standard packet, assume syn's seen. 
// 		  else if(!tcp->syn){
// 			new_conn->dir_info[dir].saw_syn = true;
// 		  }
			  
		}
		else{
			/* TCP Flows must begin with a SYN - only in strict mode*/
			if(config.rfc_strict_tcp){
			  if (!tcp->syn) {
				  if(packetmatch_debug) cout << "tcp flow must begin with syn\n";
				  return NULL;
			  }

			  /* Avoid creating a flow based on the SYN ACK */
			  if (tcp->ack) {
				  if(packetmatch_debug) cout << "avoid creating a flow based on a syn ack\n";
				  return NULL;
			  }
			}
		}
        /* Create new TCP flow */
        new_conn = new Flow(pkt_id);
        new_conn->flow_state = FLOW_STATE_NEW;
		if (packet_debug) cout << "setting flow->dir_info[dir ("<< cout.dec << dir << ")].seq to " << tcp->seq << " (ack num is) " << tcp->ack_seq << endl;

		new_conn->dir_info[dir].seq = tcp->seq;
		if(tcp->ack_seq != 0){
		  new_conn->dir_info[1-dir].seq = tcp->ack_seq;
		}

        if (config.timeout_type == NETRAMET || 
		    config.timeout_type == NETRAMET_SHORT ) {
            ord_exp_list = &expire_netramet_ordered_tcp;
        }
        else if (config.timeout_type == PGAT || config.timeout_type == MBET || config.timeout_type == DOTS){
			ord_exp_list = &ordered_flows;
		}
        else {
			if(config.timeout_type == CLAFFY || config.timeout_type == CLAFFY_TCP || config.timeout_type == CLAFFY_UNLIMITED)
			  exp_list = &expire_claffy;
			else
			  exp_list = &expire_tcp_syn;  		  
        }
    }

	/**  ICMP is outside the scope of this project. May revisit.*/
    else if (trans_proto == 1) {
		/* ICMP */
// 		  libtrace_icmp_t *icmp_hdr;
// 		  if (ip6) icmp_hdr = (libtrace_icmp_t *)trace_get_payload_from_ip6(ip6, NULL, &pkt_left);
// 		  else icmp_hdr = (libtrace_icmp_t *)trace_get_payload_from_ip(ip, NULL, &pkt_left);
// 
// 		  if (!icmp_hdr) {
// 			  if(packetmatch_debug) cout << "icmp without icmp header\n";
// 			  return NULL;
// 		  }
// 		  /* Deal with special ICMP messages, e.g. errors */
// 		  switch (icmp_hdr->type) {
// 			  /* Time exceeded is probably part of a traceroute, rather than a genuine error */
// 		  case 11:
// 			  return icmp_find_original_flow(icmp_hdr,pkt_left);
// 
// 			  /* These cases are all ICMP errors - find and expire the original flow */
// 		  case 3:
// 		  case 4:
// 		  case 12:
// 		  case 31:
// 			  icmp_error(icmp_hdr, pkt_left);
// 			  if(packetmatch_debug) cout << "icmp error packet - found and expired flow related to icmp packet\n";
// 			  return NULL;
// 		  }
// 		  /* Otherwise, we must be a legit ICMP flow */
// 		  new_conn = new Flow(pkt_id);
// 		  new_conn->flow_state = FLOW_STATE_NONE;
// 		  exp_list = &expire_udp;

		return NULL;
	}
    /** UDP */
	else if (trans_proto == 17) {
	  
        /* We don't have handy things like SYN flags to
         * mark the beginning of UDP connections */
        new_conn = new Flow(pkt_id);
		
        if (dir == 0) new_conn->saw_outbound = true;

        if (config.timeout_type == NETRAMET || 
		    config.timeout_type == NETRAMET_SHORT
		) {
            new_conn->flow_state = FLOW_STATE_NONE;
            ord_exp_list = &expire_netramet_ordered_udp;
        }
        if (config.timeout_type == PGAT || config.timeout_type == MBET ||config.timeout_type == DOTS){
			ord_exp_list = &ordered_flows;
		}
        else {

            /* If we're using the short-lived UDP expiry rules,
             * all new flows will start with the short timeout */
            if (config.short_udp) {
                new_conn->flow_state = FLOW_STATE_UDPSHORT;
				if(config.timeout_type == CLAFFY || config.timeout_type == CLAFFY_TCP || config.timeout_type == CLAFFY_UNLIMITED)
				  exp_list = &expire_claffy;
				else
				  exp_list = &expire_udpshort;                
            } else {
                /* Otherwise, use the standard UDP timeout */
                new_conn->flow_state = FLOW_STATE_UDPLONG;
			if(config.timeout_type == CLAFFY || config.timeout_type == CLAFFY_TCP || config.timeout_type == CLAFFY_UNLIMITED)
			  exp_list = &expire_claffy;
			else
			  exp_list = &expire_udp;            }
        }
    }
    /** Not UDP or TCP or ICMP */
    else {
		 /* If we're in legacy mode, we're interested in adding it to tracked flows.*/		  
		if(config.legacy == true){
			/* handle netramet other stuff */
			if (config.timeout_type == NETRAMET || 
				config.timeout_type == NETRAMET_SHORT || 
				config.timeout_type == PGAT ||
				config.timeout_type == MBET ||
				config.timeout_type == DOTS
			) {
				new_conn = new Flow(pkt_id);
				new_conn->flow_state = FLOW_STATE_NONE;
				if (dir == 0) new_conn->saw_outbound = true;
				if(
				  config.timeout_type == PGAT || 
				  config.timeout_type == MBET ||
				  config.timeout_type == DOTS
				) ord_exp_list = &ordered_flows;
				else ord_exp_list = &expire_netramet_ordered_udp;
			}
			else {
				new_conn = new Flow(pkt_id);
				if (dir == 0) new_conn->saw_outbound = true;
				
				new_conn->flow_state = FLOW_STATE_NONE;
				if (config.timeout_type == CLAFFY || 
					config.timeout_type == CLAFFY_TCP || 
					config.timeout_type == CLAFFY_UNLIMITED)
				  exp_list = &expire_claffy;
				else
				  exp_list = &expire_udp;
				if(packetmatch_debug) printf("Rejecting unknown packet. proto: %d\n",trans_proto);
			}

		}
        /* Unknown protocol - only interested in udp and tcp */
		else {
		  	if(packetmatch_debug) printf("Rejecting unknown packet. proto: %d\n",trans_proto);
			return NULL;
		}
    }

    /* Knowing the timestamp of the first packet for a flow is very handy */
    if (dir < 2 ) 
	  if(new_conn->dir_info[dir].first_pkt_ts == 0.0)
        new_conn->dir_info[dir].first_pkt_ts = trace_get_seconds(packet);

	new_conn->start_time = trace_get_seconds(packet);
	
    /* Append our new flow to the appropriate LRU */

    if (config.timeout_type == NETRAMET || 
		config.timeout_type == NETRAMET_SHORT ||
		config.timeout_type == PGAT ||
		config.timeout_type == MBET ||
		config.timeout_type == DOTS
	) {
        new_conn->ord_expire_list = ord_exp_list;
        OrderedExpireList::iterator iter = ord_exp_list->insert(new_conn);
		
		/* Add flow id to map. */
		ordered_active_flows[new_conn->id] = iter;
    }
    else {
        new_conn->expire_list = exp_list;
        exp_list->push_front(new_conn);
			
		/* Add our flow to the active flows map (or more correctly, add the
		* iterator for our new flow in the LRU to the active flows map -
		* this makes it easy for us to find the flow in the LRU)  */
		active_flows[new_conn->id] = exp_list->begin();
    }

    /* Increment the counter we use to generate flow IDs */
    next_conn_id ++;

    /* Set the is_new_flow flag to true, because we have indeed created
     * a new flow for this packet */
    *is_new_flow = true;
	
	new_conn->transport = trans_proto;

	if(packetmatch_debug) cout << "Returning new connection\n";
    return new_conn;
}
