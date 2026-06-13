// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/bitops.h>
#include <linux/slab.h>
#include <bindings/audio-codec-port-types.h>
#include <soc/soundwire.h>
#include "qmp-aggregator.h"

#define NUM_AGG_STREAMS 8
#define MAX_CHANNELS 8
#define MAX_MASTER_PORTS 10

static int num_chs[NUM_AGG_STREAMS];

enum {
	mTX1 = 2,
	mTX2 = 3,
	mTX3 = 4,
	mTX4 = 5,
};

struct ch_link {
	struct list_head list;
	uint8_t m_id;
	uint8_t m_dp;
	uint8_t m_dp_ch;
	uint8_t m_dp_ch_prepared;
	uint8_t m_dp_ch_enabled;
	uint8_t s_dev_num;
	uint8_t	s_dp;
	uint8_t	s_offset1;
	uint8_t sid;
	void *slave_priv_data;
	void (*update_offset1)(void *private_data, u8 slv_port_id, u8 offset1);
	int offset1_assigned;
	struct swr_device *peripheral;
};

struct stream_agg_instance {
	void *private_data;
	uint32_t num_channels;
	uint32_t channel_rate;
	int links;
	struct ch_link ch_links[MAX_CHANNELS]; /* Max 8 channels for now */
};
static LIST_HEAD(channel_links_list);
#define for_each_channel_link(link, _link) \
		list_for_each_entry_safe(link, _link, &channel_links_list, list)

static DEFINE_MUTEX(agg_lock);
static struct stream_agg_instance aggregator_[NUM_AGG_STREAMS];

static int retrieve_agg_stream_instance_index(void *substream)
{
	int i = 0;

	for (i = 0; i < NUM_AGG_STREAMS; ++i) {
		if (aggregator_[i].private_data == substream)
			return i;
	}

	return -EINVAL;
}

static int get_matching_stream_index_or_first_available(void *substream)
{
	int i = 0, first_available = -1;
	int sidx;
	bool found = false;

	for (i = 0; i < NUM_AGG_STREAMS; ++i) {
		if (aggregator_[i].private_data == NULL &&
				first_available == -1)
			first_available = i;

		if (aggregator_[i].private_data == substream) {
			found = true;
			break;
		}
	}
	if (!found)
		sidx = first_available;
	else
		sidx = i;

	if (sidx < 0 || sidx >= NUM_AGG_STREAMS)
		return -EINVAL;

	if (!found) /* new streams, zero out memory */
		memset(&aggregator_[sidx], 0, sizeof(struct stream_agg_instance));

	return sidx;
}

static void get_master_port_info(u8 mport_type, u8 *m_dp, u8 *m_dp_ch)
{
	if (mport_type >= SWRM_TX1_CH1 && mport_type <= SWRM_TX1_CH4) {
		*m_dp = mTX1;
		*m_dp_ch = mport_type - SWRM_TX1_CH1;
	} else if (mport_type >= SWRM_TX2_CH1 && mport_type <= SWRM_TX2_CH4) {
		*m_dp = mTX2;
		*m_dp_ch = mport_type - SWRM_TX2_CH1;
	} else if (mport_type >= SWRM_TX3_CH1 && mport_type <= SWRM_TX3_CH4) {
		*m_dp = mTX3;
		*m_dp_ch = mport_type - SWRM_TX3_CH1;
	}
}

static bool validate_ch_link(struct stream_agg_instance *agg, u8 m_dp, u8 m_dp_ch)
{
	u8 mask = 0, num_set_bits = 0, fs_bit;
	bool is_valid = true, found = false;
	u8 m_dp_ch_acc = 0;
	struct ch_link *ch = NULL, *_link;

	for_each_channel_link(ch, _link) {
		if (ch->m_dp == m_dp) {
			m_dp_ch_acc |= ch->m_dp_ch;
			found = true;
		}
	}

	if (!found)
		return true;

	/*
	 * below logic checks if the channels (on master side port) are
	 * contiguous or not.
	 * Block packing per port is used on the master and hence
	 * if the channels are connecting to the same master port,
	 * they have to be contiguous.
	 */
	m_dp_ch_acc |= BIT(m_dp_ch);
	num_set_bits = hweight8(m_dp_ch_acc);
	if (num_set_bits == 0) {
		is_valid = false;
		goto exit;
	}

	if (num_set_bits == 1)
		goto exit;

	fs_bit = ffs(m_dp_ch_acc);
	mask = ((1 << num_set_bits) - 1) << (fs_bit - 1);
	if ((m_dp_ch_acc & (~mask)) != 0)
		is_valid = false;

exit:
	return is_valid;
}

