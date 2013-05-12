/*
 * Copyright (c) 2004-2011 Atheros Communications Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <linux/ip.h>
#include <linux/jiffies.h>
#include "core.h"
#include "debug.h"

static int ath6kl_wmi_sync_point(struct ath6kl_vif *vif);
static const s32 wmi_rate_tbl_22[][2] = {
    {1000, 1000},
    {2000, 2000},
    {5500, 5500},
    {11000, 11000},
    {6000, 6000},
    {9000, 9000},
    {12000, 12000},
    {18000, 18000},
    {24000, 24000},
    {36000, 36000},
    {48000, 48000},
    {54000, 54000},
    {6500, 7200},		/* HT 20, MCS 0 */
    {13000, 14400},
    {19500, 21700},
    {26000, 28900},
    {39000, 43300},
    {52000, 57800},
    {58500, 65000},
    {65000, 72200},
    {13000, 14400},	/* HT 20, MCS 8 */
    {26000, 28900},
    {39000, 43300},
    {52000, 57800},
    {78000, 86700},
    {104000, 115600},
    {117000, 130000},
    {130000, 144400},	/* HT 20, MCS 15 */
    {13500, 15000},	/*HT 40, MCS 0 */
    {27000, 30000},
    {40500, 45000},
    {54000, 60000},
    {81000, 90000},
    {108000, 120000},
    {121500, 135000},
    {135000, 150000},
    {27000, 30000},	/*HT 40, MCS 8 */
    {54000, 60000},
    {81000, 90000},
    {108000, 120000},
    {162000, 180000},
    {216000, 240000},
    {243000, 270000},
    {270000, 300000},	/*HT 40, MCS 15 */
    {0,0}};

static const s32 wmi_rate_tbl_11[][2] = {
	/* {W/O SGI, with SGI} */
	{1000, 1000},
	{2000, 2000},
	{5500, 5500},
	{11000, 11000},
	{6000, 6000},
	{9000, 9000},
	{12000, 12000},
	{18000, 18000},
	{24000, 24000},
	{36000, 36000},
	{48000, 48000},
	{54000, 54000},
	{6500, 7200},		/* HT 20, MCS 0 */
	{13000, 14400}, 
	{19500, 21700},
	{26000, 28900},
	{39000, 43300},
	{52000, 57800},
	{58500, 65000},
	{65000, 72200},
	{13500, 15000}, /*HT 40, MCS 0 */
	{27000, 30000},
	{40500, 45000},
	{54000, 60000},
	{81000, 90000},
	{108000, 120000},
	{121500, 135000},
	{135000, 150000},
	{0, 0}
};

#define ATH6KL_LEGACY_RATE_NUM 12
#define ATH6KL_HT_RATE_MCS_RANK_11 8
#define ATH6KL_HT_RATE_MCS_RANK_22 16

/* 802.1d to AC mapping. Refer pg 57 of WMM-test-plan-v1.2 */
static const u8 up_to_ac[] = {
	WMM_AC_BE,
	WMM_AC_BK,
	WMM_AC_BK,
	WMM_AC_BE,
	WMM_AC_VI,
	WMM_AC_VI,
	WMM_AC_VO,
	WMM_AC_VO,
};

static s32 ath6kl_wmi_parse_rate_tbl(s8 rate_index, u8 *rate_sgi, u8 *rate_mcs, enum wlan_spatial_stream_state ss_state)
{
	u8 sgi = 0;
	u8 idx = 0;
	s32 bit_rate = 0;
	u16 mcs_rank = 0;

	sgi = (rate_index & 0x80)? 1: 0;
	idx = (rate_index & 0x7F);

	switch (ss_state) {
	case WLAN_SPATIAL_STREAM_11:
		bit_rate = wmi_rate_tbl_11[idx][sgi];
		mcs_rank = ATH6KL_HT_RATE_MCS_RANK_11;
		break;
	case WLAN_SPATIAL_STREAM_22:
		bit_rate = wmi_rate_tbl_22[idx][sgi];
		mcs_rank = ATH6KL_HT_RATE_MCS_RANK_22;
		break;
	default:
		return 0;
		break;
	}

	if (idx >= ATH6KL_LEGACY_RATE_NUM + mcs_rank) {
		/* HT 40 2x2 rate */
		*rate_sgi = sgi;
		*rate_mcs = idx - ATH6KL_LEGACY_RATE_NUM - mcs_rank;
	} else if (idx >= ATH6KL_LEGACY_RATE_NUM) {
		*rate_sgi = sgi;
		*rate_mcs = idx - ATH6KL_LEGACY_RATE_NUM;
	} else {
		*rate_sgi = 0;
		*rate_mcs = 0;
	}

	return bit_rate;
}

void ath6kl_wmi_set_control_ep(struct wmi *wmi, enum htc_endpoint_id ep_id)
{
	if (WARN_ON(ep_id == ENDPOINT_UNUSED || ep_id >= ENDPOINT_MAX))
		return;

	wmi->ep_id = ep_id;
}

enum htc_endpoint_id ath6kl_wmi_get_control_ep(struct wmi *wmi)
{
	return wmi->ep_id;
}

/*  Performs DIX to 802.3 encapsulation for transmit packets.
 *  Assumes the entire DIX header is contigous and that there is
 *  enough room in the buffer for a 802.3 mac header and LLC+SNAP headers.
 */
int ath6kl_wmi_dix_2_dot3(struct wmi *wmi, struct sk_buff *skb)
{
	struct ath6kl_llc_snap_hdr *llc_hdr;
	struct ethhdr *eth_hdr;
	size_t new_len;
	__be16 type;
	u8 *datap;
	u16 size;

	if (WARN_ON(skb == NULL))
		return -EINVAL;

	size = sizeof(struct ath6kl_llc_snap_hdr) + sizeof(struct wmi_data_hdr);
	if (skb_headroom(skb) < size)
		return -ENOMEM;

	eth_hdr = (struct ethhdr *) skb->data;
	type = eth_hdr->h_proto;

	if (!is_ethertype(be16_to_cpu(type))) {
		ath6kl_dbg(ATH6KL_DBG_WMI,
			"%s: pkt is already in 802.3 format\n", __func__);
		return 0;
	}

	new_len = skb->len - sizeof(*eth_hdr) + sizeof(*llc_hdr);

	skb_push(skb, sizeof(struct ath6kl_llc_snap_hdr));
	datap = skb->data;

	eth_hdr->h_proto = cpu_to_be16(new_len);

	memcpy(datap, eth_hdr, sizeof(*eth_hdr));

	llc_hdr = (struct ath6kl_llc_snap_hdr *)(datap + sizeof(*eth_hdr));
	llc_hdr->dsap = 0xAA;
	llc_hdr->ssap = 0xAA;
	llc_hdr->cntl = 0x03;
	llc_hdr->org_code[0] = 0x0;
	llc_hdr->org_code[1] = 0x0;
	llc_hdr->org_code[2] = 0x0;
	llc_hdr->eth_type = type;

	return 0;
}

static int ath6kl_wmi_meta_add(struct wmi *wmi, struct sk_buff *skb,
			       u8 *version, void *tx_meta_info)
{
	struct wmi_tx_meta_v1 *v1;
	struct wmi_tx_meta_v2 *v2;

	if (WARN_ON(skb == NULL || version == NULL))
		return -EINVAL;

	switch (*version) {
	case WMI_META_VERSION_1:
		skb_push(skb, WMI_MAX_TX_META_SZ);
		v1 = (struct wmi_tx_meta_v1 *) skb->data;
		v1->pkt_id = 0;
		v1->rate_plcy_id = 0;
		*version = WMI_META_VERSION_1;
		break;
	case WMI_META_VERSION_2:
		skb_push(skb, WMI_MAX_TX_META_SZ);
		v2 = (struct wmi_tx_meta_v2 *) skb->data;
		memcpy(v2, (struct wmi_tx_meta_v2 *) tx_meta_info,
		       sizeof(struct wmi_tx_meta_v2));
		break;
	}

	return 0;
}

int ath6kl_wmi_data_hdr_add(struct ath6kl_vif *vif, struct sk_buff *skb,
			    u8 msg_type, u32 flags,
			    enum wmi_data_hdr_data_type data_type,
			    u8 meta_ver, void *tx_meta_info)
{
	struct wmi_data_hdr *data_hdr;
	int ret;

	if (WARN_ON(skb == NULL))
		return -EINVAL;

	ret = ath6kl_wmi_meta_add(vif->ar->wmi, skb, &meta_ver, tx_meta_info);
	if (ret)
		return ret;

	skb_push(skb, sizeof(struct wmi_data_hdr));

	data_hdr = (struct wmi_data_hdr *)skb->data;
	memset(data_hdr, 0, sizeof(struct wmi_data_hdr));

	data_hdr->info = msg_type << WMI_DATA_HDR_MSG_TYPE_SHIFT;
	data_hdr->info |= data_type << WMI_DATA_HDR_DATA_TYPE_SHIFT;
	data_hdr->info2 = cpu_to_le16(meta_ver << WMI_DATA_HDR_META_SHIFT);
	data_hdr->info3 = cpu_to_le16(vif->device_index & WMI_DATA_HDR_IF_IDX_MASK);

	if (flags & WMI_DATA_HDR_FLAGS_MORE) {
		WMI_DATA_HDR_SET_MORE_BIT(data_hdr);
	}

	if (flags & WMI_DATA_HDR_FLAGS_EOSP) {
		WMI_DATA_HDR_SET_EOSP_BIT(data_hdr);
	}

	if (flags & WMI_DATA_HDR_FLAGS_TRIGGERED) {
		WMI_DATA_HDR_SET_TRIGGERED_BIT(data_hdr);
	}

	if (flags & WMI_DATA_HDR_FLAGS_PSPOLLED) {
		WMI_DATA_HDR_SET_PSPOLLED_BIT(data_hdr);
	}

	return 0;
}

u8 ath6kl_wmi_determine_user_priority(u8 *pkt, u32 layer2_pri)
{
	struct iphdr *ip_hdr = (struct iphdr *) pkt;
	u8 ip_pri;

	/*
	 * Determine IPTOS priority
	 *
	 * IP-TOS - 8bits
	 *          : DSCP(6-bits) ECN(2-bits)
	 *          : DSCP - P2 P1 P0 X X X
	 * where (P2 P1 P0) form 802.1D
	 */
	ip_pri = ip_hdr->tos >> 5;
	ip_pri &= 0x7;

	if ((layer2_pri & 0x7) > ip_pri)
		return (u8) layer2_pri & 0x7;
	else
		return ip_pri;
}

u8 ath6kl_wmi_convert_user_priority_to_traffic_class(u8 priority)
{
    return  (up_to_ac[priority & 0x7]);
}

int ath6kl_wmi_implicit_create_pstream(struct ath6kl_vif *vif, struct sk_buff *skb,
				       u32 layer2_priority, bool wmm_enabled,
				       u8 *ac)
{
	static unsigned long last_create_jiffies = 0;
	struct wmi_data_hdr *data_hdr;
	struct ath6kl_llc_snap_hdr *llc_hdr;
	struct wmi_create_pstream_cmd cmd;
	struct wmi *wmi = vif->ar->wmi;
	u32 meta_size, hdr_size;
	u16 ip_type = IP_ETHERTYPE;
	u8 stream_exist, usr_pri;
	u8 traffic_class = WMM_AC_BE;
	u8 *datap;

	if (WARN_ON(skb == NULL))
		return -EINVAL;

	datap = skb->data;
	data_hdr = (struct wmi_data_hdr *) datap;

	meta_size = ((le16_to_cpu(data_hdr->info2) >> WMI_DATA_HDR_META_SHIFT) &
		     WMI_DATA_HDR_META_MASK) ? WMI_MAX_TX_META_SZ : 0;

	if (!wmm_enabled) {
		/* If WMM is disabled all traffic goes as BE traffic */
		usr_pri = 0;
	} else {
		hdr_size = sizeof(struct ethhdr);

		llc_hdr = (struct ath6kl_llc_snap_hdr *)(datap +
							 sizeof(struct
								wmi_data_hdr) +
							 meta_size + hdr_size);

		if (llc_hdr->eth_type == htons(ip_type)) {
			/*
			 * Extract the endpoint info from the TOS field
			 * in the IP header.
			 */
			usr_pri =
			   ath6kl_wmi_determine_user_priority(((u8 *) llc_hdr) +
					sizeof(struct ath6kl_llc_snap_hdr),
					layer2_priority);
		} else
			usr_pri = layer2_priority & 0x7;
	}

	/* workaround for WMM S5 */
	if ((wmi->traffic_class == WMM_AC_VI) &&
	    ((usr_pri == 5) || (usr_pri == 4)))
		usr_pri = 1;

	/* Convert user priority to traffic class */
	traffic_class = up_to_ac[usr_pri & 0x7];

	wmi_data_hdr_set_up(data_hdr, usr_pri);

	spin_lock_bh(&wmi->lock);
	stream_exist = wmi->fat_pipe_exist;
	spin_unlock_bh(&wmi->lock);

	if (!(stream_exist & (1 << traffic_class)) &&
	    (time_after(jiffies, last_create_jiffies + HZ))) {
		memset(&cmd, 0, sizeof(cmd));
		cmd.traffic_class = traffic_class;
		cmd.user_pri = usr_pri;
		cmd.inactivity_int =
			cpu_to_le32(WMI_IMPLICIT_PSTREAM_INACTIVITY_INT);
		/* Implicit streams are created with TSID 0xFF */
		cmd.tsid = WMI_IMPLICIT_PSTREAM;
		ath6kl_wmi_create_pstream_cmd(vif, &cmd);
		last_create_jiffies = jiffies;
	}

	*ac = traffic_class;

	return 0;
}

int ath6kl_wmi_dot11_hdr_remove(struct wmi *wmi, struct sk_buff *skb)
{
	struct ieee80211_hdr_3addr *pwh, wh;
	struct ath6kl_llc_snap_hdr *llc_hdr;
	struct ethhdr eth_hdr;
	u32 hdr_size;
	u8 *datap;
	__le16 sub_type;

	if (WARN_ON(skb == NULL))
		return -EINVAL;

	datap = skb->data;
	pwh = (struct ieee80211_hdr_3addr *) datap;

	sub_type = pwh->frame_control & cpu_to_le16(IEEE80211_FCTL_STYPE);

	memcpy((u8 *) &wh, datap, sizeof(struct ieee80211_hdr_3addr));

	/* Strip off the 802.11 header */
	if (sub_type == cpu_to_le16(IEEE80211_STYPE_QOS_DATA)) {
		hdr_size = roundup(sizeof(struct ieee80211_qos_hdr),
				   sizeof(u32));
		skb_pull(skb, hdr_size);
	} else if (sub_type == cpu_to_le16(IEEE80211_STYPE_DATA))
		skb_pull(skb, sizeof(struct ieee80211_hdr_3addr));

	datap = skb->data;
	llc_hdr = (struct ath6kl_llc_snap_hdr *)(datap);

	memset(&eth_hdr, 0, sizeof(eth_hdr));
	eth_hdr.h_proto = llc_hdr->eth_type;

	switch ((le16_to_cpu(wh.frame_control)) &
		(IEEE80211_FCTL_FROMDS | IEEE80211_FCTL_TODS)) {
	case 0:
		memcpy(eth_hdr.h_dest, wh.addr1, ETH_ALEN);
		memcpy(eth_hdr.h_source, wh.addr2, ETH_ALEN);
		break;
	case IEEE80211_FCTL_TODS:
		memcpy(eth_hdr.h_dest, wh.addr3, ETH_ALEN);
		memcpy(eth_hdr.h_source, wh.addr2, ETH_ALEN);
		break;
	case IEEE80211_FCTL_FROMDS:
		memcpy(eth_hdr.h_dest, wh.addr1, ETH_ALEN);
		memcpy(eth_hdr.h_source, wh.addr3, ETH_ALEN);
		break;
	case IEEE80211_FCTL_FROMDS | IEEE80211_FCTL_TODS:
		break;
	}

	skb_pull(skb, sizeof(struct ath6kl_llc_snap_hdr));
	skb_push(skb, sizeof(eth_hdr));

	datap = skb->data;

	memcpy(datap, &eth_hdr, sizeof(eth_hdr));

	return 0;
}

/*
 * Performs 802.3 to DIX encapsulation for received packets.
 * Assumes the entire 802.3 header is contigous.
 */
int ath6kl_wmi_dot3_2_dix(struct sk_buff *skb)
{
	struct ath6kl_llc_snap_hdr *llc_hdr;
	struct ethhdr eth_hdr;
	u8 *datap;

	if (WARN_ON(skb == NULL))
		return -EINVAL;

	datap = skb->data;

	memcpy(&eth_hdr, datap, sizeof(eth_hdr));

	llc_hdr = (struct ath6kl_llc_snap_hdr *) (datap + sizeof(eth_hdr));
	eth_hdr.h_proto = llc_hdr->eth_type;

	skb_pull(skb, sizeof(struct ath6kl_llc_snap_hdr));
	datap = skb->data;

	memcpy(datap, &eth_hdr, sizeof(eth_hdr));

	return 0;
}

int ath6kl_wmi_data_hdr_remove(struct ath6kl_vif *vif, struct sk_buff *skb)
{
	if (WARN_ON(skb == NULL))
		return -EINVAL;

	skb_pull(skb, sizeof(struct wmi_data_hdr));

	return 0;
}

static int ath6kl_wmi_tx_complete_event_rx(u8 *datap, int len)
{
	struct tx_complete_msg_v1 *msg_v1;
	struct wmi_tx_complete_event *evt;
	int index;
	u16 size;

	evt = (struct wmi_tx_complete_event *) datap;

	ath6kl_dbg(ATH6KL_DBG_WMI, "comp: %d %d %d\n",
		   evt->num_msg, evt->msg_len, evt->msg_type);

	if (!AR_DBG_LVL_CHECK(ATH6KL_DBG_WMI))
		return 0;

	for (index = 0; index < evt->num_msg; index++) {
		size = sizeof(struct wmi_tx_complete_event) +
		    (index * sizeof(struct tx_complete_msg_v1));
		msg_v1 = (struct tx_complete_msg_v1 *)(datap + size);

		ath6kl_dbg(ATH6KL_DBG_WMI, "msg: %d %d %d %d\n",
			   msg_v1->status, msg_v1->pkt_id,
			   msg_v1->rate_idx, msg_v1->ack_failures);
	}

	return 0;
}

