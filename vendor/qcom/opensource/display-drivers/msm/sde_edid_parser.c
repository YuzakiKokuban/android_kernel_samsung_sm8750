// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2021-2022 Qualcomm Innovation Center, Inc. All rights reserved.
 * Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.
 */

#define pr_fmt(fmt)	"[drm:%s:%d] " fmt, __func__, __LINE__
#include <drm/drm_edid.h>
#include <linux/hdmi.h>

#include "sde_kms.h"
#include "sde_edid_parser.h"
#include "sde/sde_connector.h"

#define DBC_START_OFFSET 4
#define EDID_DTD_LEN 18

enum data_block_types {
	RESERVED,
	AUDIO_DATA_BLOCK,
	VIDEO_DATA_BLOCK,
	VENDOR_SPECIFIC_DATA_BLOCK,
	SPEAKER_ALLOCATION_DATA_BLOCK,
	VESA_DTC_DATA_BLOCK,
	RESERVED2,
	USE_EXTENDED_TAG
};

#if defined(CONFIG_SECDP)
bool secdp_panel_hdr_supported(void);
void secdp_logger_hex_dump(void *buf, void *pref, size_t size);
void secdp_info(u8* buf, size_t len);
#endif

static u8 *sde_find_edid_extension(struct edid *edid, int ext_id)
{
	u8 *edid_ext = NULL;
	int i;

	/* No EDID or EDID extensions */
	if (edid == NULL || edid->extensions == 0)
		return NULL;

	/* Find CEA extension */
	for (i = 0; i < edid->extensions; i++) {
		edid_ext = (u8 *)edid + EDID_LENGTH * (i + 1);
		if (edid_ext[0] == ext_id)
			break;
	}

	if (i == edid->extensions)
		return NULL;

	return edid_ext;
}

static u8 *sde_find_cea_extension(struct edid *edid)
{
	return sde_find_edid_extension(edid, SDE_CEA_EXT);
}

static int
sde_cea_db_payload_len(const u8 *db)
{
	return db[0] & 0x1f;
}

static int
sde_cea_db_tag(const u8 *db)
{
	return db[0] >> 5;
}

static int
sde_cea_revision(const u8 *cea)
{
	return cea[1];
}

static int
sde_cea_db_offsets(const u8 *cea, int *start, int *end)
{
	/* Data block offset in CEA extension block */
	*start = 4;
	*end = cea[2];
	if (*end == 0)
		*end = 127;
	if (*end < 4 || *end > 127)
		return -ERANGE;
	return 0;
}

#define sde_for_each_cea_db(cea, i, start, end) \
for ((i) = (start); \
(i) < (end) && (i) + sde_cea_db_payload_len(&(cea)[(i)]) < (end); \
(i) += sde_cea_db_payload_len(&(cea)[(i)]) + 1)

static const u8 *_sde_edid_find_block(const u8 *in_buf, u32 start_offset,
	u8 type, u8 *len)
{
	/* the start of data block collection, start of Video Data Block */
	u32 offset = start_offset;
	u32 dbc_offset = in_buf[2];

	SDE_EDID_DEBUG("%s +", __func__);
	/*
	 * * edid buffer 1, byte 2 being 4 means no non-DTD/Data block
	 *   collection present.
	 * * edid buffer 1, byte 2 being 0 means no non-DTD/DATA block
	 *   collection present and no DTD data present.
	 */

	if ((dbc_offset == 0) || (dbc_offset == 4)) {
		SDE_EDID_DEBUG("EDID: no DTD or non-DTD data present\n");
		return NULL;
	}

	while (offset < dbc_offset) {
		u8 block_len = in_buf[offset] & 0x1F;

		if ((offset + block_len <= dbc_offset) &&
		    (in_buf[offset] >> 5) == type) {
			*len = block_len;
			SDE_EDID_DEBUG("block=%d found @ 0x%x w/ len=%d\n",
				type, offset, block_len);

			return in_buf + offset;
		}
		offset += 1 + block_len;
	}

	return NULL;
}