void update_ch_per_substream(int ch, void *substream)
{
	int sidx = get_matching_stream_index_or_first_available(substream);

	if (ch) {
		num_chs[sidx]++;
		pr_debug("%s, %d Channel(s) added\n", __func__, num_chs[sidx]);
	} else {
		/*
		 * to remove the channels from the substream we do it in one go, unlike
		 * the addition where each channel is added one by one
		 */
		num_chs[sidx] = 0;
		pr_debug("%s, Channel(s) reset to %d\n", __func__, num_chs[sidx]);
	}
}
EXPORT_SYMBOL_GPL(update_ch_per_substream);

/*
 * stream_agg_add_channel :
 * Inputs: Substream - void *pointer stored in aggregator for stream identification
 *         Channels - Number of channels in the stream
 *         Channel_rate - Channel rate. Each channel within a given stream is expected
 *                        to have same channel rate. This API doesn't check or report
 *                        any mismatches in the channel rates among channels.
 *         Mport_type - Master port this channel is requesting to connect to
 *         slv_port_id - Slave port id of this channel
 *         dev_num - logical device number of the requesting device
 *
 *   This API creates an agg stream instance for the new streams. For the already
 *   existing streams, new channel request is added to the agg stream instance.
 *   Each agg stream instance structure holds below information :
 *     [
 *        *private_data
 *        channel_rate
 *        number of channels
 *        number of links;
 *         after all channels are added, number of links == number of channels
 *   [master_id, master_dataport_id, master_dataport_ch ]  ---
 *				[Slave port id, Slave device number, slave offset1]
 *   [master_id, master_dataport_id, master_dataport_ch ]  ---
 *				[Slave port id, Slave device number, slave offset1]
 *   .....
 *   [master_id, master_dataport_id, master_dataport_ch ]  ---
 *				[Slave port id, Slave device number, slave offset1]
 *     ]
 *
 */

int stream_agg_add_channel(void *substream, uint32_t channels,
		uint32_t channel_rate, u8 mport_type, u8 slv_port_id, u8 dev_num,
		void *slave_priv_data, struct swr_device *peripheral,
		void (*offset1_update)(void *private_data, u8 slv_port_id, u8 offset1))
{
	struct ch_link *link = NULL, *found_link = NULL, *_link;
	int sidx;
	u8 m_dp = 0, m_dp_ch = 0;

	if (!substream)
		return -EINVAL;

	if (channels > MAX_CHANNELS) {
		pr_err("unsupported channels %d\n", channels);
		return -EINVAL;
	}

	get_master_port_info(mport_type, &m_dp, &m_dp_ch);
	if (m_dp == 0) {
		pr_err("unsupported master port type %d\n", mport_type);
		return -EINVAL;
	}
	mutex_lock(&agg_lock);
	/*
	 * Assign an unused agg stream instance for new stream
	 * For existing streams, it will lookup and return the index from the agg list
	 */
	sidx = get_matching_stream_index_or_first_available(substream);
	if (sidx < 0) {
		pr_err("matching stream %pK not found or max streams reached\n", substream);
		mutex_unlock(&agg_lock);
		return -EINVAL;
	}

	aggregator_[sidx].private_data = substream;
	aggregator_[sidx].num_channels = num_chs[sidx];
	aggregator_[sidx].channel_rate = channel_rate;

	/*
	 * if multiple channels connecting to same master port, make sure
	 * they are contiguous. Soundwire controller driver uses block per port
	 * packing mode and uses the lowest offset1 among all the  multiple
	 * slave/channels if connecting to same master
	 */
	if (!validate_ch_link(&aggregator_[sidx], m_dp, m_dp_ch)) {
		pr_err("link: [stream:%pK, m_dp:%d, m_dp_ch:BIT(%d)] <--- [s_dev:%d s_dp:%d] add failed\n",
				substream, m_dp, m_dp_ch, dev_num, slv_port_id);
		mutex_unlock(&agg_lock);
		return -EINVAL;
	}

	/*
	 * find if the same channel ([s_dev_num, s_port] --> [m_dp, m_dp_ch]) already
	 * been added to the list of channels (potentially by another stream)
	 */
	for_each_channel_link(link, _link) {
		if (link->m_dp == m_dp && link->m_dp_ch == BIT(m_dp_ch) &&
			link->s_dp == slv_port_id && link->s_dev_num == dev_num) {
			found_link = link;
			break;
		}
	}
	if (found_link == NULL) {
		link = kzalloc(sizeof(*link), GFP_KERNEL);
		if (!link) {
			mutex_unlock(&agg_lock);
			return -ENOMEM;
		}
		link->m_dp = m_dp;
		link->m_dp_ch = BIT(m_dp_ch);
		link->s_dev_num = dev_num;
		link->s_dp = slv_port_id;
		link->s_offset1 = 0;
		link->m_dp_ch_prepared = 0;
		link->update_offset1 = offset1_update;
		link->slave_priv_data = slave_priv_data;
		link->offset1_assigned = 0;
		link->peripheral = peripheral;
		link->sid = 0;
		found_link = link;

		/* add to global list for new channel */
		list_add(&found_link->list, &channel_links_list);
	}

	found_link->sid |= BIT(sidx);

	++aggregator_[sidx].links;

	pr_debug("link: [stream %pK, m_dp:%d, m_dp_ch:BIT(%d)] <--- [s_dev:%d s_dp:%d] added\n",
			substream, link->m_dp, m_dp_ch, link->s_dev_num, link->s_dp);
	mutex_unlock(&agg_lock);
	return 0;
}
EXPORT_SYMBOL_GPL(stream_agg_add_channel);

