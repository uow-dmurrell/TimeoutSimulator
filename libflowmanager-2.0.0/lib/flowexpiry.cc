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


/* Finds the next available expired flow in an LRU.  
 * Fixed timeout based:
 *  if ( exp_flow->expire_time <= ts )
 *
 * Parameters:
 *  expire - the LRU to pull an expired flow from
 *  ts - the current timestamp
 *  force - if true, the next flow in the LRU will be forcibly expired,
 *   regardless of whether it was due to expire or not.
 *
 * Returns:
 *  the next flow to be expired from the LRU, or NULL if there are no
 *  flows available to expire.
 */
Flow *get_next_expired ( ExpireList *expire, double ts, bool force ) {
    ExpireList::iterator i;
    Flow *exp_flow;

    /* Ensure that there is something in the LRU */
    if ( expire->empty() ) {
            return NULL;
        }
    /* Check if the first flow in the LRU is due to expire */
    exp_flow = expire->back();
    if ( exp_flow->expire_time <= ts )
        exp_flow->expired = true;

    /* If flow was due to expire (or the force expiry flag is set),
     * remove it from the LRU and flow map and return it to the caller */
    if ( force || exp_flow->expired ) {
            expire->pop_back();
            active_flows.erase ( exp_flow->id );
			flowcounter--;
            return exp_flow;
        }
    /* Otherwise, no flows available for expiry in this LRU */
    return NULL;
}

Flow *get_next_expired_ordered ( OrderedExpireList *expire, double ts, bool force ) {

  //start of what the hell is in here...
  if(debug_activeflow) {
	OrderedExpireList::iterator it;

	if(debug_activeflow) cout <<"Size of expire list: "<< expire->size() << "\n";

	if(debug_activeflow) cout <<"Start of ordered expiry list:\n";

	double test = 0;
	
    for (it = expire->begin(); it != expire->end(); it++ ) {
        Flow *f;
        f = *it;

		if (test == 0) {
            //make the top of the list the first one.
            test = f->expire_time;
        }
        if (f->expire_time - test < 0) {
            cout << "Error - unordered list - value negative:" << f->expire_time - test << "already exired? "<< f->expired <<endl;
			printdebugflow(f,"flow that should be expired");
			cout << "wtf\n";
        }
        else {
           // cout <<"\t\t\t\t"<<  f->expire_time - test << endl;
        }

		char *src;
		src = (char*)malloc(INET_ADDRSTRLEN);
		f->id.get_client_ip_str(src);

		char *dst;
		dst = (char*)malloc(INET_ADDRSTRLEN);
		f->id.get_server_ip_str(dst);

        cout << "Debug flow expiry time: " << f->expire_time - ts << "(" << f->expire_time - test 
			 << ") [" << makehash(f->id) << "]" << "(" 
			 << src << ":"
			<< f->id.get_server_port() << " -> " 
			<< dst << ":"											  
			<< f->id.get_client_port() << ")"
			<< " pr:" << (int)f->id.get_protocol() << " " 							  
			<< "{"<< f->id.get_id_num() << "}"
			 << endl; 
        test = f->expire_time;
    }

	cout <<"End of ordered expiry list.\n";
  }
	//start of normal...
	OrderedExpireList::iterator i;
    Flow *exp_flow;

    /* Ensure that there is something in the LRU */
    if ( expire->empty() ) {
            return NULL;
        }
    /* Check if the first flow in the LRU is due to expire */
    //exp_flow = expire->back();
    i = expire->begin();
	exp_flow = *i;

    if ( exp_flow->expire_time <= ts )
        exp_flow->expired = true;

    /* If flow was due to expire (or the force expiry flag is set),
     * remove it from the LRU and flow map and return it to the caller */
    if ( force || exp_flow->expired ) {
            expire->erase(i);
            ordered_active_flows.erase ( exp_flow->id );
			flowcounter--;
            return exp_flow;
        }

    /* Otherwise, no flows available for expiry in this LRU */
    return NULL;

}

