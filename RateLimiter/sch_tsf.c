// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
// Time Smooth Filter (TSF) 
/*
 *Copyright (C) 2020-~
 *
 *
 * */
#include <net/ip.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/skbuff.h>
#include <net/netlink.h>
#include <net/sch_generic.h>
#include <net/pkt_sched.h>

#define DEBUG
#define DEIP "45.32.31.234"
#include "sch_kfifo.h"


struct tsf_sched_data {
	u64 ttime; // total time, us
	u64 tlen;  //total length, bytes
	s64 tokens; 
	s64	t_c;			/* Time check-point */
	struct kfifo_queue_data kfifo;  // refer to kfifo queue
	struct psched_ratecfg rate;
	u32		max_size;
	s64		buffer;		/* Token bucket depth/rate: MUST BE >= MTU/B */
	u32		limit;		/* Maximal length of backlog: bytes */
	struct Qdisc *sch;
	struct Qdisc	*qdisc;		/* Inner qdisc, default - bfifo queue */
	struct Qdisc	*test_qdisc;		/* Inner test qdisc, default - bfifo queue */
	struct qdisc_watchdog watchdog;
};


/* GSO packet is too big, segment it so that tbf can transmit
 * each segment in time
 */
static int tsf_segment(struct sk_buff *skb, struct Qdisc *sch,
		       struct sk_buff **to_free)
{
	struct tsf_sched_data *q = qdisc_priv(sch);
	struct sk_buff *segs, *nskb;
	netdev_features_t features = netif_skb_features(skb);
	unsigned int len = 0, prev_len = qdisc_pkt_len(skb);
	int ret, nb;

	//printk("tsf_segment start ...\n");
	segs = skb_gso_segment(skb, features & ~NETIF_F_GSO_MASK);

	if (IS_ERR_OR_NULL(segs)){
	//	printk("tsf_segment IS_ERR_OR_NULL ...\n");
		return qdisc_drop(skb, sch, to_free);
	}

