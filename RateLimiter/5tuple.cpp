#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <net/ethernet.h>
#include <netinet/ether.h>
#include <netinet/ip.h>
#include <sys/time.h>
#include <string.h>
#include <signal.h>    
#include <queue>
#include <vector>
#include <algorithm>

using namespace std;

/* tcpdump header (ether.h) defines ETHER_HDRLEN) */
#ifndef ETHER_HDRLEN
#define ETHER_HDRLEN 14
#endif


u_int16_t handle_ethernet
        (u_char *args,const struct pcap_pkthdr* pkthdr,const u_char*
        packet);
u_char* handle_IP
        (u_char *args,const struct pcap_pkthdr* pkthdr,const u_char*
        packet);


/*
 * Structure of an internet header, naked of options.
 *
 * Stolen from tcpdump source (thanks tcpdump people)
 *
 * We declare ip_len and ip_off to be short, rather than u_short
 * pragmatically since otherwise unsigned comparisons can result
 * against negative integers quite easily, and fail in subtle ways.
 */
struct my_ip {
	u_int8_t	ip_vhl;		/* header length, version */
#define IP_V(ip)	(((ip)->ip_vhl & 0xf0) >> 4)
#define IP_HL(ip)	((ip)->ip_vhl & 0x0f)
	u_int8_t	ip_tos;		/* type of service */
	u_int16_t	ip_len;		/* total length */
	u_int16_t	ip_id;		/* identification */
	u_int16_t	ip_off;		/* fragment offset field */
#define	IP_DF 0x4000			/* dont fragment flag */
#define	IP_MF 0x2000			/* more fragments flag */
#define	IP_OFFMASK 0x1fff		/* mask for fragmenting bits */
	u_int8_t	ip_ttl;		/* time to live */
	u_int8_t	ip_p;		/* protocol */
	u_int16_t	ip_sum;		/* checksum */
	struct	in_addr ip_src,ip_dst;	/* source and dest address */
};

/* looking at ethernet headers */
int tot_len = 0;
int tot_packet = 0;
int tot_ippkt_len = 0;
void my_callback(u_char *args,const struct pcap_pkthdr* pkthdr,const u_char*
        packet)
{
    u_int16_t type = handle_ethernet(args,pkthdr,packet);

    if(type == ETHERTYPE_IP)
    {/* handle IP packet */
        handle_IP(args,pkthdr,packet);
    }else if(type == ETHERTYPE_ARP)
    {/* handle arp packet */
    }
    else if(type == ETHERTYPE_REVARP)
    {/* handle reverse arp packet */
    }

}


long long start = -1;
long long nxt_end =-1;
long long last_time = -1;
int interval = 1000;

int cnt = 0;

const int PACKET_HEADER = 28;
const int QUEUE_SIZE = 4;
queue<double> q;

double get_ratio(){
	queue<double> temp = q;
	vector<double> sample;
	sample.clear();
	while(!temp.empty()) sample.push_back(temp.front()), temp.pop();
	sort(sample.begin(), sample.end());
	int sz = sample.size();
	double L = (sample[sz-1]-sample[0])/(sz-1);
	if(L == 0){
		return sample[0];
	}
	double l, r;
	l=0, r=sz-1;
	double F = -1;
	for(int i=0; i<sz; i++){
		for(int j=i+1; j<sz; j++){
			if(sample[j]-sample[i]<L) continue;
			if((j-i+1)*(j-i+1)/(sample[j]-sample[i])>F){						
				F = ((j-i+1)*(j-i+1)/(sample[j]-sample[i]));
				l=i, r=j;
			}
		}
	}
	double tot = 0;
	for(int i=l; i<=r; i++) tot+=sample[i];
	return tot/(r-l+1);

}

void toStringIP(const unsigned int ip,char* &stringIP){
   unsigned int tempIP=ip;
   for(int i=0;i<3;i++){
      unsigned char part=(char)tempIP;
      char temp[4];
      sprintf(temp,"%d.",part);
      strcat(stringIP,temp);
      tempIP=tempIP>>8;
   }
   unsigned char part=(char)tempIP;
   char temp[4];
   memset(temp, 0, sizeof(temp));
   sprintf(temp,"%d",part);
   strcat(stringIP,temp);
}

unsigned short big2small(unsigned short x){
    vector<int> q;
    q.clear();
    while(x){
        q.push_back(x%2);
        x/=2;
    }
    while(q.size()<16){
        q.push_back(0);
    }
    unsigned short now = 0;
    int pow = 1;
    for(int i=8; i<16; i++){
        now = now+q[i]*pow;
        pow *= 2;
    }
    for(int i=0; i<8; i++){
        now = now+q[i]*pow;
        pow *= 2;
    }
    return now;

}



