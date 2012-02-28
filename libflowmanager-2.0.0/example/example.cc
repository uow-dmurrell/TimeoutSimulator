/* Example program using libflowmanager to count flows in a trace file.
* Demonstrates how the libflowmanager API should be used to perform flow-based
* measurements.
*
* Author: Shane Alcock
*/

#define __STDC_FORMAT_MACROS

#include <stdio.h>
#include <assert.h>
#include <getopt.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>

#include <libtrace.h>

#include <libflowmanager.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>


using namespace std;

#define SIZE 256

FILE *file = NULL;

uint64_t flow_counter = 0;
uint64_t packet_counter = 0;
uint64_t ippacket_counter = 0;

uint64_t ip_tcp = 0;
uint64_t ip_icmp = 0;
uint64_t ip_udp = 0;
uint64_t ip_unknown = 0;

uint64_t expiredflow_counter = 0;

lfm_timeout_t timeout_type;

//Exit early runtime for testing
double runtime  = 0;
bool   runcheck = false;

//default values for netramet
int    netramet_multipler = 20;
double stream_min_time    = 5;  //seconds
double stream_short_time  = 2;  //seconds
int    mbet_config_num    = 2;
int    netramet_short_packetcount = 6; //packets

double dots_lastscantime = 0;
int    dots_flowcounter  = 0;
int    dots_pkctcounter  = 0;
double firstpacket_time  = 0;

bool debug_file = false;

char *progname;

//basic count of flows. 
double flowcounter = 0;

//high water mark for number of active flows in this program. Updated after every packet. 
int hwm = 0;

//current flow mark
int currentflowlevel = 0;

/* This data structure is used to demonstrate how to use the 'extension'
* pointer to store custom data for a flow */
typedef struct counter {
    uint64_t packets;
    uint8_t init_dir;
} CounterFlow;

/* Initialises the custom data for the given flow. Allocates memory for a
* CounterFlow structure and ensures that the extension pointer points at
* it.
*/
void init_counter_flow ( Flow *f, uint8_t dir ) {
    CounterFlow *cflow = NULL;

    cflow = ( CounterFlow * ) malloc ( sizeof ( CounterFlow ) );
    cflow->init_dir = dir;
    cflow->packets = 0;

    /* Don't forget to update our flow counter too! */
    flow_counter ++;
    f->extension = cflow;
}

/* Expires all flows that libflowmanager believes have been idle for too
* long. The exp_flag variable tells libflowmanager whether it should force
* expiry of all flows (e.g. if you have reached the end of the program and
* want the stats for all the still-active flows). Otherwise, only flows
* that have been idle for longer than their expiry timeout will be expired.
*/
void expire_counter_flows ( double ts, bool exp_flag ) {
    if ( timeout_type == DOTS) {
        if (dots_lastscantime == 0) {
            dots_lastscantime = ts;
            return;
        }
        if ((ts - dots_lastscantime) < dots_Ts ) {
            return;
        }
        dots_flowcounter = 0;
        dots_pkctcounter = 0;
    }

    Flow *expired;

    /* Loop until libflowmanager has no more expired flows available */
    while ( ( expired = lfm_expire_next_flow ( ts, exp_flag ) ) != NULL ) {
		expired->expired = true;
        //CounterFlow *cflow = ( CounterFlow * ) expired->extension;

        expiredflow_counter++;
        /* We could do something with the packet count here, e.g.
        * print it to stdout, but that would just produce lots of
        * noisy output for no real benefit */

        /* Don't forget to free our custom data structure */
//         free ( cflow );

        /* VERY IMPORTANT: delete the Flow structure itself, even
        * though we did not directly allocate the memory ourselves */


        printflow(expired,"Delete List","Expired");


        delete ( expired );
    }
}

void new_packet_debug(libtrace_ip_t *ip) {
    cout << "\nhave ip packet from ";
    printf ( "%d.%d.%d.%d", IPCOMP ( ip->ip_src.s_addr, 3 ), IPCOMP ( ip->ip_src.s_addr, 2 ), IPCOMP ( ip->ip_src.s_addr, 1 ), IPCOMP ( ip->ip_src.s_addr, 0 ) );
    cout << " to ";
    printf ( "%d.%d.%d.%d\t", IPCOMP ( ip->ip_dst.s_addr, 3 ), IPCOMP ( ip->ip_dst.s_addr, 2 ), IPCOMP ( ip->ip_dst.s_addr, 1 ), IPCOMP ( ip->ip_dst.s_addr, 0 ) );
    cout << " type: ";
}