	nb = 0;
	s64 now;
	s64 toks;
	while (segs) {
		nskb = segs->next;
		skb_mark_not_on_list(segs);
		qdisc_skb_cb(segs)->pkt_len = segs->len;
		len += segs->len;
		now = ktime_get_ns();
		toks = min_t(s64, now - q->t_c, q->buffer);
		toks += q->tokens;
		if (toks > q->buffer)
			toks = q->buffer;
		toks -= (s64) psched_l2t_ns(&q->rate, segs->len);
		if(toks >=0){
			ret = kfifo_enqueue(segs, &q->kfifo, to_free);
			q->t_c = now;
			q->tokens = toks;
		}else{
			ret = qdisc_enqueue(segs, q->qdisc, to_free);
		}
		if (ret != NET_XMIT_SUCCESS) {
			if (net_xmit_drop_count(ret))
				qdisc_qstats_drop(sch);
		} else {
			nb++;
		}
		segs = nskb;
	}
	sch->q.qlen += nb;
	if (nb > 1)
		qdisc_tree_reduce_backlog(sch, 1 - nb, prev_len - len);
	consume_skb(skb);
	return nb > 0 ? NET_XMIT_SUCCESS : NET_XMIT_DROP;
}
static char dest[16];
static int tsf_enqueue(struct sk_buff *skb, struct Qdisc *sch, struct sk_buff **to_free)
{
	struct tsf_sched_data *q = qdisc_priv(sch);
	struct sk_buff *pre_skb;

	unsigned int len = qdisc_pkt_len(skb);
	int ret;
	struct iphdr *iph = (struct iphdr *)skb_network_header(skb);

	 if(iph)
	 	snprintf(dest, 16, "%pI4", &iph->daddr);

#ifdef DEBUG
	if(strcmp(dest,DEIP) != 0){
		ret = qdisc_enqueue(skb, q->test_qdisc, to_free);
		if (ret != NET_XMIT_SUCCESS) {
			if (net_xmit_drop_count(ret))
			qdisc_qstats_drop(sch);
			return ret;
		}
		return NET_XMIT_SUCCESS;
	}else
#endif
	{
		if(qdisc_pkt_len(skb) > q->kfifo.mtu){
			printk("tsf enquque skb too length\n");
			if(skb_is_gso(skb) &&
			skb_gso_validate_mac_len(skb,q->kfifo.mtu))
				return tsf_segment(skb,sch,to_free);
		}
		s64 now;
		s64 toks;
		s64 toks2;
		unsigned int len = qdisc_pkt_len(skb);

		now = ktime_get_ns();
		toks = min_t(u64, now - q->t_c, q->buffer);
		toks += q->tokens;
		if (toks > q->buffer)
			toks = q->buffer;
		toks2 = toks;
		pre_skb = q->qdisc->ops->peek(q->qdisc); // first deal with queue;
		if(pre_skb){
			toks2 -= (s64) psched_l2t_ns(&q->rate, qdisc_pkt_len(pre_skb));
		}

		//dequeue qdis skb
		while(toks2 >= 0){
			pre_skb = qdisc_dequeue_peeked(q->qdisc);
			ret = kfifo_enqueue(pre_skb,&q->kfifo,to_free);
			if(ret != NET_XMIT_SUCCESS){
				if(net_xmit_drop_count(ret))
					qdisc_qstats_drop(sch);
				return ret;
			}
			toks = toks2;
            q->t_c = now;
			q->tokens = toks2;
			sch->qstats.backlog -= qdisc_pkt_len(pre_skb);
			sch->q.qlen--;

			pre_skb = q->qdisc->ops->peek(q->qdisc);
			if(pre_skb == NULL) break;
			toks2 -= (s64) psched_l2t_ns(&q->rate, qdisc_pkt_len(pre_skb));
		}
         
        //dequeue new skb
		toks -= (s64) psched_l2t_ns(&q->rate, len);
		if ((toks) >= 0) {
			ret = kfifo_enqueue(skb, &q->kfifo,to_free);
			q->t_c = now;
			q->tokens = toks;
			if(ret != NET_XMIT_SUCCESS){
				if(net_xmit_drop_count(ret))
					qdisc_qstats_drop(sch);
				return ret;
			}
			
		}else{
			ret = qdisc_enqueue(skb, q->qdisc, to_free);
			sch->qstats.backlog += len;
			sch->q.qlen++;
			if(pre_skb){
				qdisc_watchdog_schedule_ns(&q->watchdog, now -toks2);	
			}else{
				qdisc_watchdog_schedule_ns(&q->watchdog, now -toks);	
			}
			qdisc_qstats_overlimit(sch);
		}

		return NET_XMIT_SUCCESS;
	}
	return NET_XMIT_SUCCESS;
}
static void tsf_delay(struct tsf_sched_data *q, s64 sleep_time)
{
	s64 now,now2;
	now = ktime_get_ns();
	if(sleep_time > 1000000){
		qdisc_watchdog_schedule_ns(&q->watchdog, now +sleep_time);
	}else{
    	while(true){
     	   now2=ktime_get_ns();
       	 	if(now2-now >= sleep_time){
       	    	     return;
       		 }
    	}
    }
}
static struct sk_buff *tsf_dequeue(struct Qdisc *sch)
{
	struct tsf_sched_data *q = qdisc_priv(sch);
	struct sk_buff *skb;
	int err;
	
#ifdef DEBUG
	skb = q->qdisc->ops->peek(q->test_qdisc);
	if (skb) {
			struct iphdr *iph = (struct iphdr *)skb_network_header(skb);
			char source[16];
			snprintf(source, 16, "%pI4", &iph->saddr);
			if(strcmp(source,DEIP) != 0){
				//printk("tsf dequeue: skb is not dest, qdisc should be sent\n");
				skb = qdisc_dequeue_peeked(q->test_qdisc);
				if (unlikely(!skb))
					return NULL;
				return skb;
			}
	}
#endif
    skb = kfifo_dequeue(&q->kfifo); //push out an skb
    if (unlikely(!skb)){
    	if(q->kfifo.pread->pkt == 0 && q->kfifo.pread->sleep_time > 0){
    		u64 sleep_time = q->kfifo.pread->sleep_time;
    		spin_lock(&q->kfifo.lock);
			kfifo_finish_node(q->kfifo.pread);	
			spin_unlock(&q->kfifo.lock);
    		tsf_delay(q, sleep_time);
		}
		return NULL;
    }else
    	return skb;
    return NULL;
}