static int ath6kl_wmi_remain_on_chnl_event_rx(struct ath6kl_vif *vif, u8 *datap,
					      int len)
{
	struct wmi_remain_on_chnl_event *ev;
	u32 freq;
	u32 dur;
	struct ieee80211_channel *chan;

	if (len < sizeof(*ev))
		return -EINVAL;

	ev = (struct wmi_remain_on_chnl_event *) datap;
	freq = le32_to_cpu(ev->freq);
	dur = le32_to_cpu(ev->duration);
	ath6kl_dbg(ATH6KL_DBG_WMI, "remain_on_chnl: freq=%u dur=%u\n",
		   freq, dur);
	chan = ieee80211_get_channel(vif->wdev->wiphy, freq);
	if (!chan) {
		ath6kl_dbg(ATH6KL_DBG_WMI, "remain_on_chnl: Unknown channel "
			   "(freq=%u)\n", freq);
		return -EINVAL;
	}
	cfg80211_ready_on_channel_ath6kl(vif->net_dev, 1, chan, NL80211_CHAN_NO_HT,
				  dur, GFP_ATOMIC);

	return 0;
}

static int ath6kl_wmi_cancel_remain_on_chnl_event_rx(struct ath6kl_vif *vif,
						     u8 *datap, int len)
{
	struct wmi_cancel_remain_on_chnl_event *ev;
	u32 freq;
	u32 dur;
	struct ieee80211_channel *chan;

	if (len < sizeof(*ev))
		return -EINVAL;

	ev = (struct wmi_cancel_remain_on_chnl_event *) datap;
	freq = le32_to_cpu(ev->freq);
	dur = le32_to_cpu(ev->duration);
	ath6kl_dbg(ATH6KL_DBG_WMI, "cancel_remain_on_chnl: freq=%u dur=%u "
		   "status=%u\n", freq, dur, ev->status);
	chan = ieee80211_get_channel(vif->wdev->wiphy, freq);
	if (!chan) {
		ath6kl_dbg(ATH6KL_DBG_WMI, "cancel_remain_on_chnl: Unknown "
			   "channel (freq=%u)\n", freq);
		return -EINVAL;
	}
	cfg80211_remain_on_channel_expired_ath6kl(vif->net_dev, 1, chan,
					   NL80211_CHAN_NO_HT, GFP_ATOMIC);

	return 0;
}

static int ath6kl_wmi_tx_status_event_rx(struct ath6kl_vif *vif, u8 *datap, int len)
{
	struct wmi_tx_status_event *ev;
	u32 id;
	struct wmi *wmi = vif->ar->wmi;

	if (len < sizeof(*ev))
		return -EINVAL;

	ev = (struct wmi_tx_status_event *) datap;
	id = le32_to_cpu(ev->id);
	ath6kl_dbg(ATH6KL_DBG_WMI, "tx_status: id=%x ack_status=%u\n",
		   id, ev->ack_status);

	if (!list_empty(&wmi->mgmt_tx_frame)) {
		struct wmi_mgmt_tx_frame *mgmt_tx_frame, *tmp;
		int found = 0, seq = 0;

		list_for_each_entry_safe(mgmt_tx_frame, tmp, &wmi->mgmt_tx_frame, list) {
			seq++;
			if (mgmt_tx_frame->mgmt_tx_frame_idx == id) {
				list_del(&mgmt_tx_frame->list);
				found = 1;
				cfg80211_mgmt_tx_status(vif->net_dev, id,
					mgmt_tx_frame->mgmt_tx_frame,
					mgmt_tx_frame->mgmt_tx_frame_len,
					!!ev->ack_status, GFP_ATOMIC);
				kfree(mgmt_tx_frame->mgmt_tx_frame);
				kfree(mgmt_tx_frame);
				break;
			}
		}

		if (!found) {
			ath6kl_err("tx_status_report: "
				"unexpected report?\n");
		} else if (seq > 1) {
			ath6kl_err("tx_status_report: "
				"not by order, seq = %d\n", seq);
		}
	}

	return 0;
}

static int ath6kl_wmi_rx_probe_req_event_rx(struct ath6kl_vif *vif, u8 *datap, int len)
{
	struct wmi_p2p_rx_probe_req_event *ev;
	u32 freq;
	u16 dlen;

	if (len < sizeof(*ev))
		return -EINVAL;

	ev = (struct wmi_p2p_rx_probe_req_event *) datap;
	freq = le32_to_cpu(ev->freq);
	dlen = le16_to_cpu(ev->len);
	if (datap + len < ev->data + dlen) {
		ath6kl_err("invalid wmi_p2p_rx_probe_req_event: "
			   "len=%d dlen=%u\n", len, dlen);
		return -EINVAL;
	}
	ath6kl_dbg(ATH6KL_DBG_WMI, "rx_probe_req: len=%u freq=%u "
		   "probe_req_report=%d\n",
		   dlen, freq, vif->probe_req_report);

	if (vif->probe_req_report || vif->nw_type == AP_NETWORK)
		cfg80211_rx_mgmt(vif->net_dev, freq, ev->data, dlen, GFP_ATOMIC);

	return 0;
}

static int ath6kl_wmi_p2p_capabilities_event_rx(u8 *datap, int len)
{
	struct wmi_p2p_capabilities_event *ev;
	u16 dlen;

	if (len < sizeof(*ev))
		return -EINVAL;

	ev = (struct wmi_p2p_capabilities_event *) datap;
	dlen = le16_to_cpu(ev->len);
	ath6kl_dbg(ATH6KL_DBG_WMI, "p2p_capab: len=%u\n", dlen);

	return 0;
}

static int ath6kl_wmi_rx_action_event_rx(struct ath6kl_vif *vif, u8 *datap, int len)
{
	struct wmi_rx_action_event *ev;
	u32 freq;
	u16 dlen;

	if (len < sizeof(*ev))
		return -EINVAL;

	ev = (struct wmi_rx_action_event *) datap;
	freq = le32_to_cpu(ev->freq);
	dlen = le16_to_cpu(ev->len);
	if (datap + len < ev->data + dlen) {
		ath6kl_err("invalid wmi_rx_action_event: "
			   "len=%d dlen=%u\n", len, dlen);
		return -EINVAL;
	}
	ath6kl_dbg(ATH6KL_DBG_WMI, "rx_action: len=%u freq=%u\n", dlen, freq);
	cfg80211_rx_mgmt(vif->net_dev, freq, ev->data, dlen, GFP_ATOMIC);

	return 0;
}

static int ath6kl_wmi_p2p_info_event_rx(u8 *datap, int len)
{
	struct wmi_p2p_info_event *ev;
	u32 flags;
	u16 dlen;

	if (len < sizeof(*ev))
		return -EINVAL;

	ev = (struct wmi_p2p_info_event *) datap;
	flags = le32_to_cpu(ev->info_req_flags);
	dlen = le16_to_cpu(ev->len);
	ath6kl_dbg(ATH6KL_DBG_WMI, "p2p_info: flags=%x len=%d\n", flags, dlen);

	if (flags & P2P_FLAG_CAPABILITIES_REQ) {
		struct wmi_p2p_capabilities *cap;
		if (dlen < sizeof(*cap))
			return -EINVAL;
		cap = (struct wmi_p2p_capabilities *) ev->data;
		ath6kl_dbg(ATH6KL_DBG_WMI, "p2p_info: GO Power Save = %d\n",
			   cap->go_power_save);
	}

	if (flags & P2P_FLAG_MACADDR_REQ) {
		struct wmi_p2p_macaddr *mac;
		if (dlen < sizeof(*mac))
			return -EINVAL;
		mac = (struct wmi_p2p_macaddr *) ev->data;
		ath6kl_dbg(ATH6KL_DBG_WMI, "p2p_info: MAC Address = %pM\n",
			   mac->mac_addr);
	}

	if (flags & P2P_FLAG_HMODEL_REQ) {
		struct wmi_p2p_hmodel *mod;
		if (dlen < sizeof(*mod))
			return -EINVAL;
		mod = (struct wmi_p2p_hmodel *) ev->data;
		ath6kl_dbg(ATH6KL_DBG_WMI, "p2p_info: P2P Model = %d (%s)\n",
			   mod->p2p_model,
			   mod->p2p_model ? "host" : "firmware");
	}
	return 0;
}

static inline struct sk_buff *ath6kl_wmi_get_new_buf(u32 size)
{
	struct sk_buff *skb;

	skb = ath6kl_buf_alloc(size);
	if (!skb)
		return NULL;

	skb_put(skb, size);
	if (size)
		memset(skb->data, 0, size);

	return skb;
}

/* Send a "simple" wmi command -- one with no arguments */
static int ath6kl_wmi_simple_cmd(struct ath6kl_vif *vif, enum wmi_cmd_id cmd_id)
{
	struct sk_buff *skb;
	int ret;

	skb = ath6kl_wmi_get_new_buf(0);
	if (!skb)
		return -ENOMEM;

	ret = ath6kl_wmi_cmd_send(vif, skb, cmd_id, NO_SYNC_WMIFLAG);

	return ret;
}

static int ath6kl_wmi_ready_event_rx(struct ath6kl_vif *vif, u8 *datap, int len)
{
	struct wmi_ready_event_2 *ev = (struct wmi_ready_event_2 *) datap;

	if (len < sizeof(struct wmi_ready_event_2))
		return -EINVAL;

	vif->ar->wmi->ready = true;
	ath6kl_ready_event(vif->ar->wmi->parent_dev, ev->mac_addr,
			   le32_to_cpu(ev->sw_version),
			   le32_to_cpu(ev->abi_version));

	return 0;
}

static int ath6kl_wmi_connect_event_rx(struct ath6kl_vif *vif, u8 *datap, int len)
{
	struct wmi_connect_event *ev;
	u8 *pie, *peie;

	if (len < sizeof(struct wmi_connect_event))
		return -EINVAL;

	ev = (struct wmi_connect_event *) datap;

	if (vif->nw_type == AP_NETWORK) {
		/* AP mode start/STA connected event */
		struct net_device *dev = vif->net_dev;
		if (memcmp(dev->dev_addr, ev->u.ap_bss.bssid, ETH_ALEN) == 0) {
			ath6kl_dbg(ATH6KL_DBG_WMI, "%s: freq %d bssid %pM "
				   "(AP started)\n",
				   __func__, le16_to_cpu(ev->u.ap_bss.ch),
				   ev->u.ap_bss.bssid);
			ath6kl_connect_ap_mode_bss(
				vif, le16_to_cpu(ev->u.ap_bss.ch));
		} else {
			ath6kl_dbg(ATH6KL_DBG_WMI, "%s: aid %u mac_addr %pM "
				   "auth=%u keymgmt=%u cipher=%u apsd_info=%u "
				   "(STA connected)\n",
				   __func__, ev->u.ap_sta.aid,
				   ev->u.ap_sta.mac_addr,
				   ev->u.ap_sta.auth,
				   ev->u.ap_sta.keymgmt,
				   le16_to_cpu(ev->u.ap_sta.cipher),
				   ev->u.ap_sta.apsd_info);
			ath6kl_connect_ap_mode_sta(
				vif, ev->u.ap_sta.aid, ev->u.ap_sta.mac_addr,
				ev->u.ap_sta.keymgmt,
				le16_to_cpu(ev->u.ap_sta.cipher),
				ev->u.ap_sta.auth, ev->assoc_req_len,
				ev->assoc_info + ev->beacon_ie_len,
				ev->u.ap_sta.apsd_info);
		}
		return 0;
	}

	/* STA/IBSS mode connection event */


	ath6kl_dbg(ATH6KL_DBG_WMI, "%s: freq %d bssid %pM\n",
		    __func__, le16_to_cpu(ev->u.sta.ch), ev->u.sta.bssid);

	/* Start of assoc rsp IEs */
	pie = ev->assoc_info + ev->beacon_ie_len +
	      ev->assoc_req_len + (sizeof(u16) * 3); /* capinfo, status, aid */

	/* End of assoc rsp IEs */
	peie = ev->assoc_info + ev->beacon_ie_len + ev->assoc_req_len +
	    ev->assoc_resp_len;

	while (pie < peie) {
		switch (*pie) {
		case WLAN_EID_VENDOR_SPECIFIC:
			if (pie[1] > 3 && pie[2] == 0x00 && pie[3] == 0x50 &&
			    pie[4] == 0xf2 && pie[5] == WMM_OUI_TYPE) {
				/* WMM OUT (00:50:F2) */
				if (pie[1] > 5
				    && pie[6] == WMM_PARAM_OUI_SUBTYPE)
					vif->ar->wmi->is_wmm_enabled = true;
			}
			break;
		}

		if (vif->ar->wmi->is_wmm_enabled)
			break;

		pie += pie[1] + 2;
	}

	ath6kl_connect_event(vif, le16_to_cpu(ev->u.sta.ch),
			     ev->u.sta.bssid,
			     le16_to_cpu(ev->u.sta.listen_intvl),
			     le16_to_cpu(ev->u.sta.beacon_intvl),
			     le32_to_cpu(ev->u.sta.nw_type),
			     ev->beacon_ie_len, ev->assoc_req_len,
			     ev->assoc_resp_len, ev->assoc_info);

	return 0;
}

static int ath6kl_wmi_disconnect_event_rx(struct ath6kl_vif *vif, u8 *datap, int len)
{
	struct wmi_disconnect_event *ev;
	vif->ar->wmi->traffic_class = 100;

	if (len < sizeof(struct wmi_disconnect_event))
		return -EINVAL;

	ev = (struct wmi_disconnect_event *) datap;

	vif->ar->wmi->is_wmm_enabled = false;
	vif->ar->wmi->pair_crypto_type = NONE_CRYPT;
	vif->ar->wmi->grp_crypto_type = NONE_CRYPT;

	ath6kl_disconnect_event(vif, ev->disconn_reason,
				ev->bssid, ev->assoc_resp_len, ev->assoc_info,
				le16_to_cpu(ev->proto_reason_status));

	return 0;
}

static int ath6kl_wmi_peer_node_event_rx(struct ath6kl_vif *vif, u8 *datap, int len)
{
	struct wmi_peer_node_event *ev;

	if (len < sizeof(struct wmi_peer_node_event))
		return -EINVAL;

	ev = (struct wmi_peer_node_event *) datap;

	if (ev->event_code == PEER_NODE_JOIN_EVENT)
		ath6kl_dbg(ATH6KL_DBG_WMI, "joined node with mac addr: %pM\n",
			   ev->peer_mac_addr);
	else if (ev->event_code == PEER_NODE_LEAVE_EVENT)
		ath6kl_dbg(ATH6KL_DBG_WMI, "left node with mac addr: %pM\n",
			   ev->peer_mac_addr);

	return 0;
}

static int ath6kl_wmi_tkip_micerr_event_rx(struct ath6kl_vif *vif, u8 *datap, int len)
{
	struct wmi_tkip_micerr_event *ev;

	if (len < sizeof(struct wmi_tkip_micerr_event))
		return -EINVAL;

	ev = (struct wmi_tkip_micerr_event *) datap;

	ath6kl_tkip_micerr_event(vif, ev->key_id, ev->is_mcast);

	return 0;
}

static int ath6kl_wmi_bssinfo_event_rx(struct ath6kl_vif *vif, u8 *datap, int len)
{
	struct wmi_bss_info_hdr2 *bih;
	u8 *buf;
	struct ieee80211_channel *channel;
	struct ieee80211_mgmt *mgmt;
	struct cfg80211_bss *bss;

	if (len <= sizeof(struct wmi_bss_info_hdr2))
		return -EINVAL;

	bih = (struct wmi_bss_info_hdr2 *) datap;
	buf = datap + sizeof(struct wmi_bss_info_hdr2);
	len -= sizeof(struct wmi_bss_info_hdr2);

	ath6kl_dbg(ATH6KL_DBG_WMI,
		   "bss info evt - ch %u, snr %d, rssi %d, bssid \"%pM\" "
		   "frame_type=%d\n",
		   bih->ch, bih->snr, bih->snr - 95, bih->bssid,
		   bih->frame_type);

	if (bih->frame_type != BEACON_FTYPE &&
	    bih->frame_type != PROBERESP_FTYPE)
		return 0; /* Only update BSS table for now */

	channel = ieee80211_get_channel(vif->wdev->wiphy, le16_to_cpu(bih->ch));
	if (channel == NULL)
		return -EINVAL;

	if (len < 8 + 2 + 2)
		return -EINVAL;

	/*
	 * In theory, use of cfg80211_inform_bss_ath6kl() would be more natural here
	 * since we do not have the full frame. However, at least for now,
	 * cfg80211 can only distinguish Beacon and Probe Response frames from
	 * each other when using cfg80211_inform_bss_frame_ath6kl(), so let's build a
	 * fake IEEE 802.11 header to be able to take benefit of this.
	 */
	mgmt = kmalloc(24 + len, GFP_ATOMIC);
	if (mgmt == NULL)
		return -EINVAL;

	if (bih->frame_type == BEACON_FTYPE) {
		mgmt->frame_control = cpu_to_le16(IEEE80211_FTYPE_MGMT |
						  IEEE80211_STYPE_BEACON);
		memset(mgmt->da, 0xff, ETH_ALEN);
	} else {
		struct net_device *dev = vif->net_dev;

		mgmt->frame_control = cpu_to_le16(IEEE80211_FTYPE_MGMT |
						  IEEE80211_STYPE_PROBE_RESP);
		memcpy(mgmt->da, dev->dev_addr, ETH_ALEN);
	}
	mgmt->duration = cpu_to_le16(0);
	memcpy(mgmt->sa, bih->bssid, ETH_ALEN);
	memcpy(mgmt->bssid, bih->bssid, ETH_ALEN);
	mgmt->seq_ctrl = cpu_to_le16(0);

	memcpy(&mgmt->u.beacon, buf, len);

	bss = cfg80211_inform_bss_frame_ath6kl(vif->wdev->wiphy, channel, mgmt,
					24 + len, (bih->snr - 95) * 100,
					GFP_ATOMIC);
	kfree(mgmt);
	if (bss == NULL)
		return -ENOMEM;
	cfg80211_put_bss_ath6kl(bss);

	return 0;
}

