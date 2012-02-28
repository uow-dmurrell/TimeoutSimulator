#include <libflowmanager.h>


/* Claffy timeout inital value */
double claffy_timeout = 64;


/*****************************
 * Claffy 64 Sec timeout     *
 *****************************/

/* Claffy fixed 64 second timeout
 * Updates the timeout for a Flow.
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
void lfm_fixedtimeout_update_flow_expiry_timeout(Flow *flow, double ts) {
    ExpireList *exp_list = NULL;
	
	if(config.timeout_type == CLAFFY_UNLIMITED){
	  claffy_timeout = 31536000; //one year
	}
	
	if (timeoutbump_debug) {
	  cout.precision(3);
	  cout << fixed;
	  cout << "bumping expire time (";
	  if(flow->expire_time == 0){
		cout << "0";
	  }
	  else {
		cout << flow->expire_time-flow->start_time;
	  }
	  cout << "->" << (ts+claffy_timeout)-flow->start_time << ")";
	  cout << " ts=" << ts-flow->start_time;

	  cout << " last packet was at ";
	  if(flow->last_time == 0){
		cout << "0";
	  }else{
		cout << flow->last_time-flow->start_time << endl;
	  }
	
	}
	
	
	//ts is now()
	flow->expire_time = ts + claffy_timeout;
	flow->last_time = ts;
	exp_list = &expire_claffy;
	

    assert(exp_list);
	
	//redo this for only one expiry list (ie, delete all of the moving stuff)

    /* Remove the flow from its current position in an LRU */
    flow->expire_list->erase(active_flows[flow->id]);

    /* Push it onto its new LRU */
    flow->expire_list = exp_list;
    exp_list->push_front(flow);

    /* Update the entry in the flow map */
    active_flows[flow->id] = exp_list->begin();

}



/*****************************************
 * Claffy 64 Sec timeout with TCP checks *
 *****************************************/


/*claffy fixed 64 second timeout*/
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
void lfm_fixedtimeout_tcp_update_flow_expiry_timeout(Flow *flow, double ts) {
    ExpireList *exp_list = NULL;
    if (timeoutbump_debug) {
        cout.precision(3);
        cout << fixed;
        cout << "bumping expire time (";
        if (flow->expire_time == 0) {
            cout << "0";
        }
        else {
            cout << flow->expire_time-flow->start_time;
        }
        cout << "->" << (ts+claffy_timeout)-flow->start_time << ")";
        cout << " ts=" << ts-flow->start_time;

        cout << " last packet was at ";
        if (flow->last_time == 0) {
            cout << "0";
        } else {
            cout << flow->last_time-flow->start_time << endl;
        }
    }

    flow->expire_time = ts + claffy_timeout;
    flow->last_time = ts;
    exp_list = &expire_claffy;


    switch (flow->flow_state) {
        //close flows on rst and 2x fin.
		case FLOW_STATE_RESET:
		case FLOW_STATE_ICMPERROR:
			// We want to expire this as soon as possible
			flow->expire_time = ts;
			exp_list = &expired_flows;
			if (packetmatch_debug) cout << "flow hit reset\n";
			break;
		case FLOW_STATE_CLOSE:
			//	/* We want to expire this as soon as possible
			flow->expire_time = ts;
			if (packetmatch_debug) cout << "flow hit close\n";
			exp_list = &expired_flows;
			break;
    }


    assert(exp_list);
	
	//redo this for only one expiry list (ie, delete all of the moving stuff)

    /* Remove the flow from its current position in an LRU */
    flow->expire_list->erase(active_flows[flow->id]);

    /* Push it onto its new LRU */
    flow->expire_list = exp_list;
    exp_list->push_front(flow);

    /* Update the entry in the flow map */
    active_flows[flow->id] = exp_list->begin();

}