void lfm_print_active_flows() {
	
    if(debug_activeflow) cout << "\nPrinting active flows:\n";

	/* LRU for unestablished TCP flows */
	if(debug_activeflow) cout << "tcp est: " << expire_tcp_syn.size() << endl;
	print_expirelist_contents(expire_tcp_syn,"un-established TCP");

	/* LRU for established TCP flows */
    if(debug_activeflow) cout << "tcp est: " << expire_tcp_estab.size() << endl;
	print_expirelist_contents(expire_tcp_estab,"established TCP");

	/* LRU for UDP flows - a bit of a misnomer, all non-TCP flows end up in here */
    expire_udp.sort();
    if(debug_activeflow) cout << "udp size:" << expire_udp.size() << endl;
	print_expirelist_contents(expire_udp,"UDP flows");

	/* LRU for short-lived UDP flows (only used if the short_udp config option is set) */
    if(debug_activeflow) cout << "udpshort:" << expire_udpshort.size() << endl;
	print_expirelist_contents(expire_udpshort,"short-lived UDP");

	/* LRU for all flows that met an instant expiry condition, e.g. TCP RST */
    //ExpireList expired_flows;
    if(debug_activeflow) cout << "flows:   " << expired_flows.size() << endl;
	print_expirelist_contents(expired_flows,"Instant expiry");

    /* LRU for claffy timeout */
    if(debug_activeflow) cout << "claffy:  " << expire_claffy.size() << endl;
	print_expirelist_contents(expire_claffy,"Expire Claffy");

	/* LRU for netramet_tcp timeout */
    if(debug_activeflow) cout << "netramet tcp:  " << expire_netramet_tcp.size() << endl;
	print_expirelist_contents(expire_netramet_tcp,"Expire Netramet TCP");

	/* LRU for netramet_udp timeout */
    if(debug_activeflow) cout << "netramet udp:  " << expire_netramet_udp.size() << endl;
	print_expirelist_contents(expire_netramet_udp,"Expire Netramet UDP");

	
	//-----------Ordered printing-----------------------//
	/* LRU for netramet_tcp timeout */
    if(debug_activeflow) cout << "netramet ordered tcp:  " << expire_netramet_ordered_tcp.size() << endl;
	print_ordered_expirelist_contents(expire_netramet_ordered_tcp,"Expire Netramet Ordered TCP");

	/* LRU for netramet_udp timeout */
    if(debug_activeflow) cout << "netramet ordered udp:  " << expire_netramet_ordered_udp.size() << endl;
	print_ordered_expirelist_contents(expire_netramet_ordered_udp,"Expire Netramet Ordered UDP");
	
	/* LRU for all flows that met an instant expiry condition, e.g. TCP RST */
    if(debug_activeflow) cout << "expired_ordered_flows:   " << expired_ordered_flows.size() << endl;
	print_ordered_expirelist_contents(expired_ordered_flows,"Instant expired_ordered_flows expiry");

	
	/* LRU for all flows that met an instant expiry condition, e.g. TCP RST */
    if(debug_activeflow) cout << "ordered_flows:   " << ordered_flows.size() << endl;
	print_ordered_expirelist_contents(ordered_flows,"Instant ordered_flows expiry");
}

/*returns the sum of all active flows*/
int lfm_num_active_flows() {
	int rval = 0;
	rval += expire_tcp_estab.size();
	rval += expire_udp.size();
	rval += expire_udpshort.size();
	rval += expired_flows.size();
	rval += expired_ordered_flows.size();
	rval += ordered_flows.size();
	rval += expire_claffy.size();
	rval += expire_netramet_tcp.size();
	rval += expire_netramet_ordered_tcp.size();
	rval += expire_netramet_udp.size();
	rval += expire_netramet_ordered_udp.size();
	return rval;
}

/*Returns a pretty ip:port address*/
const char * make_ipport(FlowId f, char c){
// 	std::stringstream string;
// 	string.clear();
	
	char *rv;
	
	rv = (char*)malloc((INET_ADDRSTRLEN+6));
	memset(rv,'\0',INET_ADDRSTRLEN+6);
	
	char buffer[5] = {'\0'};

	if(c == 'c'){
	  uint32_t clientport = f.get_client_port();
	  char *clientaddress;
	  clientaddress = (char*)malloc(INET_ADDRSTRLEN);
	  memset(clientaddress,'\0',INET_ADDRSTRLEN);
	  f.get_client_ip_str(clientaddress);
	  
	  strcat(rv,clientaddress);
	  strcat(rv,":");
	  sprintf(buffer, "%d", clientport);
	  strcat(rv,buffer);

	  //tests
	  assert(clientaddress);
	  if(clientport >= 0); 
	  else{ assert(clientport); }

	}
	else{
	  uint32_t serverport = f.get_server_port();
	  char *serveraddress;
	  serveraddress = (char*)malloc(INET_ADDRSTRLEN);
	  memset(serveraddress,'\0',INET_ADDRSTRLEN);
	  f.get_server_ip_str(serveraddress);
	  
 	  strcat(rv,serveraddress);
	  strcat(rv,":");
	  sprintf(buffer, "%d", serverport);
	  strcat(rv,buffer);

	  //tests
	  assert(serveraddress);
	  if(serverport >= 0); 
	  else{ assert(serverport); }
	}

  

	const char *testval = strstr(rv,":");
	if(testval == NULL){ assert(0); }
	
	return rv;
}

