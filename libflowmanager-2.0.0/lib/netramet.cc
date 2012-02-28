#include <libflowmanager.h>

int netramet_flow_count_tcp = 0;
int netramet_flow_count_udp = 0;

/**************************************
 * Netramet Timeout, standard version *
 **************************************/


bool compare_time (Flow *first, Flow *second){
  if (first->expire_time < second->expire_time ) return true;
  else return false;
}

/*netramet timeout*/
/* Updates the timeout for a Flow.
 *
 * For netramet, this means recalculating the following: 
 * 	packet interval
 * 	stream length
 *	streamtimeout
 *
 * Parameters:
 * 	flow - the flow that is to be updated
 * 	ts - the timestamp of the last packet observed for the flow
 */
void lfm_netramet_update_flow_expiry_timeout(Flow *flow, double ts) {
	OrderedExpireList *ord_exp_list = NULL;
	cout << fixed;
	
	if(firstpacket_time == 0){
		firstpacket_time = ts;
	}
	
// 	//probably should be global.
// 	int netramet_multipler = 20;
// 	double stream_min_time = 5;  //seconds
// 	double stream_short_time = 2;  //seconds	
// 	int netramet_short_packetcount = 6; //packets
	
	
	double flowlength = flow->last_time - flow->start_time;

	double packet_count		= flow->packets_in+flow->packets_out;
	double packet_interval 	= flowlength/packet_count;
	double streamtimeout  	= packet_interval * netramet_multipler;
	double streamlasttime 	= ts;
	double inacttime		= streamlasttime - flow->last_time;

	
	//chose an expiry list
	if(flow->transport== 6) ord_exp_list = &expire_netramet_ordered_tcp;
	else ord_exp_list = &expire_netramet_ordered_udp;

	if(config.timeout_type == NETRAMET){
	  //calculate an inital timeout
	  if( flow->expire_time == 0 ){
		  flow->expire_time = flow->start_time + stream_min_time;
		  if( debug_netramet) cout << "bumping expire time to be 5 seconds after start of flow, expire time now: " << flow->expire_time << "\n";
	  }
	  //flow has started. 
	  else if (ts < flow->start_time + stream_min_time ){
		  //we're still in the minimum timeout period, do nothing
		  if( debug_netramet) cout << "we're still in the minimum timeout period, do nothing\n";
	  }
	  else {
		  //calculate new timeout 
		  flow->expire_time = streamtimeout + ts;
		  if( debug_netramet) cout << "calculating new timeout time, is now: " << flow->expire_time << " normalised is: " << flow->expire_time - firstpacket_time<< "\n";
	  }
	}
	//netramet short - if packetcount < netramet_short_packetcount (default = 6), timeout is 2 seconds. 
	else if(config.timeout_type == NETRAMET_SHORT){
	  //calculate an inital timeout
	  if( packet_count < netramet_short_packetcount){
		  flow->expire_time = flow->start_time + stream_short_time;
		  if( debug_netramet) cout << "Netramet Short: bumping expire time to be " << stream_short_time << " seconds after start of flow, expire time now: " << flow->expire_time << "\n";
	  }
 	  //flow has started. 
	  else if ( packet_count >= netramet_short_packetcount && ts < flow->start_time + stream_min_time ){
		  //we're still in the minimum timeout period, do nothing
		  if( debug_netramet) cout << "flow now past minimum 2 second timeout. active state, bumping timeout period to 5 seconds\n";
	  }
	  else {
		  //calculate new timeout 
		  flow->expire_time = streamtimeout + ts;
		  if( debug_netramet) cout << "calculating new timeout time, is now: " << flow->expire_time << " normalised is: " << flow->expire_time - firstpacket_time<< "\n";
	  }
	}

	if (timeoutbump_debug){
	  printdebugflow(flow, "new flow");
	  cout << "netramet_multipler:              " << netramet_multipler << "\n";
	  cout << "stream_min_time:                 " << stream_min_time << "\n";
 	  cout << "flow->start_time:                " << flow->start_time << "\n";
	  cout << "flow->last_time:                 " << flow->last_time << "\n";
  
 	  cout << "flow->start_time(normalised):    " << flow->start_time - firstpacket_time << "\n";
	  cout << "flow->last_time(normalised):     " << flow->last_time - firstpacket_time << "\n";
	  
	  cout << "flowlength:                      " << flowlength << "\n";
	  cout << "flow->expire_time:               " << flow->expire_time << "\n";                               
	  cout << "flow->expire_time(norm):         " << flow->expire_time - firstpacket_time << "\n";                               
  	  cout << "packets in:                      " << flow->packets_in << "\n";
	  cout << "packets out:                     " << flow->packets_out << "\n";
	  cout << "packet_count:                    " << packet_count << "\n";

	  cout << "packet_interval:                 " << packet_interval << "\n";
	  cout << "streamtimeout:                   " << streamtimeout<< "\n";
	  cout << "streamlasttime:                  " << streamlasttime<< "\n";
	  cout << "streamlasttime(normalised):      " << streamlasttime  - firstpacket_time << "\n";	  
	  cout << "inacttime:                       " << inacttime << "\n";
	  cout << "\ncurrent time:                    " << ts - firstpacket_time << "\n\n";
	  
	}

	
	//handle tcp rst/fin's
    switch (flow->flow_state) {
        //close flows on rst and 2x fin.
		case FLOW_STATE_RESET:
		case FLOW_STATE_ICMPERROR:
			// We want to expire this as soon as possible
			flow->expire_time = ts;
			ord_exp_list = &expired_ordered_flows;
			if( debug_netramet) cout << "flow hit reset\n";
			break;
		case FLOW_STATE_CLOSE:
			//	/* We want to expire this as soon as possible
			flow->expire_time = ts;
			if(debug_netramet) cout << "flow hit close, expiring flow number: "<< flow->id.get_id_num() << "\n";
			ord_exp_list = &expired_ordered_flows;
			break;
    }
	
	
	//lfm_update_flow_expiry(ord_exp_list,flow);

	
    assert(ord_exp_list);
	
	
	
	//reorder the item in the list - its time has changed, so its position needs to change.
	
    
	/* Remove the flow from its current position */
    flow->ord_expire_list->erase(ordered_active_flows[flow->id]);    
    
	/*add a reference to the current list to the flow */
	flow->ord_expire_list = ord_exp_list;
	
	/* Push it into its new expiry list */
    OrderedExpireList::iterator iter = ord_exp_list->insert(flow);
	
	
    /* Update the entry in the flow map */
    ordered_active_flows[flow->id] = iter;

}