/* Inactivity timeout of a fatpipe(pstream) at the target */
static int ath6kl_wmi_pstream_timeout_event_rx(struct ath6kl_vif *vif, u8 *datap,
					       int len)
{
	struct wmi_pstream_timeout_event *ev;

	if (len < sizeof(struct wmi_pstream_timeout_event))
		return -EINVAL;

	ev = (struct wmi_pstream_timeout_event *) datap;

	/*
	 * When the pstream (fat pipe == AC) timesout, it means there were
	 * no thinStreams within this pstream & it got implicitly created
	 * due to data flow on this AC. We start the inactivity timer only
	 * for implicitly created pstream. Just reset the host state.
	 */
	spin_lock_bh(&vif->ar->wmi->lock);
	vif->ar->wmi->stream_exist_for_ac[ev->traffic_class] = 0;
	vif->ar->wmi->fat_pipe_exist &= ~(1 << ev->traffic_class);
	spin_unlock_bh(&vif->ar->wmi->lock);

	/* Indicate inactivity to driver layer for this fatpipe (pstream) */
	ath6kl_indicate_tx_activity(vif->ar->wmi->parent_dev, ev->traffic_class, false);

	return 0;
}

static int ath6kl_wmi_bitrate_reply_rx(struct ath6kl_vif *vif, u8 *datap, int len)
{
	struct wmi_bit_rate_reply *reply;
	s32 rate;
#if 0
	u32 sgi, index;
#endif
	if (len < sizeof(struct wmi_bit_rate_reply))
		return -EINVAL;

	reply = (struct wmi_bit_rate_reply *) datap;

	ath6kl_dbg(ATH6KL_DBG_WMI, "rateindex %d\n", reply->rate_index);

	if (reply->rate_index == (s8) RATE_AUTO) {
		rate = RATE_AUTO;
	} else {
#if 0
		index = reply->rate_index & 0x7f;
		sgi = (reply->rate_index & 0x80) ? 1 : 0;
		rate = wmi_rate_tbl[index][sgi];
#endif
	}

	ath6kl_wakeup_event(vif->ar->wmi->parent_dev);

	return 0;
}

static int ath6kl_wmi_ratemask_reply_rx(struct ath6kl_vif *vif, u8 *datap, int len)
{
	if (len < sizeof(struct wmi_fix_rates_reply))
		return -EINVAL;

	ath6kl_wakeup_event(vif->ar->wmi->parent_dev);

	return 0;
}

static int ath6kl_wmi_ch_list_reply_rx(struct ath6kl_vif *vif, u8 *datap, int len)
{
	struct wmi_channel_list_reply *reply;
	int chan_num;
	if (len < sizeof(struct wmi_channel_list_reply))
		return -EINVAL;
	reply = (struct wmi_channel_list_reply *) datap;
	vif->ar->debug.channel_list_param.num_ch = reply->num_ch;

	for (chan_num = 0; chan_num < reply->num_ch; chan_num++)
		vif->ar->debug.channel_list_param.ch_list[chan_num] = reply->ch_list[chan_num];
	
	ath6kl_wakeup_event(vif->ar->wmi->parent_dev);

	return 0;
}

static int ath6kl_wmi_tx_pwr_reply_rx(struct ath6kl_vif *vif, u8 *datap, int len)
{
	struct wmi_tx_pwr_reply *reply;

	if (len < sizeof(struct wmi_tx_pwr_reply))
		return -EINVAL;

	reply = (struct wmi_tx_pwr_reply *) datap;
	ath6kl_txpwr_rx_evt(vif->ar->wmi->parent_dev, reply->dbM);

	return 0;
}

static int ath6kl_wmi_keepalive_reply_rx(struct ath6kl_vif *vif, u8 *datap, int len)
{
	if (len < sizeof(struct wmi_get_keepalive_cmd))
		return -EINVAL;

	ath6kl_wakeup_event(vif->ar->wmi->parent_dev);

	return 0;
}

static int ath6kl_wmi_scan_complete_rx(struct ath6kl_vif *vif, u8 *datap, int len)
{
	struct wmi_scan_complete_event *ev;

	ev = (struct wmi_scan_complete_event *) datap;

	ath6kl_scan_complete_evt(vif, a_sle32_to_cpu(ev->status));
	vif->ar->wmi->is_probe_ssid = false;

	return 0;
}

/*
 * Target is reporting a programming error.  This is for
 * developer aid only.  Target only checks a few common violations
 * and it is responsibility of host to do all error checking.
 * Behavior of target after wmi error event is undefined.
 * A reset is recommended.
 */
static int ath6kl_wmi_error_event_rx(struct ath6kl_vif *vif, u8 *datap, int len)
{
	const char *type = "unknown error";
	struct wmi_cmd_error_event *ev;
	ev = (struct wmi_cmd_error_event *) datap;

	switch (ev->err_code) {
	case INVALID_PARAM:
		type = "invalid parameter";
		break;
	case ILLEGAL_STATE:
		type = "invalid state";
		break;
	case INTERNAL_ERROR:
		type = "internal error";
		break;
	}

	ath6kl_dbg(ATH6KL_DBG_WMI, "programming error, cmd=%d %s\n",
		   ev->cmd_id, type);

	return 0;
}

static int ath6kl_wmi_stats_event_rx(struct ath6kl_vif *vif, u8 *datap, int len)
{
	ath6kl_tgt_stats_event(vif, datap, len);

	return 0;
}

#include "sony_athtst.h"
static int ath6kl_wmi_sony_get_reg_event_rx(struct ath6kl_vif *vif, u8 *datap, int len)
{
	ath6kl_tgt_sony_get_reg_event(vif, datap, len);

	return 0;
}
static int ath6kl_wmi_sony_get_version_info_event_rx(struct ath6kl_vif *vif, u8 *datap, int len)
{
	ath6kl_tgt_sony_get_version_info_event(vif, datap, len);

	return 0;
}
static int ath6kl_wmi_sony_get_testmode_event_rx(struct ath6kl_vif *vif, u8 *datap, int len)
{
	ath6kl_tgt_sony_get_testmode_event(vif, datap, len);

	return 0;
}
static int ath6kl_wmi_sony_get_txpow_event_rx(struct ath6kl_vif *vif, u8 *datap, int len)
{
	ath6kl_tgt_sony_get_txpow_event(vif, datap, len);

	return 0;
}
static int ath6kl_wmi_sony_get_stainfo_event_rx(struct ath6kl_vif *vif, u8 *datap, int len)
{
	ath6kl_tgt_sony_get_stainfo_event(vif, datap, len);

	return 0;
}
static int ath6kl_wmi_sony_get_stainfo_beacon_rssi_event_rx(struct ath6kl_vif *vif, u8 *datap, int len)
{
    ath6kl_tgt_sony_get_stainfo_beacon_rssi_event(vif, datap, len);
	return 0;
}
static int ath6kl_wmi_sony_get_scaninfo_event_rx(struct ath6kl_vif *vif, u8 *datap, int len)
{
    ath6kl_tgt_sony_get_scaninfo_event(vif, datap, len);
	return 0;
}
static int ath6kl_wmi_sony_set_scan_done_event_rx(struct ath6kl_vif *vif, u8 *datap, int len)
{
    ath6kl_tgt_sony_set_scan_done_event(vif, datap, len);
	return 0;
}

static u8 ath6kl_wmi_get_upper_threshold(s16 rssi,
					 struct sq_threshold_params *sq_thresh,
					 u32 size)
{
	u32 index;
	u8 threshold = (u8) sq_thresh->upper_threshold[size - 1];

	/* The list is already in sorted order. Get the next lower value */
	for (index = 0; index < size; index++) {
		if (rssi < sq_thresh->upper_threshold[index]) {
			threshold = (u8) sq_thresh->upper_threshold[index];
			break;
		}
	}

	return threshold;
}

static u8 ath6kl_wmi_get_lower_threshold(s16 rssi,
					 struct sq_threshold_params *sq_thresh,
					 u32 size)
{
	u32 index;
	u8 threshold = (u8) sq_thresh->lower_threshold[size - 1];

	/* The list is already in sorted order. Get the next lower value */
	for (index = 0; index < size; index++) {
		if (rssi > sq_thresh->lower_threshold[index]) {
			threshold = (u8) sq_thresh->lower_threshold[index];
			break;
		}
	}

	return threshold;
}

static int ath6kl_wmi_send_rssi_threshold_params(struct ath6kl_vif *vif,
			struct wmi_rssi_threshold_params_cmd *rssi_cmd)
{
	struct sk_buff *skb;
	struct wmi_rssi_threshold_params_cmd *cmd;

	skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
	if (!skb)
		return -ENOMEM;

	cmd = (struct wmi_rssi_threshold_params_cmd *) skb->data;
	memcpy(cmd, rssi_cmd, sizeof(struct wmi_rssi_threshold_params_cmd));

	return ath6kl_wmi_cmd_send(vif, skb, WMI_RSSI_THRESHOLD_PARAMS_CMDID,
				   NO_SYNC_WMIFLAG);
}

static int ath6kl_wmi_rssi_threshold_event_rx(struct ath6kl_vif *vif, u8 *datap,
					      int len)
{
	struct wmi_rssi_threshold_event *reply;
	struct wmi_rssi_threshold_params_cmd cmd;
	struct sq_threshold_params *sq_thresh;
	enum wmi_rssi_threshold_val new_threshold;
	u8 upper_rssi_threshold, lower_rssi_threshold;
	s16 rssi;
	int ret;

	if (len < sizeof(struct wmi_rssi_threshold_event))
		return -EINVAL;

	reply = (struct wmi_rssi_threshold_event *) datap;
	new_threshold = (enum wmi_rssi_threshold_val) reply->range;
	rssi = a_sle16_to_cpu(reply->rssi);

	sq_thresh = &vif->ar->wmi->sq_threshld[SIGNAL_QUALITY_METRICS_RSSI];

	/*
	 * Identify the threshold breached and communicate that to the app.
	 * After that install a new set of thresholds based on the signal
	 * quality reported by the target
	 */
	if (new_threshold) {
		/* Upper threshold breached */
		if (rssi < sq_thresh->upper_threshold[0]) {
			ath6kl_dbg(ATH6KL_DBG_WMI,
				"spurious upper rssi threshold event: %d\n",
				rssi);
		} else if ((rssi < sq_thresh->upper_threshold[1]) &&
			   (rssi >= sq_thresh->upper_threshold[0])) {
			new_threshold = WMI_RSSI_THRESHOLD1_ABOVE;
		} else if ((rssi < sq_thresh->upper_threshold[2]) &&
			   (rssi >= sq_thresh->upper_threshold[1])) {
			new_threshold = WMI_RSSI_THRESHOLD2_ABOVE;
		} else if ((rssi < sq_thresh->upper_threshold[3]) &&
			   (rssi >= sq_thresh->upper_threshold[2])) {
			new_threshold = WMI_RSSI_THRESHOLD3_ABOVE;
		} else if ((rssi < sq_thresh->upper_threshold[4]) &&
			   (rssi >= sq_thresh->upper_threshold[3])) {
			new_threshold = WMI_RSSI_THRESHOLD4_ABOVE;
		} else if ((rssi < sq_thresh->upper_threshold[5]) &&
			   (rssi >= sq_thresh->upper_threshold[4])) {
			new_threshold = WMI_RSSI_THRESHOLD5_ABOVE;
		} else if (rssi >= sq_thresh->upper_threshold[5]) {
			new_threshold = WMI_RSSI_THRESHOLD6_ABOVE;
		}
	} else {
		/* Lower threshold breached */
		if (rssi > sq_thresh->lower_threshold[0]) {
			ath6kl_dbg(ATH6KL_DBG_WMI,
				"spurious lower rssi threshold event: %d %d\n",
				rssi, sq_thresh->lower_threshold[0]);
		} else if ((rssi > sq_thresh->lower_threshold[1]) &&
			   (rssi <= sq_thresh->lower_threshold[0])) {
			new_threshold = WMI_RSSI_THRESHOLD6_BELOW;
		} else if ((rssi > sq_thresh->lower_threshold[2]) &&
			   (rssi <= sq_thresh->lower_threshold[1])) {
			new_threshold = WMI_RSSI_THRESHOLD5_BELOW;
		} else if ((rssi > sq_thresh->lower_threshold[3]) &&
			   (rssi <= sq_thresh->lower_threshold[2])) {
			new_threshold = WMI_RSSI_THRESHOLD4_BELOW;
		} else if ((rssi > sq_thresh->lower_threshold[4]) &&
			   (rssi <= sq_thresh->lower_threshold[3])) {
			new_threshold = WMI_RSSI_THRESHOLD3_BELOW;
		} else if ((rssi > sq_thresh->lower_threshold[5]) &&
			   (rssi <= sq_thresh->lower_threshold[4])) {
			new_threshold = WMI_RSSI_THRESHOLD2_BELOW;
		} else if (rssi <= sq_thresh->lower_threshold[5]) {
			new_threshold = WMI_RSSI_THRESHOLD1_BELOW;
		}
	}

	/* Calculate and install the next set of thresholds */
	lower_rssi_threshold = ath6kl_wmi_get_lower_threshold(rssi, sq_thresh,
				       sq_thresh->lower_threshold_valid_count);
	upper_rssi_threshold = ath6kl_wmi_get_upper_threshold(rssi, sq_thresh,
				       sq_thresh->upper_threshold_valid_count);

	/* Issue a wmi command to install the thresholds */
	cmd.thresh_above1_val = a_cpu_to_sle16(upper_rssi_threshold);
	cmd.thresh_below1_val = a_cpu_to_sle16(lower_rssi_threshold);
	cmd.weight = sq_thresh->weight;
	cmd.poll_time = cpu_to_le32(sq_thresh->polling_interval);

	ret = ath6kl_wmi_send_rssi_threshold_params(vif, &cmd);
	if (ret) {
		ath6kl_err("unable to configure rssi thresholds\n");
		return -EIO;
	}

	return 0;
}

static int ath6kl_wmi_cac_event_rx(struct ath6kl_vif *vif, u8 *datap, int len)
{
	struct wmi_cac_event *reply;
	struct ieee80211_tspec_ie *ts;
	u16 active_tsids, tsinfo;
	u8 tsid, index;
	u8 ts_id;

	if (len < sizeof(struct wmi_cac_event))
		return -EINVAL;

	reply = (struct wmi_cac_event *) datap;

	if ((reply->cac_indication == CAC_INDICATION_ADMISSION_RESP) &&
	    (reply->status_code != IEEE80211_TSPEC_STATUS_ADMISS_ACCEPTED)) {

		ts = (struct ieee80211_tspec_ie *) &(reply->tspec_suggestion);
		tsinfo = le16_to_cpu(ts->tsinfo);
		tsid = (tsinfo >> IEEE80211_WMM_IE_TSPEC_TID_SHIFT) &
			IEEE80211_WMM_IE_TSPEC_TID_MASK;

		ath6kl_wmi_delete_pstream_cmd(vif, reply->ac, tsid);
	} else if (reply->cac_indication == CAC_INDICATION_NO_RESP) {
		/*
		 * Following assumes that there is only one outstanding
		 * ADDTS request when this event is received
		 */
		spin_lock_bh(&vif->ar->wmi->lock);
		active_tsids = vif->ar->wmi->stream_exist_for_ac[reply->ac];
		spin_unlock_bh(&vif->ar->wmi->lock);

		for (index = 0; index < sizeof(active_tsids) * 8; index++) {
			if ((active_tsids >> index) & 1)
				break;
		}
		if (index < (sizeof(active_tsids) * 8))
			ath6kl_wmi_delete_pstream_cmd(vif, reply->ac, index);
	}

	/*
	 * Clear active tsids and Add missing handling
	 * for delete qos stream from AP
	 */
	else if (reply->cac_indication == CAC_INDICATION_DELETE) {

		ts = (struct ieee80211_tspec_ie *) &(reply->tspec_suggestion);
		tsinfo = le16_to_cpu(ts->tsinfo);
		ts_id = ((tsinfo >> IEEE80211_WMM_IE_TSPEC_TID_SHIFT) &
			 IEEE80211_WMM_IE_TSPEC_TID_MASK);

		spin_lock_bh(&vif->ar->wmi->lock);
		vif->ar->wmi->stream_exist_for_ac[reply->ac] &= ~(1 << ts_id);
		active_tsids = vif->ar->wmi->stream_exist_for_ac[reply->ac];
		spin_unlock_bh(&vif->ar->wmi->lock);

		/* Indicate stream inactivity to driver layer only if all tsids
		 * within this AC are deleted.
		 */
		if (!active_tsids) {
			ath6kl_indicate_tx_activity(vif->ar->wmi->parent_dev, reply->ac,
						    false);
			vif->ar->wmi->fat_pipe_exist &= ~(1 << reply->ac);
		}
	}

	return 0;
}

static int ath6kl_wmi_send_snr_threshold_params(struct ath6kl_vif *vif,
			struct wmi_snr_threshold_params_cmd *snr_cmd)
{
	struct sk_buff *skb;
	struct wmi_snr_threshold_params_cmd *cmd;

	skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
	if (!skb)
		return -ENOMEM;

	cmd = (struct wmi_snr_threshold_params_cmd *) skb->data;
	memcpy(cmd, snr_cmd, sizeof(struct wmi_snr_threshold_params_cmd));

	return ath6kl_wmi_cmd_send(vif, skb, WMI_SNR_THRESHOLD_PARAMS_CMDID,
				   NO_SYNC_WMIFLAG);
}

