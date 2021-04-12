// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * net/sched/sch_kfifo.c	The  ring FIFO queue.
 *
 * Authors:	
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/skbuff.h>
#include <net/pkt_sched.h>

#include "sch_kfifo.h"

int kfifo_finish_node(struct kfifo_node *read)
{
	if(read){

		read->left_time = UNIT_TIME;
		read->pkt = 0;
		read->slen = 0;
		read->sleep_time=0;
		kunit_skb_head_init(&read->kunit);
	}
    return 0;
}

int kfifo_reset(struct kfifo_queue_data *kfifo, u32 time, u32 rate)
{
	struct kfifo_node *temp = kfifo->pbase;
	if(temp){
		kfree(kfifo->pbase);
		return kfifo_init(kfifo,time,rate);
	}
	
}




int kfifo_init(struct kfifo_queue_data *kfifo, u32 time, u32 rate)
{
    struct kfifo_node *fifo;
    
	int i =0;
    u32  number = time; // every unit limit to ms
    fifo = (struct kfifo_node*)kmalloc(sizeof(struct kfifo_node)*number,GFP_KERNEL);
    if (!fifo){
    	printk("kfifo kmalloc failed\n");
    	return -1;
    }
            
    kfifo->pread = kfifo->pwrite = kfifo->pbase = &fifo[0];
    kfifo->total_len = time*rate/1000;
    kfifo->total_time = number; // ms


    spin_lock_init(&kfifo->lock);

    if(number < 2) return -1;
	for(i=0;i< number;i++){
		fifo[i].pkt = 0;
		fifo[i].rate = rate; //Bytes per second
		fifo[i].slen = 0;
		fifo[i].tlen = rate/1000; //Bytes per ms
		fifo[i].gain = 1;
		fifo[i].sleep_time = 0;
		fifo[i].left_time = UNIT_TIME;
		fifo[i].kunit.head = fifo[i].kunit.tail = NULL;
	}
    for(i=1;i<number-1;i++){
    	fifo[i].prev = (knode *)&fifo[i-1];
    	fifo[i].next = (knode *)&fifo[i+1];
    }
    fifo[0].next = (knode *)&fifo[1];
    fifo[0].prev = (knode *)&fifo[number-1];
    fifo[number-1].next = (knode *)&fifo[0];
    fifo[number-1].prev = (knode *)&fifo[number-2];
  
	return 0;
}

int kfifo_wr_conflict(struct kfifo_queue_data *kfifo,int step)
{
	if(kfifo){
		knode *temp = kfifo->pwrite;
		if((temp->pkt > 0) && (kfifo->pwrite == kfifo->pread))
			return -1;
		int i=0;
		for(i=0;i<step;i++){
			 temp = temp->next;
			 if(temp == kfifo->pread){
			 	return -1;
			 }

		}
		return 0;
	}
	return -1;
}
int kfifo_write_forward(struct kfifo_queue_data *kfifo,int step,u64 cost_time)
{
	if(kfifo){
		int i=0;
		for(i=0;i<step;i++){
			 if(kfifo->pwrite->next == NULL){
			 //	printk("kfifo_write_forward 2:temp is NULL \n");
			 	return -1;
			 }
			 if(kfifo->pwrite->next == kfifo->pread){
			 	//printk("kfifo_write_forward read equal to write \n");
			 	return -1;
			 }
			 kfifo->pwrite = kfifo->pwrite->next;
		}
		kfifo->pwrite->left_time -= cost_time;
		return 0;
	}
	return -1;
}
int kfifo_read_forward(struct kfifo_queue_data *kfifo)
{
    if(kfifo){
    	if((kfifo->pread == kfifo->pwrite)){
    		return 0;
    	} 
    //	spin_lock(&kfifo->lock);
        while(kfifo->pread->pkt==0){
        	kfifo->pread = kfifo->pread->next;
        	if(kfifo->pread == kfifo->pwrite){
        		 if(kfifo->pread->pkt == 0){
        	//	 	 spin_unlock(&kfifo->lock);   
        			 return 0;
        		 }else{
        	//	 	spin_unlock(&kfifo->lock);   
        			 return 1;
        		 }
        		
        	}
        }
       //  spin_unlock(&kfifo->lock); 
        return 1;
    }
    return -1;
}
/* if time interval is less than a unit, the skb is inserted into the queue of kfifo_node, or
*  time interval is larger than a unit, one skb is inserted into one node with several intervals
*  
*/
int kfifo_enqueue(struct sk_buff *skb, struct kfifo_queue_data *kfifo,
		struct sk_buff **to_free)
{
	unsigned int skb_len = qdisc_pkt_len(skb);
	s64 skb_time = 0;
	
