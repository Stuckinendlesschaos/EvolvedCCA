#include <linux/module.h>
#include <net/tcp.h>

#ifdef _TCP_ELASTIC_H  
#define SCALE 100
#else
#define _TCP_ELASTIC_H
#define SCALE 1
#endif

struct elastic{
	u32	ai;
	u32	maxrtt;
	u32	currtt;
	u32 basertt;
};

static void elastic_init(struct sock *sk)
{
	struct elastic *ca = inet_csk_ca(sk);

	ca->ai = 0;
	ca->maxrtt = 0;
	//因为ca->currrtt在拥塞避免阶段是余数，在内核里若在某种条件下;
	//需要初始化elasticCCA,会出现异常。在初始化时用currtt=1来做替代;
	ca->currtt = 1;
	ca->basertt = 0x7fffffff;
}

static void elastic_cong_avoid(struct sock *sk, u32 ack, u32 acked)
{
	//AI过程 目的是为了随着目标BDP的大小步调弹性调整;
	struct tcp_sock *tp = tcp_sk(sk);
	struct elastic *ca = inet_csk_ca(sk);

	//约束发包速率
	if (!tcp_is_cwnd_limited(sk))
		return;

	//满开始，仍然是reno的cwnd=cwnd+1;
	if (tcp_in_slow_start(tp))
		tcp_slow_start(tp, acked);
	else {
		/* tp->snd_cwnd/ca->currtt是实时吞吐量,ca->maxrtt是收集到的一个epoch的最大RTT;
		*BDP=BtlBW*RTT/packet_size;
		*AI增量 过程 依赖 实时BDPmax ;
		*RTTmax是这个连接中的最大时延，RTTcur是当前的RTT-上一个ACK收集的RTT;
		*固有的buffer下排队时延是固定的，传播时延一定，即maxRTT有目标最大值;
		*BDP是有最大值的;
		*/
		u32 gap = int_sqrt(tp->snd_cwnd*SCALE*SCALE*ca->maxrtt/ca->currtt);
		//退出慢启动的acked,按照数量需要计算总的AI增量;
		//处在拥塞避免状态下的，调用时acked==1;
		gap *= acked;

		//AI过程的计数器增加gap的窗口量 cwnd+=gap/cwnd;
		tp->snd_cwnd_cnt += gap;
		//计数器大于上一个RTT的窗口时;cnt计数;
		//因为从慢启动阶段的AI增量有可能会有多个cwnd的总量，这里将if改成while;
		//为了应付慢启动退出的状况;
		while (tp->snd_cwnd_cnt >= tp->snd_cwnd*SCALE) {
			u32 remain = tp->snd_cwnd_cnt;
			
			tp->snd_cwnd_cnt -= tp->snd_cwnd*SCALE;
			//报错情况下，值退出，下次再进行重复操作
			//解决回环的问题
			if (tp->snd_cwnd_cnt > remain) {
				tp->snd_cwnd_cnt = remain;
				break;
			}
			tp->snd_cwnd++;
		}

	/* 	 if (tp->snd_cwnd_cnt >= tp->snd_cwnd) {        
	*				 线性增长计数器 >= 阈值; 
    *              if (tp->snd_cwnd < tp->snd_cwnd_clamp) 
	*				 如果窗口还没有达到阈值;
    *               tp->snd_cwnd++;                
	*				 那么++增大窗口;
    *              tp->snd_cwnd_cnt = 0;
    *              } else{
    *                      tp->snd_cwnd_cnt++;   
	*				 否则仅仅是增大线性递增计数器;
	*				}
	*/
	    }
	}
//ACKed时的RTT统计消息;

// struct ack_sample {
// 	__u32 pkts_acked;
// 	__s32 rtt_us;
// 	__u32 in_flight;
// } __attribute__((preserve_access_index));

static void elastic_rtt_calc(struct sock *sk, const struct ack_sample *sample)
{
	struct elastic *ca = inet_csk_ca(sk);
	u32 rtt;

	// RTT不可能为0或者baseRTT;
	rtt = sample->rtt_us + 1;

	// baseRTT 传播时延，带宽充沛的最小RTT值;
	if (rtt < ca->basertt)
		ca->basertt = rtt;

	//一个epoch实时RTT大于前面收集的最大RTT;
	//调整收集的最大RTT;
	if (rtt > ca->maxrtt || ca->maxrtt == 0)
		ca->maxrtt = rtt;


	ca->currtt = rtt;

}

static void tcp_elastic_event(struct sock *sk, enum tcp_ca_event event)
{
	struct elastic *ca = inet_csk_ca(sk);

	switch (event) {
	//只有事件是丢包，overflow的状态，采集到的maxRTT应该是严重的，重置为0;
	//后面再具体分析
	case CA_EVENT_LOSS:
		ca->maxrtt = 0;
	default:
		break;
	}
}

static struct tcp_congestion_ops tcp_elastic __read_mostly = {
	.init		= elastic_init,
	.ssthresh	= tcp_reno_ssthresh,
	.undo_cwnd	= tcp_reno_undo_cwnd,
	.cong_avoid	= elastic_cong_avoid,
	.pkts_acked	= elastic_rtt_calc,
	.cwnd_event	= tcp_elastic_event,
	.owner		= THIS_MODULE,
	.name		= "elastic"
};

static int __init elastic_register(void)
{
	BUILD_BUG_ON(sizeof(struct elastic) > ICSK_CA_PRIV_SIZE);
	return tcp_register_congestion_control(&tcp_elastic);
}

static void __exit elastic_unregister(void)
{
	tcp_unregister_congestion_control(&tcp_elastic);
}

module_init(elastic_register);
module_exit(elastic_unregister);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Elastic TCP");