static void tsf_reset(struct Qdisc *sch)
{
	struct tsf_sched_data *q = qdisc_priv(sch);
	printk("tsf reset start\n");
	qdisc_watchdog_cancel(&q->watchdog);
//	qdisc_watchdog_init(&q->watchdog, sch);
}
struct tc_tsf_qopt{
	struct tc_ratespec rate;
	u64	time;
};

enum {
	TCA_TSF_UNSPEC,
	TCA_TSF_PARMS,
	TCA_TSF_TIME,
	TCA_TSF_RATE64,
	__TCA_TSF_MAX,
};

#define TCA_TSF_MAX (__TCA_TSF_MAX - 1)
static const struct nla_policy tsf_policy[TCA_TSF_MAX + 1] = {
	[TCA_TSF_PARMS]	= { .len = sizeof(struct tc_tsf_qopt) },
	[TCA_TSF_TIME] = { .type = NLA_U64 },
	[TCA_TSF_RATE64] = { .type = NLA_U64 },
};
static int tsf_change(struct Qdisc *sch, struct nlattr *opt, struct netlink_ext_ack *extack)
{

	int err;
	struct nlattr *tb[TCA_TSF_MAX+1];
	struct tc_tsf_qopt *qopt;
	struct psched_ratecfg rate;
//	u64 max_size;
	u64 rate64 = 0;
//	u64 time = 1;
	struct Qdisc *child = NULL;
	struct Qdisc *child_job = NULL;
	struct tsf_sched_data *q = qdisc_priv(sch);

	
	err = nla_parse_nested_deprecated(tb, TCA_TSF_MAX, opt, tsf_policy,
					  NULL);
	if (err < 0)
		return err;
	err = -EINVAL;
	
	if (tb[TCA_TSF_PARMS] == NULL)
	{
		printk("TCA_TSF_PARMS is null\n");
		return -EINVAL;
	}
	
	qopt = nla_data(tb[TCA_TSF_PARMS]);

	if(tb[TCA_TSF_TIME] == NULL)
	{
		printk("TCA_TSF_TIME is null\n");
		return -EINVAL;
	}

	if(tb[TCA_TSF_RATE64] == NULL)
	{
		printk("TCA_TSF_RATE64 is null\n");
		return -EINVAL;
	}

	sch_tree_lock(sch);
	if (tb[TCA_TSF_TIME])
		q->ttime = nla_get_u64(tb[TCA_TSF_TIME])/1000; //ms

	if (tb[TCA_TSF_RATE64])
		rate64 = nla_get_u64(tb[TCA_TSF_RATE64]);
	psched_ratecfg_precompute(&rate, &qopt->rate, rate64);

	q->tlen = rate64 * q->ttime/1000; // how many bytes should be put in the ring queue;
	q->buffer = rate64 / HZ;
	q->limit = q->buffer;
	memcpy(&q->rate, &rate, sizeof(struct psched_ratecfg));
	memcpy(&q->kfifo.rate, &rate, sizeof(struct psched_ratecfg));
	q->kfifo.mtu = min(READ_ONCE(sch->dev_queue->dev->mtu), IP_MAX_MTU);
//	printk("tsf init ttime=%u, rate64=%u, teln=%u,mtu=%u\n",q->ttime,rate64,q->tlen,q->kfifo.mtu);
#ifdef DEBUG
	if (q->tlen > 0) {
		child = fifo_create_dflt(sch, &bfifo_qdisc_ops, q->limit,
					 extack);
		if (IS_ERR(child)) {
			err = PTR_ERR(child);
			//printk("child is error\n");
			goto done;
		}
		/* child is fifo, no need to check for noop_qdisc */
		qdisc_hash_add(child, true);
	}
	if (child) {
		//printk("child is ok, qdisc put child\n");
		qdisc_tree_flush_backlog(q->test_qdisc);
		qdisc_put(q->test_qdisc);
		q->test_qdisc = child;
	}

#endif
	if (q->tlen > 0) {
		child_job = fifo_create_dflt(sch, &bfifo_qdisc_ops, q->limit,
					 extack);
		if (IS_ERR(child_job)) {
			err = PTR_ERR(child_job);
			//printk("child is error\n");
			goto done;
		}
		/* child is fifo, no need to check for noop_qdisc */
		qdisc_hash_add(child_job, true);
	}
	if (child_job) {
		//printk("child is ok, qdisc put child\n");
		qdisc_tree_flush_backlog(q->qdisc);
		qdisc_put(q->qdisc);
		q->qdisc = child_job;
	}
	err = kfifo_init(&q->kfifo,q->ttime,rate64);
	sch_tree_unlock(sch);
	if(err < 0){
		//printk("kfifo init failed\n");
		goto done;
	}
	//printk("kfifo init finish\n");

	err = 0;
	return err;
done:
	return err;
}