char *algotostring(){
    char *algorithm;
	const char *algo;
	
    switch ( timeout_type ) {
    case CLAFFY:
        algo = "claffy";
        break;
    case CLAFFY_TCP:
        algo = "claffy_tcp";
        break;
    case CLAFFY_UNLIMITED:
        algo = "claffy_unlimited";
        break;		
    case RFC:
        algo = "rfc";
        break;
    case NETRAMET:
        algo = "netramet";
        break;
    case NETRAMET_SHORT:
        algo = "netramet_short";
        break;
    case CISCO_BUCKET:
        algo = "cisco";
        break;
    case MBET:
        algo = "mbet";
        break;
    case DOTS:
        algo = "dots";
        break;
    case PGAT:
        algo = "pgat";
        break;
    default:
        algo = "FIX_SWITCH_STATEMENT";
        break;
    }
    
    //algorithm = &algo;
	//return algorithm;
	
	return strdup(algo);
	
}

void initfile(char *name,char *fn) {

    char buffer[SIZE] = {0};

    time_t curtime;
    struct tm *loctime;

    /* Get the current time.  */
    curtime = time (NULL);

    /* Convert it to local time representation.  */
    loctime = localtime (&curtime);

    /* Print out the date and time in the standard format.  */
    if(debug_file)fputs (asctime (loctime), stdout);

    /* Print it out in a nice format.  */
    if(debug_file)strftime (name, SIZE, "Today is %A, %B %d.\n", loctime);
    if(debug_file)fputs (name, stdout);
    if(debug_file)strftime (name, SIZE, "The time is %I:%M %p.\n", loctime);
    if(debug_file)fputs (name, stdout);

    char *hd = getenv ("HOME");
    if(debug_file)printf("homedir: %s\n",hd);


    // initaltracefile-yyyy-mm-dd-algorithm.txt
    strftime (name, SIZE, "%Y%m%d-%H%M%S", loctime);


    char *algo = algotostring();
	


	char *filename = strrchr(fn,'/');
	filename++;

	strcat(buffer, filename);
	strcat(buffer, "_");    
    strcat(buffer, algo);
	strcat(buffer, "_");

	strcat(buffer, name);

	strcat(buffer, ".csv");

    if(debug_file)fputs (buffer, stdout);
    if(debug_file)fputs ("\n", stdout);

    if(debug_file)printf("Filename to be written to: %s\n", buffer);

    /* open the file */
    file = fopen(buffer, "w");
    if (file == NULL) {
        printf("I couldn't open %s for writing.\n",name);
        exit(0);
    }
}

void closefile(){
      /* close the file */
	  if(file != NULL) fclose(file);
}

