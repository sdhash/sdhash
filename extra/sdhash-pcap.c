//  pcap_throughput
//
//   reads in a pcap file and outputs basic throughput statistics 

#include <stdio.h>
#include <pcap.h>
#include <stdlib.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

//defines for the packet type code in an ETHERNET header
#define ETHER_TYPE_IP (0x0800)
#define ETHER_TYPE_8021Q (0x8100)

#include <stdint.h>

//------------------------------------------------------------------- 
int main(int argc, char **argv) { 
  unsigned int pkt_counter=0;   // packet counter 
  unsigned long byte_counter=0; //total bytes seen in entire trace 
  unsigned long cur_counter=0; //counter for current 1-second interval 
  unsigned long max_volume = 0;  //max value of bytes in one-second interval 
  unsigned long current_ts=0; //current timestamp 
 
  //temporary packet buffers 
  struct pcap_pkthdr header; // The header that pcap gives us 
  const char *packet; // The actual packet 
  int fnum,i;
  FILE *outputfile;
  
  //check command line arguments 
  if (argc < 2) { 
    fprintf(stderr, "Usage: %s [input pcaps]\n", argv[0]); 
    exit(1); 
  } 
  
  //-------- Begin Main Packet Processing Loop ------------------- 
    outputfile=fopen("/tmp/file.packet","w");
  //loop through each pcap file in command line args 
  for (fnum=1; fnum < argc; fnum++) {  
    //----------------- 
    //open the pcap file 
    char errbuf[PCAP_ERRBUF_SIZE]; 
    pcap_t *handle = pcap_open_offline(argv[fnum], errbuf); 
     if (handle == NULL) { 
      fprintf(stderr,"Couldn't open pcap file %s: %s\n", argv[fnum], errbuf); 
      return(2); 
    } 
    struct bpf_program fp;		/* The compiled filter */
	char filter_exp[] = "greater 65";	/* The filter expression */
	//char filter_exp[] = "tcp and less 512";	/* The filter expression */
	if (pcap_compile(handle, &fp, filter_exp, 0, 0) == -1) {
		fprintf(stderr, "Couldn't parse filter %s: %s\n", "", pcap_geterr(handle));
		//fprintf(stderr, "Couldn't parse filter %s: %s\n", filter_exp, pcap_geterr(handle));
		return(2);
	} 
	if (pcap_setfilter(handle, &fp) == -1) {
			//fprintf(stderr, "Couldn't install filter %s: %s\n", filter_exp, pcap_geterr(handle));
			return(2);
	}
    while( packet = pcap_next(handle, &header)) { 
      char *pkt_ptr = (char *)packet; 
      
printf( "%d\n", header.caplen);  
      fprintf(outputfile, "%s", packet);
      //for (i=header.caplen; i<2048; i++) 
	//fprintf(outputfile,
	   
      pkt_counter++; //increment number of packets seen 
 
    } //end internal loop for reading packets (all in one file) 
 
    pcap_close(handle);  //close the pcap file 
 
  } //end for loop through each command line argument 
  //---------- Done with Main Packet Processing Loop --------------  
  fclose(outputfile); 
  byte_counter >>= 20;  //convert to MB
 
  printf("Processed %d packets and %u MBytes, in %d files\n", pkt_counter, byte_counter, argc-1);
  return 0; //done
} //end of main() function