static int ath6kl_wmi_snr_threshold_event_rx(struct ath6kl_vif *vif, u8 *datap,
					     int len)
{
	struct wmi_snr_threshold_event *reply;
	struct sq_threshold_params *sq_thresh;
	struct wmi_snr_threshold_params_cmd cmd;
	enum wmi_snr_threshold_val new_threshold;
	u8 upper_snr_threshold, lower_snr_threshold;
	s16 snr;
	int ret;

	if (len < sizeof(struct wmi_snr_threshold_event))
		return -EINVAL;

	reply = (struct wmi_snr_threshold_event *) datap;

	new_threshold = (enum wmi_snr_threshold_val) reply->range;
	snr = reply->snr;

	sq_thresh = &vif->ar->wmi->sq_threshld[SIGNAL_QUALITY_METRICS_SNR];

	/*
	 * Identify the threshold breached and communicate that to the app.
	 * After that install a new set of thresholds based on the signal
	 * quality reported by the target.
	 */
	if (new_threshold) {
		/* Upper threshold breached */
		if (snr < sq_thresh->upper_threshold[0]) {
			ath6kl_dbg(ATH6KL_DBG_WMI,
				"spurious upper snr threshold event: %d\n",
				snr);
		} else if ((snr < sq_thresh->upper_threshold[1]) &&
			   (snr >= sq_thresh->upper_threshold[0])) {
			new_threshold = WMI_SNR_THRESHOLD1_ABOVE;
		} else if ((snr < sq_thresh->upper_threshold[2]) &&
			   (snr >= sq_thresh->upper_threshold[1])) {
			new_threshold = WMI_SNR_THRESHOLD2_ABOVE;
		} else if ((snr < sq_thresh->upper_threshold[3]) &&
			   (snr >= sq_thresh->upper_threshold[2])) {
			new_threshold = WMI_SNR_THRESHOLD3_ABOVE;
		} else if (snr >= sq_thresh->upper_threshold[3]) {
			new_threshold = WMI_SNR_THRESHOLD4_ABOVE;
		}
	} else {
		/* Lower threshold breached */
		if (snr > sq_thresh->lower_threshold[0]) {
			ath6kl_dbg(ATH6KL_DBG_WMI,
				"spurious lower snr threshold event: %d\n",
				sq_thresh->lower_threshold[0]);
		} else if ((snr > sq_thresh->lower_threshold[1]) &&
			   (snr <= sq_thresh->lower_threshold[0])) {
			new_threshold = WMI_SNR_THRESHOLD4_BELOW;
		} else if ((snr > sq_thresh->lower_threshold[2]) &&
			   (snr <= sq_thresh->lower_threshold[1])) {
			new_threshold = WMI_SNR_THRESHOLD3_BELOW;
		} else if ((snr > sq_thresh->lower_threshold[3]) &&
			   (snr <= sq_thresh->lower_threshold[2])) {
			new_threshold = WMI_SNR_THRESHOLD2_BELOW;
		} else if (snr <= sq_thresh->lower_threshold[3]) {
			new_threshold = WMI_SNR_THRESHOLD1_BELOW;
		}
	}

	/* Calculate and install the next set of thresholds */
	lower_snr_threshold = ath6kl_wmi_get_lower_threshold(snr, sq_thresh,
				       sq_thresh->lower_threshold_valid_count);
	upper_snr_threshold = ath6kl_wmi_get_upper_threshold(snr, sq_thresh,
				       sq_thresh->upper_threshold_valid_count);

	/* Issue a wmi command to install the thresholds */
	cmd.thresh_above1_val = upper_snr_threshold;
	cmd.thresh_below1_val = lower_snr_threshold;
	cmd.weight = sq_thresh->weight;
	cmd.poll_time = cpu_to_le32(sq_thresh->polling_interval);

	ath6kl_dbg(ATH6KL_DBG_WMI,
		   "snr: %d, threshold: %d, lower: %d, upper: %d\n",
		   snr, new_threshold,
		   lower_snr_threshold, upper_snr_threshold);

	ret = ath6kl_wmi_send_snr_threshold_params(vif, &cmd);
	if (ret) {
		ath6kl_err("unable to configure snr threshold\n");
		return -EIO;
	}

	return 0;
}

static int ath6kl_wmi_aplist_event_rx(struct ath6kl_vif *vif, u8 *datap, int len)
{
	u16 ap_info_entry_size;
	struct wmi_aplist_event *ev = (struct wmi_aplist_event *) datap;
	struct wmi_ap_info_v1 *ap_info_v1;
	u8 index;

	if (len < sizeof(struct wmi_aplist_event) ||
	    ev->ap_list_ver != APLIST_VER1)
		return -EINVAL;

	ap_info_entry_size = sizeof(struct wmi_ap_info_v1);
	ap_info_v1 = (struct wmi_ap_info_v1 *) ev->ap_list;

	ath6kl_dbg(ATH6KL_DBG_WMI,
		   "number of APs in aplist event: %d\n", ev->num_ap);

	if (len < (int) (sizeof(struct wmi_aplist_event) +
			 (ev->num_ap - 1) * ap_info_entry_size))
		return -EINVAL;

	/* AP list version 1 contents */
	for (index = 0; index < ev->num_ap; index++) {
		ath6kl_dbg(ATH6KL_DBG_WMI, "AP#%d BSSID %pM Channel %d\n",
			   index, ap_info_v1->bssid, ap_info_v1->channel);
		ap_info_v1++;
	}

	return 0;
}

int __ath6kl_wmi_cmd_send(struct ath6kl_vif *vif, struct sk_buff *skb,
			enum wmi_cmd_id cmd_id, enum wmi_sync_flag sync_flag)
{
	struct wmi_cmd_hdr *cmd_hdr;
	enum htc_endpoint_id ep_id = vif->ar->wmi->ep_id;
	int ret;

	ath6kl_dbg(ATH6KL_DBG_WMI, "%s: cmd_id=%d\n", __func__, cmd_id);

	if (WARN_ON(skb == NULL))
		return -EINVAL;

	if (sync_flag >= END_WMIFLAG) {
		dev_kfree_skb(skb);
		return -EINVAL;
	}

	if ((sync_flag == SYNC_BEFORE_WMIFLAG) ||
	    (sync_flag == SYNC_BOTH_WMIFLAG)) {
		/*
		 * Make sure all data currently queued is transmitted before
		 * the cmd execution.  Establish a new sync point.
		 */
		ath6kl_wmi_sync_point(vif);
	}

	skb_push(skb, sizeof(struct wmi_cmd_hdr));

	cmd_hdr = (struct wmi_cmd_hdr *) skb->data;
	cmd_hdr->cmd_id = cpu_to_le16(cmd_id);
	cmd_hdr->info1 = cpu_to_le16(vif->device_index & WMI_CMD_HDR_IF_ID_MASK);

	/* Only for OPT_TX_CMD, use BE endpoint. */
	if (cmd_id == WMI_OPT_TX_FRAME_CMDID) {
		ret = ath6kl_wmi_data_hdr_add(vif, skb, OPT_MSGTYPE,
					      false, false, 0, NULL);
		if (ret) {
			dev_kfree_skb(skb);
			return ret;
		}
		ep_id = ath6kl_ac2_endpoint_id(vif->ar->wmi->parent_dev, WMM_AC_BE);
	}

	SKB_CB_VIF(skb) = vif;

	if (cmd_id != WMI_SYNCHRONIZE_CMDID &&
	    sync_flag != SYNC_BEFORE_WMIFLAG &&
	    sync_flag != SYNC_BOTH_WMIFLAG &&
	    sync_flag != SYNC_AFTER_WMIFLAG) {
	    ath6kl_control_tx(vif->ar, skb, ep_id);
	} else {
	    ath6kl_control_tx_sync(vif->ar, skb, ep_id);
	}

	if ((sync_flag == SYNC_AFTER_WMIFLAG) ||
	    (sync_flag == SYNC_BOTH_WMIFLAG)) {
		/*
		 * Make sure all new data queued waits for the command to
		 * execute. Establish a new sync point.
		 */
		ath6kl_wmi_sync_point(vif);
	}

	return 0;
}

int ath6kl_wmi_cmd_send(struct ath6kl_vif *vif, struct sk_buff *skb,
			enum wmi_cmd_id cmd_id, enum wmi_sync_flag sync_flag)
{
	int ret;

	if (vif->ar == NULL || vif->ar->wmi == NULL)
		return -EINVAL;

	if (cmd_id != WMI_SYNCHRONIZE_CMDID &&
	    sync_flag != SYNC_BEFORE_WMIFLAG &&
	    sync_flag != SYNC_BOTH_WMIFLAG &&
	    sync_flag != SYNC_AFTER_WMIFLAG) {
		spin_lock_bh(&vif->ar->wmi->lock);
		ret = __ath6kl_wmi_cmd_send(vif, skb, cmd_id, sync_flag);
		spin_unlock_bh(&vif->ar->wmi->lock);
	} else {
		if (down_interruptible(&vif->ar->wmi->sem)) {
			ath6kl_err("can't gain access to wmi\n");
			return -ERESTARTSYS;
		}
		ret = __ath6kl_wmi_cmd_send(vif, skb, cmd_id, sync_flag);
		up(&vif->ar->wmi->sem);
	}

	return ret;
}

int ath6kl_wmi_connect_cmd(struct ath6kl_vif *vif, enum network_type nw_type,
			   enum dot11_auth_mode dot11_auth_mode,
			   enum auth_mode auth_mode,
			   enum crypto_type pairwise_crypto,
			   u8 pairwise_crypto_len,
			   enum crypto_type group_crypto,
			   u8 group_crypto_len, int ssid_len, u8 *ssid,
			   u8 *bssid, u16 channel, u32 ctrl_flags)
{
	struct sk_buff *skb;
	struct wmi_connect_cmd *cc;
	int ret;

	vif->ar->wmi->traffic_class = 100;

	if ((pairwise_crypto == NONE_CRYPT) && (group_crypto != NONE_CRYPT))
		return -EINVAL;

	if ((pairwise_crypto != NONE_CRYPT) && (group_crypto == NONE_CRYPT))
		return -EINVAL;

	skb = ath6kl_wmi_get_new_buf(sizeof(struct wmi_connect_cmd));
	if (!skb)
		return -ENOMEM;

	cc = (struct wmi_connect_cmd *) skb->data;

	if (ssid_len)
		memcpy(cc->ssid, ssid, ssid_len);

	cc->ssid_len = ssid_len;
	cc->nw_type = nw_type;
	cc->dot11_auth_mode = dot11_auth_mode;
	cc->auth_mode = auth_mode;
	cc->prwise_crypto_type = pairwise_crypto;
	cc->prwise_crypto_len = pairwise_crypto_len;
	cc->grp_crypto_type = group_crypto;
	cc->grp_crypto_len = group_crypto_len;
	cc->ch = cpu_to_le16(channel);
	cc->ctrl_flags = cpu_to_le32(ctrl_flags);

	if (bssid != NULL)
		memcpy(cc->bssid, bssid, ETH_ALEN);

	vif->ar->wmi->pair_crypto_type = pairwise_crypto;
	vif->ar->wmi->grp_crypto_type = group_crypto;

	ret = ath6kl_wmi_cmd_send(vif, skb, WMI_CONNECT_CMDID, NO_SYNC_WMIFLAG);

	return ret;
}

int ath6kl_wmi_reconnect_cmd(struct ath6kl_vif *vif, u8 *bssid, u16 channel)
{
	struct sk_buff *skb;
	struct wmi_reconnect_cmd *cc;
	int ret;

	vif->ar->wmi->traffic_class = 100;

	skb = ath6kl_wmi_get_new_buf(sizeof(struct wmi_reconnect_cmd));
	if (!skb)
		return -ENOMEM;

	cc = (struct wmi_reconnect_cmd *) skb->data;
	cc->channel = cpu_to_le16(channel);

	if (bssid != NULL)
		memcpy(cc->bssid, bssid, ETH_ALEN);

	ret = ath6kl_wmi_cmd_send(vif, skb, WMI_RECONNECT_CMDID,
				  NO_SYNC_WMIFLAG);

	return ret;
}

int ath6kl_wmi_disconnect_cmd(struct ath6kl_vif *vif)
{
	int ret = 0;

	if (vif->ar->wmi) {
		vif->ar->wmi->traffic_class = 100;

		/* Disconnect command does not need to do a SYNC before. */
		ret = ath6kl_wmi_simple_cmd(vif, WMI_DISCONNECT_CMDID);
	}

	return ret;
}

int ath6kl_wmi_startscan_cmd(struct ath6kl_vif *vif, enum wmi_scan_type scan_type,
			     u32 force_fgscan, u32 is_legacy,
			     u32 home_dwell_time, u32 force_scan_interval,
			     s8 num_chan, u16 *ch_list)
{
	struct sk_buff *skb;
	struct wmi_start_scan_cmd *sc;
	s8 size;
	int i, ret;

	size = sizeof(struct wmi_start_scan_cmd);

	if ((scan_type != WMI_LONG_SCAN) && (scan_type != WMI_SHORT_SCAN))
		return -EINVAL;

	if (num_chan > WMI_MAX_CHANNELS)
		return -EINVAL;

	if (num_chan)
		size += sizeof(u16) * (num_chan - 1);

	skb = ath6kl_wmi_get_new_buf(size);
	if (!skb)
		return -ENOMEM;

	sc = (struct wmi_start_scan_cmd *) skb->data;
	sc->scan_type = scan_type;
	sc->force_fg_scan = cpu_to_le32(force_fgscan);
	sc->is_legacy = cpu_to_le32(is_legacy);
	sc->home_dwell_time = cpu_to_le32(home_dwell_time);
	sc->force_scan_intvl = cpu_to_le32(force_scan_interval);
	sc->num_ch = num_chan;

	if (num_chan) {
		for (i = 0; i < num_chan; i++)
			sc->ch_list[i] = cpu_to_le16(ch_list[i]);
	}

	ret = ath6kl_wmi_cmd_send(vif, skb, WMI_START_SCAN_CMDID,
				  NO_SYNC_WMIFLAG);

	return ret;
}

int ath6kl_wmi_scanparams_cmd(struct ath6kl_vif *vif, u16 fg_start_sec,
			      u16 fg_end_sec, u16 bg_sec,
			      u16 minact_chdw_msec, u16 maxact_chdw_msec,
			      u16 pas_chdw_msec, u8 short_scan_ratio,
			      u8 scan_ctrl_flag, u32 max_dfsch_act_time,
			      u16 maxact_scan_per_ssid)
{
	struct sk_buff *skb;
	struct wmi_scan_params_cmd *sc;
	int ret;

	skb = ath6kl_wmi_get_new_buf(sizeof(*sc));
	if (!skb)
		return -ENOMEM;

	sc = (struct wmi_scan_params_cmd *) skb->data;
	sc->fg_start_period = cpu_to_le16(fg_start_sec);
	sc->fg_end_period = cpu_to_le16(fg_end_sec);
	sc->bg_period = cpu_to_le16(bg_sec);
	sc->minact_chdwell_time = cpu_to_le16(minact_chdw_msec);
	sc->maxact_chdwell_time = cpu_to_le16(maxact_chdw_msec);
	sc->pas_chdwell_time = cpu_to_le16(pas_chdw_msec);
	sc->short_scan_ratio = short_scan_ratio;
	sc->scan_ctrl_flags = scan_ctrl_flag;
	sc->max_dfsch_act_time = cpu_to_le32(max_dfsch_act_time);
	sc->maxact_scan_per_ssid = cpu_to_le16(maxact_scan_per_ssid);

	ret = ath6kl_wmi_cmd_send(vif, skb, WMI_SET_SCAN_PARAMS_CMDID,
				  NO_SYNC_WMIFLAG);
	return ret;
}

int
ath6kl_wmi_set_roam_ctrl_cmd_for_lowerrssi(struct ath6kl_vif *vif, 
                      u16  lowrssi_scan_period,
                      u16  lowrssi_scan_threshold,
                      u16  lowrssi_roam_threshold,
                      u8   roam_rssi_floor)
{
	struct sk_buff *skb;
	struct wmi_set_roam_ctrl_cmd *sc;
	int ret;

	skb = ath6kl_wmi_get_new_buf(sizeof(*sc));
	if (!skb)
		return -ENOMEM;

    sc = (struct wmi_set_roam_ctrl_cmd *) skb->data;

    sc->roamCtrlType = WMI_SET_LOWRSSI_SCAN_PARAMS;
    sc->info.lrScanParams.lowrssi_scan_period = cpu_to_le16(lowrssi_scan_period);
    sc->info.lrScanParams.lowrssi_scan_threshold = cpu_to_le16(lowrssi_scan_threshold);
    sc->info.lrScanParams.lowrssi_roam_threshold = cpu_to_le16(lowrssi_roam_threshold);
    sc->info.lrScanParams.roam_rssi_floor = roam_rssi_floor;
    
	ret = ath6kl_wmi_cmd_send(vif, skb, WMI_SET_ROAM_CTRL_CMDID,
				  NO_SYNC_WMIFLAG);
	return ret;

}


int ath6kl_wmi_bssfilter_cmd(struct ath6kl_vif *vif, u8 filter, u32 ie_mask)
{
	struct sk_buff *skb;
	struct wmi_bss_filter_cmd *cmd;
	int ret;

	if (filter >= LAST_BSS_FILTER)
		return -EINVAL;

	skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
	if (!skb)
		return -ENOMEM;

	cmd = (struct wmi_bss_filter_cmd *) skb->data;
	cmd->bss_filter = filter;
	cmd->ie_mask = cpu_to_le32(ie_mask);

	ret = ath6kl_wmi_cmd_send(vif, skb, WMI_SET_BSS_FILTER_CMDID,
				  NO_SYNC_WMIFLAG);
	return ret;
}

int ath6kl_wmi_probedssid_cmd(struct ath6kl_vif *vif, u8 index, u8 flag,
			      u8 ssid_len, u8 *ssid)
{
	struct sk_buff *skb;
	struct wmi_probed_ssid_cmd *cmd;
	int ret;

	if (index > MAX_PROBED_SSID_INDEX)
		return -EINVAL;

	if (ssid_len > sizeof(cmd->ssid))
		return -EINVAL;

	if ((flag & (DISABLE_SSID_FLAG | ANY_SSID_FLAG)) && (ssid_len > 0))
		return -EINVAL;

	if ((flag & SPECIFIC_SSID_FLAG) && !ssid_len)
		return -EINVAL;

	if (flag & SPECIFIC_SSID_FLAG)
		vif->ar->wmi->is_probe_ssid = true;

	skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
	if (!skb)
		return -ENOMEM;

	cmd = (struct wmi_probed_ssid_cmd *) skb->data;
	cmd->entry_index = index;
	cmd->flag = flag;
	cmd->ssid_len = ssid_len;
	memcpy(cmd->ssid, ssid, ssid_len);

	ret = ath6kl_wmi_cmd_send(vif, skb, WMI_SET_PROBED_SSID_CMDID,
				  NO_SYNC_WMIFLAG);
	return ret;
}

int ath6kl_wmi_listeninterval_cmd(struct ath6kl_vif *vif, u16 listen_interval,
				  u16 listen_beacons)
{
	struct sk_buff *skb;
	struct wmi_listen_int_cmd *cmd;
	int ret;

	skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
	if (!skb)
		return -ENOMEM;

	cmd = (struct wmi_listen_int_cmd *) skb->data;
	cmd->listen_intvl = cpu_to_le16(listen_interval);
	cmd->num_beacons = cpu_to_le16(listen_beacons);

	ret = ath6kl_wmi_cmd_send(vif, skb, WMI_SET_LISTEN_INT_CMDID,
				  NO_SYNC_WMIFLAG);
	return ret;
}

