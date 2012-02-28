#include <libflowmanager.h>

/*** Study of Dynamic Timeout Strategy based on Flow Rate Metrics in High-Speed Networks (DOTS)
 * ZHOU Ming-zhong, GONG Jian, DING Wei, 2006
 */


double dots_lastscantime = 0;
int dots_flowcounter = 0;

/* Initalise arrays */
int dmbet_To = 4;
int dmbet_S = 5;
int dmbet_P[5] = {21,18,15,12,9};	//dots mbet cfg
int dmbet_T[5] = {64,32,16,8,4};

/* Static values from paper. Commented lines are calculated in lfm_dots_update_flow_expiry_timeout
 * Method is from Section A. Values are from Section V */
double dots_N	= 5;  //packets
double dots_Ts	= 15; //seconds
int dots_TL = 64; //seconds
int dots_Z  = 16; // is actually 1/dots_Z (zeta)
bool dots_abnormal = false;
//int dots_Td = packetnum * dots_Ts / dots_N;


/* DOTS Timeout
 *
 * Updates the timeout for a Flow.
 *
 *
 * Parameters:
 * 	flow - the flow that is to be updated
 * 	ts - the timestamp of the last packet observed for the flow
 */
void lfm_dots_update_flow_expiry_timeout(Flow *flow, double ts) {
//     ExpireList *exp_list = NULL;
	
	OrderedExpireList *ord_exp_list = NULL;
	ord_exp_list = &ordered_flows;

    cout << fixed;

    //if (timeoutbump_debug) cout << " ("<<  <<") " << endl;

    //setting previous_packet_time
    if (flow->last_time == 0) {
        flow->last_time = ts;
    }

    if (timeoutbump_debug) cout << "ts:" << ts << endl;
    if (timeoutbump_debug) cout << "packets in("<< flow->packets_in <<"), out("<< flow->packets_out <<") total:("<< flow->packets_in+flow->packets_out <<")" << endl;
    if (timeoutbump_debug) {
		cout << "prev packet time("<< flow->last_time <<")n:("<< flow->last_time-flow->start_time <<") ";
		cout << "current packet time("<< ts << ")n:("<< ts-flow->start_time <<") ";
		cout << "diff("<<  ts - flow->last_time <<")" << endl;
	}
    double flow_length = flow->packets_in + flow->packets_out;



    double previous_packet_time = flow->last_time;
    if (flow->start_time == 0) flow->start_time = ts;
    flow->last_time = ts;




    int transport_tcp = 6;
    int transport_udp = 17;

    //DOTS for tcp
    if (flow->transport == transport_tcp) {

        if (timeoutbump_debug) cout << "DOTS, is TCP\n";

        double dots_Td = (flow->packets_in + flow->packets_out) * dots_Ts / dots_N;

        if (timeoutbump_debug) cout << "calcd td("<< dots_Td <<") =  (flow->packets_in ("<< flow->packets_in <<
                                        ") + flow->packets_out (" << flow->packets_out <<
                                        ") ) * dots_Ts ("<< dots_Ts <<
                                        ")/ dots_N ("<< dots_N <<");\n";

        double flow_duration = flow->last_time - flow->start_time;

        if (timeoutbump_debug) cout << "flow_duration:" << flow_duration << " (seconds) " << endl;
        if (timeoutbump_debug) cout << "flow_length:" << flow_length << " (packets) " << endl;

        /*** Section IV, part A, point 3
         * If flow has
         * 		length <= dots_N(5packets) and
         * 		duration <= dots_Td(calc above seconds) and
         * 		last packet arrived >= Ts(16 seconds),
         *  expire flow
         *
         * dots_N == short flow packet count
         * dots_Td == short flow duration
         *
         */

        if (     dots_Ts       <= ts-previous_packet_time
			  && flow_length   <= dots_N
              && flow_duration <= dots_Td
           ) {
            if (timeoutbump_debug) cout << "Matched Short Flow Expiry";
            if (timeoutbump_debug && flow_length > dots_N) cout << "\tflow_length("<< flow_length <<") !<= dots_N("<< dots_N << ")" << endl;
            if (timeoutbump_debug && flow_duration > dots_Td) cout << "\tflow_duration("<< flow_duration <<") !<= dots_Td("<< dots_Td <<") "<<endl;
            if (timeoutbump_debug && dots_Ts > ts-previous_packet_time) cout << "\tdots_Ts(" << dots_Ts << ") !<= ts-previous_packet_time(" << ts-previous_packet_time << ")" << endl;

            flow->expire_time = ts;
        }
        //otherwise, update flow with long timeout
        else {
            if (timeoutbump_debug) cout << "Did Not match Short Flow Expiry, is long timeout" << endl;

            if (timeoutbump_debug && flow_length <= dots_N) cout << "\tflow_length("<< flow_length <<") <= dots_N("<< dots_N << ")" << endl;
            if (timeoutbump_debug && flow_duration <= dots_Td) cout << "\tflow_duration("<< flow_duration <<") <= dots_Td("<< dots_Td <<") "<<endl;
            if (timeoutbump_debug && dots_Ts <= ts-previous_packet_time) cout << "\tdots_Ts(" << dots_Ts << ") <= ts-previous_packet_time(" << ts-previous_packet_time << ")" << endl;

            flow->expire_time = ts+dots_TL;
        }




        //all expiry done by udp_expiry list.

       if(timeoutbump_debug) cout << endl;


    }
    //mbet for udp
    else if (flow->transport == transport_udp) {
	  
        if (timeoutbump_debug) cout << "DOTS, is UDP flow, so mbet cfg3\n";
		
		
		//new flow
        if (flow_length == 1) {

            //flow->expire_time = mbet_T[0] + ts;
            flow->Pptr = 1;
            flow->Tptr = 0;
        }
        else if (flow_length == dmbet_P[flow->Pptr]) {
            if (timeoutbump_debug) cout << "Hit pointer update. packet_count == mbet_P[flow->Pptr]: " << flow_length << " == " << dmbet_P[flow->Pptr] << endl;

            //advance pointer to next packet checkpoint
            if (flow->Pptr != dmbet_S) { //check that we haven't gone past the end of the array.
                if (timeoutbump_debug) cout << "Moving pointer from : " << flow->Pptr;
                flow->Pptr++;
                if (timeoutbump_debug) cout << " to " << flow->Pptr << endl;
            }

            if (timeoutbump_debug) cout << "ts - flow->start_time: " << ts - flow->start_time << " < mbet_T[flow->Tptr]: " << dmbet_T[flow->Tptr] << endl;


            //we only drop timeout counting from the beginning of the flow, not from last packet position.
            //as timer drops, we're expecting higher and higher packet rates, or lower and lower inter packet distances.
            //this is in some senses an alternate netramet method
			
            if (ts - flow->start_time < dmbet_T[flow->Tptr] ) {
                //advance timing pointer along.
                if (flow->Tptr != dmbet_S) { //check that we haven't gone past the end of the array.
                    if (timeoutbump_debug) cout << "Reducing timer from " << dmbet_T[flow->Tptr] << " to ";
                    flow->Tptr++;
                    if (timeoutbump_debug) cout << dmbet_T[flow->Tptr] << endl;
                }
            }
        }

        flow->expire_time = ts + dmbet_T[flow->Tptr];

        if (timeoutbump_debug) cout << "Bumping  flow expiry time: now + " 
			  << dmbet_T[flow->Pptr] << " sec: (ts|expire_time|expnorm): " 
			  << ts << "|" << flow->expire_time << "|" << flow->expire_time-ts << endl;

    }
    else {
        //not sure what we're doing here.
        return;
    }

	lfm_update_flow_expiry(ord_exp_list,flow);



//     exp_list = &expire_udp;
// 
//     assert(exp_list);
// 
//     //redo this for only one expiry list (ie, delete all of the moving stuff)
// 
//     /* Remove the flow from its current position in an LRU */
//     flow->expire_list->erase(active_flows[flow->id]);
// 
//     /* Push it onto its new LRU */
//     flow->expire_list = exp_list;
//     exp_list->push_front(flow);
// 
//     /* Update the entry in the flow map */
//     active_flows[flow->id] = exp_list->begin();*/

}


