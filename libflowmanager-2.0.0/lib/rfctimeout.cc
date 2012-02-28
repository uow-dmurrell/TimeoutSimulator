#include <libflowmanager.h>



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
Flow *lfm_match_packet_to_flow(libtrace_packet_t *packet, uint8_t dir, bool *is_new_flow) {

    uint16_t src_port, dst_port;
    uint8_t trans_proto = 0;
    libtrace_ip_t *ip;
    libtrace_ip6_t *ip6 = NULL;
    FlowId pkt_id;
    Flow *new_conn;
    ExpireList *exp_list;
    uint16_t l3_type;
    uint32_t pkt_left = 0;
    uint32_t rem;
    uint16_t vlan_id = 0;

    ip = (libtrace_ip_t *)trace_get_layer3(packet, &l3_type, &pkt_left);
    if (ip == NULL) {
        //cout << "ip == null\n";
        return NULL;
    }
    if (ip->ip_v == 6) {
        if (config.disable_ipv6) {
            //cout << "ipv6 disabled\n";
            return NULL;
        }
        ip6 = (libtrace_ip6_t*)ip;
    } else {
        if (config.disable_ipv4) {
            //cout << "conf disable ipv4\n";
            return NULL;
        }
    }

    trace_get_transport(packet, &trans_proto, &rem);

    /* For now, deal with IPv4 only */
    if (l3_type != 0x0800 && l3_type != 0x86DD) {
        //cout << "ipv4 only (this isn't ipv4)\n";
        return NULL;
    }

    /* If the VLAN key option is set, we'll need the VLAN id */
    if (config.key_vlan) vlan_id = extract_vlan_id(packet);

    /* Get port numbers for our 5-tuple */
    src_port = dst_port = 0;
    src_port = trace_get_source_port(packet);
    dst_port = trace_get_destination_port(packet);

    /* Ignore any RFC1918 addresses, if requested */
    if (l3_type == 0x0800 && config.ignore_rfc1918 && rfc1918_ip(ip)) {
        //cout << "ignoring rfc1918\n";
        return NULL;
    }

    /* Fragmented TCP and UDP packets will have port numbers of zero. We
     * don't do fragment reassembly, so we will want to ignore them.
     */
    if (src_port == 0 && dst_port == 0 && (trans_proto == 6 || trans_proto == 17)) {
        //cout << "ignoring fragmented packet segments\n";
        return NULL;
    }
    /* Generate the flow key for this packet */

    /* Force ICMP flows to have port numbers of zero, rather than
     * whatever random values trace_get_X_port might give us */
    if (trans_proto == 1 && dir == 0) {
        if (ip6) pkt_id = FlowId(ip6->ip_src.s6_addr, ip6->ip_dst.s6_addr,0, 0, trans_proto, vlan_id, next_conn_id);
        else     pkt_id = FlowId(ip->ip_src.s_addr, ip->ip_dst.s_addr,0, 0, ip->ip_p, vlan_id, next_conn_id);
    } else if (ip->ip_p == 1) {
        if (ip6) pkt_id = FlowId(ip6->ip_dst.s6_addr, ip6->ip_src.s6_addr,0, 0, trans_proto, vlan_id, next_conn_id);
        else     pkt_id = FlowId(ip->ip_dst.s_addr, ip->ip_src.s_addr,0, 0, ip->ip_p, vlan_id, next_conn_id);
    }
    else if (
        (src_port == dst_port && dir == 1)
        ||
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

    /* Try to find the flow key in our active flows map */
    FlowMap::iterator i = active_flows.find(pkt_id);

    //cout << "number of active_flows:\t" << active_flows.size() << "\n";

    if (i != active_flows.end()) {
        /* Found the flow in the map! */
        Flow *pkt_conn = *((*i).second);

        /* Update UDP "state" */
        if (trans_proto == 17) update_udp_state(pkt_conn, dir);

        *is_new_flow = false;
 		//cout << "\t packet matched flow: " << pkt_conn->id.get_id_num() << "\n";
        return pkt_conn;
    }

    /* If we reach this point, we must be dealing with a new flow */

    if (trans_proto == 6) {
        /* TCP */
        libtrace_tcp_t *tcp = trace_get_tcp(packet);

        /* TCP Flows must begin with a SYN */
        if (!tcp) {
            //cout << "tcp flow must actually be tcp\n";
            return NULL;
        }

        if (!tcp->syn) {
           //cout << "tcp flow must begin with syn\n";
            return NULL;
        }

        /* Avoid creating a flow based on the SYN ACK */
        if (tcp->ack) {
            //cout << "avoid creating a flow based on a syn ack\n";
            return NULL;
        }
        /* Create new TCP flow */
        new_conn = new Flow(pkt_id);
        new_conn->flow_state = FLOW_STATE_NEW;
        exp_list = &expire_tcp_syn;
    }

    else if (trans_proto == 1) {
        /* ICMP */
        libtrace_icmp_t *icmp_hdr;
        if (ip6)
            icmp_hdr = (libtrace_icmp_t *)trace_get_payload_from_ip6(ip6,
                       NULL, &pkt_left);
        else
            icmp_hdr = (libtrace_icmp_t *)trace_get_payload_from_ip(ip,
                       NULL, &pkt_left);

        if (!icmp_hdr) {
            //cout << "icmp without icmp header\n";
            return NULL;
        }
        /* Deal with special ICMP messages, e.g. errors */
        switch (icmp_hdr->type) {
            /* Time exceeded is probably part of a traceroute,
             * rather than a genuine error */
        case 11:
            return icmp_find_original_flow(icmp_hdr,pkt_left);

            /* These cases are all ICMP errors - find and expire
             * the original flow */
        case 3:
        case 4:
        case 12:
        case 31:
            icmp_error(icmp_hdr, pkt_left);
            //cout << "icmp error packet - fouund and expired flow related to icmp packet\n";
            return NULL;
        }

        /* Otherwise, we must be a legit ICMP flow */
        new_conn = new Flow(pkt_id);
        new_conn->flow_state = FLOW_STATE_NONE;
        exp_list = &expire_udp;
    } else {

        /* We don't have handy things like SYN flags to
         * mark the beginning of UDP connections */
        new_conn = new Flow(pkt_id);
        if (trans_proto == 17) {
            /* UDP */


            if (dir == 0)
                new_conn->saw_outbound = true;

            /* If we're using the short-lived UDP expiry rules,
             * all new flows will start with the short timeout */
            if (config.short_udp) {
                new_conn->flow_state = FLOW_STATE_UDPSHORT;
                exp_list = &expire_udpshort;
            } else {
                /* Otherwise, use the standard UDP timeout */
                new_conn->flow_state = FLOW_STATE_UDPLONG;
                exp_list = &expire_udp;
            }

        }
        else {
            /* Unknown protocol - follow the standard UDP expiry
             * rules */
            new_conn->flow_state = FLOW_STATE_NONE;
            exp_list = &expire_udp;
        }

    }

    /* Knowing the timestamp of the first packet for a flow is very
     * handy */
    if (dir < 2 && new_conn->dir_info[dir].first_pkt_ts == 0.0)
        new_conn->dir_info[dir].first_pkt_ts = trace_get_seconds(packet);

	new_conn->start_time = trace_get_seconds(packet);
	
    /* Append our new flow to the appropriate LRU */
    new_conn->expire_list = exp_list;
    exp_list->push_front(new_conn);

    /* Add our flow to the active flows map (or more correctly, add the
     * iterator for our new flow in the LRU to the active flows map -
     * this makes it easy for us to find the flow in the LRU)  */
    active_flows[new_conn->id] = exp_list->begin();

    /* Increment the counter we use to generate flow IDs */
    next_conn_id ++;

    /* Set the is_new_flow flag to true, because we have indeed created
     * a new flow for this packet */
    *is_new_flow = true;

    return new_conn;
}



/* Updates the timeout for a Flow.
 *
 * The flow state determines how long the flow can be idle before it times out.
 *
 * Many of the values are selected based on best practice for NAT devices,
 * which tend to be quite conservative.
 *
 * Parameters:
 * 	flow - the flow that is to be updated
 * 	ts - the timestamp of the last packet observed for the flow
 */
void lfm_update_flow_expiry_timeout(Flow *flow, double ts) {
	
	flow->last_time = ts;
  
    ExpireList *exp_list = NULL;
    switch (flow->flow_state) {

	  
    case FLOW_STATE_RESET:
    case FLOW_STATE_ICMPERROR:
        /* We want to expire this as soon as possible */
        flow->expire_time = ts;
        exp_list = &expired_flows;
        break;

        /* Unestablished TCP connections expire after 2 * the
         * maximum segment lifetime (RFC 1122)
         *
         * XXX include half-closed flows in here to try and expire
         * single FIN flows reasonably quickly */
    case FLOW_STATE_NEW:
    case FLOW_STATE_CONN:
    case FLOW_STATE_HALFCLOSE:
        flow->expire_time = ts + 240.0;
        exp_list = &expire_tcp_syn;
        break;

        /* Established TCP connections expire after 2 hours and 4
         * minutes (RFC 5382) */
    case FLOW_STATE_ESTAB:
        flow->expire_time = ts + 7440.0;
        exp_list = &expire_tcp_estab;
        break;

        /* UDP flows expire after 2 minutes (RFC 4787) */
    case FLOW_STATE_NONE:
    case FLOW_STATE_UDPLONG:
        flow->expire_time = ts + 120.0;
        exp_list = &expire_udp;
        break;

        /* UDP flows that have not seen more than one outgoing packet
         * expire after just 10 seconds. This is an experimental
         * technique that is not defined in any RFC or Internet draft,
         * but has proven effective in reducing the number of UDP flows
         * in the flow map. The 10 second value was selected
         * arbitrarily - I hope to one day figure out what the right
         * value is */
    case FLOW_STATE_UDPSHORT:
        flow->expire_time = ts + 10.0;
        exp_list = &expire_udpshort;
        break;


    case FLOW_STATE_CLOSE:
        /* If the timewait option is set for closed TCP
         * connections, keep the flow for an extra 2 minutes */
        if (config.tcp_timewait) {
            flow->expire_time = ts + 120.0;
            exp_list = &expire_udp;
        } else {
            /* We want to expire this as soon as possible */
            flow->expire_time = ts;
            exp_list = &expired_flows;
        }
        break;


    }

    assert(exp_list);

    /* Remove the flow from its current position in an LRU */
    flow->expire_list->erase(active_flows[flow->id]);

    /* Push it onto its new LRU */
    flow->expire_list = exp_list;
    exp_list->push_front(flow);

    /* Update the entry in the flow map */
    active_flows[flow->id] = exp_list->begin();

}
