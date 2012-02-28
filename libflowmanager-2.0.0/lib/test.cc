#include "libflowmanager.h"
#include <arpa/inet.h>

OrderedExpireList *expire = &expired_ordered_flows;
double ts = 0;


//print out content of the current *expire list. 
void whatshere(){
    OrderedExpireList::iterator it;

    cout <<"\nSize of expire list: "<< expire->size() << "\n";

    cout <<"Start of ordered expiry list:\n";

    double test = 0;

    for (it = expire->begin(); it != expire->end(); it++ ) {
        Flow *f;
        f = *it;


        if (test == 0) {
            //make the top of the list the first one.
            test = f->expire_time;
        }
        
		
        if (f->expire_time - test < 0) {
            cout << "\nError - unordered list - value negative: " << f->expire_time - test << " already exired? "<< f->expired <<endl;
            printdebugflow(f,"\nFlow that should be expired:");
        }
        assert(f->expire_time - test >= 0);


        cout << "\tDebug flow expiry time: " << f->expire_time - ts << "(" << f->expire_time - test << ") [" << makehash(f->id) << "]"<< endl; ;
        test = f->expire_time;



    }

    cout <<"End of ordered expiry list.\n\n";
  
}

void netramet_delete_item(){
  


    //FlowId::FlowId ( uint32_t ip_src, uint32_t ip_dst, uint16_t port_src,
    //               uint16_t port_dst, uint8_t protocol, uint16_t vlan_id,
    //               uint32_t id )


	
    whatshere();



	bool exp_flag = false;
	int ts = 6;
	
	Flow *x = lfm_expire_next_flow ( ts, exp_flag );
	//Flow *x = get_next_expired_ordered(expire,ts,false);
	
	whatshere();	
		
	cout << "TEST assert(x != NULL): ";
	assert(x != NULL);
	cout << "PASSED\n";
	
	cout << "hash: [" << makehash(x->id) << "] ts: " << ts << "\n";
	printdebugflow(x,"\nFlow that is to be deleted :( ");
	
	delete ( x );
	whatshere();	

	
	cout << "TEST assert(expire->size() == 3): ";
	assert(expire->size() == 3);
	cout << "PASSED\n";
}

void netramet_update_item(){
  	whatshere();	

  //get last item in the list. 
  Flow *flow3 = *expire->begin();
  
  /** update expire time of flow */
  flow3->expire_time = 8;

  OrderedExpireList::iterator iter3 = ordered_active_flows[flow3->id];
  
// 	//need to re-insert flow 3, without ordering everything again. 

 	expire->erase(iter3);
 	//expire->erase(flow3);
 	
 	//flow3->expire_time = 2;
 	expire->insert(flow3);
	
	whatshere();	

}

