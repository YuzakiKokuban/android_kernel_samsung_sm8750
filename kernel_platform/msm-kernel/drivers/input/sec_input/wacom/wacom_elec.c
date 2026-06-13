/*
 *  wacom_elec.c - Wacom G5 Digitizer Controller (I2C bus)
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "wacom_dev.h"

long long median(long long *arr, int length)
{
	long long ret = -1;
	int i, j;
	long long temp;

	for (i = 0; i < length; i++) {
		for (j = 0; j < length - (i + 1); j++) {
			if (arr[j] > arr[j + 1]) {
				temp = arr[j + 1];
				arr[j + 1] = arr[j];
				arr[j] = temp;
			}
		}
	}

	if (length % 2 == 0)
		ret = (arr[length / 2 - 1] + arr[length / 2]) / 2;
	else
		ret = arr[length / 2];

	return ret;
}

int builds(long long *x, long long *m, int start, int end, int ind, long long *data)
{
	int i;
	long long *temp;

	temp = kmalloc_array((end - start + 1), sizeof(long long), GFP_KERNEL);
	if (!temp)
		return -ENOMEM;

	memcpy(temp, &x[start], sizeof(long long)*(end - start + 1));
	for (i = 0; i < end - start + 1; i++) {
		temp[i] -= m[ind];
		temp[i] = (temp[i] > 0) ? temp[i] : -temp[i];
	}

	*data = median(temp, end - start + 1);

	kfree(temp);

	return 0;
}

int hampel(long long *x, int length, int k, int nsig)
{
	const long long kappa = 14826;
	int i;
	long long *m;
	long long *s;
	long long *temp;
	long long data;

	m = kmalloc_array(length, sizeof(long long), GFP_KERNEL);
	if (!m)
		return -ENOMEM;
	s = kmalloc_array(length, sizeof(long long), GFP_KERNEL);
	if (!s) {
		kfree(m);
		return -ENOMEM;
	}
	temp = kmalloc_array(length, sizeof(long long), GFP_KERNEL);
	if (!temp) {
		kfree(m);
		kfree(s);
		return -ENOMEM;
	}

	memset(m, 0x0, length * sizeof(long long));
	memset(s, 0x0, length * sizeof(long long));
	memset(temp, 0x0, length * sizeof(long long));

	for (i = 0; i < length; i++) {
		if (i < k) {
			if (i + k + 1 > length)
				continue;

			memcpy(temp, x, sizeof(long long) * (i + k + 1));
			m[i] = median(temp, i + k + 1);
		} else if (i >= length - k) {
			memcpy(temp, &x[i - k], sizeof(long long) * (length - i + k));
			m[i] = median(temp, length - i + k);
		} else {
			memcpy(temp, &x[i - k], sizeof(long long) * (2 * k + 1));
			m[i] = median(temp, 2 * k + 1);
		}
	}

	for (i = 0; i < length; i++) {
		if (i < k) {
			if (builds(x, m, 0, i + k, i, &data) < 0)
				goto error;
			s[i] = data;
		} else if (i >= length - k) {
			if (builds(x, m, i - k, length - 1, i, &data) < 0)
				goto error;
			s[i] = data;
		} else {
			if (builds(x, m, i - k, i + k, i, &data) < 0)
				goto error;
			s[i] = data;
		}

		s[i] = (s[i] * kappa) / 10000;
	}

	for (i = 0; i < length; i++) {
		if (x[i] - m[i] > nsig * s[i] || x[i] - m[i] < -(nsig * s[i]))
			x[i] = m[i];
	}

	kfree(m);
	kfree(s);
	kfree(temp);

	return 0;
error:
	kfree(m);
	kfree(s);
	kfree(temp);

	pr_err("%s: %s failed", SECLOG, __func__);

	return -ENOMEM;
}

long long mean(long long *arr, int length)
{
	long long ret = arr[0];
	int n;

	for (n = 1; n < length; n++)
		ret = (n * ret + arr[n]) / (n + 1);

	return ret;
}

int calibration_trx_data(struct wacom_data *wacom)
{
	struct wacom_elec_data *edata = wacom->edata;
	long long *cal_xx_raw, *cal_xy_raw, *cal_yx_raw, *cal_yy_raw, *cal_xy_edg_raw, *cal_yx_edg_raw;
	int i;

	cal_xx_raw = cal_xy_raw = cal_yx_raw = cal_yy_raw = cal_xy_edg_raw = cal_yx_edg_raw = NULL;
	edata->cal_xx = edata->cal_xy = edata->cal_yx = edata->cal_yy = edata->cal_xy_edg = edata->cal_yx_edg = 0;

	cal_xx_raw = kzalloc(edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	cal_xy_raw = kzalloc(edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	cal_yx_raw = kzalloc(edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	cal_yy_raw = kzalloc(edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	cal_xy_edg_raw = kzalloc(edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	cal_yx_edg_raw = kzalloc(edata->max_y_ch * sizeof(long long), GFP_KERNEL);

	if (!cal_xx_raw || !cal_xy_raw || !cal_yx_raw || !cal_yy_raw || !cal_xy_edg_raw || !cal_yx_edg_raw) {
		if (cal_xx_raw)
			kfree(cal_xx_raw);
		if (cal_xy_raw)
			kfree(cal_xy_raw);
		if (cal_yx_raw)
			kfree(cal_yx_raw);
		if (cal_yy_raw)
			kfree(cal_yy_raw);
		if (cal_xy_edg_raw)
			kfree(cal_xy_edg_raw);
		if (cal_yx_edg_raw)
			kfree(cal_yx_edg_raw);

		return -ENOMEM;
	}

	for (i = 0; i < edata->max_x_ch; i++) {
		cal_xx_raw[i] = edata->xx_ref[i] * POWER_OFFSET / edata->xx[i];
		cal_xy_raw[i] = edata->xy_ref[i] * POWER_OFFSET / edata->xy[i];
		cal_xy_edg_raw[i] = edata->xy_edg_ref[i] * POWER_OFFSET / edata->xy_edg[i];
	}

	for (i = 0; i < edata->max_y_ch; i++) {
		cal_yx_raw[i] = edata->yx_ref[i] * POWER_OFFSET / edata->yx[i];
		cal_yy_raw[i] = edata->yy_ref[i] * POWER_OFFSET / edata->yy[i];
		cal_yx_edg_raw[i] = edata->yx_edg_ref[i] * POWER_OFFSET / edata->yx_edg[i];
	}

	hampel(cal_xx_raw, edata->max_x_ch, 3, 3);
	hampel(cal_xy_raw, edata->max_x_ch, 3, 3);
	hampel(cal_yx_raw, edata->max_y_ch, 3, 3);
	hampel(cal_yy_raw, edata->max_y_ch, 3, 3);
	hampel(cal_xy_edg_raw, edata->max_x_ch, 3, 3);
	hampel(cal_yx_edg_raw, edata->max_y_ch, 3, 3);

	edata->cal_xx = mean(cal_xx_raw, edata->max_x_ch);
	edata->cal_xy = mean(cal_xy_raw, edata->max_x_ch);
	edata->cal_yx = mean(cal_yx_raw, edata->max_y_ch);
	edata->cal_yy = mean(cal_yy_raw, edata->max_y_ch);
	edata->cal_xy_edg = mean(cal_xy_edg_raw, edata->max_x_ch);
	edata->cal_yx_edg = mean(cal_yx_edg_raw, edata->max_y_ch);

	for (i = 0; i < edata->max_x_ch; i++) {
		edata->xx_xx[i] = edata->cal_xx * edata->xx[i];
		edata->xy_xy[i] = edata->cal_xy * edata->xy[i];
		edata->xy_xy_edg[i] = edata->cal_xy_edg * edata->xy_edg[i];
	}

	for (i = 0; i < edata->max_y_ch; i++) {
		edata->yx_yx[i] = edata->cal_yx * edata->yx[i];
		edata->yy_yy[i] = edata->cal_yy * edata->yy[i];
		edata->yx_yx_edg[i] = edata->cal_yx_edg * edata->yx_edg[i];
	}

	input_info(true, wacom->dev, "%s: cal_xx(%lld), cal_xy(%lld), cal_yx(%lld), cal_yy(%lld) , cal_xy_edg(%lld), cal_yx_edg(%lld)\n",
			__func__, edata->cal_xx, edata->cal_xy, edata->cal_yx, edata->cal_yy, edata->cal_xy_edg, edata->cal_yx_edg);

	kfree(cal_xx_raw);
	kfree(cal_xy_raw);
	kfree(cal_yx_raw);
	kfree(cal_yy_raw);
	kfree(cal_xy_edg_raw);
	kfree(cal_yx_edg_raw);

	return 0;
}

void calculate_ratio(struct wacom_data *wacom)
{
	struct wacom_elec_data *edata = wacom->edata;
	int i;

	for (i = 0; i < edata->max_x_ch; i++)
		edata->rxx[i] = edata->xx_ref[i] * POWER_OFFSET / edata->xx[i];

	for (i = 0; i < edata->max_x_ch; i++) {
		edata->rxy[i] = edata->xy_ref[i] * POWER_OFFSET / edata->xy[i];
		edata->rxy_edg[i] = edata->xy_edg_ref[i] * POWER_OFFSET / edata->xy_edg[i];
	}

	for (i = 0; i < edata->max_y_ch; i++) {
		edata->ryx[i] = edata->yx_ref[i] * POWER_OFFSET / edata->yx[i];
		edata->ryx_edg[i] = edata->yx_edg_ref[i] * POWER_OFFSET / edata->yx_edg[i];
	}

	for (i = 0; i < edata->max_y_ch; i++)
		edata->ryy[i] = edata->yy_ref[i] * POWER_OFFSET / edata->yy[i];

}

void make_decision(struct wacom_data *wacom, u16 *arrResult)
{
	struct wacom_elec_data *edata = wacom->edata;
	u32 open_count, short_count;
	int i;

	open_count = short_count = 0;
	for (i = 0; i < edata->max_x_ch; i++) {
		edata->drxx[i] = edata->rxx[i] - edata->cal_xx;
		edata->drxy[i] = edata->rxy[i] - edata->cal_xy;
		edata->drxy_edg[i] = edata->rxy_edg[i] - edata->cal_xy_edg;

		if (edata->xy[i] > (edata->xy_ref[i] * 14) / 10 || edata->xx[i] > (edata->xx_ref[i]  * 14) / 10) {
			input_info(true, wacom->dev, "%s: [%d] xy:%d, xy_ref:%lld, xx:%d, xx_ref:%lld\n", __func__, i,
				edata->xy[i], edata->xy_ref[i], edata->xx[i], edata->xx_ref[i]);
			arrResult[i + 1] |= SEC_SHORT;
		}

		if (edata->xy[i] < edata->xy_ref[i] / 2 || edata->xx[i] < edata->xx_ref[i] / 2)
			arrResult[i + 1] |= SEC_OPEN;

		if (edata->xy_xy[i] > edata->xy_spec[i] || edata->xx_xx[i] > edata->xx_spec[i])
			arrResult[i + 1] |= SEC_SHORT;

		if (edata->xx_self[i] > edata->xx_self_spec[i])
			arrResult[i + 1] |= SEC_OPEN;

		if (edata->drxy[i] > edata->drxy_spec[i] || edata->drxy[i] < -edata->drxy_spec[i])
			arrResult[i + 1] |= SEC_SHORT;

		if (edata->drxx[i] > edata->drxx_spec[i] || edata->drxx[i] < -edata->drxx_spec[i])
			arrResult[i + 1] |= SEC_SHORT;

		if (edata->drxy_edg[i] > edata->drxy_edg_spec[i] || edata->drxy_edg[i] < -edata->drxy_edg_spec[i])
			arrResult[i + 1] |= SEC_SHORT;

		if (arrResult[i + 1] & SEC_OPEN)
			open_count++;

		if (arrResult[i + 1] & SEC_SHORT)
			short_count++;
	}

	for (i = 0; i < edata->max_y_ch; i++) {
		edata->dryy[i] = edata->ryy[i] - edata->cal_yy;
		edata->dryx[i] = edata->ryx[i] - edata->cal_yx;
		edata->dryx_edg[i] = edata->ryx_edg[i] - edata->cal_yx_edg;

		if (edata->yx[i]  > (edata->yx_ref[i] * 14) / 10 || edata->yy[i] > (edata->yy_ref[i] * 14) / 10) {
			input_info(true, wacom->dev, "%s: [%d] yx:%d, yx_ref:%lld, yy:%d, yy_ref:%lld\n", __func__, i,
				edata->yx[i], edata->yx_ref[i], edata->yy[i], edata->yy_ref[i]);
			arrResult[i + 1 + edata->max_x_ch] |= SEC_SHORT;
		}

		if (edata->yx[i] < edata->yx_ref[i] / 2 || edata->yy[i] < edata->yy_ref[i] / 2)
			arrResult[i + 1 + edata->max_x_ch] |= SEC_OPEN;

		if (edata->yx_yx[i] > edata->yx_spec[i] || edata->yy_yy[i] > edata->yy_spec[i])
			arrResult[i + 1 + edata->max_x_ch] |= SEC_SHORT;

		if (edata->yy_self[i] > edata->yy_self_spec[i])
			arrResult[i + 1 + edata->max_x_ch] |= SEC_OPEN;

		if (edata->dryx[i] > edata->dryx_spec[i] || edata->dryx[i] < -edata->dryx_spec[i])
			arrResult[i + 1 + edata->max_x_ch] |= SEC_SHORT;

		if (edata->dryy[i] > edata->dryy_spec[i] || edata->dryy[i] < -edata->dryy_spec[i])
			arrResult[i + 1 + edata->max_x_ch] |= SEC_SHORT;

		if (edata->dryx_edg[i] > edata->dryx_edg_spec[i] || edata->dryx_edg[i] < -edata->dryx_edg_spec[i])
			arrResult[i + 1 + edata->max_x_ch] |= SEC_SHORT;

		if (arrResult[i + 1 + edata->max_x_ch] & SEC_OPEN)
			open_count++;

		if (arrResult[i + 1 + edata->max_x_ch] & SEC_SHORT)
			short_count++;
	}

	arrResult[0] = (short_count << 8) + open_count;
}

void print_elec_data(struct wacom_data *wacom)
{
	struct wacom_elec_data *edata = wacom->edata;
	u8 *pstr = NULL;
	u8 ptmp[WACOM_CMD_RESULT_WORD_LEN] = { 0 };
	int chsize, lsize;
	int i, j;

	input_info(true, wacom->dev, "%s\n", __func__);

	chsize = edata->max_x_ch + edata->max_y_ch;
	lsize = WACOM_CMD_RESULT_WORD_LEN * (chsize + 1);

	pstr = kzalloc(lsize, GFP_KERNEL);
	if (pstr == NULL)
		return;

	memset(pstr, 0x0, lsize);
	snprintf(ptmp, sizeof(ptmp), "      TX");
	strlcat(pstr, ptmp, lsize);

	for (i = 0; i < chsize; i++) {
		snprintf(ptmp, sizeof(ptmp), " %02d ", i);
		strlcat(pstr, ptmp, lsize);
	}

	input_info(true, wacom->dev, "%s\n", pstr);
	memset(pstr, 0x0, lsize);
	snprintf(ptmp, sizeof(ptmp), " +");
	strlcat(pstr, ptmp, lsize);

	for (i = 0; i < chsize; i++) {
		snprintf(ptmp, sizeof(ptmp), "----");
		strlcat(pstr, ptmp, lsize);
	}

	input_info(true, wacom->dev, "%s\n", pstr);

	for (i = 0; i < chsize; i++) {
		memset(pstr, 0x0, lsize);
		snprintf(ptmp, sizeof(ptmp), "Rx%02d | ", i);
		strlcat(pstr, ptmp, lsize);

		for (j = 0; j < chsize; j++) {
			snprintf(ptmp, sizeof(ptmp), " %4d",
					edata->elec_data[(i * chsize) + j]);

			strlcat(pstr, ptmp, lsize);
		}
		input_info(true, wacom->dev, "%s\n", pstr);
	}
	kfree(pstr);
}

void print_trx_data(struct wacom_data *wacom)
{
	struct wacom_elec_data *edata = wacom->edata;
	u8 tmp_buf[WACOM_CMD_RESULT_WORD_LEN] = { 0 };
	u8 *buff;
	int buff_size;
	int i;

	buff_size = edata->max_x_ch > edata->max_y_ch ? edata->max_x_ch : edata->max_y_ch;
	buff_size = WACOM_CMD_RESULT_WORD_LEN * (buff_size + 1);

	buff = kzalloc(buff_size, GFP_KERNEL);
	if (buff == NULL)
		return;

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "xx: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%d ", edata->xx[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "xy: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%d ", edata->xy[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "yx: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%d ", edata->yx[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "yy: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%d ", edata->yy[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "xx_self: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%d ", edata->xx_self[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "yy_self: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%d ", edata->yy_self[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "xy_edg: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%d ", edata->xy_edg[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "yx_edg: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%d ", edata->yx_edg[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	kfree(buff);
}

void print_cal_trx_data(struct wacom_data *wacom)
{
	struct wacom_elec_data *edata = wacom->edata;
	u8 tmp_buf[WACOM_CMD_RESULT_WORD_LEN] = { 0 };
	u8 *buff;
	int buff_size;
	int i;

	buff_size = edata->max_x_ch > edata->max_y_ch ? edata->max_x_ch : edata->max_y_ch;
	buff_size = WACOM_CMD_RESULT_WORD_LEN * (buff_size + 1);

	buff = kzalloc(buff_size, GFP_KERNEL);
	if (buff == NULL)
		return;

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "xx_xx: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ", edata->xx_xx[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "xy_xy: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ", edata->xy_xy[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "yx_yx: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ", edata->yx_yx[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "yy_yy: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ", edata->yy_yy[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "xy_xy_edg: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ", edata->xy_xy_edg[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "yx_yx_edg: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ", edata->yx_yx_edg[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	kfree(buff);
}

void print_ratio_trx_data(struct wacom_data *wacom)
{
	struct wacom_elec_data *edata = wacom->edata;
	u8 tmp_buf[WACOM_CMD_RESULT_WORD_LEN] = { 0 };
	u8 *buff;
	int buff_size;
	int i;

	buff_size = edata->max_x_ch > edata->max_y_ch ? edata->max_x_ch : edata->max_y_ch;
	buff_size = WACOM_CMD_RESULT_WORD_LEN * (buff_size + 1);

	buff = kzalloc(buff_size, GFP_KERNEL);
	if (buff == NULL)
		return;

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "rxx: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ", edata->rxx[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "rxy: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ", edata->rxy[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "ryx: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ", edata->ryx[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "ryy: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ", edata->ryy[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "rxy_edg: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ", edata->rxy_edg[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "ryx_edg: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ", edata->ryx_edg[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	kfree(buff);
}

void print_difference_ratio_trx_data(struct wacom_data *wacom)
{
	struct wacom_elec_data *edata = wacom->edata;
	u8 tmp_buf[WACOM_CMD_RESULT_WORD_LEN] = { 0 };
	u8 *buff;
	int buff_size;
	int i;

	buff_size = edata->max_x_ch > edata->max_y_ch ? edata->max_x_ch : edata->max_y_ch;
	buff_size = WACOM_CMD_RESULT_WORD_LEN * (buff_size + 1);

	buff = kzalloc(buff_size, GFP_KERNEL);
	if (buff == NULL)
		return;

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "drxx: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ", edata->drxx[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "drxy: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ", edata->drxy[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "dryx: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ", edata->dryx[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "dryy: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ", edata->dryy[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "drxy_edg: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ", edata->drxy_edg[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "dryx_edg: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, WACOM_CMD_RESULT_WORD_LEN, "%lld ", edata->dryx_edg[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, WACOM_CMD_RESULT_WORD_LEN);
	}

	input_info(true, wacom->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	kfree(buff);
}
