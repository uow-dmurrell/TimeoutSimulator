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
 * $Id: libflowmanager.h 41 2010-09-20 22:34:18Z salcock $
 *
 */

#ifndef LIBFLOWMANAGER_H_
#define LIBFLOWMANAGER_H_

#define IPCOMP(addr, n) ((addr >> (24 - 8 * n)) & 0xFF)


#include <map>
#include <list>
#include <set>
#include <iostream>
#include <iomanip>
#include <inttypes.h>
#include <sstream>
#include <time.h>

#include <libtrace.h>

#include "helpers.h"

using std::cout;
using std::endl;
using std::setw;
using std::fixed;
using std::dec;
using namespace std;

/***** debugging.... */
extern bool packet_debug;
extern bool timeoutbump_debug;
extern bool packetmatch_debug;
extern bool flowexpire_debug;
extern bool debug_netramet;
extern bool debug_optarg;
extern bool debug_activeflow;

/***** output filehandle */

extern FILE *file;

#ifdef __cplusplus
extern "C" {
#endif

#include <libtrace.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>

#include <errno.h>
#include <string.h>
#include <math.h>

    /**********
     * Config *
     **********/

	
	
	
    /* Support configuration options for libflowmanager */
    typedef enum {
        /* Ignore packets containing RFC 1918 addresses */
        LFM_CONFIG_IGNORE_RFC1918,

        /* Wait a short time before expiring flows for TCP connections that
         * have closed via FIN packets */
        LFM_CONFIG_TCP_TIMEWAIT,

        /* Use experimental fast expiry for short-lived UDP flows */
        LFM_CONFIG_SHORT_UDP,

        /* Use VLAN Id as part of flow keys */
        LFM_CONFIG_VLAN,

        /* Ignore ICMP errors that would normally expire a flow immediately */
        LFM_CONFIG_IGNORE_ICMP_ERROR,

        /* handle IPv6 only */
        LFM_CONFIG_DISABLE_IPV4,

        /* handle IPv4 only */
        LFM_CONFIG_DISABLE_IPV6

    } lfm_config_t;


    /* List of timeout types */
    typedef enum {
        /* Standard RFC, as defined by salcock */
        RFC,

        /* Claffy 64 second timeout */
        CLAFFY,

        /* Claffy 64 second, with TCP knowledge */
        CLAFFY_TCP,

        /* Claffy 64 second, with TCP knowledge */
        CLAFFY_UNLIMITED,

        /* Cisco 15 minute bucketing */
        CISCO_BUCKET,

        /* Netramet */
        NETRAMET,

        /* Netramet with early timeout */
        NETRAMET_SHORT,

        /* Measurement Binary Exponential Timeout */
        MBET,

        /* Dynamic Timeout Strategy based on Flow Rate Metrics  */
        DOTS,

        /* probability-guaranteed adaptive timeout  */
        PGAT

    } lfm_timeout_t;

    extern lfm_timeout_t timeout_type;

    /* We just use a standard 5-tuple as a flow key (with rudimentary support for VLAN Ids) */
    /* XXX: Consider expanding this to support mac addresses as well */
    class FlowId {
    private:
        int cmp ( const FlowId &b ) const ;

        /* The five tuple */
        union {
            uint32_t ip4_a;
            uint8_t ip6_a[16];
        } ip_a;
        union {
            uint32_t ip4_b;
            uint8_t ip6_b[16];
        } ip_b;

        uint16_t port_a;
        uint16_t port_b;
        uint8_t proto;

        /* IP version, 4 or 6 */
        uint8_t ip_v;

        /* VLAN Id */
        uint16_t vlan;
        /* Unique flow ID number */
        uint32_t id_num;

    public:
        FlowId();

        FlowId ( uint32_t ip_src, uint32_t ip_dst, uint16_t port_src,
                 uint16_t port_dst, uint8_t protocol, uint16_t vlan,
                 uint32_t id );

        FlowId ( uint8_t ip_src[16], uint8_t ip_dst[16], uint16_t port_src,
                 uint16_t port_dst, uint8_t protocol, uint16_t vlan,
                 uint32_t id );

        bool operator< ( const FlowId &b ) const ;

        /* Accessor functions */
        uint32_t get_id_num() const ;
        //const char *get_server_ip_str() const ;
        //const char *get_client_ip_str() const ;
        void get_server_ip_str ( char * ret ) const ;
        void get_client_ip_str ( char * ret ) const ;
        uint32_t get_server_ip() const ;
        uint8_t * get_server_ip6() const ;
        uint32_t get_client_ip() const ;
        uint8_t * get_client_ip6() const ;
        uint16_t get_server_port() const ;
        uint16_t get_client_port() const ;
        uint16_t get_vlan_id() const ;
        uint8_t get_protocol() const ;
        uint8_t get_ip_version() const ;

    };


    /* List of flow states - flow state often determines how long a flow must be idle before expiring */
    typedef enum {
        /* Unknown protocol - no sensible state is possible */
        FLOW_STATE_NONE,

        /* New TCP connection */
        FLOW_STATE_NEW,

        /* Unestablished TCP connection */
        FLOW_STATE_CONN,

        /* Established TCP connection */
        FLOW_STATE_ESTAB,

        /* Half-closed TCP connection */
        FLOW_STATE_HALFCLOSE,

        /* Reset TCP connection */
        FLOW_STATE_RESET,

        /* Closed TCP connection */
        FLOW_STATE_CLOSE,

        /* UDP flow where only one outgoing packet has been observed */
        FLOW_STATE_UDPSHORT,

        /* UDP flow where multiple outgoing packets have been seen */
        FLOW_STATE_UDPLONG,

        /* Flow experienced an ICMP error */
        FLOW_STATE_ICMPERROR
    } flow_state_t;

    /* Data that must be stored separately for each half of the flow */
    class DirectionInfo {
    public:
        double first_pkt_ts; /* Timestamp of first observed packet */
        bool saw_fin; /* Have we seen a TCP FIN in this direction */
        bool saw_syn; /* Have we seen a TCP SYN in this direction */
        uint32_t seq; /* starting sequence number */

        DirectionInfo();
    };

	

    /*********
     * Flows *
     *********/
	/* An list of flows, ordered by expire time */
    typedef std::list<class Flow *> ExpireList;
    typedef std::map<FlowId, ExpireList::iterator> FlowMap;

	

	struct comp {
		 bool operator()(const Flow *a, const Flow *b) const;
	};
	
	/* An actually ordered list of flows, ordered by expire time */
	typedef std::multiset<class Flow *, comp> OrderedExpireList;
    typedef std::map<FlowId, OrderedExpireList::iterator> OrderedFlowMap;

	

    class Flow {

    public:
        /* The flow key for this flow */
        FlowId id;

        /* Per-direction information */
        DirectionInfo dir_info[2];

        /* The LRU that the flow is currently in */
        ExpireList *expire_list;
        OrderedExpireList *ord_expire_list;

        /* The timestamp that the flow is due to expire */
        double expire_time;

        /* The timestamp that the stream was first seen */
        double start_time;

        /* The timestamp that the stream was last seen */
        double last_time;

        /* Number of packets */
        double packets_in;
        double packets_out;

        /* Number of bytes */
        double bytes_in;
        double bytes_out;

        /* Transport */
        uint8_t transport;

        /* Current flow state */
        flow_state_t flow_state;

        /* Have we seen a reset for this flow? */
        bool saw_rst;

        /* Has the flow been expired by virtue of being idle for too
         * long (vs a forced expiry, for instance) */
        bool expired;

        /* Has an outbound packet been observed for this flow */
        bool saw_outbound;

        /* Users of this library can use this pointer to store
         * per-flow data they require above and beyond what is
         * defined in this class definition */
        void *extension;

        double packet_interval;

        /*mbet extension*/
        int Tptr;	//timeout pointer
        int Pptr;	//next packet pointer

        /*pgat extension*/
        int col; //protocol column, set if tcp

        bool operator< (  const Flow& b ) const ;
		
		uint32_t tcp_lengthened;

		/* Constructor */
        Flow ( const FlowId conn_id );
    };



    /***************************
     * Timeout packet matchers *
     ***************************/

    /* Returns a pointer to the Flow that matches the packet provided. If no such Flow exists, a new Flow is created and added to the flow map before being returned.
     *
     * Parameters:
     *      packet - the packet that is to be matched with a flow
     *      dir - the direction of the packet. 0 indicates outgoing, 1 indicates
     *            incoming.
     *      is_new_flow - a boolean flag that is set to true by this function if
     *                    a new flow was created for this packet. It is set to
     *                    false if the returned flow already existed in the flow
     *                    map.
     *
     * Returns:
     *      a pointer to the entry in the active flow map that matches the packet
     *      provided, or NULL if the packet cannot be matched to a flow
     *
     */
    /* RFC - Shane's original method */
    Flow *lfm_match_packet_to_flow ( libtrace_packet_t *packet, uint8_t dir, bool *is_new_flow );
	
	/* all other matchers */
    Flow *lfm_consolidated_match_packet_to_flow ( libtrace_packet_t *packet, uint8_t dir, bool *is_new_flow );



    /* Examines the TCP flags in a packet and updates the flow state accordingly
     * - only called on algos that do tcp expiry.
     * Parameters:
     *      flow - the flow that the TCP packet belongs to
     *      tcp - a pointer to the TCP header in the packet
     *      dir - the direction of the packet (0=outgoing, 1=incoming)
     *      ts - the timestamp from the packet
     */
    void lfm_check_tcp_flags ( Flow *flow, libtrace_tcp_t *tcp, uint8_t dir, double ts );

	
	/* Syncs the tcp flags in a packet to the flow. Is lfm_check_tcp_flags split into the update flow
	 * based on tcp flags, but don't make any decisions. 
	 */
	
	void lfm_sync_tcp_flags ( Flow *flow, libtrace_tcp_t *tcp, uint8_t dir, double ts );
	
	/*Checks the current packet to see if it is part of a lengthening event
	 * This is where a flow has been ended, but started again without being timed out properly.
	 */
	void check_lengthening ( Flow *flow, libtrace_tcp_t *tcp, uint8_t dir, double ts );
	
	
    /*****************************
     * Timeout Expiry Algorithms *
     *****************************/

    /* Updates the timeout for a Flow.
     *
     * The flow state determines how long the flow can be idle before it times out.
     *
     * Many of the values are selected based on best practice for NAT devices,
     * which tend to be quite conservative.
     *
     * Parameters:
     *      flow - the flow that is to be updated
     *      ts - the timestamp of the last packet observed for the flow
     */
    void lfm_update_flow_expiry_timeout ( Flow *flow, double ts );

    /* Updates the timeout for a Flow.
     *
     * The flow state determines how long the flow can be idle before it times out.
     *
     * Many of the values are selected based on best practice for NAT devices,
     * which tend to be quite conservative.
     *
     * Parameters:
     *      flow - the flow that is to be updated
     *      ts - the timestamp of the last packet observed for the flow
     */
    void lfm_fixedtimeout_update_flow_expiry_timeout ( Flow *flow, double ts );

    /* Claffy 64 sec timeout with early tcp expiry */
    void lfm_fixedtimeout_tcp_update_flow_expiry_timeout ( Flow *flow, double ts );

    /* Netramet */
    void lfm_netramet_update_flow_expiry_timeout(Flow *flow, double ts);

    /* MBET */
    void lfm_mbet_update_flow_expiry_timeout(Flow *flow, double ts);

    /* DOTS */
    void lfm_dots_update_flow_expiry_timeout(Flow *flow, double ts);

    /* PGAT */
    void lfm_pgat_update_flow_expiry_timeout(Flow* flow, double ts);

    /* Debug for end of run, for printing contents of expire lists */
    void lfm_print_active_flows();

    /* Returns number of active flows, fork of print active flows*/
    int lfm_num_active_flows();

	/* Prints contents of expirelists */
    void print_expirelist_contents(ExpireList e, const char* name);
    void print_ordered_expirelist_contents(OrderedExpireList e, const char* name);

    /* Finds and returns the next available flow that has expired.
     *
     * Parameters:
     *      ts - the current timestamp
     *      force - if true, the next flow in the LRU will be forcibly expired,
     *              regardless of whether it was due to expire or not.
     *
     * Returns:
     *      a flow that has expired, or NULL if there are no expired flows
     *      available in any of the LRUs
     *
     * NOTE: you MUST call delete() yourself on the flow that is returned by this
     * function once you are finished with it.
     * It is also your responsibility to free any memory that is stored in the
     * "extension" pointer before deleting the flow.
     *
     */
    Flow *lfm_expire_next_flow ( double ts, bool force );

	/* Update ordering for flow in an ordered list */
	void lfm_update_flow_expiry(OrderedExpireList *ord_exp_list,Flow *flow);

	
	//testing
	Flow *get_next_expired_ordered ( OrderedExpireList *expire, double ts, bool force );

	
    /* Debug for flow expiry */
    void expiredebug(Flow* exp_flow, double ts);

    /* Search the active flows map for a flow matching the given 5-tuple
     *
     * Parameters:
     *      ip_a - the IP address of the first endpoint
     *      ip_b - the IP address of the second endpoint
     *      port_a - the port number used by the first endpoint
     *      port_b - the port number used by the second endpoint
     *      proto - the transport protocol
     *
     * Returns:
     *  a pointer to the flow matching the provided 5-tuple, or NULL if no
     *  matching flow exists.
     */
    Flow *lfm_find_managed_flow ( uint32_t ip_a, uint32_t ip_b, uint16_t port_a, uint16_t port_b, uint8_t proto );

    /* Sets a libflowmanager configuration option.
     *
     * Note that config options should be set BEFORE any packets are passed in
     * to other libflowmanager functions.
     *
     * Parameters:
     *      opt - the config option that is being changed
     *      value - the value to set the option to
     *
     * Returns:
     *      1 if the option is set successfully, 0 otherwise
     */
    int lfm_set_config_option ( lfm_config_t opt, void *value );

    Flow *icmp_find_original_flow ( libtrace_icmp_t *icmp_hdr, uint32_t rem );
    void icmp_error ( libtrace_icmp_t *icmp_hdr, uint32_t rem );
    void update_udp_state ( Flow *f, uint8_t dir );


    /*fast hash for tuples*/
    uint32_t makehash( FlowId f );
	//hash that does the actual work.
	uint32_t SuperFastHash (const char * data, int len);

	
	
    /*Print to file output for flows. Called at end of program to dump state.
	Printed in csv format. 
	*/
    void printflow(Flow* f, const char* n, const char* status);

    /*Print to screen debugged output for flows. Called at end of program to dump state.
	 Pretty printed, not csv. 
	 */
    void printdebugflow(Flow* f,  const char* status);
	
	


    /* Struct containing values for all the configuration options */
    struct lfm_config_opts {

        /* If true, ignore all packets that contain RFC1918 private addresses */
        bool ignore_rfc1918;

        /* If true, do not immediately expire flows after seeing FIN ACKs in
        * both directions - wait for a short time first */
        bool tcp_timewait;

        /* If true, UDP sessions for which there has been only one outbound
        * packet will be expired much more quickly */
        bool short_udp;

        /* If true, the VLAN Id will be used to form the flow key */
        bool key_vlan;

        bool ignore_icmp_errors;

        /* IPv6 Only */
        bool disable_ipv4;

        /* IPv4 Only */
        bool disable_ipv6;

        lfm_timeout_t timeout_type;
		
		/* If we're working with shanes old code and comparing between the two */
		bool legacy;
        bool oldfn;
		
		/* If true, only create flows that start with tcp. False by default.*/
        bool rfc_strict_tcp;

    };

    /* The current set of config options */
    extern lfm_config_opts config;

    /* Set up defalt values for flow manager */
    void lfm_init();

    /**********************************/
    /* Expiry lists
    *
    * Each of the following lists is acting as a LRU. Because each LRU only
    * contains flows where the expiry condition is the same, we can easily
    * expire flows by popping off one end of the list until we reach a flow that
    * is not due to expire. Similarly, we can always insert at the other end and
    * remain certain that the list is maintained in order of expiry.
    *
    * It does mean that as more expiry rules are added, a new LRU has to also be
    * added to deal with it :(
    */

    /* LRU for unestablished TCP flows */
    extern ExpireList expire_tcp_syn;

    /* LRU for established TCP flows */
    extern  ExpireList expire_tcp_estab;

    /* LRU for UDP flows - a bit of a misnomer, all non-TCP flows end up in here */
    extern ExpireList expire_udp;

    /* LRU for short-lived UDP flows (only used if the short_udp config option is set) */
    extern ExpireList expire_udpshort;

    /* LRU for all flows that met an instant expiry condition, e.g. TCP RST */
    extern ExpireList expired_flows;
    extern OrderedExpireList expired_ordered_flows;
	
	/* A generic ordered LRU */
	extern OrderedExpireList ordered_flows;

	
    /* LRU for claffy timeout */
    extern ExpireList expire_claffy;

    /* LRU for netramet tcp flows */
    extern ExpireList expire_netramet_tcp;
    extern OrderedExpireList expire_netramet_ordered_tcp;

    /* LRU for netramet udp flows */
    extern ExpireList expire_netramet_udp;
    extern OrderedExpireList expire_netramet_ordered_udp;

	/* Printing simple ip and port for client or server (c == s||c) */
	const char * make_ipport(FlowId f, char c);

	
	/*Unit tests */
	void tests();


    /****************************************/

    /* Map containing all flows that are present in one of the LRUs */
    extern FlowMap active_flows;
    extern OrderedFlowMap ordered_active_flows;	

    /* Each flow has a unique ID number - it is set to the value of this variable
    * when created and next_conn_id is incremented */
    extern int next_conn_id;

    /* Each stream has a unique ID number - it is set to the value of this variable
    * when created and next_conn_id is incremented */
    // static int next_stream_conn_id = 0;

    /* global start time */
    extern double firstpacket_time;
    /*** Claffy Fixed Timeout Globals ***/


    /* Timeout for claffy fixed timeout. Default is 64, but can be overridden. Init in claffy.cc */
    extern double claffy_timeout;

    /*** Netramet Globals ***/
    extern int last_check_time;
    extern int netramet_multipler;
    extern double stream_min_time;
    extern double stream_short_time;
    extern int netramet_short_packetcount;

    /*** MBET Globals ***/
    /* Init in example.cc, default is 3 */
    extern int mbet_config_num;

    /*** DOTS Globals ***/
    extern double dots_lastscantime;
    extern int dots_flowcounter;
    extern int dots_pkctcounter;
    extern double dots_Ts;
    extern bool dots_abnormal;
    extern int dots_Z;

    extern double flowcounter;


#ifdef __cplusplus
}
#endif

#endif