static void sde_edid_extract_vendor_id(struct sde_edid_ctrl *edid_ctrl)
{
	char *vendor_id;
	u32 id_codes;

	SDE_EDID_DEBUG("%s +", __func__);
	if (!edid_ctrl) {
		SDE_ERROR("%s: invalid input\n", __func__);
		return;
	}

	vendor_id = edid_ctrl->vendor_id;
	id_codes = ((u32)edid_ctrl->edid->mfg_id[0] << 8) +
		edid_ctrl->edid->mfg_id[1];

	vendor_id[0] = 'A' - 1 + ((id_codes >> 10) & 0x1F);
	vendor_id[1] = 'A' - 1 + ((id_codes >> 5) & 0x1F);
	vendor_id[2] = 'A' - 1 + (id_codes & 0x1F);
	vendor_id[3] = 0;
	SDE_EDID_DEBUG("vendor id is %s ", vendor_id);
	SDE_EDID_DEBUG("%s -", __func__);
}

#if defined(CONFIG_SECDP_SWITCH)
static struct sde_edid_ctrl *g_edid_ctrl;

int secdp_get_audio_ch(void)
{
	if (g_edid_ctrl)
		return g_edid_ctrl->audio_channel_info;

	return 0;
}
EXPORT_SYMBOL(secdp_get_audio_ch);
#endif

#if defined(CONFIG_SECDP)
static int secdp_copy_lpcm_audio_data_only(struct sde_edid_ctrl *edid_ctrl,
			u8 *lpcm_adb, const u8 *adb_no_header, int len)
{
	u16 audio_ch = 0;
	u32 bit_rate = 0;
	int lpcm_size = 0;
	const int one_adb_size = 3;
	int adb_count;

	if (len <= 0)
		return 0;

	adb_count = len / one_adb_size;
	while(adb_count > 0) {
		if ((adb_no_header[0] >> 3) == 1) {
			/* to support legacy audio info */
			audio_ch |= (1 << (adb_no_header[0] & 0x7));
			if ((adb_no_header[0] & 0x7) > 0x04)
				audio_ch |= 0x20;

			bit_rate = adb_no_header[2] & 0x7;
			bit_rate |= (adb_no_header[1] & 0x7F) << 3;

			/* copy LPCM codec */
			memcpy(lpcm_adb + lpcm_size,
					adb_no_header, one_adb_size);
			lpcm_size += one_adb_size;
		}

		adb_no_header += one_adb_size;
		adb_count--;
	}

	edid_ctrl->audio_channel_info |= (bit_rate << 16);
	edid_ctrl->audio_channel_info |= audio_ch;

	return lpcm_size;
}
#endif

static void _sde_edid_extract_audio_data_blocks(
	struct sde_edid_ctrl *edid_ctrl)
{
	u8 len = 0;
	u8 adb_max = 0;
	const u8 *adb = NULL;
	u32 offset = DBC_START_OFFSET;
	u8 *cea = NULL;
#if defined(CONFIG_SECDP)
	u8 *in_buf;
	int lpcm_size = 0;
#endif

	if (!edid_ctrl) {
		SDE_ERROR("invalid edid_ctrl\n");
		return;
	}

#if defined(CONFIG_SECDP)
	in_buf = (u8 *)edid_ctrl->edid;
	if (in_buf[3] & (1<<6)) {
		pr_info("default audio\n");
		edid_ctrl->audio_channel_info |= 2;
	}
#endif

	SDE_EDID_DEBUG("%s +", __func__);
	cea = sde_find_cea_extension(edid_ctrl->edid);
	if (!cea) {
		SDE_DEBUG("CEA extension not found\n");
		return;
	}

	edid_ctrl->adb_size = 0;

	memset(edid_ctrl->audio_data_block, 0,
		sizeof(edid_ctrl->audio_data_block));