int ath6kl_wmi_powermode_cmd(struct ath6kl_vif *vif, u8 pwr_mode)
{
	struct sk_buff *skb;
	struct wmi_power_mode_cmd *cmd;
	int ret;

	skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
	if (!skb)
		return -ENOMEM;

	cmd = (struct wmi_power_mode_cmd *) skb->data;
	cmd->pwr_mode = pwr_mode;
	vif->ar->wmi->pwr_mode = pwr_mode;

	ret = ath6kl_wmi_cmd_send(vif, skb, WMI_SET_POWER_MODE_CMDID,
				  NO_SYNC_WMIFLAG);
	return ret;
}

int ath6kl_wmi_pmparams_cmd(struct ath6kl_vif *vif, u16 idle_period,
			    u16 ps_poll_num, u16 dtim_policy,
			    u16 tx_wakeup_policy, u16 num_tx_to_wakeup,
			    u16 ps_fail_event_policy)
{
	struct sk_buff *skb;
	struct wmi_power_params_cmd *pm;
	int ret;

	skb = ath6kl_wmi_get_new_buf(sizeof(*pm));
	if (!skb)
		return -ENOMEM;

	pm = (struct wmi_power_params_cmd *)skb->data;
	pm->idle_period = cpu_to_le16(idle_period);
	pm->pspoll_number = cpu_to_le16(ps_poll_num);
	pm->dtim_policy = cpu_to_le16(dtim_policy);
	pm->tx_wakeup_policy = cpu_to_le16(tx_wakeup_policy);
	pm->num_tx_to_wakeup = cpu_to_le16(num_tx_to_wakeup);
	pm->ps_fail_event_policy = cpu_to_le16(ps_fail_event_policy);

	ret = ath6kl_wmi_cmd_send(vif, skb, WMI_SET_POWER_PARAMS_CMDID,
				  NO_SYNC_WMIFLAG);
	return ret;
}

int ath6kl_wmi_disctimeout_cmd(struct ath6kl_vif *vif, u8 timeout)
{
	struct sk_buff *skb;
	struct wmi_disc_timeout_cmd *cmd;
	int ret;

	skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
	if (!skb)
		return -ENOMEM;

	cmd = (struct wmi_disc_timeout_cmd *) skb->data;
	cmd->discon_timeout = timeout;

	ret = ath6kl_wmi_cmd_send(vif, skb, WMI_SET_DISC_TIMEOUT_CMDID,
				  NO_SYNC_WMIFLAG);
	return ret;
}

int ath6kl_wmi_addkey_cmd(struct ath6kl_vif *vif, u8 key_index,
			  enum crypto_type key_type,
			  u8 key_usage, u8 key_len,
			  u8 *key_rsc, u8 *key_material,
			  u8 key_op_ctrl, u8 *mac_addr,
			  enum wmi_sync_flag sync_flag)
{
	struct sk_buff *skb;
	struct wmi_add_cipher_key_cmd *cmd;
	int ret;

	ath6kl_dbg(ATH6KL_DBG_ANY, "addkey cmd: key_index=%u key_type=%d "
		   "key_usage=%d key_len=%d key_op_ctrl=%d\n",
		   key_index, key_type, key_usage, key_len, key_op_ctrl);

	if ((key_index > WMI_MAX_KEY_INDEX) || (key_len > WMI_MAX_KEY_LEN) ||
	    (key_material == NULL))
		return -EINVAL;

	if ((WEP_CRYPT != key_type) && (NULL == key_rsc))
		return -EINVAL;

	skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
	if (!skb)
		return -ENOMEM;

	cmd = (struct wmi_add_cipher_key_cmd *) skb->data;
	cmd->key_index = key_index;
	cmd->key_type = key_type;
	cmd->key_usage = key_usage;
	cmd->key_len = key_len;
	memcpy(cmd->key, key_material, key_len);

	if (key_rsc != NULL)
		memcpy(cmd->key_rsc, key_rsc, sizeof(cmd->key_rsc));

	cmd->key_op_ctrl = key_op_ctrl;

	if (mac_addr)
		memcpy(cmd->key_mac_addr, mac_addr, ETH_ALEN);

	ret = ath6kl_wmi_cmd_send(vif, skb, WMI_ADD_CIPHER_KEY_CMDID,
				  sync_flag);

	return ret;
}

int ath6kl_wmi_add_krk_cmd(struct ath6kl_vif *vif, u8 *krk)
{
	struct sk_buff *skb;
	struct wmi_add_krk_cmd *cmd;
	int ret;

	skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
	if (!skb)
		return -ENOMEM;

	cmd = (struct wmi_add_krk_cmd *) skb->data;
	memcpy(cmd->krk, krk, WMI_KRK_LEN);

	ret = ath6kl_wmi_cmd_send(vif, skb, WMI_ADD_KRK_CMDID, NO_SYNC_WMIFLAG);

	return ret;
}

int ath6kl_wmi_deletekey_cmd(struct ath6kl_vif *vif, u8 key_index)
{
	struct sk_buff *skb;
	struct wmi_delete_cipher_key_cmd *cmd;
	int ret;

	if (key_index > WMI_MAX_KEY_INDEX)
		return -EINVAL;

	skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
	if (!skb)
		return -ENOMEM;

	cmd = (struct wmi_delete_cipher_key_cmd *) skb->data;
	cmd->key_index = key_index;

	ret = ath6kl_wmi_cmd_send(vif, skb, WMI_DELETE_CIPHER_KEY_CMDID,
				  NO_SYNC_WMIFLAG);

	return ret;
}

int ath6kl_wmi_setpmkid_cmd(struct ath6kl_vif *vif, const u8 *bssid,
			    const u8 *pmkid, bool set)
{
	struct sk_buff *skb;
	struct wmi_setpmkid_cmd *cmd;
	int ret;

	if (bssid == NULL)
		return -EINVAL;

	if (set && pmkid == NULL)
		return -EINVAL;

	skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
	if (!skb)
		return -ENOMEM;

	cmd = (struct wmi_setpmkid_cmd *) skb->data;
	memcpy(cmd->bssid, bssid, ETH_ALEN);
	if (set) {
		memcpy(cmd->pmkid, pmkid, sizeof(cmd->pmkid));
		cmd->enable = PMKID_ENABLE;
	} else {
		memset(cmd->pmkid, 0, sizeof(cmd->pmkid));
		cmd->enable = PMKID_DISABLE;
	}

	ret = ath6kl_wmi_cmd_send(vif, skb, WMI_SET_PMKID_CMDID,
				  NO_SYNC_WMIFLAG);

	return ret;
}

static int ath6kl_wmi_data_sync_send(struct ath6kl_vif *vif, struct sk_buff *skb,
			      enum htc_endpoint_id ep_id)
{
	struct wmi_data_hdr *data_hdr;
	int ret;

	if (WARN_ON(skb == NULL || ep_id == vif->ar->wmi->ep_id))
		return -EINVAL;

	skb_push(skb, sizeof(struct wmi_data_hdr));

	data_hdr = (struct wmi_data_hdr *) skb->data;
	data_hdr->info = SYNC_MSGTYPE << WMI_DATA_HDR_MSG_TYPE_SHIFT;
	data_hdr->info3 = cpu_to_le16(vif->device_index & WMI_DATA_HDR_IF_IDX_MASK);

	SKB_CB_VIF(skb) = vif;
	ret = ath6kl_control_tx_sync(vif->ar->wmi->parent_dev, skb, ep_id);

	return ret;
}

static int ath6kl_wmi_sync_point(struct ath6kl_vif *vif)
{
	struct sk_buff *skb;
	struct wmi_sync_cmd *cmd;
	struct wmi_data_sync_bufs data_sync_bufs[WMM_NUM_AC];
	enum htc_endpoint_id ep_id;
	u8 index, num_pri_streams = 0;
	int ret = 0;

	memset(data_sync_bufs, 0, sizeof(data_sync_bufs));

	for (index = 0; index < WMM_NUM_AC; index++) {
		if (vif->ar->wmi->fat_pipe_exist & (1 << index)) {
			num_pri_streams++;
			data_sync_bufs[num_pri_streams - 1].traffic_class =
			    index;
		}
	}

	skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
	if (!skb) {
		ret = -ENOMEM;
		goto free_skb;
	}

	cmd = (struct wmi_sync_cmd *) skb->data;

	/*
	 * In the SYNC cmd sent on the control Ep, send a bitmap
	 * of the data eps on which the Data Sync will be sent
	 */
	cmd->data_sync_map = vif->ar->wmi->fat_pipe_exist;

	for (index = 0; index < num_pri_streams; index++) {
		data_sync_bufs[index].skb = ath6kl_buf_alloc(0);
		if (data_sync_bufs[index].skb == NULL) {
			ret = -ENOMEM;
			break;
		}
	}

	/*
	 * If buffer allocation for any of the dataSync fails,
	 * then do not send the Synchronize cmd on the control ep
	 */
	if (ret)
		goto free_skb;

	/*
	 * Send sync cmd followed by sync data messages on all
	 * endpoints being used
	 */

	ret = __ath6kl_wmi_cmd_send(vif, skb, WMI_SYNCHRONIZE_CMDID,
				  NO_SYNC_WMIFLAG);

	if (ret)
		goto free_skb;

	/* cmd buffer sent, we no longer own it */
	skb = NULL;

	for (index = 0; index < num_pri_streams; index++) {

		if (WARN_ON(!data_sync_bufs[index].skb))
			break;

		ep_id = ath6kl_ac2_endpoint_id(vif->ar->wmi->parent_dev,
					       data_sync_bufs[index].
					       traffic_class);
		ret =
		    ath6kl_wmi_data_sync_send(vif, data_sync_bufs[index].skb,
					      ep_id);

		if (ret)
			break;

		data_sync_bufs[index].skb = NULL;
	}

free_skb:
	/* free up any resources left over (possibly due to an error) */
	if (skb)
		dev_kfree_skb(skb);

	for (index = 0; index < num_pri_streams; index++) {
		if (data_sync_bufs[index].skb != NULL) {
			dev_kfree_skb((struct sk_buff *)data_sync_bufs[index].
				      skb);
		}
	}

	return ret;
}

int ath6kl_wmi_create_pstream_cmd(struct ath6kl_vif *vif,
				  struct wmi_create_pstream_cmd *params)
{
	struct sk_buff *skb;
	struct wmi_create_pstream_cmd *cmd;
	u8 fatpipe_exist_for_ac = 0;
	s32 min_phy = 0;
	s32 nominal_phy = 0;
	int ret;

	if (!((params->user_pri < 8) &&
	      (params->user_pri <= 0x7) &&
	      (up_to_ac[params->user_pri & 0x7] == params->traffic_class) &&
	      (params->traffic_direc == UPLINK_TRAFFIC ||
	       params->traffic_direc == DNLINK_TRAFFIC ||
	       params->traffic_direc == BIDIR_TRAFFIC) &&
	      (params->traffic_type == TRAFFIC_TYPE_APERIODIC ||
	       params->traffic_type == TRAFFIC_TYPE_PERIODIC) &&
	      (params->voice_psc_cap == DISABLE_FOR_THIS_AC ||
	       params->voice_psc_cap == ENABLE_FOR_THIS_AC ||
	       params->voice_psc_cap == ENABLE_FOR_ALL_AC) &&
	      (params->tsid == WMI_IMPLICIT_PSTREAM ||
	       params->tsid <= WMI_MAX_THINSTREAM))) {
		return -EINVAL;
	}

	/*
	 * Check nominal PHY rate is >= minimalPHY,
	 * so that DUT can allow TSRS IE
	 */

	/* Get the physical rate (units of bps) */
	min_phy = ((le32_to_cpu(params->min_phy_rate) / 1000) / 1000);

	/* Check minimal phy < nominal phy rate */
	if (params->nominal_phy >= min_phy) {
		/* unit of 500 kbps */
		nominal_phy = (params->nominal_phy * 1000) / 500;
		ath6kl_dbg(ATH6KL_DBG_WMI,
			   "TSRS IE enabled::MinPhy %x->NominalPhy ===> %x\n",
			   min_phy, nominal_phy);

		params->nominal_phy = nominal_phy;
	} else {
		params->nominal_phy = 0;
	}

	skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
	if (!skb)
		return -ENOMEM;

	ath6kl_dbg(ATH6KL_DBG_WMI,
		   "sending create_pstream_cmd: ac=%d  tsid:%d\n",
		   params->traffic_class, params->tsid);

	cmd = (struct wmi_create_pstream_cmd *) skb->data;
	memcpy(cmd, params, sizeof(*cmd));

	/* This is an implicitly created Fat pipe */
	if ((u32) params->tsid == (u32) WMI_IMPLICIT_PSTREAM) {
		spin_lock_bh(&vif->ar->wmi->lock);
		fatpipe_exist_for_ac = (vif->ar->wmi->fat_pipe_exist &
					(1 << params->traffic_class));
		vif->ar->wmi->fat_pipe_exist |= (1 << params->traffic_class);
		spin_unlock_bh(&vif->ar->wmi->lock);
	} else {
		/* explicitly created thin stream within a fat pipe */
		spin_lock_bh(&vif->ar->wmi->lock);
		fatpipe_exist_for_ac = (vif->ar->wmi->fat_pipe_exist &
					(1 << params->traffic_class));
		vif->ar->wmi->stream_exist_for_ac[params->traffic_class] |=
		    (1 << params->tsid);
		/*
		 * If a thinstream becomes active, the fat pipe automatically
		 * becomes active
		 */
		vif->ar->wmi->fat_pipe_exist |= (1 << params->traffic_class);
		spin_unlock_bh(&vif->ar->wmi->lock);
	}

	/*
	 * Indicate activty change to driver layer only if this is the
	 * first TSID to get created in this AC explicitly or an implicit
	 * fat pipe is getting created.
	 */
	if (!fatpipe_exist_for_ac)
		ath6kl_indicate_tx_activity(vif->ar->wmi->parent_dev,
					    params->traffic_class, true);

	ret = ath6kl_wmi_cmd_send(vif, skb, WMI_CREATE_PSTREAM_CMDID,
				  NO_SYNC_WMIFLAG);
	return ret;
}

int ath6kl_wmi_delete_pstream_cmd(struct ath6kl_vif *vif, u8 traffic_class, u8 tsid)
{
	struct sk_buff *skb;
	struct wmi_delete_pstream_cmd *cmd;
	u16 active_tsids = 0;
	int ret;

	if (traffic_class > 3) {
		ath6kl_err("invalid traffic class: %d\n", traffic_class);
		return -EINVAL;
	}

	skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
	if (!skb)
		return -ENOMEM;

	cmd = (struct wmi_delete_pstream_cmd *) skb->data;
	cmd->traffic_class = traffic_class;
	cmd->tsid = tsid;

	spin_lock_bh(&vif->ar->wmi->lock);
	active_tsids = vif->ar->wmi->stream_exist_for_ac[traffic_class];
	spin_unlock_bh(&vif->ar->wmi->lock);

	if (!(active_tsids & (1 << tsid))) {
		dev_kfree_skb(skb);
		ath6kl_dbg(ATH6KL_DBG_WMI,
			   "TSID %d doesn't exist for traffic class: %d\n",
			   tsid, traffic_class);
		return -ENODATA;
	}

	ath6kl_dbg(ATH6KL_DBG_WMI,
		   "sending delete_pstream_cmd: traffic class: %d tsid=%d\n",
		   traffic_class, tsid);

	ret = ath6kl_wmi_cmd_send(vif, skb, WMI_DELETE_PSTREAM_CMDID,
				  SYNC_BEFORE_WMIFLAG);

	spin_lock_bh(&vif->ar->wmi->lock);
	vif->ar->wmi->stream_exist_for_ac[traffic_class] &= ~(1 << tsid);
	active_tsids = vif->ar->wmi->stream_exist_for_ac[traffic_class];
	spin_unlock_bh(&vif->ar->wmi->lock);

	/*
	 * Indicate stream inactivity to driver layer only if all tsids
	 * within this AC are deleted.
	 */
	if (!active_tsids) {
		ath6kl_indicate_tx_activity(vif->ar->wmi->parent_dev,
					    traffic_class, false);
		vif->ar->wmi->fat_pipe_exist &= ~(1 << traffic_class);
	}

	return ret;
}

int ath6kl_wmi_set_ip_cmd(struct ath6kl_vif *vif, struct wmi_set_ip_cmd *ip_cmd)
{
	struct sk_buff *skb;
	struct wmi_set_ip_cmd *cmd;
	int ret;

	/* Multicast address are not valid */
	if ((*((u8 *) &ip_cmd->ips[0]) >= 0xE0) ||
	    (*((u8 *) &ip_cmd->ips[1]) >= 0xE0))
		return -EINVAL;

	skb = ath6kl_wmi_get_new_buf(sizeof(struct wmi_set_ip_cmd));
	if (!skb)
		return -ENOMEM;

	cmd = (struct wmi_set_ip_cmd *) skb->data;
	memcpy(cmd, ip_cmd, sizeof(struct wmi_set_ip_cmd));

	ret = ath6kl_wmi_cmd_send(vif, skb, WMI_SET_IP_CMDID, NO_SYNC_WMIFLAG);
	return ret;
}

static int ath6kl_wmi_get_wow_list_event_rx(struct ath6kl_vif *vif, u8 * datap,
					    int len)
{
	if (len < sizeof(struct wmi_get_wow_list_reply))
		return -EINVAL;

	return 0;
}

static int ath6kl_wmi_cmd_send_xtnd(struct ath6kl_vif *vif, struct sk_buff *skb,
				    enum wmix_command_id cmd_id,
				    enum wmi_sync_flag sync_flag)
{
	struct wmix_cmd_hdr *cmd_hdr;
	int ret;

	skb_push(skb, sizeof(struct wmix_cmd_hdr));

	cmd_hdr = (struct wmix_cmd_hdr *) skb->data;
	cmd_hdr->cmd_id = cpu_to_le32(cmd_id);

	ret = ath6kl_wmi_cmd_send(vif, skb, WMI_EXTENSION_CMDID, sync_flag);

	return ret;
}

int ath6kl_wmi_get_challenge_resp_cmd(struct ath6kl_vif *vif, u32 cookie, u32 source)
{
	struct sk_buff *skb;
	struct wmix_hb_challenge_resp_cmd *cmd;
	int ret;

	skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
	if (!skb)
		return -ENOMEM;

	cmd = (struct wmix_hb_challenge_resp_cmd *) skb->data;
	cmd->cookie = cpu_to_le32(cookie);
	cmd->source = cpu_to_le32(source);

	ret = ath6kl_wmi_cmd_send_xtnd(vif, skb, WMIX_HB_CHALLENGE_RESP_CMDID,
				       NO_SYNC_WMIFLAG);
	return ret;
}