/*Hashes the 5 tuple, useful for post processing*/
uint32_t makehash(FlowId f){
  		std::stringstream s;

		s << f.get_client_ip()
		  << f.get_client_port()
		  << f.get_server_ip()
		  << f.get_server_port();
		
		const char *c = s.str().c_str();
		int ci = s.str().length();

		return SuperFastHash(c, ci);
}

/* Prints details of flow to a file. Takes a FlowId, type of expire list its in and a status*/
void printflow(Flow *f, const char* n, const char* status) {
	FlowId id = f->id;

	fprintf ( file, "%s,", n );
	fprintf ( file, "%s,", status);
    fprintf ( file, "%s,", 	make_ipport( id, 'c' ) );
    fprintf ( file, "%s,",	make_ipport( id, 's' ) );
    fprintf ( file, "%d,", id.get_protocol() );
    fprintf ( file, "%u,", makehash(id) );
	fprintf ( file, "%.0f,%.0f,", f->packets_in, f->packets_out );
	fprintf ( file, "%.0f,%.0f,", f->bytes_in, f->bytes_out );	
	fprintf ( file, "%f,%f,%f,", f->start_time-firstpacket_time,f->last_time-firstpacket_time,f->expire_time-firstpacket_time );
	fprintf ( file, "%f,%f,%f,", f->start_time,f->last_time,f->expire_time);
	fprintf ( file, "%u", f->tcp_lengthened);
	fprintf ( file, "\n" );
}

/* Prints details of flow to a file. Takes a FlowId, type of expire list its in and a status*/
void printdebugflow(Flow *f, const char* status) {
	FlowId id = f->id;
	
	printf ("Debug output of this flow:\n");
	printf ( "status: %s\n", status);
    printf ( "make_ipport( id, 'client' ): %s\n", make_ipport( id, 'c' ) );
    printf ( "make_ipport( id, 'server' ): %s\n", make_ipport( id, 's' ) );
    printf ( "id.get_protocol(): %d\n", id.get_protocol() );
    printf ( "makehash(id):      %u\n", makehash(id) );
	printf ( "f->packets_in:     %.0f\n", f->packets_out);
	printf ( "f->packets_out:    %.0f\n", f->packets_in);
	printf ( "f->bytes_in:       %.0f\n", f->bytes_in);
	printf ( "f->bytes_out:      %.0f\n", f->bytes_out );	
	printf ( "f->start_time-firstpacket_time:  %f\n",  f->start_time  - firstpacket_time);
	printf ( "f->last_time-firstpacket_time:   %f\n",  f->last_time   - firstpacket_time);
	printf ( "f->expire_time-firstpacket_time: %f\n",  f->expire_time - firstpacket_time );
	printf ( "f->start_time:     %f\n",   f->start_time);
	printf ( "f->last_time:      %f\n",   f->last_time);
	printf ( "f->expire_time:    %f\n",   f->expire_time);
	printf ( "f->dir[0]->seq:    %u\n",   f->dir_info[0].seq);
	printf ( "f->dir[1]->seq:    %u\n",   f->dir_info[1].seq);	
	printf ( "Joined TCP sescnt: %u",     f->tcp_lengthened);
	printf ( "\n" );
}

/* Debug output for flows that are to be expired. */
void expiredebug(Flow *exp_flow, double ts){

  double diff = exp_flow->expire_time - exp_flow->last_time;
  double age = exp_flow->last_time - exp_flow->start_time;
  //cout << fixed;
  cout << "expiring flow number: " << ( int ) exp_flow->id.get_id_num() << " ";
  //    cout << "\t age: " << age;
  cout << "\t timeout time: " << diff;
  //cout << dec;
  cout << "\t pkt in/out: " << ( int ) exp_flow->packets_in << "|" << ( int ) exp_flow->packets_out;
  //    cout << fixed;
  cout << "\t start/end/last: " << exp_flow->start_time - exp_flow->start_time << "|" << exp_flow->last_time - 
     exp_flow->expire_time << "\tnow time is: ts/diff: " << ts << "/"<< diff << endl;

  
}