	do {
		len = 0;
		adb = _sde_edid_find_block(cea, offset, AUDIO_DATA_BLOCK,
			&len);

		if ((adb == NULL) || (len > MAX_AUDIO_DATA_BLOCK_SIZE ||
			adb_max >= MAX_NUMBER_ADB)) {
			if (!edid_ctrl->adb_size) {
				SDE_DEBUG("No/Invalid Audio Data Block\n");
				return;
			}

			continue;
		}

#if !defined(CONFIG_SECDP)
		memcpy(edid_ctrl->audio_data_block + edid_ctrl->adb_size,
			adb + 1, len);
#else
		lpcm_size += secdp_copy_lpcm_audio_data_only(edid_ctrl, edid_ctrl->audio_data_block + lpcm_size,
				adb + 1, len);
#endif
		offset = (adb - cea) + 1 + len;

		edid_ctrl->adb_size += len;
		adb_max++;
	} while (adb);

#if defined(CONFIG_SECDP)
	edid_ctrl->adb_size = lpcm_size;
	pr_info("DP Audio info: 0x%x\n", edid_ctrl->audio_channel_info);
#endif
	SDE_EDID_DEBUG("%s -", __func__);
}

static void sde_edid_parse_hdr_plus_info(struct drm_connector *connector,
	const u8 *db)
{
	struct sde_connector *c_conn;

	c_conn = to_sde_connector(connector);
	c_conn->hdr_plus_app_ver = db[5] & VSVDB_HDR10_PLUS_APP_VER_MASK;
}

static void sde_edid_parse_vsvdb_info(struct drm_connector *connector,
	const u8 *db)
{
	u8 db_len = 0;
	u32 ieee_code = 0;

	SDE_EDID_DEBUG("%s +\n", __func__);

	db_len = sde_cea_db_payload_len(db);

	if (db_len < 5)
		return;

	/* Bytes 2-4: IEEE 24-bit code, LSB first */
	ieee_code = db[2] | (db[3] << 8) | (db[4] << 16);

	if (ieee_code == VSVDB_HDR10_PLUS_IEEE_CODE)
		sde_edid_parse_hdr_plus_info(connector, db);

	SDE_EDID_DEBUG("%s -\n", __func__);
}

static bool sde_edid_is_luminance_value_present(u32 block_length,
	enum luminance_value value)
{
	return block_length > NO_LUMINANCE_DATA && value <= block_length;
}

/*
 * sde_edid_parse_hdr_db - Parse the HDR extended block
 * @connector: connector for the external sink
 * @db: start of the HDR extended block
 *
 * Parses the HDR extended block to extract sink info for @connector.
 */
static void
sde_edid_parse_hdr_db(struct drm_connector *connector, const u8 *db)
{

	u8 len = 0;
	struct sde_connector *c_conn;

	c_conn = to_sde_connector(connector);

	if (!db)
		return;

#if defined(CONFIG_SECDP)
	if (!secdp_panel_hdr_supported()) {
		SDE_EDID_DEBUG("connected dongle does not support HDR\n");
		return;
	}
#endif

	len = db[0] & 0x1f;
	/* Byte 3: Electro-Optical Transfer Functions */
	c_conn->hdr_eotf = db[2] & 0x3F;

	/* Byte 4: Static Metadata Descriptor Type 1 */
	c_conn->hdr_metadata_type_one = (db[3] & BIT(0));

	/* Byte 5: Desired Content Maximum Luminance */
	if (sde_edid_is_luminance_value_present(len, MAXIMUM_LUMINANCE))
		c_conn->hdr_max_luminance = db[MAXIMUM_LUMINANCE];

	/* Byte 6: Desired Content Max Frame-average Luminance */
	if (sde_edid_is_luminance_value_present(len, FRAME_AVERAGE_LUMINANCE))
		c_conn->hdr_avg_luminance = db[FRAME_AVERAGE_LUMINANCE];

	/* Byte 7: Desired Content Min Luminance */
	if (sde_edid_is_luminance_value_present(len, MINIMUM_LUMINANCE))
		c_conn->hdr_min_luminance = db[MINIMUM_LUMINANCE];

	c_conn->hdr_supported = true;
	SDE_EDID_DEBUG("HDR electro-optical %d\n", c_conn->hdr_eotf);
	SDE_EDID_DEBUG("metadata desc 1 %d\n", c_conn->hdr_metadata_type_one);
	SDE_EDID_DEBUG("max luminance %d\n", c_conn->hdr_max_luminance);
	SDE_EDID_DEBUG("avg luminance %d\n", c_conn->hdr_avg_luminance);
	SDE_EDID_DEBUG("min luminance %d\n", c_conn->hdr_min_luminance);
}