int ath6kl_wmi_config_debug_module_cmd(struct ath6kl_vif *vif, u32 valid, u32 config)
{
	struct ath6kl_wmix_dbglog_cfg_module_cmd *cmd;
	struct sk_buff *skb;
	int ret;

	skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
	if (!skb)
		return -ENOMEM;

	cmd = (struct ath6kl_wmix_dbglog_cfg_module_cmd *) skb->data;
	cmd->valid = cpu_to_le32(valid);
	cmd->config = cpu_to_le32(config);

	ret = ath6kl_wmi_cmd_send_xtnd(vif, skb, WMIX_DBGLOG_CFG_MODULE_CMDID,
				       NO_SYNC_WMIFLAG);
	return ret;
}

int ath6kl_wmi_get_stats_cmd(struct ath6kl_vif *vif)
{
	return ath6kl_wmi_simple_cmd(vif, WMI_GET_STATISTICS_CMDID);
}

int ath6kl_wmi_get_channel_list_cmd(struct ath6kl_vif *vif)
{
	return ath6kl_wmi_simple_cmd(vif, WMI_GET_CHANNEL_LIST_CMDID);
}

int ath6kl_wmi_set_tx_pwr_cmd(struct ath6kl_vif *vif, u8 dbM)
{
	struct sk_buff *skb;
	struct wmi_set_tx_pwr_cmd *cmd;
	int ret;

	skb = ath6kl_wmi_get_new_buf(sizeof(struct wmi_set_tx_pwr_cmd));
	if (!skb)
		return -ENOMEM;

	cmd = (struct wmi_set_tx_pwr_cmd *) skb->data;
	cmd->dbM = dbM;

	ret = ath6kl_wmi_cmd_send(vif, skb, WMI_SET_TX_PWR_CMDID,
				  NO_SYNC_WMIFLAG);

	return ret;
}

int ath6kl_wmi_get_tx_pwr_cmd(struct ath6kl_vif *vif)
{
	return ath6kl_wmi_simple_cmd(vif, WMI_GET_TX_PWR_CMDID);
}

int ath6kl_wmi_set_lpreamble_cmd(struct ath6kl_vif *vif, u8 status, u8 preamble_policy)
{
	struct sk_buff *skb;
	struct wmi_set_lpreamble_cmd *cmd;
	int ret;

	skb = ath6kl_wmi_get_new_buf(sizeof(struct wmi_set_lpreamble_cmd));
	if (!skb)
		return -ENOMEM;

	cmd = (struct wmi_set_lpreamble_cmd *) skb->data;
	cmd->status = status;
	cmd->preamble_policy = preamble_policy;

	ret = ath6kl_wmi_cmd_send(vif, skb, WMI_SET_LPREAMBLE_CMDID,
				  NO_SYNC_WMIFLAG);
	return ret;
}

int ath6kl_wmi_set_rts_cmd(struct ath6kl_vif *vif, u16 threshold)
{
	struct sk_buff *skb;
	struct wmi_set_rts_cmd *cmd;
	int ret;

	skb = ath6kl_wmi_get_new_buf(sizeof(struct wmi_set_rts_cmd));
	if (!skb)
		return -ENOMEM;

	cmd = (struct wmi_set_rts_cmd *) skb->data;
	cmd->threshold = cpu_to_le16(threshold);

	ret = ath6kl_wmi_cmd_send(vif, skb, WMI_SET_RTS_CMDID, NO_SYNC_WMIFLAG);
	return ret;
}

int ath6kl_wmi_set_tx_sgi_cmd(struct ath6kl_vif *vif, u32* mask, u8 per_threshold)
{

	struct sk_buff *skb;
	struct wmi_set_tx_sgi_param_cmd *cmd;

	skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
	if (!skb)
		return -ENOMEM;

	ath6kl_dbg(ATH6KL_DBG_WMI, "set SGI mask: %0x %0x per_threshold:%u\n", mask[0], mask[1], per_threshold);

	cmd = (struct wmi_set_tx_sgi_param_cmd*) skb->data;
	memset(cmd, 0, sizeof(*cmd));
	cmd->mask[0] = mask[0];
	cmd->mask[1] = mask[1];
	cmd->per_threshold = per_threshold;

        return ath6kl_wmi_cmd_send(vif, skb, WMI_SET_TX_SGI_PARAM_CMDID,
                                   NO_SYNC_WMIFLAG);

}

int ath6kl_wmi_set_wmm_cmd(struct ath6kl_vif *vif, bool enable)
{
	struct sk_buff *skb;
	struct wmi_set_wmm_cmd *cmd;

	skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
	if (!skb)
		return -ENOMEM;

	ath6kl_dbg(ATH6KL_DBG_WMI, "set wmm: %d\n", enable);

	cmd = (struct wmi_set_wmm_cmd*) skb->data;
	cmd->status = enable ? 1 : 0;

        return ath6kl_wmi_cmd_send(vif, skb, WMI_SET_WMM_CMDID,
                                   NO_SYNC_WMIFLAG);
}


int ath6kl_wmi_set_dtim_cmd(struct ath6kl_vif *vif, u8 dtim)
{
	struct sk_buff *skb;
	struct wmi_set_dtim_cmd *cmd;

	skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
	if (!skb)
		return -ENOMEM;

	ath6kl_dbg(ATH6KL_DBG_WMI, "set dtim: %u\n", dtim);

	cmd = (struct wmi_set_dtim_cmd*) skb->data;
	cmd->dtim = dtim;

        return ath6kl_wmi_cmd_send(vif, skb, WMI_AP_SET_DTIM_CMDID,
                                   NO_SYNC_WMIFLAG);
}


int ath6kl_wmi_set_wmm_txop(struct ath6kl_vif *vif, enum wmi_txop_cfg cfg)
{
	struct sk_buff *skb;
	struct wmi_set_wmm_txop_cmd *cmd;
	int ret;

	if (!((cfg == WMI_TXOP_DISABLED) || (cfg == WMI_TXOP_ENABLED)))
		return -EINVAL;

	skb = ath6kl_wmi_get_new_buf(sizeof(struct wmi_set_wmm_txop_cmd));
	if (!skb)
		return -ENOMEM;

	cmd = (struct wmi_set_wmm_txop_cmd *) skb->data;
	cmd->txop_enable = cfg;

	ret = ath6kl_wmi_cmd_send(vif, skb, WMI_SET_WMM_TXOP_CMDID,
				  NO_SYNC_WMIFLAG);
	return ret;
}

int ath6kl_wmi_set_keepalive_cmd(struct ath6kl_vif *vif, u8 keep_alive_intvl)
{
	struct sk_buff *skb;
	struct wmi_set_keepalive_cmd *cmd;
	int ret;

	skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
	if (!skb)
		return -ENOMEM;

	cmd = (struct wmi_set_keepalive_cmd *) skb->data;
	cmd->keep_alive_intvl = keep_alive_intvl;
	vif->ar->wmi->keep_alive_intvl = keep_alive_intvl;

	ret = ath6kl_wmi_cmd_send(vif, skb, WMI_SET_KEEPALIVE_CMDID,
				  NO_SYNC_WMIFLAG);
	return ret;
}

s32 ath6kl_wmi_get_rate(s8 rate_index, u8 *rate_sgi, u8 *rate_mcs, 
		int ss_state)
{
	if (rate_index == RATE_AUTO)
		return 0;
	return ath6kl_wmi_parse_rate_tbl (rate_index, 
			rate_sgi, rate_mcs, ss_state);
	
}

static int ath6kl_wmi_get_pmkid_list_event_rx(struct ath6kl_vif *vif, u8 *datap,
					      u32 len)
{
	struct wmi_pmkid_list_reply *reply;
	u32 expected_len;

	if (len < sizeof(struct wmi_pmkid_list_reply))
		return -EINVAL;

	reply = (struct wmi_pmkid_list_reply *)datap;
	expected_len = sizeof(reply->num_pmkid) +
		le32_to_cpu(reply->num_pmkid) * WMI_PMKID_LEN;

	if (len < expected_len)
		return -EINVAL;

	return 0;
}

static int ath6kl_wmi_addba_req_event_rx(struct ath6kl_vif *vif, u8 *datap, int len)
{
	struct wmi_addba_req_event *cmd = (struct wmi_addba_req_event *) datap;

	aggr_recv_addba_req_evt(vif, cmd->tid,
				le16_to_cpu(cmd->st_seq_no), cmd->win_sz);

	return 0;
}

static int ath6kl_wmi_addba_resp_event_rx(struct ath6kl_vif *vif, u8 *datap, int len)
{
	struct wmi_addba_resp_event *cmd = (struct wmi_addba_resp_event *) datap;

	aggr_recv_addba_resp_evt(vif, cmd->tid,
				le16_to_cpu(cmd->amsdu_sz), cmd->status);

	return 0;
}

static int ath6kl_wmi_delba_req_event_rx(struct ath6kl_vif *vif, u8 *datap, int len)
{
	struct wmi_delba_event *cmd = (struct wmi_delba_event *) datap;

	aggr_recv_delba_req_evt(vif, cmd->tid);

	return 0;
}

/*  AP mode functions */

/*
 * When AR6K in AP mode, This command will be called after
 * changing ssid, channel etc. It will pass the profile to
 * target with a flag which will indicate which parameter changed,
 * also if this flag is 0, there was no change in parametes, so
 * commit cmd will not be sent to target. Without calling this IOCTL
 * the changes will not take effect.
 */

int ath6kl_wmi_ap_profile_commit(struct ath6kl_vif *vif, struct wmi_connect_cmd *p)
{
	struct sk_buff *skb;
	struct wmi_connect_cmd *cm;
	int res;

	skb = ath6kl_wmi_get_new_buf(sizeof(*cm));
	if (!skb)
		return -ENOMEM;

	cm = (struct wmi_connect_cmd *) skb->data;
	memcpy(cm, p, sizeof(*cm));

	res = ath6kl_wmi_cmd_send(vif, skb, WMI_AP_CONFIG_COMMIT_CMDID,
				  NO_SYNC_WMIFLAG);
	ath6kl_dbg(ATH6KL_DBG_WMI, "%s: nw_type=%u auth_mode=%u ch=%u "
		   "ctrl_flags=0x%x-> res=%d\n",
		   __func__, p->nw_type, p->auth_mode, le16_to_cpu(p->ch),
		   le32_to_cpu(p->ctrl_flags), res);
	return res;
}

int ath6kl_wmi_ap_set_mlme(struct ath6kl_vif *vif, u8 cmd, const u8 *mac, u16 reason)
{
	struct sk_buff *skb;
	struct wmi_ap_set_mlme_cmd *cm;

	skb = ath6kl_wmi_get_new_buf(sizeof(*cm));
	if (!skb)
		return -ENOMEM;

	cm = (struct wmi_ap_set_mlme_cmd *) skb->data;
	memcpy(cm->mac, mac, ETH_ALEN);
	cm->reason = cpu_to_le16(reason);
	cm->cmd = cmd;

	return ath6kl_wmi_cmd_send(vif, skb, WMI_AP_SET_MLME_CMDID,
				   NO_SYNC_WMIFLAG);
}


/*
 * This command will be used to enable/disable AP uAPSD feature
 */
int ath6kl_wmi_ap_set_apsd(struct ath6kl_vif *vif, u8 enable)
{
	struct wmi_ap_set_apsd_cmd *p;
	struct sk_buff *skb;

	skb = ath6kl_wmi_get_new_buf(sizeof(*p));
	if (!skb)
		return -ENOMEM;

	p = (struct wmi_ap_set_apsd_cmd *)skb->data;
	p->enable = enable;

	return ath6kl_wmi_cmd_send(vif, skb, WMI_AP_SET_APSD_CMDID,
				   NO_SYNC_WMIFLAG);
}

int ath6kl_wmi_set_apsd_buffered_traffic_cmd(struct ath6kl_vif *vif, u16 aid, u16 bitmap, u32 flags)
{
	struct wmi_ap_apsd_buffered_traffic_cmd *p;
	struct sk_buff *skb;

	skb = ath6kl_wmi_get_new_buf(sizeof(*p));
	if (!skb)
		return -ENOMEM;

	p = (struct wmi_ap_apsd_buffered_traffic_cmd *)skb->data;
	p->aid = aid;
	p->bitmap = bitmap;
	p->flags = flags;

	return ath6kl_wmi_cmd_send(vif, skb, WMI_AP_APSD_BUFFERED_TRAFFIC_CMDID,
				   NO_SYNC_WMIFLAG);
}


static int ath6kl_wmi_pspoll_event_rx(struct ath6kl_vif *vif, u8 *datap, int len)
{
	struct wmi_pspoll_event *ev;

	if (len < sizeof(struct wmi_pspoll_event))
		return -EINVAL;

	ev = (struct wmi_pspoll_event *) datap;

	ath6kl_pspoll_event(vif, le16_to_cpu(ev->aid));

	return 0;
}

static int ath6kl_wmi_dtimexpiry_event_rx(struct ath6kl_vif *vif, u8 *datap, int len)
{
	ath6kl_dtimexpiry_event(vif);

	return 0;
}

int ath6kl_wmi_set_pvb_cmd(struct ath6kl_vif *vif, u16 aid, bool flag)
{
	struct sk_buff *skb;
	struct wmi_ap_set_pvb_cmd *cmd;
	int ret;

	skb = ath6kl_wmi_get_new_buf(sizeof(struct wmi_ap_set_pvb_cmd));
	if (!skb)
		return -ENOMEM;

	cmd = (struct wmi_ap_set_pvb_cmd *) skb->data;
	cmd->aid = cpu_to_le16(aid);
	cmd->rsvd = cpu_to_le16(0);
	cmd->flag = cpu_to_le32(flag);

	ret = ath6kl_wmi_cmd_send(vif, skb, WMI_AP_SET_PVB_CMDID,
				  NO_SYNC_WMIFLAG);

	return 0;
}

int ath6kl_wmi_set_rx_frame_format_cmd(struct ath6kl_vif *vif, u8 rx_meta_ver,
				       bool rx_dot11_hdr, bool defrag_on_host)
{
	struct sk_buff *skb;
	struct wmi_rx_frame_format_cmd *cmd;
	int ret;

	skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
	if (!skb)
		return -ENOMEM;

	cmd = (struct wmi_rx_frame_format_cmd *) skb->data;
	cmd->dot11_hdr = rx_dot11_hdr ? 1 : 0;
	cmd->defrag_on_host = defrag_on_host ? 1 : 0;
	cmd->meta_ver = rx_meta_ver;

	/* Delete the local aggr state, on host */
	ret = ath6kl_wmi_cmd_send(vif, skb, WMI_RX_FRAME_FORMAT_CMDID,
				  NO_SYNC_WMIFLAG);

	return ret;
}

int ath6kl_wmi_disable_11b_rates_cmd(struct ath6kl_vif *vif, bool disable)
{
	struct sk_buff *skb;
	struct wmi_disable_11b_rates_cmd *cmd;

	skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
	if (!skb)
		return -ENOMEM;

	ath6kl_dbg(ATH6KL_DBG_WMI, "disable_11b_rates_cmd: disable=%u\n",
		   disable);
	cmd = (struct wmi_disable_11b_rates_cmd *) skb->data;
	cmd->disable = disable ? 1 : 0;

	return ath6kl_wmi_cmd_send(vif, skb, WMI_DISABLE_11B_RATES_CMDID,
				   NO_SYNC_WMIFLAG);
}

int ath6kl_wmi_remain_on_chnl_cmd(struct ath6kl_vif *vif, u32 freq, u32 dur)
{
	struct sk_buff *skb;
	struct wmi_remain_on_chnl_cmd *p;

	skb = ath6kl_wmi_get_new_buf(sizeof(*p));
	if (!skb)
		return -ENOMEM;

	ath6kl_dbg(ATH6KL_DBG_WMI, "remain_on_chnl_cmd: freq=%u dur=%u\n",
		   freq, dur);
	p = (struct wmi_remain_on_chnl_cmd *) skb->data;
	p->freq = cpu_to_le32(freq);
	p->duration = cpu_to_le32(dur);
	return ath6kl_wmi_cmd_send(vif, skb, WMI_REMAIN_ON_CHNL_CMDID,
				   NO_SYNC_WMIFLAG);
}

int ath6kl_wmi_send_action_cmd(struct ath6kl_vif *vif, u32 id, u32 freq, u32 wait,
			       const u8 *data, u16 data_len)
{
	struct sk_buff *skb;
	struct wmi_send_action_cmd *p;
	struct wmi_mgmt_tx_frame *mgmt_tx_frame = NULL;
	struct ieee80211_mgmt *mgmt = (struct ieee80211_mgmt *) data;
	u8 *buf = NULL;

	if (wait)
		return -EINVAL; /* Offload for wait not supported */

	/* Only need to take care of Action frames in current application. */
	if (ieee80211_is_action(mgmt->frame_control)) {
		buf = kmalloc(data_len, GFP_KERNEL);
		if (!buf)
			return -ENOMEM;
		mgmt_tx_frame = kmalloc(sizeof(struct wmi_mgmt_tx_frame), GFP_KERNEL);
		if (!mgmt_tx_frame) {
			kfree(buf);
			return -ENOMEM;
		}

		memcpy(buf, data, data_len);
		mgmt_tx_frame->mgmt_tx_frame = buf;
		mgmt_tx_frame->mgmt_tx_frame_len = data_len;
		mgmt_tx_frame->mgmt_tx_frame_idx = id;
		list_add_tail(&mgmt_tx_frame->list, &vif->ar->wmi->mgmt_tx_frame);
	}

	skb = ath6kl_wmi_get_new_buf(sizeof(*p) + data_len);
	if (!skb) {
		kfree(buf);
		kfree(mgmt_tx_frame);
		return -ENOMEM;
	}

	ath6kl_dbg(ATH6KL_DBG_WMI, "send_action_cmd: id=%u freq=%u wait=%u "
		   "len=%u\n", id, freq, wait, data_len);
	p = (struct wmi_send_action_cmd *) skb->data;
	p->id = cpu_to_le32(id);
	p->freq = cpu_to_le32(freq);
	p->wait = cpu_to_le32(wait);
	p->len = cpu_to_le16(data_len);
	memcpy(p->data, data, data_len);
	return ath6kl_wmi_cmd_send(vif, skb, WMI_SEND_ACTION_CMDID,
				   NO_SYNC_WMIFLAG);
}