/* called under agg_lock held */
static void assign_offset1(int sidx)
{
	int i;
	u8 m_dp = 0;
	u8 base_offset1 = 0, bit, mask, max_offset1, min_offset1;
	u8 acc_ch_masks[MAX_MASTER_PORTS];
	unsigned long temp_mask = 0;
	struct ch_link *ch, *_link;
	struct stream_agg_instance *agg = NULL;
	/* assign offset1's for all the slave devices
	 * compute base_offset1 to be used for this stream/channels
	 * there may be other streams already running at this time with some offset1's
	 * Available offset1's depend on the current bus clock frequency, channel rate and
	 * lane control
	 * for bus-clk of 4.8 and channel rate of 2.4Mhz, SI would be 4 so the available
	 * offset1's are 1, 2, 3.  If the bus clock fequency increases to 9.6MHz with
	 * same channel rate of 2.4M, SI would be 8 and hence possible offset1's are
	 * 1, 2, 3, 4, 5, 6, 7 on the other hand, if the channel rate also increases
	 * (send sample on the bitslot frequently), then available offset1's would
	 * be limited.
	 */
	max_offset1 = 0;
	min_offset1 = 0;

	agg = &aggregator_[sidx];
	for_each_channel_link(ch, _link) {
		if (ch->offset1_assigned) {
			max_offset1 = max(max_offset1, ch->s_offset1);
			if (min_offset1 == 0)
				min_offset1 = ch->s_offset1;
			else
				min_offset1 = min(min_offset1, ch->s_offset1);
		}
	}

	if ((max_offset1 == 0) || (min_offset1 > agg->num_channels))
		base_offset1 = 0;
	else
		base_offset1 = max_offset1;

	memset(acc_ch_masks, 0, MAX_MASTER_PORTS * sizeof(u8));

	for_each_channel_link(ch, _link) {
		if (BIT(sidx) & ch->sid) {
			m_dp = ch->m_dp;
			acc_ch_masks[m_dp] |= ch->m_dp_ch;
		}
	}
	for (i = 0; i < MAX_MASTER_PORTS; ++i) {
		if (acc_ch_masks[i] == 0)
			continue;

		m_dp = i;
		temp_mask = acc_ch_masks[i];
		/* for each channel in the master port */
		/* assign offset1 from the lower channel number to higher channel */
		for_each_set_bit(bit, &temp_mask, 8) {
			mask = BIT(bit);
			pr_debug("(ch) mask : %d, master port overall channel mask: %lu\n",
					mask, temp_mask);
			for_each_channel_link(ch, _link) {
				if (BIT(sidx) & ch->sid) {
					if ((ch->m_dp == m_dp) &&
						((temp_mask & mask) == ch->m_dp_ch) &&
						(ch->offset1_assigned == 0)) {
						ch->s_offset1 = base_offset1 + 1;
						ch->offset1_assigned = 1;
						base_offset1 += 1;
						pr_debug("offset1 %d updated for m_dp:%d, m_dp_ch:CH%d, s_dev:%d, s_dp:%d, substream:%pK\n",
								ch->s_offset1, ch->m_dp,
								ffs(ch->m_dp_ch), ch->s_dev_num,
								ch->s_dp, agg->private_data);
					}

				}
			}
		}
	}
}