/*
 * drm_extract_clrmetry_db - Parse the HDMI colorimetry extended block
 * @connector: connector corresponding to the HDMI sink
 * @db: start of the HDMI colorimetry extended block
 *
 * Parses the HDMI colorimetry block to extract sink info for @connector.
 */
static void
sde_parse_clrmetry_db(struct drm_connector *connector, const u8 *db)
{

	struct sde_connector *c_conn;

	c_conn = to_sde_connector(connector);

	if (!db) {
		DRM_ERROR("invalid db\n");
		return;
	}

	/* Byte 3 Bit 0: xvYCC_601 */
	if (db[2] & BIT(0))
		c_conn->color_enc_fmt |= DRM_EDID_CLRMETRY_xvYCC_601;
	/* Byte 3 Bit 1: xvYCC_709 */
	if (db[2] & BIT(1))
		c_conn->color_enc_fmt |= DRM_EDID_CLRMETRY_xvYCC_709;
	/* Byte 3 Bit 2: sYCC_601 */
	if (db[2] & BIT(2))
		c_conn->color_enc_fmt |= DRM_EDID_CLRMETRY_sYCC_601;
	/* Byte 3 Bit 3: ADOBE_YCC_601 */
	if (db[2] & BIT(3))
		c_conn->color_enc_fmt |= DRM_EDID_CLRMETRY_ADOBE_YCC_601;
	/* Byte 3 Bit 4: ADOBE_RGB */
	if (db[2] & BIT(4))
		c_conn->color_enc_fmt |= DRM_EDID_CLRMETRY_ADOBE_RGB;
	/* Byte 3 Bit 5: BT2020_CYCC */
	if (db[2] & BIT(5))
		c_conn->color_enc_fmt |= DRM_EDID_CLRMETRY_BT2020_CYCC;
	/* Byte 3 Bit 6: BT2020_YCC */
	if (db[2] & BIT(6))
		c_conn->color_enc_fmt |= DRM_EDID_CLRMETRY_BT2020_YCC;
	/* Byte 3 Bit 7: BT2020_RGB */
	if (db[2] & BIT(7))
		c_conn->color_enc_fmt |= DRM_EDID_CLRMETRY_BT2020_RGB;
	/* Byte 4 Bit 7: DCI-P3 */
	if (db[3] & BIT(7))
		c_conn->color_enc_fmt |= DRM_EDID_CLRMETRY_DCI_P3;

	DRM_DEBUG_KMS("colorimetry fmts = 0x%x\n", c_conn->color_enc_fmt);
}

/*
 * sde_edid_parse_extended_blk_info - Parse the HDMI extended tag blocks
 * @connector: connector corresponding to external sink
 * @edid: handle to the EDID structure
 * Parses the all extended tag blocks extract sink info for @connector.
 */
static void
sde_edid_parse_extended_blk_info(struct drm_connector *connector,
	struct edid *edid)
{
	const u8 *cea = sde_find_cea_extension(edid);
	const u8 *db = NULL;

	if (cea && sde_cea_revision(cea) >= 3) {
		int i, start, end;

		if (sde_cea_db_offsets(cea, &start, &end))
			return;

		sde_for_each_cea_db(cea, i, start, end) {
			db = &cea[i];

			if (sde_cea_db_tag(db) == USE_EXTENDED_TAG) {
				SDE_EDID_DEBUG("found ext tag block = %d\n",
						db[1]);
				switch (db[1]) {
				case VENDOR_SPECIFIC_VIDEO_DATA_BLOCK:
					sde_edid_parse_vsvdb_info(connector,
							db);
					break;
				case HDR_STATIC_METADATA_DATA_BLOCK:
					sde_edid_parse_hdr_db(connector, db);
					break;
				case COLORIMETRY_EXTENDED_DATA_BLOCK:
					sde_parse_clrmetry_db(connector, db);
					break;
				default:
					break;
				}
			}
		}
	}
}