void callhelp(char *progname){
	cout << "Usage: (simplest) "<< progname <<" traceformat://filename.trace\n";
	cout << "\ttraceformat is libtrace trace type supported types: http://wand.net.nz/trac/libtrace/wiki/SupportedTraceFormats\n\n";
	cout << "Usage: "<< progname <<" -e expirytype [expiry optional args] -d debugtype -r runtime trace1 trace2 ... traceN\n";
	cout << "\nExpiry algorithms [expirytype]:\n";
	cout << "\t[claffy] 64 sec fixed timeout\n";
	cout << "\tOptional: -c timeout (seconds). default = 64\n";
	cout << "\n";
	cout << "\t[claffy_unlimited] unlimited timeout (bigint val)\n";
	cout << "\n";
	cout << "\t[claffy_tcp] 64 sec fixed timeout, with tcp knowledge\n";
	cout << "\tOptional: -c claffy_timeout (seconds). default = 64\n";
	cout << "\n";
	cout << "\t[rfc] libflowtrace timeout - values from various RFCs\n";
	cout << "\tOptional: -q (noarg) Set TCP flow creation to occur on on SYN packets only.\n";
	cout << "\n";
	cout << "\t[netramet] Nevil Brownlee's adaptive timeout\n";
	cout << "\tOptional: -x netramet_multipler. default = 20\n";
	cout << "\tOptional: -s stream_short_time (min expiry time for a flow) (seconds). default = 5\n";
	cout << "\n";
	cout << "\t[netramet_short] Netramet with early expiry for udp\n";
	cout << "\tOptional: -p netramet_short_packetcount early expiry value ( early expiry applies for < n packets). default = 6\n";
	cout << "\tOptional: -m stream_min_time (seconds). default = 2\n";
	cout << "\n";
	cout << "\t[cisco] 15 minute time bucketing\n";
	cout << "\n";
	cout << "\t[mbet] Measured Binary Exponential Timeout\n";
	cout << "\tOptional: -b mbet_config_num - set timeout configuration for mbet. default = 3\n";
	cout << "\n";
	cout << "\t[dots] Dynamic something something \n";
	cout << "\n";
	cout << "\t[pgat] Probability Guaranteed Adaptive Timeout\n";
	cout << "\n";
	cout << "Debug Options [debugtype]: specify a -d for each debug option required. Default is off.\n";
	cout << "\t[packet]      - specifies each packet being read in\n";
	cout << "\t[timeoutbump] - notes when a timeout for a stream gets recalculated\n";
	cout << "\t[packetmatch] - notes when a packet is matched to a stream\n";
	cout << "\t[flowexpire]  - outputs when a stream is expired\n";
	cout << "\t[netramet]    - netramet specific calculations\n";
	cout << "\t[activeflow]  - outputs counts of active flows\n";
	cout << "\t[optarg]      - outputs program arguments\n";
	cout << "Run Time\n";
	cout << "\t-r runtime in seconds since the beginning of the trace, used to stop early in a trace file.\n";
	cout << "Tests\n";
	cout << "\t-t Run Tests in the test.cc file. Currently are around ordering of flows. Exits after running, doesn't process trace files.\n";
	cout << "Legacy Mode\n";
	cout << "\t-l count flows that aren't tcp or udp. Only works for claffy and rfc.\n";
	cout << "Old Function Mode\n";
	cout << "\t-o Run old packetmatch functions instead of new consoldiated functions.\n";

	exit(0);
}