u_char* handle_IP
        (u_char *args,const struct pcap_pkthdr* pkthdr,const u_char*
        packet)
{
    const struct my_ip* ip;
    u_int length = pkthdr->len;
    u_int hlen,off,version;
    int i;

    int len;

    /* jump pass the ethernet header */
    ip = (struct my_ip*)(packet + sizeof(struct ether_header));
    length -= sizeof(struct ether_header);

    /* check to see we have a packet of valid length */
    if (length < sizeof(struct my_ip))
    {
        printf("truncated ip %d",length);
        return NULL;
    }

    len     = ntohs(ip->ip_len);
    hlen    = IP_HL(ip); /* header length */
    version = IP_V(ip);/* ip version */

    char* stringIP = (char*)malloc(16);
    memset(stringIP, 0, sizeof(stringIP));
    toStringIP(ip->ip_src.s_addr, stringIP);
    printf("%hu %s ", ip->ip_p, stringIP);
    memset(stringIP, 0, sizeof(stringIP));

    toStringIP(ip->ip_dst.s_addr, stringIP);
    printf("%s ", stringIP);
    unsigned short *sp = (unsigned short*)(packet+ETHER_HDRLEN+sizeof(struct my_ip));
    unsigned short *dstp = (unsigned short*)(packet+ETHER_HDRLEN+sizeof(struct my_ip)+2);
    printf("%hu %hu\n", big2small(*(sp)), big2small(*(dstp)));
    free(stringIP);
    //printf("%s\n", tmp);
    /* check version */
    if(version != 4)
    {
      fprintf(stdout,"Unknown version %d\n",version);
      return NULL;
    }

    /* check header length */
    if(hlen < 5 )
    {
        fprintf(stdout,"bad-hlen %d \n",hlen);
    }

    /* see if we have as much packet as we should */
    if(length < len)
        printf("\ntruncated IP - %d bytes missing\n",len - length);

    /* Check to see if we have the first fragment */
    /*
	off = ntohs(ip->ip_off);
    if((off & 0x1fff) == 0 )// aka no 1's in first 13 bits
    { // print SOURCE DESTINATION hlen version len offset 
        fprintf(stdout,"IP: ");
        fprintf(stdout,"%s ",
                inet_ntoa(ip->ip_src));
        fprintf(stdout,"%s %d %d %d %d\n",
                inet_ntoa(ip->ip_dst),
                hlen,version,len,off);
    }
	*/
	/*

    if(len<66){
        return NULL;
    }
	*/
    //printf("%u\n", ip->ip_src.s_addr);
    //toStringIP(ip->ip_src.s_addr, stringIP);
    //puts(stringIP);
    //printf("%s\n", stringIP);

    //toStringIP(ip->ip_dst.s_addr, stringIP);
    //puts(stringIP);

    //printf("%s\n", stringIP);
	//printf("%d\n", len);
	//printf("%d\n", tot_packet);
    return NULL;
}

/* handle ethernet packets, much of this code gleaned from
 * print-ether.c from tcpdump source
 */

u_int16_t handle_ethernet
        (u_char *args,const struct pcap_pkthdr* pkthdr,const u_char*
        packet)
{
    u_int caplen = pkthdr->caplen;
    u_int length = pkthdr->len;
	tot_len += length;
    struct ether_header *eptr;  /* net/ethernet.h */
    u_short ether_type;

    if (caplen < ETHER_HDRLEN)
    {
        fprintf(stdout,"Packet length less than ethernet header length\n");
        return -1;
    }

    /* lets start with the ether header... */
    eptr = (struct ether_header *) packet;
    ether_type = ntohs(eptr->ether_type);

    /* Lets print SOURCE DEST TYPE LENGTH */
	/*
    fprintf(stdout,"ETH: ");
    fprintf(stdout,"%s "
            ,ether_ntoa((struct ether_addr*)eptr->ether_shost));
    fprintf(stdout,"%s "
            ,ether_ntoa((struct ether_addr*)eptr->ether_dhost));
	*/
    /* check to see if we have an ip packet */
    if (ether_type == ETHERTYPE_IP)
    {
        //fprintf(stdout,"(IP)");
    }else  if (ether_type == ETHERTYPE_ARP)
    {
        fprintf(stdout,"(ARP)");
    }else  if (eptr->ether_type == ETHERTYPE_REVARP)
    {
        fprintf(stdout,"(RARP)");
    }else {
        fprintf(stdout,"(?)");
    }
    //fprintf(stdout," %d\n",length);

    return ether_type;
}

int c2number(char* c){
	int ans = 0;
	int len = strlen(c);
	int i=0;
	for(;i<len; i++){
		ans = ans*10+c[i]-'0';
	}
	return ans;
}

void handler(int sig)
{
	//fclose(fp);
//	printf("hey");
	exit(0);
}


int main(int argc,char **argv)
{
	signal(SIGINT,handler);
    
    char *dev;
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t* descr;
    struct bpf_program fp;      /* hold compiled program     */
    bpf_u_int32 maskp;          /* subnet mask               */
    bpf_u_int32 netp;           /* ip                        */
    u_char* args = NULL;

    /* Options must be passed in as a string because I am lazy */
    
    /* grab a device to peak into... */
    //dev = pcap_lookupdev(errbuf);
	dev = argv[1];
    if(dev == NULL)
    { printf("%s\n",errbuf); exit(1); }

    /* ask pcap for the network address and mask of the device */
    pcap_lookupnet(dev,&netp,&maskp,errbuf);

    /* open device for reading. NOTE: defaulting to
     * promiscuous mode*/
    descr = pcap_open_live(dev,BUFSIZ,1,-1,errbuf);
    if(descr == NULL)
    { printf("pcap_open_live(): %s\n",errbuf); exit(1); }


    if(argc > 2)
    {
        /* Lets try and compile the program.. non-optimized */
        if(pcap_compile(descr,&fp,argv[2],0,netp) == -1)
        { fprintf(stderr,"Error calling pcap_compile\n"); exit(1); }

        /* set the compiled program as the filter */
        if(pcap_setfilter(descr,&fp) == -1)
        { fprintf(stderr,"Error setting filter\n"); exit(1); }
    }
	if(argc > 3){
		interval = c2number(argv[3]);
	}

    /* ... and loop */
    pcap_loop(descr,-1,my_callback,args);

    fprintf(stdout,"\nfinished\n");
    return 0;
}