static void _sde_edid_extract_speaker_allocation_data(
	struct sde_edid_ctrl *edid_ctrl)
{
	u8 len;
	const u8 *sadb = NULL;
	u8 *cea = NULL;
#if defined(CONFIG_SECDP)
	u16 speaker_allocation = 0;
#endif

	if (!edid_ctrl) {
		SDE_ERROR("invalid edid_ctrl\n");
		return;
	}
	SDE_EDID_DEBUG("%s +", __func__);
	cea = sde_find_cea_extension(edid_ctrl->edid);
	if (!cea) {
		SDE_DEBUG("CEA extension not found\n");
		return;
	}

	sadb = _sde_edid_find_block(cea, DBC_START_OFFSET,
		SPEAKER_ALLOCATION_DATA_BLOCK, &len);
	if ((sadb == NULL) || (len != MAX_SPKR_ALLOC_DATA_BLOCK_SIZE)) {
		SDE_DEBUG("No/Invalid Speaker Allocation Data Block\n");
		return;
	}

	memcpy(edid_ctrl->spkr_alloc_data_block, sadb + 1, len);
	edid_ctrl->sadb_size = len;
#if defined(CONFIG_SECDP)
	speaker_allocation |= (sadb[1] & 0x7F);
#endif

	SDE_EDID_DEBUG("speaker alloc data SP byte = %08x %s%s%s%s%s%s%s\n",
		sadb[1],
		(sadb[1] & BIT(0)) ? "FL/FR," : "",
		(sadb[1] & BIT(1)) ? "LFE," : "",
		(sadb[1] & BIT(2)) ? "FC," : "",
		(sadb[1] & BIT(3)) ? "RL/RR," : "",
		(sadb[1] & BIT(4)) ? "RC," : "",
		(sadb[1] & BIT(5)) ? "FLC/FRC," : "",
		(sadb[1] & BIT(6)) ? "RLC/RRC," : "");
	SDE_EDID_DEBUG("%s -", __func__);

#if defined(CONFIG_SECDP)
	edid_ctrl->audio_channel_info |= (speaker_allocation << 8);
#endif
}

struct sde_edid_ctrl *sde_edid_init(void)
{
	struct sde_edid_ctrl *edid_ctrl = NULL;

	SDE_EDID_DEBUG("%s +\n", __func__);
	edid_ctrl = kzalloc(sizeof(*edid_ctrl), GFP_KERNEL);
	if (!edid_ctrl) {
		SDE_ERROR("edid_ctrl alloc failed\n");
		return NULL;
	}
	memset((edid_ctrl), 0, sizeof(*edid_ctrl));
	SDE_EDID_DEBUG("%s -\n", __func__);
	return edid_ctrl;
}

void sde_free_edid(void **input)
{
	struct sde_edid_ctrl *edid_ctrl = (struct sde_edid_ctrl *)(*input);

	SDE_EDID_DEBUG("%s +", __func__);
	kfree(edid_ctrl->edid);
	edid_ctrl->edid = NULL;
}

void sde_edid_deinit(void **input)
{
	struct sde_edid_ctrl *edid_ctrl = (struct sde_edid_ctrl *)(*input);

	SDE_EDID_DEBUG("%s +", __func__);
	sde_free_edid((void *)&edid_ctrl);
	kfree(edid_ctrl);
	SDE_EDID_DEBUG("%s -", __func__);
}

int _sde_edid_update_modes(struct drm_connector *connector,
	void *input)
{
	int rc = 0;
	struct sde_edid_ctrl *edid_ctrl = (struct sde_edid_ctrl *)(input);

	SDE_EDID_DEBUG("%s +", __func__);
	if (edid_ctrl->edid) {
		drm_connector_update_edid_property(connector,
			edid_ctrl->edid);

		rc = drm_add_edid_modes(connector, edid_ctrl->edid);
		sde_edid_parse_extended_blk_info(connector,
				edid_ctrl->edid);
		SDE_EDID_DEBUG("%s -", __func__);
		return rc;
	}

	drm_connector_update_edid_property(connector, NULL);
	SDE_EDID_DEBUG("%s null edid -", __func__);
	return rc;
}