int ath6kl_wmi_send_probe_response_cmd(struct ath6kl_vif *vif, u32 freq,
				       const u8 *dst,
				       const u8 *data, u16 data_len)
{
	struct sk_buff *skb;
	struct wmi_p2p_probe_response_cmd *p;
	size_t cmd_len = sizeof(*p) + data_len;

	if (data_len == 0)
		cmd_len++; /* work around target minimum length requirement */
	skb = ath6kl_wmi_get_new_buf(cmd_len);
	if (!skb)
		return -ENOMEM;

	ath6kl_dbg(ATH6KL_DBG_WMI, "send_probe_response_cmd: freq=%u dst=%pM "
		   "len=%u\n", freq, dst, data_len);
	p = (struct wmi_p2p_probe_response_cmd *) skb->data;
	p->freq = cpu_to_le32(freq);
	memcpy(p->destination_addr, dst, ETH_ALEN);
	p->len = cpu_to_le16(data_len);
	if (data_len > 0) {
		memcpy(p->data, data, data_len);
	}
	return ath6kl_wmi_cmd_send(vif, skb, WMI_SEND_PROBE_RESPONSE_CMDID,
				   NO_SYNC_WMIFLAG);
}

int ath6kl_wmi_probe_report_req_cmd(struct ath6kl_vif *vif, bool enable)
{
	struct sk_buff *skb;
	struct wmi_probe_req_report_cmd *p;

	skb = ath6kl_wmi_get_new_buf(sizeof(*p));
	if (!skb)
		return -ENOMEM;

	ath6kl_dbg(ATH6KL_DBG_WMI, "probe_report_req_cmd: enable=%u\n",
		   enable);
	p = (struct wmi_probe_req_report_cmd *) skb->data;
	p->enable = enable ? 1 : 0;
	return ath6kl_wmi_cmd_send(vif, skb, WMI_PROBE_REQ_REPORT_CMDID,
				   NO_SYNC_WMIFLAG);
}

int ath6kl_wmi_info_req_cmd(struct ath6kl_vif *vif, u32 info_req_flags)
{
	struct sk_buff *skb;
	struct wmi_get_p2p_info *p;

	skb = ath6kl_wmi_get_new_buf(sizeof(*p));
	if (!skb)
		return -ENOMEM;

	ath6kl_dbg(ATH6KL_DBG_WMI, "info_req_cmd: flags=%x\n",
		   info_req_flags);
	p = (struct wmi_get_p2p_info *) skb->data;
	p->info_req_flags = cpu_to_le32(info_req_flags);
	return ath6kl_wmi_cmd_send(vif, skb, WMI_GET_P2P_INFO_CMDID,
				   NO_SYNC_WMIFLAG);
}

int ath6kl_wmi_cancel_remain_on_chnl_cmd(struct ath6kl_vif *vif)
{
	ath6kl_dbg(ATH6KL_DBG_WMI, "cancel_remain_on_chnl_cmd\n");
	return ath6kl_wmi_simple_cmd(vif, WMI_CANCEL_REMAIN_ON_CHNL_CMDID);
}

int ath6kl_wmi_abort_scan_cmd(struct ath6kl_vif *vif)
{
	ath6kl_dbg(ATH6KL_DBG_WMI, "abort_scan_cmd\n");
	return ath6kl_wmi_simple_cmd(vif, WMI_ABORT_SCAN_CMDID);
}

int ath6kl_wmi_set_appie_cmd(struct ath6kl_vif *vif, u8 mgmt_frm_type, const u8 *ie,
			     u8 ie_len)
{
	struct sk_buff *skb;
	struct wmi_set_appie_cmd *p;

	skb = ath6kl_wmi_get_new_buf(sizeof(*p) + ie_len);
	if (!skb)
		return -ENOMEM;

	ath6kl_dbg(ATH6KL_DBG_WMI, "set_appie_cmd: mgmt_frm_type=%u "
		   "ie_len=%u\n", mgmt_frm_type, ie_len);
	p = (struct wmi_set_appie_cmd *) skb->data;
	p->mgmt_frm_type = mgmt_frm_type;
	p->ie_len = ie_len;
	memcpy(p->ie_info, ie, ie_len);
	return ath6kl_wmi_cmd_send(vif, skb, WMI_SET_APPIE_CMDID,
				   NO_SYNC_WMIFLAG);
}

int ath6kl_wmi_setratectrl_cmd(struct ath6kl_vif *vif, u32 ratemode)
{
	struct sk_buff *skb;
	struct  wmi_set_ratectrl_parm_cmd *cmd;

	skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
	if (!skb)
		return -ENOMEM;

	ath6kl_dbg(ATH6KL_DBG_WMI, "ath6kl_wmi_setratectrl_cmd: mode=%d\n",
		   ratemode);
	cmd = (struct wmi_set_ratectrl_parm_cmd *) skb->data;
	cmd->mode= ratemode ? 1 : 0;

	return ath6kl_wmi_cmd_send(vif, skb, WMI_SET_RATECTRL_PARM_CMDID,
	                NO_SYNC_WMIFLAG);
}

int ath6kl_wmi_set_tx_select_rates_on_all_mode(struct ath6kl_vif *vif, u64 mask)
{
	struct sk_buff *skb;
	struct wmi_set_tx_select_rate_cmd *cmd;
	int ret;
    int i;
    u64 txselectmask;

	skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
	if (!skb)
		return -ENOMEM;

    cmd = (struct wmi_set_tx_select_rate_cmd *) skb->data;
    txselectmask = cpu_to_le64(mask);

    for (i = 0; i < (WMI_MODE_MAX*WMI_MAX_RATE_MASK); i += 2)
    {
        memcpy((char *)(cmd->rateMasks + i), (char *)&txselectmask, sizeof(txselectmask));
    }
     
	ret = ath6kl_wmi_cmd_send(vif, skb, WMI_SET_TX_SELECT_RATES_CMDID,
				  NO_SYNC_WMIFLAG);
	return ret;
}

static int ath6kl_wmi_control_rx_xtnd(struct ath6kl_vif *vif, struct sk_buff *skb)
{
	struct wmix_cmd_hdr *cmd;
	u32 len;
	u16 id;
	u8 *datap;
	int ret = 0;
	__le32 val;
	int i;

	if (skb->len < sizeof(struct wmix_cmd_hdr)) {
		ath6kl_err("bad packet 1\n");
		vif->ar->wmi->stat.cmd_len_err++;
		return -EINVAL;
	}

	cmd = (struct wmix_cmd_hdr *) skb->data;
	id = le32_to_cpu(cmd->cmd_id);

	skb_pull(skb, sizeof(struct wmix_cmd_hdr));

	datap = skb->data;
	len = skb->len;

	switch (id) {
	case WMIX_HB_CHALLENGE_RESP_EVENTID:
		break;
	case WMIX_DBGLOG_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMIX_DBGLOG_EVENTID: %u bytes\n",
			   len);
		ath6kl_debug_fwlog_event(vif->ar->wmi->parent_dev, datap, len);

		val = cpu_to_le32(jiffies);
		ath6kl_dbglog_add((char *) &val, 4);
		val = cpu_to_le32(len);
		ath6kl_dbglog_add((char *) &val, 4);
		ath6kl_dbglog_add(datap, len);
		val = cpu_to_le32(0);
		for (i = len; i < 1500; i += 4)
			ath6kl_dbglog_add((char *) &val, 4);
		break;
	default:
		ath6kl_err("unknown cmd id 0x%x\n", id);
		vif->ar->wmi->stat.cmd_id_err++;
		ret = -EINVAL;
		break;
	}

	return ret;
}

/* Control Path */
int ath6kl_wmi_control_rx(struct ath6kl_vif *vif, struct sk_buff *skb)
{
	extern athcfg_wcmd_testmode_t testmode_private;  
	struct wmi_cmd_hdr *cmd;
	u32 len;
	u16 id;
	u8 *datap;
	int ret = 0;
	u32 regcode;
	struct ath6kl *ar = vif->ar;

	if (WARN_ON(skb == NULL))
		return -EINVAL;

	if (skb->len < sizeof(struct wmi_cmd_hdr)) {
		ath6kl_err("bad packet 1\n");
		dev_kfree_skb(skb);
		vif->ar->wmi->stat.cmd_len_err++;
		return -EINVAL;
	}

	cmd = (struct wmi_cmd_hdr *) skb->data;
	id = le16_to_cpu(cmd->cmd_id);

	skb_pull(skb, sizeof(struct wmi_cmd_hdr));

	datap = skb->data;
	len = skb->len;

	ath6kl_dbg(ATH6KL_DBG_WMI, "%s: wmi id: %d\n", __func__, id);
	ath6kl_dbg_dump(ATH6KL_DBG_RAW_BYTES, "msg payload ", datap, len);

	/* Keep WMI event be processed in sequence. */
	if (down_interruptible(&ar->wmi_evt_sem)) {
		ath6kl_err("ath6kl_wmi_control_rx busy, couldn't get access\n");
		return -ERESTARTSYS;
	}

	switch (id) {
	case WMI_GET_BITRATE_CMDID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_GET_BITRATE_CMDID\n");
		ret = ath6kl_wmi_bitrate_reply_rx(vif, datap, len);
		break;
	case WMI_GET_CHANNEL_LIST_CMDID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_GET_CHANNEL_LIST_CMDID\n");
		ret = ath6kl_wmi_ch_list_reply_rx(vif, datap, len);
		break;
	case WMI_GET_TX_PWR_CMDID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_GET_TX_PWR_CMDID\n");
		ret = ath6kl_wmi_tx_pwr_reply_rx(vif, datap, len);
		break;
	case WMI_READY_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_READY_EVENTID\n");
		ret = ath6kl_wmi_ready_event_rx(vif, datap, len);
		break;
	case WMI_CONNECT_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_CONNECT_EVENTID\n");
		ret = ath6kl_wmi_connect_event_rx(vif, datap, len);
		break;
	case WMI_DISCONNECT_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_DISCONNECT_EVENTID\n");
		ret = ath6kl_wmi_disconnect_event_rx(vif, datap, len);
		break;
	case WMI_PEER_NODE_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_PEER_NODE_EVENTID\n");
		ret = ath6kl_wmi_peer_node_event_rx(vif, datap, len);
		break;
	case WMI_TKIP_MICERR_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_TKIP_MICERR_EVENTID\n");
		ret = ath6kl_wmi_tkip_micerr_event_rx(vif, datap, len);
		break;
	case WMI_BSSINFO_EVENTID:
        //printk( "%s[%d] WMI_BSSINFO_EVENTID,vif=0x%x\n",__func__,__LINE__,(int*)vif);
        if(test_bit(SONY_WMI_UPDATE, &vif->flag)) {
            ret = ath6kl_wmi_sony_get_stainfo_beacon_rssi_event_rx(vif, datap, len);
            break;
        }
        if(test_bit(SONY_WMI_SCAN, &vif->flag)) {
            ret = ath6kl_wmi_sony_get_scaninfo_event_rx(vif, datap, len);
            printk( "%s[%d] SONY_WMI_SCAN\n",__func__,__LINE__);
            break;
        }    
        if(test_bit(SONY_WMI_TESTMODE_RX, &vif->flag)) {
            struct wmi_bss_info_hdr2 *bih;
            u8 *buf;
//            struct ieee80211_channel *channel;

            if (len <= sizeof(struct wmi_bss_info_hdr2))
                break;

            bih = (struct wmi_bss_info_hdr2 *) datap;
            buf = datap + sizeof(struct wmi_bss_info_hdr2);
            len -= sizeof(struct wmi_bss_info_hdr2);
//			extern athcfg_wcmd_testmode_t testmode_private;  
            if(bih->frame_type == PROBERESP_FTYPE) {
                printk( "%s[%d] PROBERESP_FTYPE\n",__func__,__LINE__);
            } else if(bih->frame_type == BEACON_FTYPE) {
                //printk( "%s[%d] BEACON_FTYPE\n",__func__,__LINE__);
            }
            if( memcmp(testmode_private.bssid, bih->bssid,sizeof(testmode_private.bssid)) == 0) {
                printk(
                       "bss info evt - ch %u, snr %d, rssi %d, bssid \"%pM\" "
                       "frame_type=%d\n",
                       bih->ch, bih->snr, bih->snr - 95, bih->bssid,
                       bih->frame_type);
                testmode_private.rssi_combined = bih->snr;
            } else {
                //printk( "%s[%d] \"%pM\" \n",__func__,__LINE__,bih->bssid);
            }
            break;
        }        

		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_BSSINFO_EVENTID\n");
		ret = ath6kl_wmi_bssinfo_event_rx(vif, datap, len);
		break;
	case WMI_REGDOMAIN_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_REGDOMAIN_EVENTID\n");
		memcpy(&regcode, datap, 4);
		ath6kl_info("%s: 0x%x WWR:%d\n",
			    regcode & BIT(31) ? "Countery Code" : "Reg Domain",
			    regcode & 0xfff, !!(regcode & BIT(30)));
		break;
	case WMI_PSTREAM_TIMEOUT_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_PSTREAM_TIMEOUT_EVENTID\n");
		ret = ath6kl_wmi_pstream_timeout_event_rx(vif, datap, len);
		break;
	case WMI_NEIGHBOR_REPORT_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_NEIGHBOR_REPORT_EVENTID\n");
		break;
	case WMI_SCAN_COMPLETE_EVENTID:
        //printk( "%s[%d] WMI_SCAN_COMPLETE_EVENTID\n",__func__,__LINE__);
        if(test_bit(SONY_WMI_SCAN, &vif->flag)) {
            ret = ath6kl_wmi_sony_set_scan_done_event_rx(vif, datap, len);
            break;
        }
        if(test_bit(SONY_WMI_TESTMODE_RX, &vif->flag)) {
            //issue a scan again
            extern int ath6kl_wmi_set_customer_testmode_cmd(struct ath6kl_vif *vif, athcfg_wcmd_testmode_t *val);
            athcfg_wcmd_testmode_t testmode;
            testmode.operation = ATHCFG_WCMD_TESTMODE_RX;
            testmode.rx = 1;
            ath6kl_wmi_set_customer_testmode_cmd(vif,&testmode);
            break;
        }

		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_SCAN_COMPLETE_EVENTID\n");
		ret = ath6kl_wmi_scan_complete_rx(vif, datap, len);
		break;
	case WMI_CMDERROR_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_CMDERROR_EVENTID\n");
		ret = ath6kl_wmi_error_event_rx(vif, datap, len);
		break;
	case WMI_REPORT_STATISTICS_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_REPORT_STATISTICS_EVENTID\n");
		ret = ath6kl_wmi_stats_event_rx(vif, datap, len);
		break;
	case WMI_RSSI_THRESHOLD_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_RSSI_THRESHOLD_EVENTID\n");
		ret = ath6kl_wmi_rssi_threshold_event_rx(vif, datap, len);
		break;
	case WMI_ERROR_REPORT_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_ERROR_REPORT_EVENTID\n");
		break;
	case WMI_OPT_RX_FRAME_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_OPT_RX_FRAME_EVENTID\n");
		/* this event has been deprecated */
		break;
	case WMI_REPORT_ROAM_TBL_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_REPORT_ROAM_TBL_EVENTID\n");
		break;
	case WMI_EXTENSION_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_EXTENSION_EVENTID\n");
		ret = ath6kl_wmi_control_rx_xtnd(vif, skb);
		break;
	case WMI_CAC_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_CAC_EVENTID\n");
		ret = ath6kl_wmi_cac_event_rx(vif, datap, len);
		break;
	case WMI_CHANNEL_CHANGE_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_CHANNEL_CHANGE_EVENTID\n");
		break;
	case WMI_REPORT_ROAM_DATA_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_REPORT_ROAM_DATA_EVENTID\n");
		break;
	case WMI_GET_FIXRATES_CMDID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_GET_FIXRATES_CMDID\n");
		ret = ath6kl_wmi_ratemask_reply_rx(vif, datap, len);
		break;
	case WMI_TX_RETRY_ERR_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_TX_RETRY_ERR_EVENTID\n");
		break;
	case WMI_SNR_THRESHOLD_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_SNR_THRESHOLD_EVENTID\n");
		ret = ath6kl_wmi_snr_threshold_event_rx(vif, datap, len);
		break;
	case WMI_LQ_THRESHOLD_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_LQ_THRESHOLD_EVENTID\n");
		break;
	case WMI_APLIST_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_APLIST_EVENTID\n");
		ret = ath6kl_wmi_aplist_event_rx(vif, datap, len);
		break;
	case WMI_GET_KEEPALIVE_CMDID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_GET_KEEPALIVE_CMDID\n");
		ret = ath6kl_wmi_keepalive_reply_rx(vif, datap, len);
		break;
	case WMI_GET_WOW_LIST_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_GET_WOW_LIST_EVENTID\n");
		ret = ath6kl_wmi_get_wow_list_event_rx(vif, datap, len);
		break;
	case WMI_GET_PMKID_LIST_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_GET_PMKID_LIST_EVENTID\n");
		ret = ath6kl_wmi_get_pmkid_list_event_rx(vif, datap, len);
		break;
	case WMI_PSPOLL_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_PSPOLL_EVENTID\n");
		ret = ath6kl_wmi_pspoll_event_rx(vif, datap, len);
		break;
	case WMI_DTIMEXPIRY_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_DTIMEXPIRY_EVENTID\n");
		ret = ath6kl_wmi_dtimexpiry_event_rx(vif, datap, len);
		break;
	case WMI_SET_PARAMS_REPLY_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_SET_PARAMS_REPLY_EVENTID\n");
		break;
	case WMI_ADDBA_REQ_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_ADDBA_REQ_EVENTID\n");
		ret = ath6kl_wmi_addba_req_event_rx(vif, datap, len);
		break;
	case WMI_ADDBA_RESP_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_ADDBA_RESP_EVENTID\n");
		ret = ath6kl_wmi_addba_resp_event_rx(vif, datap, len);
		break;
	case WMI_DELBA_REQ_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_DELBA_REQ_EVENTID\n");
		ret = ath6kl_wmi_delba_req_event_rx(vif, datap, len);
		break;
	case WMI_REPORT_BTCOEX_CONFIG_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI,
			   "WMI_REPORT_BTCOEX_CONFIG_EVENTID\n");
		break;
	case WMI_REPORT_BTCOEX_STATS_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI,
			   "WMI_REPORT_BTCOEX_STATS_EVENTID\n");
		break;
	case WMI_TX_COMPLETE_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_TX_COMPLETE_EVENTID\n");
		ret = ath6kl_wmi_tx_complete_event_rx(datap, len);
		break;
	case WMI_REMAIN_ON_CHNL_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_REMAIN_ON_CHNL_EVENTID\n");
		ret = ath6kl_wmi_remain_on_chnl_event_rx(vif, datap, len);
		break;
	case WMI_CANCEL_REMAIN_ON_CHNL_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI,
			   "WMI_CANCEL_REMAIN_ON_CHNL_EVENTID\n");
		ret = ath6kl_wmi_cancel_remain_on_chnl_event_rx(vif, datap,
								len);
		break;
	case WMI_TX_STATUS_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_TX_STATUS_EVENTID\n");
		ret = ath6kl_wmi_tx_status_event_rx(vif, datap, len);
		break;
	case WMI_RX_PROBE_REQ_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_RX_PROBE_REQ_EVENTID\n");
		ret = ath6kl_wmi_rx_probe_req_event_rx(vif, datap, len);
		break;
	case WMI_P2P_CAPABILITIES_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_P2P_CAPABILITIES_EVENTID\n");
		ret = ath6kl_wmi_p2p_capabilities_event_rx(datap, len);
		break;
	case WMI_RX_ACTION_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_RX_ACTION_EVENTID\n");
		ret = ath6kl_wmi_rx_action_event_rx(vif, datap, len);
		break;
	case WMI_P2P_INFO_EVENTID:
		ath6kl_dbg(ATH6KL_DBG_WMI, "WMI_P2P_INFO_EVENTID\n");
		ret = ath6kl_wmi_p2p_info_event_rx(datap, len);
		break;
	case WMI_GET_REG_EVENTID:
		printk( "%s[%d] WMI_GET_REG_EVENTID\n",__func__,__LINE__);
		ret = ath6kl_wmi_sony_get_reg_event_rx(vif, datap, len);
		break;
	case WMI_GET_STAINFO_EVENTID:
		printk( "%s[%d] WMI_GET_STAINFO_EVENTID\n",__func__,__LINE__);
		ret = ath6kl_wmi_sony_get_stainfo_event_rx(vif, datap, len);
		break;    
	case WMI_GET_TXPOW_EVENTID:
		printk( "%s[%d] WMI_GET_TXPOW_EVENTID\n",__func__,__LINE__);
		ret = ath6kl_wmi_sony_get_txpow_event_rx(vif, datap, len);
		break;
	case WMI_GET_VERSION_INFO_EVENTID:
		printk( "%s[%d] WMI_GET_VERSION_INFO_EVENTID\n",__func__,__LINE__);
		ret = ath6kl_wmi_sony_get_version_info_event_rx(vif, datap, len);
		break;    
	case WMI_GET_TESTMODE_EVENTID:
		printk( "%s[%d] WMI_GET_TESTMODE_EVENTID\n",__func__,__LINE__);
		ret = ath6kl_wmi_sony_get_testmode_event_rx(vif, datap, len);
		break;
	default:
		ath6kl_dbg(ATH6KL_DBG_WMI, "unknown cmd id 0x%x\n", id);
		vif->ar->wmi->stat.cmd_id_err++;
		ret = -EINVAL;
		break;
	}
	up(&ar->wmi_evt_sem);
	
	dev_kfree_skb(skb);

	return ret;
}

