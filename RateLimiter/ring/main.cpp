#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <queue>
#include <string>
#include <iostream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <signal.h>
#include <stdlib.h>

using namespace std;
typedef long long ll;

struct Packet{
    double arrival_time;
    int len;
};

struct timeval start;
ll start_usec;
queue<Packet> input;
queue<Packet> inque;

double get_msec(struct timeval now){
    ll u1 = start.tv_sec*1000000+start.tv_usec;
    ll u2 = now.tv_sec*1000000+now.tv_usec;
    return 1.0*(u2-u1)/1000;
}

static int tot_round = 0;


struct Ring{
    static const int block = 1000;
    static const int qlen = 80;
    double rate = 1024.0;
    Packet ring_queue[block][qlen];
    //It can be changed with the change of tunnel condition.
    double gaing_rate = 1.0;
    int now_qlen[block];
    int now_bytes[block];
    //real timer and inqueue pointer.
    int now_postion = 0;
    int pre_pointer = 0;
    int time_interval = 1;//the default interval is 1ms.
    double latency = 40;
    ll tot_bytes = 0;
    int max_byte_per_block = 3000;

    Ring(){
        for(int i=0; i<block; i++) now_qlen[i] = now_bytes[i] = 0;
    }
    Ring(double _rate){
        rate = _rate;
        for(int i=0; i<block; i++) now_qlen[i] = now_bytes[i] = 0;
        max_byte_per_block = rate*1024/block;

    }
    int cal_interval(int pre, int after){
        if(after>=pre){
            return after-pre;
        }
        else{
            return after+block-pre;
        }
    }
    void in_ring(){
        while(!inque.empty()){
            if(inque.front().arrival_time+latency<tot_round*0.1){
                inque.pop();
                continue;
            }
            else{
                int interval = cal_interval(now_postion, pre_pointer);
				//filter big packet
				if(inque.front().len/rate/1024>1){
					inque.pop();
					break;
				}
                int interval_block = (1.0*(inque.front().len+tot_bytes)/(rate*1024))*block;
				//printf("interval: %d %d\n", interval, interval_block);
				
                if(interval_block>block) break;

                if(interval_block<=interval) {
                    ring_queue[pre_pointer][now_qlen[pre_pointer]] = inque.front();
                    now_qlen[pre_pointer] += 1;
                    now_bytes[pre_pointer] += inque.front().len;
					//printf("test: %d\n", pre_pointer);
                }
                else {
                    pre_pointer = ((interval_block-interval)+pre_pointer)%block;
                    ring_queue[pre_pointer][0] = inque.front();
                    now_qlen[pre_pointer] = 1;
                    now_bytes[pre_pointer] = inque.front().len;
                }
                
                //else printf("%d %d\n", now_postion, pre_pointer);
                tot_bytes+=inque.front().len;
                inque.pop();
            }
        }
    }

    void de_ring(){
        if(now_qlen[now_postion]!=0){
            //all packets can dequeue
            for(int i=0; i<now_qlen[now_postion]; i++){
                printf("%lf %d\n", tot_round*0.1+1.0*time_interval/now_qlen[now_postion]*i, ring_queue[now_postion][i].len);
                tot_bytes -= ring_queue[now_postion][i].len;
            }
            now_qlen[now_postion] = 0;
            now_bytes[now_postion] = 0;
        }
    }

    void status_update(){
        now_qlen[now_postion] = 0;
        int interval = cal_interval(now_postion, pre_pointer);
        if(interval == 0) {
			pre_pointer = (pre_pointer+1)%block;
			now_postion = (now_postion+1)%block;
		}
		else{
			now_postion = (now_postion+1)%block;
		}
    }

    void process(){
        in_ring();
        de_ring();
        status_update();
    }
};

struct Ring ring(1024.0/8);

void test_func(int sig){
   	tot_round++;
	double msec = tot_round*0.1;
    //printf("%lf\n", msec);
	while(!input.empty()){
            if(input.front().arrival_time+ring.latency<msec){
                input.pop();
                continue;
            }
            if(msec>=input.front().arrival_time){
                inque.push(input.front());
                input.pop();
                while(!input.empty()){
                    if(msec>=input.front().arrival_time){
                        inque.push(input.front());
                        input.pop();
                    }
                    else break;
                }
				ring.in_ring();
            }
			else break;
	}
	if(tot_round%10 == 0){
		ring.de_ring();
		ring.status_update();
	}
	
}

void init_sigaction(){
    struct sigaction act;
    act.sa_handler = test_func;
    act.sa_flags  = 0;
    sigemptyset(&act.sa_mask);
    sigaction(SIGPROF, &act, NULL);
}

void init_time(){
    struct itimerval val;
    val.it_value.tv_sec = 0;
    val.it_value.tv_usec = ring.time_interval*100;
    val.it_interval = val.it_value;
    setitimer(ITIMER_PROF, &val, NULL);
}



int main()
{
    freopen("in.txt", "r", stdin);
    //freopen("out.txt", "w", stdout);
    while(!input.empty())input.pop();
    string s;
    Packet temp;
    while(getline(cin, s)){
        stringstream ss(s);
        ss>>temp.arrival_time>>temp.len;
        input.push(temp);
    }
    //default set up speed is 1024KB.
    init_sigaction();
    init_time();
    while(true){
        //if(input.empty() && inque.empty()) break;
    }
    return 0;
}