u8 sde_get_edid_checksum(void *input)
{
	struct sde_edid_ctrl *edid_ctrl = (struct sde_edid_ctrl *)(input);
	struct edid *edid = NULL, *last_block = NULL;
	u8 *raw_edid = NULL;

	if (!edid_ctrl || !edid_ctrl->edid) {
		SDE_ERROR("invalid edid input\n");
		return 0;
	}

	edid = edid_ctrl->edid;

	raw_edid = (u8 *)edid;
#if !defined(CONFIG_SECDP)
	raw_edid += (edid->extensions * EDID_LENGTH);
#else
	/* fix Prevent_CXX Major defect.
	 * Dangerous cast: Pointer "raw_edid"( 8 ) to int( 4 ).
	 */
	raw_edid = raw_edid + (edid->extensions * EDID_LENGTH);
#endif
	last_block = (struct edid *)raw_edid;

	if (last_block)
		return last_block->checksum;

	SDE_ERROR("Invalid block, no checksum\n");
	return 0;
}

bool sde_detect_hdmi_monitor(void *input)
{
	struct sde_edid_ctrl *edid_ctrl = (struct sde_edid_ctrl *)(input);

	return drm_detect_hdmi_monitor(edid_ctrl->edid);
}

void sde_parse_edid(void *input)
{
	struct sde_edid_ctrl *edid_ctrl;

	if (!input) {
		SDE_ERROR("Invalid input\n");
		return;
	}

	edid_ctrl = (struct sde_edid_ctrl *)(input);

	if (edid_ctrl->edid) {
#if defined(CONFIG_SECDP)
		edid_ctrl->audio_channel_info = 1 << 26;
#endif
		sde_edid_extract_vendor_id(edid_ctrl);
		_sde_edid_extract_audio_data_blocks(edid_ctrl);
		_sde_edid_extract_speaker_allocation_data(edid_ctrl);
	} else {
		SDE_ERROR("edid not present\n");
	}
}

#if defined(CONFIG_SECDP)
static int edid_hfeeodb_extension_block_count(const struct edid *edid);

static int edid_hfeeodb_block_count(const struct edid *edid)
{
	int eeodb = edid_hfeeodb_extension_block_count(edid);

	return eeodb ? eeodb + 1 : 0;
}

static int edid_extension_block_count(const struct edid *edid)
{
	return edid->extensions;
}

static int edid_block_count(const struct edid *edid)
{
	return edid_extension_block_count(edid) + 1;
}

static const void *edid_block_data(const struct edid *edid, int index)
{
	BUILD_BUG_ON(sizeof(*edid) != EDID_LENGTH);

	return edid + index;
}

static const void *edid_extension_block_data(const struct edid *edid, int index)
{
	return edid_block_data(edid, index + 1);
}

static int edid_block_tag(const void *_block)
{
	const u8 *block = _block;

	return block[0];
}

/* CTA-861-H Table 60 - CTA Tag Codes */
#define CTA_DB_EXTENDED_TAG		7

/* CTA-861-H Table 62 - CTA Extended Tag Codes */
#define CTA_EXT_DB_HF_EEODB		0x78

/*
 * FIXME: sde_cea_revision() ?
 */
static int
cea_revision(const u8 *cea)
{
	/*
	 * FIXME is this correct for the DispID variant?
	 * The DispID spec doesn't really specify whether
	 * this is the revision of the CEA extension or
	 * the DispID CEA data block. And the only value
	 * given as an example is 0.
	 */
	return cea[1];
}

/* CTA-861-H section 7.4 CTA Data BLock Collection */
struct cea_db {
	u8 tag_length;
	u8 data[];
} __packed;

static int cea_db_tag(const struct cea_db *db)
{
	return db->tag_length >> 5;
}