static struct ch_link *lookup_channel(int sidx, u8 m_dp, u8 m_dp_ch)
{
	struct ch_link *ch = NULL, *_link;

	for_each_channel_link(ch, _link) {
		if (BIT(sidx) & ch->sid) {
			if ((ch->m_dp == m_dp) && (ch->m_dp_ch == BIT(m_dp_ch)))
				return ch;
		}
	}
	return NULL;
}

static int get_ch_unprepare_count(int sidx)
{
	int unprepare_cnt = 0;
	struct ch_link *ch, *_link;

	for_each_channel_link(ch, _link) {
		if (BIT(sidx) & ch->sid) {
			if (!(ch->m_dp_ch_prepared & BIT(sidx)))
				++unprepare_cnt;
		}
	}
	return unprepare_cnt;
}

static int get_ch_prepare_count(int sidx)
{
	int prepare_cnt = 0;
	struct ch_link *ch, *_link;

	for_each_channel_link(ch, _link) {
		if (BIT(sidx) & ch->sid) {
			if (ch->m_dp_ch_prepared & BIT(sidx))
				++prepare_cnt;
		}
	}
	return prepare_cnt;
}

/* returns ch_link pointer for which ch_enabled is 0 */
static struct ch_link *get_ch_not_enabled(int sidx)
{
	struct ch_link *ch, *_link;

	for_each_channel_link(ch, _link) {
		if (BIT(sidx) & ch->sid) {
			if (ch->m_dp_ch_enabled == 0)
				return ch;
		}
	}
	return NULL;
}

/* returns ch_link pointer for which ch_enabled is 1 */
static struct ch_link *get_ch_not_disabled(int sidx)
{
	struct ch_link *ch, *_link;

	for_each_channel_link(ch, _link) {
		if (BIT(sidx) & ch->sid) {
			if (hweight8(ch->m_dp_ch_enabled) == 1)
				return ch;
		}
	}
	return NULL;
}

int stream_agg_prepare_channel(void *substream, u8 mport_type,
		u8 slv_port_id, u8 dev_num)
{
	struct stream_agg_instance *agg = NULL;
	int prepare_cnt = 0;
	u8 m_dp = 0, m_dp_ch = 0;
	int rc, sidx;
	struct ch_link *ch, *enable_pending_ch = NULL, *_link;
	u8 ch_mask = 0x01; /* only DpnChannelEN1 register is available */
	u8 num_ch = 1;
	u8 port_type = mport_type;

	get_master_port_info(mport_type, &m_dp, &m_dp_ch);
	if (m_dp == 0) {
		pr_debug("unsupported master port type %d\n", mport_type);
		return -EINVAL;
	}

	mutex_lock(&agg_lock);
	sidx = retrieve_agg_stream_instance_index(substream);
	if (sidx < 0) {
		pr_err("substream: %pK not found\n", substream);
		mutex_unlock(&agg_lock);
		return -EINVAL;
	}

	ch = lookup_channel(sidx, m_dp, m_dp_ch);
	if (!ch) {
		pr_err("substream %pK port_type: %d,m_dp:%d,m_dp_ch:BIT(%d) not found, cannot prepare\n",
				substream, mport_type, m_dp, m_dp_ch);
		mutex_unlock(&agg_lock);
		return -EINVAL;
	}
	ch->m_dp_ch_prepared |= BIT(sidx);
	agg = &aggregator_[sidx];
	/* if this channel is being prepared first time by the stream with "sidx",
	 * swr_connect_port( ) gets called otherwise skip connect port.
	 * Hamming distance function is used here. If it is greater than 1, means
	 * that some other stream already prepared the channel.
	 */
	if (hweight8(ch->m_dp_ch_prepared) > 1) {
		pr_debug("link: m_dp:%d, m_dp_ch:BIT(%d)] <--- [s_dev:%d s_dp:%d] already prepared\n",
				m_dp, m_dp_ch, ch->s_dev_num, ch->s_dp);
	} else {
		rc = swr_connect_port(ch->peripheral, &ch->s_dp, 1, &ch_mask,
				&agg->channel_rate, &num_ch, &port_type);
		pr_debug("link: [stream %p, m_dp:%d, m_dp_ch:BIT(%d)] <--- [s_dev:%d s_dp:%d] prepared\n",
				substream, m_dp, m_dp_ch, ch->s_dev_num, ch->s_dp);
	}

	rc = 0;
	prepare_cnt = get_ch_prepare_count(sidx);
	if (prepare_cnt == agg->num_channels) {
		pr_debug("all channels %d in stream %pK are prepared\n",
				agg->num_channels, substream);

		/* one or more NEW channels might have been prepared, but not enabled */
		enable_pending_ch = get_ch_not_enabled(sidx);
		if (enable_pending_ch) {
			assign_offset1(sidx);

			for_each_channel_link(ch, _link) {
				if (BIT(sidx) & ch->sid) {
					if (ch->update_offset1)
						ch->update_offset1(ch->slave_priv_data,
								ch->s_dp, ch->s_offset1);
				}
			}
			rc = swr_slvdev_datapath_control(enable_pending_ch->peripheral,
					enable_pending_ch->peripheral->dev_num, true);
		}
		for_each_channel_link(ch, _link) {
			if (BIT(sidx) & ch->sid)
				ch->m_dp_ch_enabled |= BIT(sidx);
		}
	}

	mutex_unlock(&agg_lock);
	return rc;
}
EXPORT_SYMBOL_GPL(stream_agg_prepare_channel);