void per_packet ( libtrace_packet_t *packet ) {
    Flow *f;
    CounterFlow *cflow = NULL;
    uint8_t dir;
    bool is_new = false;

    libtrace_tcp_t *tcp = NULL;
    libtrace_ip_t *ip = NULL;
    double ts;

    uint16_t l3_type;

    uint32_t rem;
    uint8_t trans_proto = 0;

    packet_counter++;
	
	ts = trace_get_seconds ( packet );
    if (firstpacket_time == 0) {
        firstpacket_time = ts;
    }
	
	if(runcheck){
	  if(runtime < firstpacket_time - ts) { 
		printf("Exiting at time %.0f\n",runtime);
		exit(1); 
	  }
	}


    /* Libflowmanager only deals with IP traffic, so ignore anything
    * that does not have an IP header */
    ip = ( libtrace_ip_t * ) trace_get_layer3 ( packet, &l3_type, NULL );

    if ( l3_type != 0x0800 ) {
        if ( packet_debug ) cout << "\nLayer 3 type not IP, discarding.\n";
        return;
    }
    if ( ip == NULL ) {
        if ( packet_debug ) cout << "\nIP but null packet, discarding.\n";
        return;
    }
    
    ippacket_counter++;


    trace_get_transport ( packet, &trans_proto, &rem );


	if ( packet_debug ) new_packet_debug(ip);

    switch ( trans_proto ) {
        //http://en.wikipedia.org/wiki/List_of_IP_protocol_numbers
    case 1:
        ip_icmp++;
        if ( packet_debug ) cout << "ICMP\n";
        break;
    case 6:
        ip_tcp++;
        if ( packet_debug ) cout << "TCP\n";
        break;
    case 17:
        ip_udp++;
        if ( packet_debug ) cout << "UDP\n";
        break;
    default:
        ip_unknown++;
        if ( packet_debug ) cout << "Unknown: " << ( int )(trans_proto) << "\n";
        break;
    }
    
    /* Many trace formats do not support direction tagging (e.g. PCAP), so
    * using trace_get_direction() is not an ideal approach. The one we
    * use here is not the nicest, but it is pretty consistent and
    * reliable. Feel free to replace this with something more suitable
    * for your own needs!.
    */
    if ( ip->ip_src.s_addr < ip->ip_dst.s_addr )
        dir = 0;
    else
        dir = 1;

    /* Ignore packets where the IP addresses are the same - something is
    * probably screwy and it's REALLY hard to determine direction */
    if ( ip->ip_src.s_addr == ip->ip_dst.s_addr ) {
        if ( packet_debug ) cout << "\tDUMPING: ipaddr src == ipaddr dst\n";
        return;
    }

    /* Match the packet to a Flow - this will create a new flow if
    * there is no matching flow already in the Flow map and set the
    * is_new flag to true. */
    if (config.oldfn) switch (timeout_type) {
        case RFC:
            f = lfm_match_packet_to_flow ( packet, dir, &is_new );
            break;

            /* Claffy 64 second fixed timeouts */
        case CLAFFY:
        case CLAFFY_UNLIMITED:
        case CLAFFY_TCP:
        case NETRAMET:
        case NETRAMET_SHORT:
		case MBET:  /* Measurement Binary Exponential Timeout               */
        case DOTS:  /* Dynamic Timeout Strategy based on Flow Rate Metrics  */
        case PGAT:  /* probability-guaranteed adaptive timeout              */
            cout << "Legacy packet matching functions removed for claffy && netramet. Use default match instead.\n";
            exit(1);

            /* Cisco 15 minute bucketing */
        case CISCO_BUCKET:
            //f = lfm_cisco_match_packet_to_flow ( packet, dir, &is_new );
            break;
		default:
			cout << "Timeout type: " << timeout_type << " is not implemented yet.\n";
			callhelp(progname);
    }
	else {
	  f = lfm_consolidated_match_packet_to_flow ( packet, dir, &is_new );
	}
	
	if(config.timeout_type == DOTS) dots_pkctcounter++;
	
    if(is_new){
	  flowcounter++;
	  if(config.timeout_type == DOTS) dots_flowcounter++;
	}
    
	/*Figure out simultanious flow count*/
	int thwm = flowcounter;
	if(thwm > hwm){
		hwm = thwm;
	}
	if (ts - firstpacket_time == 0 ){
   	    fprintf( file, "List,Status,From,To,Proto,Hash,Packets In,Packets Out,Bytes In,Bytes Out,Start Time(n), End Time(n), Expiretime(n), Start Time, End Time, Expire Time,TCP Lengthened\n");
		fprintf( file, "Flowlevel,%i,%.6f\n",currentflowlevel,ts-firstpacket_time);
		currentflowlevel = thwm;
	}
	if(currentflowlevel != thwm){
		currentflowlevel = thwm;
		fprintf( file, "Flowlevel,%i,%.6f\n",currentflowlevel,ts-firstpacket_time);
	}


    /* Libflowmanager did not like something about that packet - best to
    * just ignore it and carry on */
    if ( f == NULL ) {
        if ( packet_debug ) cout << "\tCorrupt packet, skipping.\n";
        return;
    }

    if ( dir == 0 ){
        f->packets_in++;
		f->bytes_in += ntohs(ip->ip_len);
	}
    else {
        f->packets_out++;
		f->bytes_out += ntohs(ip->ip_len);
	}

    /* If the returned flow is new, you will probably want to allocate and
    * initialise any custom data that you intend to track for the flow */
    if ( is_new )
        init_counter_flow ( f, dir );

    /* Cast the extension pointer to match the custom data type */
    cflow = ( CounterFlow * ) f->extension;

    /* Increment our packet counter */
    cflow->packets ++;

    /* Update TCP state for TCP flows. The TCP state determines how long
    * the flow can be idle before being expired by libflowmanager. For
    * instance, flows for which we have only seen a SYN will expire much
    * quicker than a TCP connection that has completed the handshake */
    tcp = trace_get_tcp ( packet );

    if ( packet_debug ) {
        if ( tcp ) {
            cout << "TCP: ";
            if ( tcp->syn ) {
                cout << "SYN ";
            }
            if ( tcp->fin ) {
                cout << "FIN ";
            }
            if ( tcp->ack ) {
                cout << "ACK ";
            }
            if ( tcp->rst ) {
                cout << "RST ";
            }
            if ( tcp->check ) {
                cout << "CHK ";
            }
            cout << endl;
        } else {
            cout << "UDP\n";
        }
    }


	/* Update the tcp flags on a flow - not all use this, but useful for lengthening purposes. */
	
	if ( tcp ){
	  lfm_sync_tcp_flags ( f, tcp, dir, ts );


	  /* Check to see that this isn't a SYN packet in the middle of what should otherwise
		* be an established flow */
	  check_lengthening(f, tcp, dir, ts);
 
	}





    switch (timeout_type) {
    case RFC:
        if ( tcp ) lfm_check_tcp_flags ( f, tcp, dir, ts );
        /* Tell libflowmanager to update the expiry time for this flow */
        lfm_update_flow_expiry_timeout ( f, ts );
        break;

        /* Claffy 64 second fixed timeouts */
    case CLAFFY:
    case CLAFFY_UNLIMITED:
        lfm_fixedtimeout_update_flow_expiry_timeout ( f, ts );
        break;

        /* Claffy 64 second, with TCP knowledge */
    case CLAFFY_TCP:
        if ( tcp ) lfm_check_tcp_flags ( f, tcp, dir, ts );
        lfm_fixedtimeout_tcp_update_flow_expiry_timeout ( f, ts );
        break;

        /* Cisco 15 minute bucketing */
    case CISCO_BUCKET:
        //if ( tcp ) lfm_check_tcp_flags ( f, tcp, dir, ts );
        //lfm_cisco_update_flow_expiry_timeout ( f, ts );
        break;

        /* Netramet */
    case NETRAMET:
    case NETRAMET_SHORT:
        f->last_time=ts;
        if ( tcp ) lfm_check_tcp_flags ( f, tcp, dir, ts );
        lfm_netramet_update_flow_expiry_timeout ( f, ts );
        break;

        /* Measurement Binary Exponential Timeout */
    case MBET:
        //no tcp state for mbet
        lfm_mbet_update_flow_expiry_timeout ( f, ts );
        break;

        /* Dynamic Timeout Strategy based on Flow Rate Metrics  */
    case DOTS:
        if ( tcp ) lfm_check_tcp_flags ( f, tcp, dir, ts );
        lfm_dots_update_flow_expiry_timeout ( f, ts );
        break;

        /* probability-guaranteed adaptive timeout  */
    case PGAT:
        if ( tcp ) lfm_check_tcp_flags ( f, tcp, dir, ts );
        lfm_pgat_update_flow_expiry_timeout( f, ts );
        break;

    default:
        cout << "Default, DOH!\n";
        return;
    }
    
    

    /* Expire all suitably idle flows */
    expire_counter_flows ( ts, false );

}