static int cea_db_payload_len(const void *_db)
{
	/* FIXME: Transition to passing struct cea_db * everywhere. */
	const struct cea_db *db = _db;

	return db->tag_length & 0x1f;
}

static bool cea_db_is_extended_tag(const struct cea_db *db, int tag)
{
	return cea_db_tag(db) == CTA_DB_EXTENDED_TAG &&
		cea_db_payload_len(db) >= 1 &&
		db->data[0] == tag;
}

/*
 * References:
 * - CTA-861-H section 7.3.3 CTA Extension Version 3
 */
static int cea_db_collection_size(const u8 *cta)
{
	u8 d = cta[2];

	if (d < 4 || d > 127)
		return 0;

	return d - 4;
}

static bool cea_db_is_hdmi_forum_eeodb(const void *db)
{
	return cea_db_is_extended_tag(db, CTA_EXT_DB_HF_EEODB) &&
		cea_db_payload_len(db) >= 2;
}

/*
 * Get the HF-EEODB override extension block count from EDID.
 *
 * The passed in EDID may be partially read, as long as it has at least two
 * blocks (base block and one extension block) if EDID extension count is > 0.
 *
 * Note that this is *not* how you should parse CTA Data Blocks in general; this
 * is only to handle partially read EDIDs. Normally, use the CTA Data Block
 * iterators instead.
 *
 * References:
 * - HDMI 2.1 section 10.3.6 HDMI Forum EDID Extension Override Data Block
 */
static int edid_hfeeodb_extension_block_count(const struct edid *edid)
{
	const u8 *cta;

	/* No extensions according to base block, no HF-EEODB. */
	if (!edid_extension_block_count(edid))
		return 0;

	/* HF-EEODB is always in the first EDID extension block only */
	cta = edid_extension_block_data(edid, 0);
	if (edid_block_tag(cta) != CEA_EXT || cea_revision(cta) < 3)
		return 0;

	/* Need to have the data block collection, and at least 3 bytes. */
	if (cea_db_collection_size(cta) < 3)
		return 0;

	/*
	 * Sinks that include the HF-EEODB in their E-EDID shall include one and
	 * only one instance of the HF-EEODB in the E-EDID, occupying bytes 4
	 * through 6 of Block 1 of the E-EDID.
	 */
	if (!cea_db_is_hdmi_forum_eeodb(&cta[4]))
		return 0;

	return cta[4 + 2];
}

static void secdp_dump_edid(struct edid *edid)
{
	int num_blocks = edid_block_count(edid);
	int eeodb = edid_hfeeodb_block_count(edid);
	u8 i, buf[256] = { '\0', };
	size_t len;

	len = sprintf(buf, "eeodb %d, blocks %d\n", eeodb, num_blocks);
	secdp_info(buf, len);

	if (eeodb > num_blocks)
		num_blocks = eeodb;
	
	for (i = 0; i < num_blocks; i++) {
		print_hex_dump(KERN_DEBUG, "EDID: ",
			DUMP_PREFIX_NONE, 16, 1, edid + i, EDID_LENGTH, false);
		secdp_logger_hex_dump(edid + i, "EDID:", EDID_LENGTH);
	}
}
#endif/*CONFIG_SECDP*/

void sde_get_edid(struct drm_connector *connector,
				  struct i2c_adapter *adapter, void **input)
{
	struct sde_edid_ctrl *edid_ctrl = (struct sde_edid_ctrl *)(*input);

	edid_ctrl->edid = drm_get_edid(connector, adapter);
#if defined(CONFIG_SECDP)
	if (edid_ctrl->edid)
		secdp_dump_edid(edid_ctrl->edid);

#if defined(CONFIG_SECDP_SWITCH)
	g_edid_ctrl = edid_ctrl;
#endif
#endif/*CONFIG_SECDP*/

	SDE_EDID_DEBUG("%s +\n", __func__);

	if (!edid_ctrl->edid)
		SDE_ERROR("EDID read failed\n");

	if (edid_ctrl->edid)
		sde_parse_edid(edid_ctrl);

	SDE_EDID_DEBUG("%s -\n", __func__);
};
