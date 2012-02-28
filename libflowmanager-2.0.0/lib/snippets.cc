//
// /* Finds the next available expired flow in an LRU.
// * Fixed timeout based:
// *  if ( exp_flow->expire_time <= ts )
// *
// * Parameters:
// *  expire - the LRU to pull an expired flow from
// *  ts - the current timestamp
// *  force - if true, the next flow in the LRU will be forcibly expired,
// *   regardless of whether it was due to expire or not.
// *
// * Returns:
// *  the next flow to be expired from the LRU, or NULL if there are no
// *  flows available to expire.
// */
//Flow *get_next_expired_netramet_ordered ( OrderedExpireList *expire, double ts, bool force ) {
//
//	if (debug_netramet) cout << "get_next_expired_netramet_ordered called list is: ";
//	if (expire->empty()){
//		if (debug_netramet) cout << "empty\n";
//	}
//	else{
//		if (debug_netramet) cout << "contains " << expire->size() << " flows\n";
//	}
//
//
//	Flow *exp_flow;
//
//    /* Ensure that there is something in the LRU */
//    if ( expire->empty() ) return NULL;
//
//	/* Check if the first/oldest flow in the LRU is due to expire */
//    OrderedExpireList::iterator i = expire->begin();
//	exp_flow = *i;
//
//	if ( exp_flow->expire_time <= ts ){
//		if (debug_netramet) cout << "Netramet Ordered Expiring flow: " << exp_flow->id.get_id_num() << "\n";
//		exp_flow->expired = true;
//	}
//	else{
//		if (debug_netramet) cout << "Netramet Ordered not Expiring flow: " << exp_flow->id.get_id_num() << " as exp_flow->expire_time: " <<  exp_flow->expire_time << " is less than ts: "<< ts <<
//		" (" << exp_flow->expire_time-ts << " seconds)\n";
//	}
//
//	/* If flow was due to expire (or the force expiry flag is set),
//     * remove it from the LRU and flow map and return it to the caller */
//    if ( force || exp_flow->expired ) {
//            expire->erase(i);
//            ordered_active_flows.erase ( exp_flow->id );
//            return exp_flow;
//	}
//
//    /* Otherwise, no flows available for expiry in this LRU */
//    return NULL;
//}


//Flow *get_next_expired_netramet ( ExpireList *expire, double ts, bool force ) {
//	if (debug_netramet) cout << "get_next_expired_netramet called list is: ";
//	if (expire->empty()){
//		if (debug_netramet) cout << "empty\n";
//	}
//	else{
//		if (debug_netramet) cout << "contains " << expire->size() << " flows\n";
//	}
//
//  	ExpireList::iterator i;
//    Flow *exp_flow;
//
//    /* Ensure that there is something in the LRU */
//    if ( expire->empty() ) return NULL;
//
//	/* Check if the first/oldest flow in the LRU is due to expire */
//    exp_flow = expire->back();
//
//	if ( exp_flow->expire_time <= ts ){
//		if (debug_netramet) cout << "Netramet Expiring flow: " << exp_flow->id.get_id_num() << "\n";
//		exp_flow->expired = true;
//	}
//	else{
//		if (debug_netramet) cout << "Netramet not Expiring flow: " << exp_flow->id.get_id_num() << " as exp_flow->expire_time: " <<  exp_flow->expire_time << " is less than ts: "<< ts <<
//		" (" << exp_flow->expire_time-ts << " seconds)\n";
//	}
//
//	/* If flow was due to expire (or the force expiry flag is set),
//     * remove it from the LRU and flow map and return it to the caller */
//    if ( force || exp_flow->expired ) {
//            expire->pop_back();
//            active_flows.erase ( exp_flow->id );
//            return exp_flow;
//	}
//
//    /* Otherwise, no flows available for expiry in this LRU */
//    return NULL;
//}