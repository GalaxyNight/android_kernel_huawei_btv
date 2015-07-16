/*
 * net/tipc/link.h: Include file for TIPC link code
 *
 * Copyright (c) 1995-2006, 2013-2014, Ericsson AB
 * Copyright (c) 2004-2005, 2010-2011, Wind River Systems
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the names of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _TIPC_LINK_H
#define _TIPC_LINK_H

#include <net/genetlink.h>
#include "msg.h"
#include "node.h"

/* TIPC-specific error codes
*/
#define ELINKCONG EAGAIN	/* link congestion <=> resource unavailable */

/* Out-of-range value for link sequence numbers
 */
#define INVALID_LINK_SEQ 0x10000


/* Link endpoint receive states
 */
enum {
	TIPC_LINK_OPEN,
	TIPC_LINK_BLOCKED,
	TIPC_LINK_TUNNEL
};

/* Events occurring at packet reception or at timeout
 */
enum {
	TIPC_LINK_UP_EVT       = 1,
	TIPC_LINK_DOWN_EVT     = (1 << 1)
};

/* Starting value for maximum packet size negotiation on unicast links
 * (unless bearer MTU is less)
 */
#define MAX_PKT_DEFAULT 1500

struct tipc_stats {
	u32 sent_info;		/* used in counting # sent packets */
	u32 recv_info;		/* used in counting # recv'd packets */
	u32 sent_states;
	u32 recv_states;
	u32 sent_probes;
	u32 recv_probes;
	u32 sent_nacks;
	u32 recv_nacks;
	u32 sent_acks;
	u32 sent_bundled;
	u32 sent_bundles;
	u32 recv_bundled;
	u32 recv_bundles;
	u32 retransmitted;
	u32 sent_fragmented;
	u32 sent_fragments;
	u32 recv_fragmented;
	u32 recv_fragments;
	u32 link_congs;		/* # port sends blocked by congestion */
	u32 deferred_recv;
	u32 duplicates;
	u32 max_queue_sz;	/* send queue size high water mark */
	u32 accu_queue_sz;	/* used for send queue size profiling */
	u32 queue_sz_counts;	/* used for send queue size profiling */
	u32 msg_length_counts;	/* used for message length profiling */
	u32 msg_lengths_total;	/* used for message length profiling */
	u32 msg_length_profile[7]; /* used for msg. length profiling */
};

/**
 * struct tipc_link - TIPC link data structure
 * @addr: network address of link's peer node
 * @name: link name character string
 * @media_addr: media address to use when sending messages over link
 * @timer: link timer
 * @owner: pointer to peer node
 * @refcnt: reference counter for permanent references (owner node & timer)
 * @peer_session: link session # being used by peer end of link
 * @peer_bearer_id: bearer id used by link's peer endpoint
 * @bearer_id: local bearer id used by link
 * @tolerance: minimum link continuity loss needed to reset link [in ms]
 * @keepalive_intv: link keepalive timer interval
 * @abort_limit: # of unacknowledged continuity probes needed to reset link
 * @state: current state of link FSM
 * @silent_intv_cnt: # of timer intervals without any reception from peer
 * @proto_msg: template for control messages generated by link
 * @pmsg: convenience pointer to "proto_msg" field
 * @priority: current link priority
 * @net_plane: current link network plane ('A' through 'H')
 * @exec_mode: transmit/receive mode for link endpoint instance
 * @backlog_limit: backlog queue congestion thresholds (indexed by importance)
 * @exp_msg_count: # of tunnelled messages expected during link changeover
 * @reset_rcv_checkpt: seq # of last acknowledged message at time of link reset
 * @mtu: current maximum packet size for this link
 * @advertised_mtu: advertised own mtu when link is being established
 * @transmitq: queue for sent, non-acked messages
 * @backlogq: queue for messages waiting to be sent
 * @snt_nxt: next sequence number to use for outbound messages
 * @last_retransmitted: sequence number of most recently retransmitted message
 * @stale_count: # of identical retransmit requests made by peer
 * @rcv_nxt: next sequence number to expect for inbound messages
 * @deferred_queue: deferred queue saved OOS b'cast message received from node
 * @unacked_window: # of inbound messages rx'd without ack'ing back to peer
 * @inputq: buffer queue for messages to be delivered upwards
 * @namedq: buffer queue for name table messages to be delivered upwards
 * @next_out: ptr to first unsent outbound message in queue
 * @wakeupq: linked list of wakeup msgs waiting for link congestion to abate
 * @long_msg_seq_no: next identifier to use for outbound fragmented messages
 * @reasm_buf: head of partially reassembled inbound message fragments
 * @stats: collects statistics regarding link activity
 */
