#include <libflowmanager.h>
#include <pgat.h>

float probability = 0.8;


/**************************************
 * PGAT                               *
 **************************************/

//sets the protocol column based on the tcp server port.
void set_protocolumn( Flow *f) {
	bool d = timeoutbump_debug;
  
	if (d) cout << "Setting protocol column for PGAT: ";
    switch (f->id.get_server_port()) {
        //http
    case 80:
        f->col = 0;
		if (d) cout << "TCP Port 80\n";
        break;

        //https 443
    case 443:
        f->col = 1;
        if (d) cout << "TCP Port 443\n";
		break;

    case 21:
    case 20:
        //ftp 21/20
        f->col = 2;
		if (d) cout << "TCP Port 20/21\n";
        break;

    case 23:
        //telnet 23
        f->col = 3;
		if (d) cout << "TCP Port 23\n";
        break;

    case 25:
        //smtp 25
        f->col = 4;
		if (d) cout << "TCP Port 25\n";
        break;

    case 110:
        //pop3 110
        f->col = 5;
		if (d) cout << "TCP Port 110\n";
        break;

    default:
        //other *
        f->col = 6;
		if (d) cout << "TCP Port other\n";
    }


}


void mbet_calc(Flow *flow, int mb_T0, int mb_S, int mb_P[6], int mb_T[6],double ts ) {
  bool d = timeoutbump_debug;

    int packet_count = flow->packets_in + flow->packets_out;

    if (d) cout << "DOTS, is UDP flow, so mbet cfg3\n";


    //new flow
    if (packet_count == 1) {

        //flow->expire_time = mbet_T[0] + ts;
        flow->Pptr = 1;
        flow->Tptr = 0;
    }
    else if (packet_count == mb_P[flow->Pptr]) {
        if (d) cout << "Hit pointer update. packet_count == mbet_P[flow->Pptr]: " << packet_count << " == " << mb_P[flow->Pptr] << endl;

        //advance pointer to next packet checkpoint
        if (flow->Pptr != mb_S) { //check that we haven't gone past the end of the array.
            if (d) cout << "Moving pointer from : " << flow->Pptr;
            flow->Pptr++;
            if (d) cout << " to " << flow->Pptr << endl;
        }

        if (d) cout << "ts - flow->start_time: " << ts - flow->start_time << " < mbet_T[flow->Tptr]: " << mb_T[flow->Tptr] << endl;


        //we only drop timeout counting from the beginning of the flow, not from last packet position.
        //as timer drops, we're expecting higher and higher packet rates, or lower and lower inter packet distances.
        //this is in some senses an alternate netramet method

        if (ts - flow->start_time < mb_T[flow->Tptr] ) {
            //advance timing pointer along.
            if (flow->Tptr != mb_S) { //check that we haven't gone past the end of the array.
                if (d) cout << "Reducing timer from " << mb_T[flow->Tptr] << " to ";
                flow->Tptr++;
                if (d) cout << mb_T[flow->Tptr] << endl;
            }
        }
    }

    flow->expire_time = ts + mb_T[flow->Tptr];

    if (d) cout << "Bumping  flow expiry time: now + "
                    << mb_T[flow->Pptr] << " sec: (ts|expire_time|expnorm): "
                    << ts << "|" << flow->expire_time << "|" << flow->expire_time-ts << endl;


}

/*PGAT timeout*/
/* Updates the timeout for a Flow.
 *
 * each flow has a dual pointer to each of mbet_P and mbet_S,
 * packets are timed out if they're
 *
 * Parameters:
 * 	flow - the flow that is to be updated
 * 	ts - the timestamp of the last packet observed for the flow
 */
void lfm_pgat_update_flow_expiry_timeout(Flow *flow, double ts ) {
	bool d = timeoutbump_debug;

	OrderedExpireList *ord_exp_list = NULL;
	ord_exp_list = &ordered_flows;

    cout << fixed;

    int packet_count = flow->packets_in + flow->packets_out;
	
	flow->last_time = ts;
  

    //not tcp
    if (flow->transport != 6 ) {
        if (d) cout << "Transport not tcp" << endl;
        //mbet, cfg2
        int mb_T0 = 4;
        int mb_S = 6;
        int mb_P[6] =  { 10,  8,  6,  4, 3, 2};
        int mb_T[6]	 = {128, 64, 32, 16, 8, 4};	//cfg-2

        mbet_calc(flow, mb_T0, mb_S, mb_P, mb_T, ts);

    }
    //must be tcp
    else {

        if (flow->col == -1) set_protocolumn(flow);
		
        int streamtime = ts - flow->start_time;

        int row = -1;
        if (packet_count ==   1) row = 0;
        if (packet_count ==  10) row = 1;
        if (packet_count == 100) row = 2;
        if (packet_count == 500) row = 3;

		if (d) cout << "Streamtime: " << streamtime << ", packetcount: " << packet_count << ", row is: " << row << endl;
		
        if (row != -1) {

            int val = -1;

            for (int x = 0; x < 7; x++) {
                float tmp = pgat_lookup[flow->col][row][x];
				
				if (d) cout << "x: " << x <<  ", tmp: " << tmp << ", probability: " << probability <<  ", val:" << val << endl;
				
                if ( probability <= tmp ) {
                    x++;
                    val = x;
                    break;
                }

            }
            
            flow->expire_time = pow(2,val);
            flow->expire_time += ts;
			
			if(d) cout << "val: " << val << " pow(2,val): " << pow(2,val) << ", expiretime now: " << flow->expire_time-flow->start_time << endl;
			
        }
        // col = QueryF(F[index][row], Q)
        // T = 2.0 · 2col, lastcol = col, ﬂag = 0


    }
    
	 lfm_update_flow_expiry(ord_exp_list,flow);



	
//     assert(ord_exp_list);
// 
//     
// 
// 	//get a reference to the flow
// 	OrderedExpireList::iterator iter = ordered_active_flows[flow->id];
//  	ord_exp_list->erase(iter);
// 
// 	/*add a reference to the current list to the flow */
// 	flow->ord_expire_list = ord_exp_list;
// 
// 	//inserts it at the correct point in the list
//  	iter = ord_exp_list->insert(flow);
// 
// 	
// 	ordered_active_flows[flow->id] = iter;

	//old stuff:
	//assert(exp_list);
    /* Remove the flow from its current position in an LRU */
    //flow->expire_list->erase(active_flows[flow->id]);

    /* Push it onto its new LRU */
    //flow->expire_list = exp_list;
    //exp_list->push_front(flow);

    /* Update the entry in the flow map */
    //active_flows[flow->id] = exp_list->begin();
	

}