int stream_agg_remove_channel(void *substream, u8 mport_type,
		u8 slv_port_id, u8 dev_num)
{
	int rc = 0, sidx, unprepare_cnt = 0;
	u8 m_dp = 0, m_dp_ch = 0;
	struct stream_agg_instance *agg = NULL;
	u8 ch_mask = 0x01;
	u8 port_type = mport_type;
	struct ch_link *ch, *disable_pending_ch = NULL, *tmp;

	if (!substream)
		return -EINVAL;

	get_master_port_info(mport_type, &m_dp, &m_dp_ch);
	if (m_dp == 0) {
		pr_err("channel not found, cannot remove, mport_type:%d, slv dev_num:%d, slv port:%d\n",
				mport_type, dev_num, slv_port_id);
		return -EINVAL;
	}

	mutex_lock(&agg_lock);
	sidx = retrieve_agg_stream_instance_index(substream);
	if (sidx < 0) {
		pr_err("substream: %pK not found\n", substream);
		mutex_unlock(&agg_lock);
		return -EINVAL;
	}
	ch = lookup_channel(sidx, m_dp, m_dp_ch);
	if (!ch) {
		pr_err("substream %pK port_type: %d,m_dp:%d,m_dp_ch:BIT(%d) not found, cannot remove\n",
				substream, mport_type, m_dp, m_dp_ch);
		mutex_unlock(&agg_lock);
		return -EINVAL;
	}

	ch->m_dp_ch_prepared &= ~BIT(sidx);
	if (hweight8(ch->m_dp_ch_prepared) >= 1) {
		pr_debug("link: m_dp:%d, m_dp_ch:BIT(%d)] <--- [s_dev:%d s_dp:%d] in use\n",
				m_dp, m_dp_ch, ch->s_dev_num, ch->s_dp);
	} else {
		rc = swr_disconnect_port(ch->peripheral, &ch->s_dp, 1, &ch_mask, &port_type);
		pr_debug("link: [stream %p, m_dp:%d, m_dp_ch:BIT(%d)] <--- [s_dev:%d s_dp:%d] unprepared\n",
				substream, m_dp, m_dp_ch, ch->s_dev_num, ch->s_dp);
	}
	unprepare_cnt = get_ch_unprepare_count(sidx);
	agg = &aggregator_[sidx];
	if (unprepare_cnt == agg->num_channels) {
		pr_debug("all channels %d in stream %pK are unprepared\n",
				agg->num_channels, substream);

		disable_pending_ch = get_ch_not_disabled(sidx);
		if (disable_pending_ch) {
			rc = swr_slvdev_datapath_control(disable_pending_ch->peripheral,
					disable_pending_ch->peripheral->dev_num, false);
		}
		list_for_each_entry_safe(ch, tmp, &channel_links_list, list) {
			if (BIT(sidx) & ch->sid)
				ch->m_dp_ch_enabled &= ~BIT(sidx);
			ch->sid &= ~BIT(sidx);
			if (ch->sid == 0) {
				list_del(&ch->list);
				kfree(ch);
			}
		}
		pr_debug("all channels in stream %pK removed\n", substream);
		agg->private_data = NULL;
		agg->channel_rate = 0;
		agg->num_channels = 0;
		update_ch_per_substream(agg->num_channels, substream);
		agg->links = 0;
	}

	mutex_unlock(&agg_lock);
	return rc;
}
EXPORT_SYMBOL_GPL(stream_agg_remove_channel);