struct tipc_link {
	u32 addr;
	char name[TIPC_MAX_LINK_NAME];
	struct tipc_media_addr media_addr;
	struct timer_list timer;
	struct tipc_node *owner;
	struct kref ref;

	/* Management and link supervision data */
	u32 peer_session;
	u32 peer_bearer_id;
	u32 bearer_id;
	u32 tolerance;
	unsigned long keepalive_intv;
	u32 abort_limit;
	int state;
	u32 silent_intv_cnt;
	struct {
		unchar hdr[INT_H_SIZE];
		unchar body[TIPC_MAX_IF_NAME];
	} proto_msg;
	struct tipc_msg *pmsg;
	u32 priority;
	char net_plane;
	u8 exec_mode;
	u16 synch_point;

	/* Failover */
	u16 failover_pkts;
	u16 failover_checkpt;
	struct sk_buff *failover_skb;

	/* Max packet negotiation */
	u16 mtu;
	u16 advertised_mtu;

	/* Sending */
	struct sk_buff_head transmq;
	struct sk_buff_head backlogq;
	struct {
		u16 len;
		u16 limit;
	} backlog[5];
	u16 snd_nxt;
	u16 last_retransm;
	u32 window;
	u32 stale_count;

	/* Reception */
	u16 rcv_nxt;
	u32 rcv_unacked;
	struct sk_buff_head deferdq;
	struct sk_buff_head *inputq;
	struct sk_buff_head *namedq;

	/* Congestion handling */
	struct sk_buff_head wakeupq;

	/* Fragmentation/reassembly */
	struct sk_buff *reasm_buf;

	/* Statistics */
	struct tipc_stats stats;
};

struct tipc_port;

struct tipc_link *tipc_link_create(struct tipc_node *n,
				   struct tipc_bearer *b,
				   const struct tipc_media_addr *maddr,
				   struct sk_buff_head *inputq,
				   struct sk_buff_head *namedq);
void tipc_link_delete(struct tipc_link *link);
void tipc_link_delete_list(struct net *net, unsigned int bearer_id);
void tipc_link_failover_send_queue(struct tipc_link *l_ptr);
void tipc_link_dup_queue_xmit(struct tipc_link *l_ptr, struct tipc_link *dest);
void tipc_link_reset_fragments(struct tipc_link *l_ptr);
int tipc_link_is_up(struct tipc_link *l_ptr);
int tipc_link_is_active(struct tipc_link *l_ptr);
void tipc_link_purge_queues(struct tipc_link *l_ptr);
void tipc_link_purge_backlog(struct tipc_link *l);
void tipc_link_reset_all(struct tipc_node *node);
void tipc_link_reset(struct tipc_link *l_ptr);
int __tipc_link_xmit(struct net *net, struct tipc_link *link,
		     struct sk_buff_head *list);
int tipc_link_xmit(struct tipc_link *link,	struct sk_buff_head *list,
		   struct sk_buff_head *xmitq);
void tipc_link_proto_xmit(struct tipc_link *l_ptr, u32 msg_typ, int prob,
			  u32 gap, u32 tolerance, u32 priority);
void tipc_link_push_packets(struct tipc_link *l_ptr);
u32 tipc_link_defer_pkt(struct sk_buff_head *list, struct sk_buff *buf);
void tipc_link_set_queue_limits(struct tipc_link *l_ptr, u32 window);
void tipc_link_retransmit(struct tipc_link *l_ptr,
			  struct sk_buff *start, u32 retransmits);
struct sk_buff *tipc_skb_queue_next(const struct sk_buff_head *list,
				    const struct sk_buff *skb);

int tipc_nl_link_dump(struct sk_buff *skb, struct netlink_callback *cb);
int tipc_nl_link_get(struct sk_buff *skb, struct genl_info *info);
int tipc_nl_link_set(struct sk_buff *skb, struct genl_info *info);
int tipc_nl_link_reset_stats(struct sk_buff *skb, struct genl_info *info);
int tipc_nl_parse_link_prop(struct nlattr *prop, struct nlattr *props[]);
void link_prepare_wakeup(struct tipc_link *l);
int tipc_link_timeout(struct tipc_link *l, struct sk_buff_head *xmitq);

static inline u32 link_own_addr(struct tipc_link *l)
{
	return msg_prevnode(l->pmsg);
}

#endif