int main ( int argc, char *argv[] ) {
    progname = argv[0];
    char *name = NULL;
	char buffer[SIZE] = {0};
	name = buffer;
	
	//default timeout type
	timeout_type = CLAFFY;

    lfm_init();

    libtrace_t *trace;
    libtrace_packet_t *packet;

    bool opt_true = true;
    bool opt_false = false;

    int i;

    packet = trace_create_packet();
    if ( packet == NULL ) {
        perror ( "Creating libtrace packet" );
        return -1;
    }
	

    /* This tells libflowmanager to ignore any flows where an RFC1918
    * private IP address is involved */
//     if (lfm_set_config_option(LFM_CONFIG_IGNORE_RFC1918, &opt_true) == 0)
//         return -1;

    /* This tells libflowmanager not to replicate the TCP timewait
    * behaviour where closed TCP connections are retained in the Flow
    * map for an extra 2 minutes */
    if ( lfm_set_config_option ( LFM_CONFIG_TCP_TIMEWAIT, &opt_false ) == 0 )
        return -1;

    /* This tells libflowmanager not to utilise the fast expiry rules for
    * short-lived UDP connections - these rules are experimental
    * behaviour not in line with recommended "best" practice */
    if ( lfm_set_config_option ( LFM_CONFIG_SHORT_UDP, &opt_false ) == 0 )
        return -1;

    optind = 1;

	if(argc == 1){
		callhelp(progname);
	}
	
    for ( i = optind; i < argc; i++ ) {
        double ts;
	
        if (debug_optarg) printf( "i:%d,optind:%d,argc:%d, argv value:%s\n",i,optind,argc,argv[i] );

        if ( argv[i][0] == '-' && argv[i][1] == 'e' ) {
            i++;
            char *type = argv[i];

            for ( int j = 0; j < strlen(type); j++) {
                type[j] = tolower(type[j]);
            }

            if (strcmp(type,"claffy") == 0) timeout_type = CLAFFY;
            else if (strcmp(type,"claffy_tcp") == 0) timeout_type = CLAFFY_TCP;
            else if (strcmp(type,"claffy_unlimited") == 0) timeout_type = CLAFFY_UNLIMITED;			
            else if (strcmp(type,"rfc") == 0) timeout_type = RFC;
            else if (strcmp(type,"netramet") == 0) timeout_type = NETRAMET;
            else if (strcmp(type,"netramet_short") == 0) timeout_type = NETRAMET_SHORT;
            else if (strcmp(type,"cisco") == 0) timeout_type = CISCO_BUCKET;
            else if (strcmp(type,"mbet") == 0) timeout_type = MBET;
            else if (strcmp(type,"dots") == 0) timeout_type = DOTS;
            else if (strcmp(type,"pgat") == 0) timeout_type = PGAT;
			else callhelp(argv[0]); 
            config.timeout_type = timeout_type;
            continue;
        }

        /*command line debug settings
         * bool packet_debug 		= true;
         * bool timeoutbump_debug 	= true;
         * bool packetmatch_debug 	= true;
         * bool flowexpire_debug 	= true;
         * bool debug_netramet 	    = false;
         * bool debug_optarg		= true;
         * bool debug_activeflow	= true;
         */

        if ( argv[i][0] == '-' && argv[i][1] == 'd' ) {
            i++;
            char *type = argv[i];

            for ( int j = 0; j < strlen(type); j++) {
                type[j] = tolower(type[j]);
            }

            if (strcmp(type,"packet"     ) == 0) packet_debug 	   = true;
            if (strcmp(type,"timeoutbump") == 0) timeoutbump_debug = true;
            if (strcmp(type,"packetmatch") == 0) packetmatch_debug = true;
            if (strcmp(type,"flowexpire" ) == 0) flowexpire_debug  = true;
            if (strcmp(type,"netramet"   ) == 0) debug_netramet    = true;
            if (strcmp(type,"optarg"     ) == 0) debug_optarg	   = true;
            if (strcmp(type,"activeflow" ) == 0) debug_activeflow  = true;
			if (strcmp(type,"file"       ) == 0) debug_file        = true;
            if (strcmp(type,"all"        ) == 0) {
			  packet_debug 		= true;
			  timeoutbump_debug = true;
			  packetmatch_debug = true;
			  flowexpire_debug 	= true;
			  debug_netramet 	= true;
			  debug_optarg		= true;
			  debug_activeflow	= true;
			  debug_file        = true;
			}
            continue;
        }
        
		if ( argv[i][0] == '-' &&  argv[i][1] == 'r' ) {
            i++;
            runtime = atoi(argv[i]);
			runcheck = true;
            if (debug_optarg) cout << "Tracefile runtime = " << runtime << endl;
            continue;
        }


		if ( argv[i][0] == '-' &&  argv[i][1] == 'h' ) {
			callhelp(argv[0]);
        }
        
        //claffy fixed timeout option
        if ( argv[i][0] == '-' &&  argv[i][1] == 'c' ) {
            i++;
            claffy_timeout = atoi(argv[i]);
            if (debug_optarg) cout << "Claffy timeout = " << claffy_timeout << endl;
            continue;
        }

        //netramet options...
        if ( argv[i][0] == '-' &&  argv[i][1] == 'p' ) {
            i++;
            netramet_short_packetcount = atoi(argv[i]);
            if (debug_optarg) cout << "Fixed netramet_short_packetcount to: " << netramet_short_packetcount << endl;
            continue;
        }

        if ( argv[i][0] == '-' &&  argv[i][1] == 'x' ) {
            i++;
            netramet_multipler = atoi(argv[i]);
            if (debug_optarg) cout << "Fixed netramet_multipler to: " << netramet_multipler << endl;
            continue;
        }

        if ( argv[i][0] == '-' &&  argv[i][1] == 'm' ) {
            i++;
            stream_min_time = atoi(argv[i]);
            if (debug_optarg) cout << "Fixed stream_min_time (seconds) to: " << stream_min_time << endl;
            continue;
        }

        if ( argv[i][0] == '-' &&  argv[i][1] == 's' ) {
            i++;
            stream_short_time = atoi(argv[i]);
            if (debug_optarg) cout << "Fixed stream_short_time (seconds) to: " << stream_short_time << endl;
            continue;
        }

        //mbet options...
        if ( argv[i][0] == '-' &&  argv[i][1] == 'b' ) {
            i++;
            mbet_config_num = atoi(argv[i]);
            if (debug_optarg) cout << "Fixed mbet_config_num to: " << mbet_config_num << endl;
            continue;
        }
        
        if ( argv[i][0] == '-' &&  argv[i][1] == 't' ) {
		    cout << "Running Simple Testing in test.cc\n\n";
		   	tests();
			exit(1);
		}
        if ( argv[i][0] == '-' &&  argv[i][1] == 'l' ) {
		    cout << "Legacy mode: Enabled. Counting flows that aren't UDP and TCP. Not counting ICMP.\n";
			config.legacy = true;
			continue;
		}
        if ( argv[i][0] == '-' &&  argv[i][1] == 'o' ) {
		    cout << "Old packetmatch mode: Enabled. Running old packetmatch functions. Used for comparisions.\n";
			config.oldfn = true;
			continue;
		}

        if ( argv[i][0] == '-' &&  argv[i][1] == 'q' ) {
		    cout << "RFC Enabled tcp flow creation on SYN packets only.\n";
			config.rfc_strict_tcp = true;
			continue;
		}


        /* Bog-standard libtrace stuff for reading trace files */
        trace = trace_create ( argv[i] );

        if ( !trace ) {
            perror ( "Creating libtrace trace" );
            return -1;
        }

        if ( trace_is_err ( trace ) ) {
            trace_perror ( trace, "Opening trace file" );
            trace_destroy ( trace );
            continue;
        }

        if ( trace_start ( trace ) == -1 ) {
            trace_perror ( trace, "Starting trace" );
            trace_destroy ( trace );
            continue;
        }

		/* write filename into the file */
        if(name != NULL){
		  initfile(name,argv[i]);
		}
		
	    fprintf(file, "Trace filepath: %s, Algorithm:%s\n",argv[i],algotostring());

		while ( trace_read_packet ( trace, packet ) > 0 ) {
            ts = trace_get_seconds ( packet );
            per_packet ( packet );

        }

        if ( trace_is_err ( trace ) ) {
            trace_perror ( trace, "Reading packets" );
            trace_destroy ( trace );
            continue;
        }

        trace_destroy ( trace );

    }

    trace_destroy_packet ( packet );

    lfm_print_active_flows();

    /* And finally, print something useful to make the exercise worthwhile */
	//cout << "Algorithm: " << timeout_type << "(" << algotostring() << ")" << endl;
	fprintf ( file, "Algorithm: %i (%s)\n",timeout_type,algotostring());
	
    if ( timeout_type == CLAFFY_TCP || timeout_type == CLAFFY ) {
		fprintf ( file, "Claffy timeout: %.0f\n",claffy_timeout);
        //cout << "Claffy timeout: " << claffy_timeout << endl;
    }

    fprintf ( file, "Final expired flow count  : %" PRIu64 "\n", expiredflow_counter );
    fprintf ( file, "Final flow count  : %" PRIu64 "\n", flow_counter );
    fprintf ( file, "Final packet count: %" PRIu64 "\n", packet_counter );
    fprintf ( file, "Final ip pkt count: %" PRIu64 "\n", ippacket_counter );
    fprintf ( file, "Final ip tcp count: %" PRIu64 "\n", ip_tcp );
    fprintf ( file, "Final ip icmp count: %" PRIu64 "\n", ip_icmp );
    fprintf ( file, "Final ip udp count: %" PRIu64 "\n", ip_udp );
    fprintf ( file, "Final ip unknown count: %" PRIu64 "\n", ip_unknown );
	fprintf ( file, "Flow highwater mark: %i\n", hwm);
	fprintf ( file, "Global starttime: %.6f\n", firstpacket_time );
	
	
	
	closefile();
    return 0;
}