void fill_ordered4(){
  

	/** flow 1  */
    //10.0.0.1, 10.0.0.2, 10000, 25, tcp, no vlan, id = 1;
    FlowId pkt_id1 = FlowId(184549377,184549378, 10000, 25, 6, 0, 1);
    Flow *flow1 = new Flow(pkt_id1);

    flow1->expire_time = 4;


    /* Remove the flow from its current position in an LRU */
    //flow1->ord_expire_list->erase(ordered_active_flows[flow1->id]);

    /* Push it onto its new LRU */
    flow1->ord_expire_list = expire;
    OrderedExpireList::iterator iter1 = expire->insert(flow1);


    /* Update the entry in the flow map */
    //active_flows[flow->id] = exp_list->begin();
    //ordered_active_flows[flow->id] = expire->find();
    ordered_active_flows[flow1->id] = iter1;


	/** flow 2  */
	//10.0.0.3, 10.0.0.4, 10000, 25, tcp, no vlan, id = 1;
    FlowId pkt_id2 = FlowId(184549379,184549310, 10000, 25, 6, 0, 1);
    Flow *flow2 = new Flow(pkt_id2);

    flow2->expire_time = 5;


    /* Remove the flow from its current position in an LRU */
    //flow->ord_expire_list->erase(ordered_active_flows[flow->id]);

    /* Push it onto its new LRU */
    flow2->ord_expire_list = expire;
    OrderedExpireList::iterator iter2 = expire->insert(flow2);


    /* Update the entry in the flow map */
    //active_flows[flow->id] = exp_list->begin();
    //ordered_active_flows[flow->id] = expire->find();
    ordered_active_flows[flow2->id] = iter2;
	
	
	/** flow 3 */	
	//10.0.0.5, 10.0.0.6, 10000, 25, tcp, no vlan, id = 1;
    FlowId pkt_id3 = FlowId(184549380,184549311, 10000, 25, 6, 0, 1);
    Flow *flow3 = new Flow(pkt_id3);

    flow3->expire_time = 6;


    /* Remove the flow from its current position in an LRU */
    //flow->ord_expire_list->erase(ordered_active_flows[flow->id]);

    /* Push it onto its new LRU */
    flow3->ord_expire_list = expire;
    OrderedExpireList::iterator iter3 = expire->insert(flow3);


    /* Update the entry in the flow map */
    //active_flows[flow->id] = exp_list->begin();
    //ordered_active_flows[flow->id] = expire->find();
    ordered_active_flows[flow3->id] = iter3;
	
	
	/** flow 4  */	
	//10.0.0.7, 10.0.0.8, 10000, 25, tcp, no vlan, id = 1;
    FlowId pkt_id4 = FlowId(184549381,184549313, 10000, 25, 6, 0, 1);
    Flow *flow4 = new Flow(pkt_id4);

    flow4->expire_time = 7;


    /* Remove the flow from its current position in an LRU */
    //flow->ord_expire_list->erase(ordered_active_flows[flow->id]);

    /* Push it onto its new LRU */
    flow4->ord_expire_list = expire;
    OrderedExpireList::iterator iter4 = expire->insert(flow4);


    /* Update the entry in the flow map */
    //active_flows[flow->id] = exp_list->begin();
    //ordered_active_flows[flow->id] = expire->find();
    ordered_active_flows[flow4->id] = iter4;
	
	cout << "TEST assert(expire->size() == 4): ";
	assert(expire->size() == 4);
	cout << "PASSED\n";
	
	whatshere();
	
}

void stringoutput(){
  //explore getting string output from get_client_ip_str()
  
  //get last item in the list. 
  Flow *flowx = *expire->begin();
  OrderedExpireList::iterator it = expire->begin();
  struct in_addr inp;
  inp.s_addr = flowx->id.get_client_ip();
  printf("%s\n",inet_ntoa ( inp ));
  
  
  //char str[INET_ADDRSTRLEN];
  char *str;
  str = (char*)malloc(INET_ADDRSTRLEN);
  
  
  struct in_addr inp4;
  inp4.s_addr = flowx->id.get_client_ip();

  inet_ntop ( AF_INET, &inp4, str, INET_ADDRSTRLEN );

  printf(">%s<\n",str);
  
  it++;
  flowx = *it;
  
  memset(str,'\0',INET_ADDRSTRLEN);
  flowx->id.get_client_ip_str(str);
  printf(">%s<\n",str);
  

}

void tests() {
  debug_activeflow = true;
  
  
  cout << "\nfill_ordered4();\n";
  fill_ordered4();
  
  cout << "\nnetramet_update_item();\n";
  config.timeout_type = NETRAMET;
  netramet_update_item();
  cout << "\nnetramet_delete_item();\n";
  netramet_delete_item();

  expire->clear();
  cout << "TEST assert(expire->size() == 0): "; 
  assert(expire->size() == 0);
  cout << "PASSED\n";
  
  cout << "\nfill_ordered4();\n";
  fill_ordered4();

  stringoutput();
  
  
  
}