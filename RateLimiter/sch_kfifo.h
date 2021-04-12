/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __NET_SCHED_
#define __NET_SCHED_TSF_H

#include <linux/netdevice.h>
#include <linux/types.h>
#include <linux/rcupdate.h>
#include <linux/pkt_sched.h>
#include <linux/pkt_cls.h>
#include <linux/percpu.h>
#include <linux/dynamic_queue_limits.h>
#include <linux/list.h>
#include <linux/refcount.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <net/gen_stats.h>
#include <net/rtnetlink.h>
#include <net/flow_offload.h>


#define	UNIT_TIME 1000000 //1000000 ns per unit
/*
 *queue's unit struct
 * */
struct kunit_skb_head{
	struct sk_buff *head;
	struct sk_buff *tail;
};

/*ngx_queue to storage all apcket*/
typedef struct kfifo_node knode;
struct kfifo_node{
	u64 	last_time;
	__u32	pkt;
	u64	    left_time; //how many time(us) left for enqueueing us
	u64		sleep_time; //how many time the node should sleep  us
	knode 	*prev;
	knode	*next;
	u32     gain; //rate gain
	u32 	rate;
	u32     slen;  // how many bytes are used in the node
	u32     tlen;  // how many bytes should be put in the node
	struct kunit_skb_head	kunit;
	spinlock_t	lock;
};


struct kfifo_queue_data {
	u32	total_time; //ms
	u64	total_len;  //Bytest
	knode	*pbase;
	knode	*pread;
	knode	*pwrite;
	u32 gain;
	u64	slen;
	u32 stime;
	u32 mtu;   //current network mtu value
	struct psched_ratecfg rate;
	spinlock_t	lock;
};

int kfifo_read_forward(struct kfifo_queue_data *kfifo);
int kfifo_write_forward(struct kfifo_queue_data *kfifo, int step,u64 cost_time);
int kfifo_wr_conflict(struct kfifo_queue_data *kfifo,int step);
int kfifo_reset(struct kfifo_queue_data *kfifo, u32 time, u32 rate);
int kfifo_finish_node(struct kfifo_node *read);
int kfifo_enqueue(struct sk_buff *skb, struct kfifo_queue_data *kfifo,
		struct sk_buff **to_free);
int kfifo_init(struct kfifo_queue_data *kfifo, u32 time, u32 rate);
struct sk_buff* kfifo_dequeue(struct kfifo_queue_data *kfifo);
///////////////////////////////////////////////////////////////////
static inline void kunit_skb_head_init(struct kunit_skb_head *kuh)
{
	kuh->head = NULL;
	kuh->tail = NULL;
}


static inline void kunit_enqueue_tail(struct sk_buff *skb,
					struct kunit_skb_head *kuh)
{
	struct sk_buff *last = kuh->tail;

	if (last) {
		skb->next = NULL;
		last->next = skb;
		kuh->tail = skb;
	} else {
		kuh->tail = skb;
		kuh->head = skb;
	}
}

static inline int kunit_enqueue_head(struct sk_buff *skb, struct kunit_skb_head *kuh)
{
	skb->next = kuh->head;
	if(!kuh->head)
		kuh->tail = skb;
	kuh->head = skb;
	return 0;
}

static inline struct sk_buff *kunit_dequeue_head(struct kunit_skb_head *kuh)
{
	struct sk_buff *skb = kuh->head;
	if(likely(skb!= NULL)){
		kuh->head = skb->next;
//		kuh->qlen--;
		if (kuh->head == NULL)
			kuh->tail = NULL;
		return skb;
	}
	////return skb;
	return NULL;
	

}
#endif
