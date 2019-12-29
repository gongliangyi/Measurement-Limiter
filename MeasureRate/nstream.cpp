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
#include <unordered_map>
#include <map>
#include <cmath>
#include <string>
#include <cstring>

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


long long nxt_end =-1;
long long last_time = -1;
int interval = 1000;

int cnt = 0;

const int PACKET_HEADER = 28;
const int QUEUE_SIZE = 4;

//multi stream settings

const int qn = 65536;
double start[qn];
double nxt[qn];
queue<double> q[qn];
typedef pair<int, int> pii;
#define mk(x, y) make_pair(x, y)
map<pii, int> mp;
int tot_ippkt_len[qn];
int stream_id=0;


double get_ratio(int idx){
	queue<double> temp = q[idx];
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

void test_func(int signal_number){
    for(int now_id = 1; now_id<=stream_id; now_id++){
        double ratio = 1.0*tot_ippkt_len[now_id]/(interval)*8/1000;
        if(q[now_id].size()<QUEUE_SIZE){
            q[now_id].push(ratio);
        }
        else{
            q[now_id].pop();
            q[now_id].push(ratio);
        }
        double smooth_ratio = get_ratio(now_id);
        printf("stream%d: %lf\n",now_id, smooth_ratio);
        start[now_id] += interval;
        nxt[now_id] += interval;
        tot_ippkt_len[now_id] = 0;
     }

}

void init_sigaction()
{
    struct sigaction act;

    act.sa_handler = test_func;
    act.sa_flags  = 0;

    sigemptyset(&act.sa_mask);
    sigaction(SIGPROF, &act, NULL);
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
    //printf("%u\n", ip->ip_src.s_addr);
    unsigned short* s_port = (unsigned short*)(packet+35);
    //printf("%hu\n", *s_port);
    //toStringIP(*(int*)(ip+20), stringIP);
    //puts(stringIP);

    char* stringIP = (char*)malloc(16);
    memset(stringIP, 0, sizeof(stringIP));
    toStringIP(ip->ip_src.s_addr, stringIP);
    //printf("%hu %s ", ip->ip_p, stringIP);
    memset(stringIP, 0, sizeof(stringIP));

    toStringIP(ip->ip_dst.s_addr, stringIP);
    //printf("%s ", stringIP);
    /*
    memset(stringIP, 0, sizeof(stringIP));
    if(length <= 20) printf("fuck\n");
    char tmp[6];
    memset(tmp, 0, sizeof(tmp));
    printf("233\n");
    sprintf(tmp, "%x", ((unsigned short *)(ip+20)));
    printf("2331\n");
    strcat(stringIP, tmp);
    printf("%s\n", stringIP);
    memset(tmp, 0, sizeof(tmp));
    sprintf(tmp, "%x", ((unsigned char *)(ip+20)));
    strcat(stringIP, tmp);
    printf("%s", stringIP);
    */
    unsigned short *sp = (unsigned short*)(packet+ETHER_HDRLEN+sizeof(struct my_ip));
    
    unsigned short *dstp = (unsigned short*)(packet+ETHER_HDRLEN+sizeof(struct my_ip)+2);
	unsigned short src_port = big2small(*(sp)), dst_port = big2small(*(dstp));
	
    //printf("%hu %hu\n", src_port, dst_port);
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
	if(mp[mk(src_port, dst_port)] == 0){
		mp[mk(src_port, dst_port)] = ++stream_id;
	}
	int now_id = mp[mk(src_port, dst_port)];
    //printf("%d\n", now_id);
    /*
    if(fabs(start[now_id]+1)<1e-2){
        start[now_id] = tv.tv_sec*1000 + tv.tv_usec/1000;
		nxt[now_id] = start[now_id]+interval;
    }
    */
	/*
	if(tv.tv_sec*1000 + tv.tv_usec/1000 > start+1.5*interval){
		start = tv.tv_sec*1000 + tv.tv_usec/1000;
		nxt_end = start+interval;
	}
	*/
    /*
	if(now-start[now_id]>=2*interval){
		start[now_id] = tv.tv_sec*1000 + tv.tv_usec/1000;
		nxt[now_id] = start[now_id]+interval;
		tot_ippkt_len[now_id] = 0;
	}
	*/
	//last_time = tv.tv_sec*1000 + tv.tv_usec/1000;	
    /*
    if(tv.tv_sec*1000 + tv.tv_usec/1000 >= nxt[now_id]){
	double ratio = 1.0*tot_ippkt_len[now_id]/(interval)*8/1000;
	if(q[now_id].size()<QUEUE_SIZE){
		q[now_id].push(ratio);
	}
	else{
		q[now_id].pop();
		q[now_id].push(ratio);
	}
	double smooth_ratio = get_ratio(now_id);
        printf("stream%d: %lf\n",now_id, smooth_ratio);
		start[now_id] += interval;
		nxt[now_id] += interval;
        tot_ippkt_len[now_id] = 0;
	}
    */
	//tot_ippkt_len += 1500;
	tot_ippkt_len[now_id] += (len-PACKET_HEADER);
	tot_packet++;
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

void init_time()
{
    struct itimerval val;

    val.it_value.tv_sec = interval/1000;
    val.it_value.tv_usec = (interval%1000)*1000;
    val.it_interval = val.it_value;
    setitimer(ITIMER_PROF, &val, NULL);
}


int main(int argc,char **argv)
{
	signal(SIGINT,handler);
    for(int i=0; i<qn; i++){
        start[i] = nxt[i] = -1;
    }
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
    init_sigaction();
    init_time();
    /* ... and loop */
    pcap_loop(descr,-1,my_callback,args);

    fprintf(stdout,"\nfinished\n");
    return 0;
}