static void ath6kl_wmi_qos_state_init(struct wmi *wmi)
{
	if (!wmi)
		return;

	spin_lock_bh(&wmi->lock);

	wmi->fat_pipe_exist = 0;
	memset(wmi->stream_exist_for_ac, 0, sizeof(wmi->stream_exist_for_ac));

	spin_unlock_bh(&wmi->lock);
}

void *ath6kl_wmi_init(struct ath6kl *dev)
{
	struct wmi *wmi;

	wmi = kzalloc(sizeof(struct wmi), GFP_KERNEL);
	if (!wmi)
		return NULL;

	spin_lock_init(&wmi->lock);

	wmi->parent_dev = dev;

	ath6kl_wmi_qos_state_init(wmi);

	wmi->pwr_mode = REC_POWER;
	wmi->phy_mode = WMI_11G_MODE;

	wmi->pair_crypto_type = NONE_CRYPT;
	wmi->grp_crypto_type = NONE_CRYPT;

	wmi->ht_allowed[A_BAND_24GHZ] = 1;
	wmi->ht_allowed[A_BAND_5GHZ] = 1;

	INIT_LIST_HEAD(&wmi->mgmt_tx_frame);

	sema_init(&wmi->sem, 1);

	return wmi;
}

void ath6kl_wmi_shutdown(struct wmi *wmi)
{
	struct wmi_mgmt_tx_frame *mgmt_tx_frame, *tmp;

	if (!wmi)
		return;

	list_for_each_entry_safe(mgmt_tx_frame, tmp, &wmi->mgmt_tx_frame, list) {
		list_del(&mgmt_tx_frame->list);
		kfree(mgmt_tx_frame->mgmt_tx_frame);
		kfree(mgmt_tx_frame);
	}

	kfree(wmi);
}

int ath6kl_wmi_set_bundle_param_cmd(struct ath6kl_vif *vif,
		u8 frame, u8 timeout)
{
    struct sk_buff *skb;
    struct wmi_set_bundle_param_cmd *cmd;
    int ret = 0;

    skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
	if(!skb)
		return -ENOMEM;

    cmd = (struct wmi_set_bundle_param_cmd *)skb->data;
	cmd->min_rx_bundle_frame = frame;
    cmd->min_rx_bundle_timeout = timeout;

	ret = ath6kl_wmi_cmd_send(vif, skb, WMI_SET_BUNDLE_PARAM_CMDID, 
				NO_SYNC_WMIFLAG);

	return ret;
}

int ath6kl_wmi_add_wow_pattern_cmd(struct ath6kl_vif *vif,
			    struct wmi_add_wow_pattern_cmd *add_wow_cmd,
			    u8 *pattern, u8 *mask, u8 pattern_size)
{
	struct sk_buff *skb;
	struct wmi_add_wow_pattern_cmd *cmd;
	u16 size;
	u8 *filter_mask = NULL;
	int ret;

	size = sizeof(struct wmi_add_wow_pattern_cmd) +
	    ((2 * add_wow_cmd->filter_size) * sizeof(u8));

	skb = ath6kl_wmi_get_new_buf(size);
	if (!skb)
		return -ENOMEM;

	cmd = (struct wmi_add_wow_pattern_cmd *) skb->data;
	cmd->filter_list_id = add_wow_cmd->filter_list_id;
	cmd->filter_offset = add_wow_cmd->filter_offset;
	cmd->filter_size = add_wow_cmd->filter_size;

	memcpy(cmd->filter, pattern, add_wow_cmd->filter_size);

	filter_mask = (u8 *) (cmd->filter + cmd->filter_size);
	memcpy(filter_mask, mask, add_wow_cmd->filter_size);

	ath6kl_dbg(ATH6KL_DBG_WOWLAN, "Adding pattern");
	ret = ath6kl_wmi_cmd_send(vif, skb, WMI_ADD_WOW_PATTERN_CMDID,
				  NO_SYNC_WMIFLAG);
	return ret;
}

int ath6kl_wmi_add_wow_ext_pattern_cmd(struct ath6kl_vif *vif,
		struct wmi_add_wow_ext_pattern_cmd *wow_ext_cmd,
		u8 *pattern, u8 *mask)
{
	struct sk_buff *skb;
	struct wmi_add_wow_ext_pattern_cmd *cmd;
	int size;
	u8 *filter_mask;
	int mask_size;
	int ret = 0;

	if (0 == wow_ext_cmd->filter_size) {
		return -EINVAL;
	}

	mask_size = ((int)wow_ext_cmd->filter_size) / 8;
	if (wow_ext_cmd->filter_size % 8) {
		mask_size++;
	}

	size = sizeof(struct wmi_add_wow_ext_pattern_cmd);
	size += (int)wow_ext_cmd->filter_size;
	size += mask_size;

	skb = ath6kl_wmi_get_new_buf(size);
	if(!skb)
		return -ENOMEM;

	cmd = (struct wmi_add_wow_ext_pattern_cmd *)skb->data;

	cmd->filter_list_id = wow_ext_cmd->filter_list_id;
	cmd->filter_id = wow_ext_cmd->filter_id;
	cmd->filter_offset = wow_ext_cmd->filter_offset;
	cmd->filter_size = wow_ext_cmd->filter_size;

	ath6kl_dbg(ATH6KL_DBG_WOWLAN, "Adding wow pattern\n");
	ath6kl_dbg(ATH6KL_DBG_WOWLAN, "\t cmd->filter_list_id: %d", 
		wow_ext_cmd->filter_list_id);
	ath6kl_dbg(ATH6KL_DBG_WOWLAN, "\t cmd->filter_id: %d", 
		wow_ext_cmd->filter_id);
	ath6kl_dbg(ATH6KL_DBG_WOWLAN, "\t cmd->filter_offset: %d", 
		wow_ext_cmd->filter_offset);
	ath6kl_dbg(ATH6KL_DBG_WOWLAN, "\t cmd->filter_size: %d", 
		wow_ext_cmd->filter_size);
	ath6kl_dbg(ATH6KL_DBG_WOWLAN, "\t mask-size: %d", mask_size);

	memcpy(cmd->filter, pattern, cmd->filter_size);
	filter_mask = (u8 *)(cmd->filter + cmd->filter_size);
	memcpy(filter_mask, mask, mask_size);
	ath6kl_dbg(ATH6KL_DBG_WOWLAN, "\t mask: %x", 
		cmd->filter[cmd->filter_size]);
	ret = ath6kl_wmi_cmd_send(vif, skb, 
			WMI_ADD_WOW_EXT_PATTERN_CMDID, NO_SYNC_WMIFLAG);

	return ret;
}

int ath6kl_wmi_set_host_sleep_mode_cmd(struct ath6kl_vif *vif,
		struct wmi_set_host_sleep_mode_cmd *host_mode_cmd)
{
	struct sk_buff *skb;
	struct wmi_set_host_sleep_mode_cmd *cmd;
	struct wmi *wmi = vif->ar->wmi;
	u16 active_tsids = 0;
	u8 stream_exist = 0;
	u8 index;
	int ret;

	if (host_mode_cmd->awake == host_mode_cmd->asleep)
		return -EINVAL;

	skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
	if (!skb)
		return -ENOMEM;

	cmd = (struct wmi_set_host_sleep_mode_cmd *) skb->data;
	memcpy(cmd, host_mode_cmd, sizeof(struct wmi_set_host_sleep_mode_cmd));

	if (host_mode_cmd->asleep) {
		/*
		 * Relinquish credits from all implicitly created pstreams
		 * since when we go to sleep. If user created explicit
		 * thinstreams exists with in a fatpipe leave them intact
		 * for the user to delete.
		 */
		spin_lock_bh(&wmi->lock);
		stream_exist = wmi->fat_pipe_exist;
		spin_unlock_bh(&wmi->lock);

		for (index = 0; index < WMM_NUM_AC; index++) {
			if (stream_exist & (1 << index)) {
				spin_lock_bh(&wmi->lock);
				active_tsids = wmi->stream_exist_for_ac[index];
				spin_unlock_bh(&wmi->lock);

				/*
				 * If there are no user created thin
				 * streams delete the fatpipe.
				 */
				if (!active_tsids) {
					stream_exist &= ~(1 << index);
					/*
					 * Indicate inactivity to drv layer
					 * for this fatpipe(pstream).
					 */
					ath6kl_indicate_tx_activity(vif->ar,
								    index,
								    false);
				}
			}
		}

		/* Update the fatpipes that exists */
		spin_lock_bh(&wmi->lock);
		wmi->fat_pipe_exist = stream_exist;
		spin_unlock_bh(&wmi->lock);
	}

	ret = ath6kl_wmi_cmd_send(vif, skb, WMI_SET_HOST_SLEEP_MODE_CMDID,
				  NO_SYNC_WMIFLAG);
	return ret;
}

int ath6kl_wmi_set_wow_mode_cmd(struct ath6kl_vif *vif,
			struct wmi_set_wow_mode_cmd *wow_mode_cmd)
{
	struct sk_buff *skb;
	struct wmi_set_wow_mode_cmd *cmd;
	int ret = 0;

	skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
	if(!skb)
		return -ENOMEM;

	cmd = (struct wmi_set_wow_mode_cmd *)skb->data;
	memcpy(cmd, wow_mode_cmd, sizeof(struct wmi_set_wow_mode_cmd));

	ret = ath6kl_wmi_cmd_send(vif, skb, WMI_SET_WOW_MODE_CMDID, 
				NO_SYNC_WMIFLAG);

	return ret;
}

int ath6kl_wm_set_gtk_offload(struct ath6kl_vif *vif,
		u8 *kek, u8 *kck, u8 *replay_ctr )
{
	int ret = 0;
	struct sk_buff *skb;
	struct wmi_gtk_offload_op *cmd;
	u64 replay_counter;

	if( WARN_ON(!kek) || WARN_ON(!kck) || WARN_ON(!replay_ctr))
		return ret;

	skb = ath6kl_wmi_get_new_buf(sizeof(struct wmi_gtk_offload_op));
	if(!skb)
		return -ENOMEM;

	cmd = (struct wmi_gtk_offload_op *)skb->data;

	memset(cmd, 0, sizeof(struct wmi_gtk_offload_op));

	replay_counter = cpu_to_be64(*replay_ctr);
	memcpy(cmd->kek, kek, NL80211_KEK_LEN);
	memcpy(cmd->kck, kck, NL80211_KCK_LEN);
	memcpy(cmd->replay_counter, (u8 *)&replay_counter, NL80211_REPLAY_CTR_LEN);

	cmd->opcode = WMI_GTK_OFFLOAD_OPCODE_SET;

	ret = ath6kl_wmi_cmd_send(vif, skb, 
			WMI_GTK_OFFLOAD_OP_CMDID, NO_SYNC_WMIFLAG);

	return ret;
}

int ath6kl_wmi_set_green_tx_params(struct ath6kl_vif *vif, struct wmi_green_tx_params *params)
{
	struct sk_buff *skb;
	struct wmi_green_tx_params *cmd;
	int ret = 0;

	skb = ath6kl_wmi_get_new_buf(sizeof(struct wmi_green_tx_params));

	if (skb == NULL)
		return -ENOMEM;

	cmd = (struct wmi_green_tx_params *)skb->data;
	memset(cmd, 0, sizeof(struct wmi_green_tx_params));

	memcpy(cmd, params, sizeof(struct wmi_green_tx_params));

    /* change the byte order */
    cmd->enable = cpu_to_le32(cmd->enable);

	ret = ath6kl_wmi_cmd_send(vif, skb, 
			WMI_GREENTX_PARAMS_CMDID, NO_SYNC_WMIFLAG);

	return ret;
}

int ath6kl_wmi_smps_config(struct ath6kl_vif *vif, struct wmi_config_smps_cmd *options)
{
	struct sk_buff *skb;
	struct wmi_config_smps_cmd *cmd;
	int ret = 0;

	skb = ath6kl_wmi_get_new_buf(sizeof(struct wmi_config_smps_cmd));

	if (skb == NULL)
		return -ENOMEM;

	cmd = (struct wmi_config_smps_cmd *)skb->data;
	memset(cmd, 0, sizeof(struct wmi_config_smps_cmd));

	memcpy(cmd, options, sizeof(struct wmi_config_smps_cmd));

	ret = ath6kl_wmi_cmd_send(vif, skb, 
			WMI_SMPS_CONFIG_CMDID, NO_SYNC_WMIFLAG);

	return ret;
}

int ath6kl_wmi_smps_enable(struct ath6kl_vif *vif, struct wmi_config_enable_cmd *options)
{
	struct sk_buff *skb;
	struct wmi_config_enable_cmd *cmd;
	int ret = 0;

	skb = ath6kl_wmi_get_new_buf(sizeof(struct wmi_config_enable_cmd));

	if (skb == NULL)
		return -ENOMEM;

	cmd = (struct wmi_config_enable_cmd *)skb->data;
	memset(cmd, 0, sizeof(struct wmi_config_enable_cmd));

	memcpy(cmd, options, sizeof(struct wmi_config_enable_cmd));

	ret = ath6kl_wmi_cmd_send(vif, skb, 
			WMI_SMPS_ENABLE_CMDID, NO_SYNC_WMIFLAG);

	return ret;
}

int ath6kl_wmi_lpl_enable_cmd(struct ath6kl_vif *vif, struct wmi_lpl_force_enable_cmd *force_enable_cmd)
{
	struct sk_buff *skb;
	struct wmi_lpl_force_enable_cmd *cmd;
	int ret = 0;

	skb = ath6kl_wmi_get_new_buf(sizeof(struct wmi_lpl_force_enable_cmd));

	if (skb == NULL)
		return -ENOMEM;

	cmd = (struct wmi_lpl_force_enable_cmd *)skb->data;
	memset(cmd, 0, sizeof(struct wmi_lpl_force_enable_cmd));

	memcpy(cmd, force_enable_cmd, sizeof(struct wmi_lpl_force_enable_cmd));

	ret = ath6kl_wmi_cmd_send(vif, skb, 
			WMI_LPL_FORCE_ENABLE_CMDID, NO_SYNC_WMIFLAG);

	return ret;
}

int ath6kl_wmi_set_ht_cap_cmd(struct ath6kl_vif *vif,
	u8 band, u8 chan_width_40M_supported, u8 short_GI)
{
	int ret = 0;
	struct sk_buff *skb;
	struct wmi_set_ht_cap *cmd;

	if( WARN_ON((band != A_BAND_24GHZ) && (band != A_BAND_5GHZ)) || 
		WARN_ON(chan_width_40M_supported > 1) || 
		WARN_ON(short_GI > 1))
		return ret;

	skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
	if (!skb)
		return -ENOMEM;

	ath6kl_dbg(ATH6KL_DBG_WMI, "set_ht_cap_cmd: band=%d chan_width_40M_supported=%d short_GI=%d\n",
		   band, chan_width_40M_supported, short_GI);

	cmd = (struct wmi_set_ht_cap *) skb->data;
	cmd->band = band;
	cmd->enable = 1;
	cmd->chan_width_40M_supported = chan_width_40M_supported;
	cmd->short_GI_20MHz = short_GI;
	cmd->short_GI_40MHz = short_GI;
	cmd->intolerance_40MHz = 1;
	cmd->max_ampdu_len_exp = 2;	/* TBD : 32K for 2 chain, 16K for 1 chain */

	ret = ath6kl_wmi_cmd_send(vif, skb, WMI_SET_HT_CAP_CMDID,
				   NO_SYNC_WMIFLAG);

	return ret;
}