void print_expirelist_contents(ExpireList e, const char *name){
	if(e.size() == 0) return;
	
  	for ( std::list<Flow*>::iterator i = e.begin(); i != e.end(); i++ ) {
		Flow *f = (*i);
		
		printflow(f,name,"Active");
	}
}

void print_ordered_expirelist_contents(OrderedExpireList e, const char *name){
	if(e.size() == 0) return;
	
  	for ( std::multiset<Flow*>::iterator i = e.begin(); i != e.end(); i++ ) {
		Flow *f = (*i);
		
		printflow(f,name,"Active");
	}
}

/* Finds and returns the next available flow that has expired.
 *
 * This is essentially the API-exported version of get_next_expired()
 *
 * Since we maintain multiple LRUS, they all need to be checked for
 * expirable flows before we can consider returning NULL.
 *
 * Parameters:
 *  ts - the current timestamp
 *  force - if true, the next flow in the LRU will be forcibly expired,
 *   regardless of whether it was due to expire or not.
 *
 * Returns:
 *  a flow that has expired, or NULL if there are no expired flows
 *  available in any of the LRUs
 */
Flow *lfm_expire_next_flow ( double ts, bool force ) {
    Flow *exp_flow;
	
    /* Check each of the LRUs in turn */
    if (
	  config.timeout_type == NETRAMET || 
	  config.timeout_type == NETRAMET_SHORT || 
	  config.timeout_type == PGAT ||
	  config.timeout_type == MBET ||
	  config.timeout_type == DOTS
	) {
		if ( flowexpire_debug ) cout << "Walking ordered_flows\n";
        exp_flow = get_next_expired_ordered ( &ordered_flows, ts, force );

        if ( exp_flow != NULL ) {
            if ( flowexpire_debug ) expiredebug(exp_flow,ts);
            return exp_flow;
        }

        if ( flowexpire_debug ) cout << "Walking expire now list\n";
        exp_flow = get_next_expired_ordered ( &expired_ordered_flows, ts, force );

        if ( exp_flow != NULL ) {
            if ( flowexpire_debug ) expiredebug(exp_flow,ts);
            return exp_flow;
        }

        if ( flowexpire_debug ) cout << "Walking netramet ordered TCP\n";
        exp_flow = get_next_expired_ordered ( &expire_netramet_ordered_tcp, ts, force );
        if ( exp_flow != NULL ) {
            if ( flowexpire_debug ) expiredebug(exp_flow,ts);
            return exp_flow;
        }

        if ( flowexpire_debug ) cout << "Walking netramet ordered UDP\n";
        exp_flow = get_next_expired_ordered ( &expire_netramet_ordered_udp, ts, force );

        if ( exp_flow != NULL ) {
            if ( flowexpire_debug ) expiredebug(exp_flow,ts);
            return exp_flow;
        }
    }
	else{
	  exp_flow = get_next_expired ( &expire_tcp_syn, ts, force );
	  if ( exp_flow != NULL ) {
		  if ( flowexpire_debug ) expiredebug(exp_flow,ts);
		  return exp_flow;
	  }

	  exp_flow = get_next_expired ( &expire_tcp_estab, ts, force );
	  if ( exp_flow != NULL ) {
		  if ( flowexpire_debug ) expiredebug(exp_flow,ts);
		  return exp_flow;
	  }

	  exp_flow = get_next_expired ( &expire_claffy, ts, force );
	  if ( exp_flow != NULL ) {
		  if ( flowexpire_debug ) expiredebug(exp_flow,ts);
		  return exp_flow;
	  }

	  exp_flow = get_next_expired ( &expire_udp, ts, force );
	  if ( exp_flow != NULL ) {
		  if ( flowexpire_debug ) expiredebug(exp_flow,ts);
		  return exp_flow;
	  }

	  exp_flow = get_next_expired ( &expire_udpshort, ts, force );
	  if ( exp_flow != NULL ) {
		  if ( flowexpire_debug ) expiredebug(exp_flow,ts);
		  return exp_flow;
	  }
	}

    return get_next_expired ( &expired_flows, ts, force );
}

/* Updates ordered flow expiry */
void lfm_update_flow_expiry(OrderedExpireList *ord_exp_list,Flow *flow) { 
    assert(ord_exp_list);

	//delete from the old list
	ord_exp_list->erase(ordered_active_flows[flow->id]);
	
	/*add a reference to the current list to the flow */
	flow->ord_expire_list = ord_exp_list;

	/* Update the entry in the flow map with the iterator from the insert*/
	ordered_active_flows[flow->id] = ord_exp_list->insert(flow);
}