static int tsf_init(struct Qdisc *sch,struct nlattr *opt,
		struct netlink_ext_ack *extrack)
{
	struct tsf_sched_data *q = qdisc_priv(sch);

	qdisc_watchdog_init(&q->watchdog, sch);
	if (!opt)
		return -EINVAL;
	printk("tsf init start\n");
#ifdef DEBUG
	q->test_qdisc = &noop_qdisc;
#endif
	q->qdisc = &noop_qdisc;
	q->sch = sch;


	return tsf_change(sch, opt, extrack);
}

static void tsf_destroy(struct Qdisc *sch)
{

	struct tsf_sched_data *q = qdisc_priv(sch);
	printk("tsf_destroy start\n");
	qdisc_watchdog_cancel(&q->watchdog);
	if(q->kfifo.pbase){
		kfree(q->kfifo.pbase);
		printk("tsf_destroy free success\n");
	}
}

static int tsf_dump(struct Qdisc *sch, struct sk_buff *skb)
{
	struct tsf_sched_data *q = qdisc_priv(sch);
	struct nlattr *nest;
	struct tc_tsf_qopt opt;

	//printk("tsf dump start...\n");
	nest = nla_nest_start_noflag(skb, TCA_OPTIONS);
	if (nest == NULL)
		goto nla_put_failure;
	psched_ratecfg_getrate(&opt.rate, &q->kfifo.rate);
	opt.time = PSCHED_NS2TICKS(q->ttime);
	if (nla_put(skb, TCA_TSF_PARMS, sizeof(opt), &opt))
		goto nla_put_failure;
	if (q->kfifo.rate.rate_bytes_ps >= (1ULL << 32) &&
	    nla_put_u64_64bit(skb, TCA_TSF_RATE64, q->kfifo.rate.rate_bytes_ps,
			      TCA_TSF_RATE64))
		goto nla_put_failure;
	//printk("tsf dump end\n");
	return nla_nest_end(skb, nest);
nla_put_failure:
	printk("tsf dump failed\n");
	nla_nest_cancel(skb, nest);
	return -1;
}


static struct Qdisc_ops tsf_qdisc_ops __read_mostly = {
	.next		=	NULL,
	.cl_ops		= 	NULL,
	.id		=	"tsf",
	.priv_size	=	sizeof(struct tsf_sched_data),
	.enqueue	=	tsf_enqueue,
	.dequeue	=	tsf_dequeue,
	.peek		= 	qdisc_peek_dequeued,
	.init		=	tsf_init,
	.reset		=	tsf_reset,
	.destroy	=	tsf_destroy,
	.change		=	tsf_change,
	.dump		=	tsf_dump,
	.owner		=	THIS_MODULE,
};

static int __init tsf_module_init(void)
{

	return register_qdisc(&tsf_qdisc_ops);
}

static void __exit tsf_module_exit(void)
{
	unregister_qdisc(&tsf_qdisc_ops);
}
module_init(tsf_module_init);
module_exit(tsf_module_exit);
MODULE_LICENSE("GPL");
