#include <libflowmanager.h>

/**************************************
 * MBET                               *
 **************************************/

/* Initalise arrays */
int mbet_To[6] = {4,4,4,8,8,8};
int mbet_S[6] = {5,5,5,4,4,4};
int mbet_P[6][7] = {
	 {2,3,6,9,12,15},	//cfg-1
	 {2,3,4,6,8,10},	//cfg-2
	 {2,3,4,5,6,7},		//cfg-3
	 {3,6,9,12,15},		//cfg-4
	 {2,4,6,8,10},		//cfg-5
	 {3,4,5,6,7}		//cfg-6
  };
int mbet_T[6][7] = {
	 {128,64,32,16,8,4},	//cfg-1
	 {128,64,32,16,8,4},	//cfg-2
	 {128,64,32,16,8,4},	//cfg-3
	 {128,64,32,16,8},		//cfg-4
	 {128,64,32,16,8},		//cfg-5
	 {128,64,32,16,8}		//cfg-6
  };
  



  

/*mbet timeout*/
/* Updates the timeout for a Flow.
 *
 * each flow has a dual pointer to each of mbet_P and mbet_S, 
 * packets are timed out if they're 
 *
 * Parameters:
 * 	flow - the flow that is to be updated
 * 	ts - the timestamp of the last packet observed for the flow
 */
void lfm_mbet_update_flow_expiry_timeout(Flow *flow, double ts) {
    bool p = packetmatch_debug;
    ExpireList *exp_list = NULL;
	OrderedExpireList *ord_exp_list = NULL;
	ord_exp_list = &ordered_flows;

    cout << fixed;

    int packet_count = flow->packets_in + flow->packets_out;
	
	flow->last_time = ts;

    //new flow
    if (packet_count == 1) {
		
        //flow->expire_time = mbet_T[mbet_config_num][0] + ts;
        flow->Pptr = 1;
        flow->Tptr = 0;
    }
    else if (packet_count == mbet_P[mbet_config_num][flow->Pptr]) {
        if(p) cout << "Hit pointer update. packet_count == mbet_P[mbet_config_num][flow->Pptr]: " << packet_count
             << " == " << mbet_P[mbet_config_num][flow->Pptr] << endl;

        //advance pointer to next packet checkpoint
        if (flow->Pptr != mbet_S[mbet_config_num]) { //check that we haven't gone past the end of the array.
            if(p) cout << "Moving pointer from : " << flow->Pptr;
            flow->Pptr++;
            if(p) cout << " to " << flow->Pptr << endl;
        }

		
		
		if(p) cout << "ts - flow->start_time: " << ts - flow->start_time << " < mbet_T[mbet_config_num][flow->Tptr]: " << mbet_T[mbet_config_num][flow->Tptr] << endl;


        //we only drop timeout counting from the beginning of the flow, not from last packet position.
        //as timer drops, we're expecting higher and higher packet rates, or lower and lower inter packet distances.
        //this is in some senses an alternate netramet method
        if (ts - flow->start_time < mbet_T[mbet_config_num][flow->Tptr] ) {
            //advance timing pointer along.
            if (flow->Tptr != mbet_S[mbet_config_num]) { //check that we haven't gone past the end of the array.
                if(p) cout << "Reducing timer from " << mbet_T[mbet_config_num][flow->Tptr] << " to ";
                flow->Tptr++;
                if(p) cout << mbet_T[mbet_config_num][flow->Tptr] << endl;
            }
        }
    }

    flow->expire_time = ts + mbet_T[mbet_config_num][flow->Tptr];

    if(p) cout << "Bumping  flow expiry time: now + " << mbet_T[mbet_config_num][flow->Pptr] << " sec: (ts|expire_time|expnorm): " << ts << "|" << flow->expire_time << "|" << flow->expire_time-ts << endl;


    //exp_list = &expire_udp;


    /* exp_list = &expire_udp; moves it onto this list
     * flow->expire_time is our lever here.
     */

	 lfm_update_flow_expiry(ord_exp_list,flow);


	
	
    //assert(exp_list);

    //redo this for only one expiry list (ie, delete all of the moving stuff)

    /* Remove the flow from its current position in an LRU */
    //flow->expire_list->erase(active_flows[flow->id]);

    /* Push it onto its new LRU */
    //flow->expire_list = exp_list;
    //exp_list->push_front(flow);

    /* Update the entry in the flow map */
    //active_flows[flow->id] = exp_list->begin();

}