	int err = 0;
	//printk("kfifo enqueue a skb\n");
	if((skb_len+kfifo->slen)>kfifo->total_len){
	//	printk("kfifo enqueue is full\n");
		goto drop;
	}
	skb_time = (s64) psched_l2t_ns(&kfifo->rate, skb_len); // len to us
	//printk("skb_time is %u skb_len is %u\n",skb_time,skb_len);
	if(skb_time > kfifo->pwrite->left_time){ //current node can't include the skb
			//if(now_node->left_time != UNIT_TIME) // if now_node has some pkts.
			//	now_node = now_node->next;   //get next node
	//	printk("1: skb_time is larger than left_time skb_len=%u skb_time=%u left_time=%u\n",skb_len,skb_time,kfifo->pwrite->left_time);
		int step = skb_time/UNIT_TIME;    //get the all steps, if skb_time is less unit time, step is 0
		if(kfifo_wr_conflict(kfifo,step+1) == -1){
			//printk("1: kfifo wr conflict error\n");
			goto drop;
		}
		int cost_time=skb_time-step*UNIT_TIME;  //cost_time is skb_time - n*unit_time
		spin_lock(&kfifo->lock);
	
		if(kfifo_write_forward(kfifo, step+1, cost_time) < 0){
			//printk("1: skb time write forward failed\n");
			spin_unlock(&kfifo->lock);
			goto drop;
		}
	//	printk("1: kfifo insert  a skb step=%d\n",step+1);
		kfifo->pwrite->sleep_time += skb_time;
		kfifo->pwrite->pkt += 1;
		kfifo->pwrite->slen += skb_len;
		kfifo->slen += skb_len;
		kunit_enqueue_tail(skb,&kfifo->pwrite->kunit);
		//printk("1: kfifo enqueu skb tail\n");
		spin_unlock(&kfifo->lock);
	}else{
	//	printk("2: skb_time is less than left_time skb_len=%u skb_time=%u left_time=%u\n",skb_len,skb_time,kfifo->pwrite->left_time);
		if(kfifo_wr_conflict(kfifo,0) == -1){
		//	printk("2: kfifo wr conflick error \n");
			goto drop;
		}
		spin_lock(&kfifo->lock);
		kfifo->pwrite->left_time -= skb_time;
		kfifo->pwrite->sleep_time += skb_time;
		kfifo->pwrite->pkt += 1;
		kfifo->pwrite->slen += skb_len;
		kfifo->slen += skb_len;
		kunit_enqueue_tail(skb,&kfifo->pwrite->kunit);
	//	printk("2: kfifo enqueu skb tail\n");
		spin_unlock(&kfifo->lock);

	}
	//printk("kfifo enqueue return NET_XMIT_SUCCESS %u\n",kfifo->slen);
	return NET_XMIT_SUCCESS;
drop:
	//printk("kfifo enqueue return NET_XMIT_DROP\n");
	return NET_XMIT_DROP;
}

/*  if time interval is less than a unit, the skb is pushed out one by one, then watchdog sleep left time
*   if time interval is larger than a unit, the skb is pushed out and read point goes to the next read point of skb 
*
*/
struct sk_buff* kfifo_dequeue(struct kfifo_queue_data *kfifo)
{
	if(kfifo==NULL){
	//	printk("kfifo deque pkt is 0\n");
		return NULL;
	}
	 if(kfifo->pread->pkt == 0){
	 	if(kfifo->pread->sleep_time > 0)
	 		return NULL;
	 	if(kfifo_read_forward(kfifo) != 1)
	 		return NULL;
	 }

	struct sk_buff *skb;
	skb = kunit_dequeue_head(&kfifo->pread->kunit);
	if(skb){
        unsigned int skb_len = qdisc_pkt_len(skb);
       // s64 skb_time = 0;
       // skb_time = (s64) psched_l2t_ns(&kfifo->rate, skb_len);
		spin_lock(&kfifo->lock);
		kfifo->pread->pkt -= 1;
		kfifo->pread->slen -= skb_len;
		kfifo->slen -= skb_len;
		spin_unlock(&kfifo->lock);
		//printk("kfifo dequeue send skb %u, kfifo left_len %u\n",skb_len,kfifo->slen);
		return skb;

	}
	return NULL;
}

