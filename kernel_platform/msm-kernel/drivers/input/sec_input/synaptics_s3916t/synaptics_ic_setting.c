#include "synaptics_dev.h"
#include "synaptics_reg.h"

#define PER_PAGE_ERASE_DELAY_MS_3916T (88)
#define PER_BLOCK_WRITE_DELAY_MS_3916T (90)
#define FLASH_WRITE_DELAY_MS_DEFAULT (20)
#define SYNAPTICS_IC_S3916T	"S3916T"

void synaptics_ts_fw_delay_setting(struct synaptics_ts_data *ts)
{
	if (strncmp(ts->id_info.part_number, SYNAPTICS_IC_S3916T, 6) == 0) {
		input_info(true, ts->dev, "%s: %s require product-specified settings (S3916T)\n",
				__func__, ts->id_info.part_number);
		ts->fw_erase_delay = PER_PAGE_ERASE_DELAY_MS_3916T;
		ts->fw_write_block_delay = PER_BLOCK_WRITE_DELAY_MS_3916T;
	} else {
		input_info(true, ts->dev, "%s: %s Other IC\n",
				__func__, ts->id_info.part_number);
	}
}

int synaptics_ts_fw_resp_delay_ms(struct synaptics_ts_data *ts,
	unsigned int xfer_length, unsigned int wr_delay_ms, unsigned int num_blocks)
{
	unsigned int resp_delay;

	if (wr_delay_ms == FORCE_ATTN_DRIVEN) {
		input_dbg(true, ts->dev, "%s: xfer: %d (blocks: %d), delay: ATTN-driven\n", __func__, xfer_length, num_blocks);
		resp_delay = FORCE_ATTN_DRIVEN;
	} else {
		resp_delay = (wr_delay_ms * num_blocks) / 1000;
		//input_dbg(true, ts->dev, "%s: xfer: %d (blocks: %d), delay: %d ms\n", __func__, xfer_length, num_blocks, resp_delay);
	}

	return resp_delay;
}

int synaptics_ts_fw_set_page_count(struct synaptics_ts_data *ts,
			struct synaptics_ts_reflash_data_blob *reflash_data, unsigned int size)
{
	return synaptics_ts_pal_ceil_div(size, reflash_data->page_size);
}

int synaptics_ts_fw_set_erase_delay(struct synaptics_ts_data *ts,
		unsigned int erase_delay_ms, unsigned int page_count)
{
	int resp_delay = erase_delay_ms;

	if (erase_delay_ms == FORCE_ATTN_DRIVEN)
		resp_delay = FORCE_ATTN_DRIVEN;
	else
		resp_delay = erase_delay_ms * page_count;

	return resp_delay;
}
