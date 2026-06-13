/*
** Copyright (c) 2019, 2021 The Linux Foundation. All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above
**     copyright notice, this list of conditions and the following
**     disclaimer in the documentation and/or other materials provided
**     with the distribution.
**   * Neither the name of The Linux Foundation nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
** WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
** ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
** BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
** BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
** WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
** OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
** IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**/

//#include "pch.h"
#include <agm/agm_api.h>
#include <stdio.h>

typedef int(*testcase)(void);

struct agm_session_config stream_config = { 1, false, true, 0, 0 };
struct agm_media_config media_config = { 48000, 2, 16, 1};
struct agm_buffer_config buffer_config = { 4, 320 };

uint32_t session_id_rx1 = 1;
uint32_t session_id_rx2 = 2;

struct session_obj *sess_handle_rx1 = NULL;
struct session_obj *sess_handle_rx2 = NULL;

uint32_t aif_id_rx1 = 1;
uint32_t aif_id_rx2 = 2;
uint32_t aif_id_rx3 = 3;

uint32_t session_id_tx1 = 3;
uint32_t aif_id_tx = 4;

struct session_obj *sess_handle_tx1 = NULL;


uint32_t stream_metadata[] = {
		1, /* No of GKVS*/
		0xA1000000, 0xA1000001, /*GKVS*/
		2, /* No of CKVS*/
		0xA5000000, 48000, 0xA6000000, 16, /*CKVS*/
		1, /* Property ID*/
		2, /* No of Properties*/
		1, 2, /* Properties*/
};

uint32_t dev_rx_metadata[] = {
		3, /* No of GKVS*/
		0xA2000000, 0xA2000001, 0xA2000000, 0xA2000001, 0xA2000000, 0xA2000001, /*GKVS*/
		2, /* No of CKVS*/
		0xA5000000, 48000, 0xA6000000, 16, /*CKVS*/
		1, /* Property ID*/
		3, /* No of Properties*/
		5,/* Properties*/
		105,
		205,
};

uint32_t dev_tx_metadata[] = {
		1, /* No of GKVS*/
		0xA3000000, 0xA3000001, /*GKVS*/
		2, /* No of CKVS*/
		0xA5000000, 48000, 0xA6000000, 16, /*CKVS*/
		1, /* Property ID*/
		1, /* No of Properties*/
		5, /* Properties*/
};

uint32_t dev_rx2_metadata[] = {
		3, /* No of GKVS*/
		5555, 6666, 7777, 8888, 9999, 1000, /*GKVS*/
		2, /* No of CKVS*/
		0xA5000000, 48000, 0xA6000000, 16, /*CKVS*/
		1, /* Property ID*/
		1, /* No of Properties*/
		5, /* Properties*/
};

uint32_t dev_rx3_metadata[] = {
		3, /* No of GKVS*/
		5555, 6666, 7777, 8888, 9999, 1000, /*GKVS*/
		2, /* No of CKVS*/
		0xA5000000, 48000, 0xA6000000, 16, /*CKVS*/
		1, /* Property ID*/
		1, /* No of Properties*/
		5, /* Properties*/
};

struct agm_tag_config tag_config = {
		123, 1,
		{{0x2, 0x3}},
};

struct agm_cal_config cal = {
			2, {{0xA5000000, 48000}, {0xA6000000, 16}},
};

int testcase_common_init(const char *caller) {
	printf("%s: init\n", caller);
	agm_init();
	return 0;
}

int testcase_common_deinit(const char *caller) {
	agm_deinit();
	printf("%s: deinit\n", caller);
	return 0;
}

int setup_device_rx()
{
	int ret = 0;

	// device config
	ret = agm_aif_set_media_config(aif_id_rx1, &media_config);
	if (ret) {
		goto done;
	}

	// device metadata
	ret = agm_aif_set_metadata(aif_id_rx1, sizeof(dev_rx_metadata), dev_rx_metadata);
	if (ret) {
		goto done;
	}

	// stream metadata
	ret = agm_session_aif_set_metadata(session_id_rx1, aif_id_rx1, sizeof(dev_rx_metadata), dev_rx_metadata);
	if (ret) {
		goto done;
	}

done:
	return ret;
}

int setup_device_tx()
{
	int ret = 0;

	// device config
	ret = agm_aif_set_media_config(aif_id_tx, &media_config);
	if (ret) {
		goto done;
	}

	// device metadata
	ret = agm_aif_set_metadata(aif_id_tx, sizeof(dev_tx_metadata), dev_tx_metadata);
	if (ret) {
		goto done;
	}

	// stream metadata
	ret = agm_session_aif_set_metadata(session_id_tx1, aif_id_tx, sizeof(dev_tx_metadata), dev_tx_metadata);
	if (ret) {
		goto done;
	}

done:
	return ret;
}


int setup_device_rx2()
{
	int ret = 0;

	// device config
	ret = agm_aif_set_media_config(aif_id_rx2, &media_config);
	if (ret) {
		goto done;
	}

	// device metadata
	ret = agm_aif_set_metadata(aif_id_rx3, sizeof(dev_rx2_metadata), dev_rx2_metadata);
	if (ret) {
		goto done;
	}

	// stream device2 metadata
	ret = agm_session_aif_set_metadata(session_id_rx1, aif_id_rx2, sizeof(dev_rx2_metadata), dev_rx2_metadata);
	if (ret) {
		goto done;
	}

done:
	return ret;
}

int setup_device_rx3()
{
	int ret = 0;

	// device config
	ret = agm_aif_set_media_config(aif_id_rx3, &media_config);
	if (ret) {
		goto done;
	}

	// device3 metadata
	ret = agm_aif_set_metadata(aif_id_rx3, sizeof(dev_rx3_metadata), dev_rx3_metadata);
	if (ret) {
		goto done;
	}

	// stream device3 metadata
	ret = agm_session_aif_set_metadata(session_id_rx1, aif_id_rx3, sizeof(dev_rx3_metadata), dev_rx3_metadata);
	if (ret) {
		goto done;
	}

done:
	return ret;
}

int setup_playback_stream() {

	int ret =0;

	// stream metadata
	ret = agm_session_set_metadata(session_id_rx1, sizeof(stream_metadata), stream_metadata);
	if (ret) {
		goto done;
	}

done:
	return ret;
}

int setup_playback_stream_open_prepare_start_with_device_rx()
{
	int ret = 0;
	ret = agm_session_aif_connect(session_id_rx1, aif_id_rx1, true);
	if (ret) {
		goto done;
	}

	ret = agm_session_open(session_id_rx1, AGM_SESSION_DEFAULT, &sess_handle_rx1);
	if (ret) {
		goto done;
	}

	ret = agm_session_set_config(sess_handle_rx1, &stream_config, &media_config,
			&buffer_config);
	if (ret) {
		goto done;
	}

	ret = agm_session_prepare(sess_handle_rx1);
	if (ret) {
		goto done;
	}

	ret = agm_session_start(sess_handle_rx1);
	if (ret) {
		goto done;
	}

done:
	return ret;
}

int setup_playback_stream_stop_close() {
	int ret = 0;
	ret = agm_session_stop(sess_handle_rx1);
	if (ret) {
		goto done;
	}

	ret = agm_session_close(sess_handle_rx1);
	if (ret) {
		goto done;
	}

	done: return ret;
}

int setup_playback_stream_2() {

	int ret =0;

	// stream metadata
	ret = agm_session_set_metadata(session_id_rx2, sizeof(stream_metadata), stream_metadata);
	if (ret) {
		goto done;
	}

done:
	return ret;
}

int setup_playback_stream_2_open_prepare_start_with_device_rx()
{
	int ret = 0;
	ret = agm_session_aif_connect(session_id_rx2, aif_id_rx1, true);
	if (ret) {
		goto done;
	}

	ret = agm_session_open(session_id_rx2, AGM_SESSION_DEFAULT, &sess_handle_rx2);
	if (ret) {
		goto done;
	}

	ret = agm_session_set_config(sess_handle_rx2, &stream_config, &media_config,
			&buffer_config);
	if (ret) {
		goto done;
	}

	ret = agm_session_prepare(sess_handle_rx2);
	if (ret) {
		goto done;
	}

	ret = agm_session_start(sess_handle_rx2);
	if (ret) {
		goto done;
	}

done:
	return ret;
}

int setup_playback_stream_2_stop_close() {
	int ret = 0;
	ret = agm_session_stop(sess_handle_rx2);
	if (ret) {
		goto done;
	}

	ret = agm_session_close(sess_handle_rx2);
	if (ret) {
		goto done;
	}

	done: return ret;
}


int setup_capture_stream() {

	int ret =0;

	// stream metadata
	agm_session_set_metadata(session_id_tx1, sizeof(stream_metadata), stream_metadata);
	if (ret) {
		goto done;
	}

done:
	return ret;
}

int setup_capture_stream_open_prepare_start_with_device_tx()
{
	int ret = 0;
	struct agm_session_config capture_stream_config = { TX, false, true, 0, 0 };

	ret = agm_session_aif_connect(session_id_tx1, aif_id_tx, true);
	if (ret) {
		goto done;
	}

	ret = agm_session_open(session_id_tx1, AGM_SESSION_DEFAULT, &sess_handle_tx1);
	if (ret) {
		goto done;
	}

	ret = agm_session_set_config(sess_handle_tx1, &capture_stream_config, &media_config,
			&buffer_config);
	if (ret) {
		goto done;
	}

	ret = agm_session_prepare(sess_handle_tx1);
	if (ret) {
		goto done;
	}

	ret = agm_session_start(sess_handle_tx1);
	if (ret) {
		goto done;
	}

done:
	return ret;
}

int setup_capture_stream_stop_close() {
	int ret = 0;
	ret = agm_session_stop(sess_handle_tx1);
	if (ret) {
		goto done;
	}

	ret = agm_session_close(sess_handle_tx1);
	if (ret) {
		goto done;
	}

	done: return ret;
}

int test_stream_open_without_aif_connected(void) {
	int ret = 0;

	ret = testcase_common_init(__func__);
	if (ret) {
		goto fail;
	}

	ret = setup_device_rx();
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream();
	if (ret) {
		goto fail;
	}

	// stream open with no device connected state
	ret = agm_session_open(session_id_rx1, AGM_SESSION_DEFAULT, &sess_handle_rx1);
	if (ret == 0) {
		goto fail;
	}
	else {
		//overwrite ret so test passess as its adverserial test case
		ret = 0;
	}

	printf("TEST PASS: %s()\n", __func__);
	goto done;

fail:
	printf("TEST FAIL: %s()\n", __func__);
	goto done;

done:
	testcase_common_deinit(__func__);
	return ret;
}

int test_device_get_aif_list() {
	int ret = 0;
	size_t num_aif_info = 0;
	struct aif_info *aifinfo = NULL;
        int i = 0;

	ret = testcase_common_init(__func__);
	if (ret) {
		goto fail;
	}

	ret = agm_get_aif_info_list(aifinfo, &num_aif_info);
	if (ret) {
		goto fail;
	}

	if (num_aif_info > 0) {
		aifinfo = calloc(num_aif_info, sizeof(struct aif_info));
		if (aifinfo == NULL) {
			ret = -1;
			goto fail;
		}
	} else {
		ret = -1;
		goto fail;
	}

	ret = agm_get_aif_info_list(aifinfo, &num_aif_info);
	if (ret) {
		goto fail;
	}

	for (i = 0; i < num_aif_info; i++)
		printf("aif %d %s %d\n", i, aifinfo[i].aif_name, aifinfo[i].dir);

	printf("TEST PASS: %s()\n", __func__);
	goto done;

fail:
	printf("TEST FAIL: %s()\n", __func__);
	goto done;

done:
	testcase_common_deinit(__func__);
	return ret;

}

int test_stream_sssd_with_buf_writes(void) {
	int ret = 0;
	char buff[512] = {0};
	int i = 0;
	size_t count = 0;
	size_t size = 512;

	ret = testcase_common_init(__func__);
	if (ret) {
		goto fail;
	}

	ret = setup_device_rx();
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream();
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream_open_prepare_start_with_device_rx();
	if (ret) {
		goto fail;
	}

	for(i = 0; i < 10; i++) {
		ret = agm_session_write(sess_handle_rx1, buff, &size);
		if (ret) {
			printf("%s: Error:%d, session write  failed\n", __func__, ret);
			goto fail;
		}

		count = agm_get_hw_processed_buff_cnt(sess_handle_rx1, RX);
		if (!count) {
			printf("%s: Error:%d, getting session buf count failed\n", __func__, ret);
			goto fail;
		}
	}

	ret = setup_playback_stream_stop_close();
	if (ret) {
		goto fail;
	}

	printf("TEST PASS: %s()\n", __func__);
	goto done;

fail:
	printf("TEST FAIL: %s()\n", __func__);
	goto done;

done:
	testcase_common_deinit(__func__);
	return ret;
}

int test_stream_pause_resume(void) {
	int ret = 0;

	ret = testcase_common_init(__func__);
	if (ret) {
		goto fail;
	}

	ret = setup_device_rx();
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream();
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream_open_prepare_start_with_device_rx();
	if (ret) {
		goto fail;
	}

	ret = agm_session_pause(sess_handle_rx1);
	if (ret) {
		goto fail;
	}

	ret = agm_session_resume(sess_handle_rx1);
	if (ret) {
		goto fail;
	}

	ret = agm_session_stop(sess_handle_rx1);
	if (ret) {
		goto fail;
	}
	
	ret = agm_session_pause(sess_handle_rx1);
	if (ret == 0) {
		goto fail;
	} else {
		//overwrite ret so test passess
		ret = 0;
	}

	printf("TEST PASS: %s()\n", __func__);
	goto done;

fail:
	printf("TEST FAIL: %s()\n", __func__);
	goto done;

done:
	testcase_common_deinit(__func__);
	return ret;
}

int test_stream_open_with_same_aif_twice(void) {
	int ret = 0;

	ret = testcase_common_init(__func__);
	if (ret) {
		goto fail;
	}

	ret = setup_device_rx();
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream();
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream_open_prepare_start_with_device_rx();
	if (ret) {
		goto fail;
	}

	ret = agm_session_aif_connect(session_id_rx1, aif_id_rx1, true);
	if (ret == 0) {
		goto fail;
	}
	else {
		ret = 0;
	}

	printf("TEST PASS: %s()\n", __func__);
	goto done;

fail:
	printf("TEST FAIL: %s()\n", __func__);
	goto done;

done:
	testcase_common_deinit(__func__);
	return ret;
}

int test_stream_sssd_deviceswitch(void) {
	int ret = 0;

	ret = testcase_common_init(__func__);
	if (ret) {
		goto fail;
	}

	ret = setup_device_rx();
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream();
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream_open_prepare_start_with_device_rx();
	if (ret) {
		goto fail;
	}

	// disconnect device 1
	ret = agm_session_aif_connect(session_id_rx1, aif_id_rx1, false);
	if (ret) {
		goto fail;
	}

	ret = setup_device_rx2();
	if (ret) {
		goto fail;
	}

	ret = agm_session_aif_connect(session_id_rx1, aif_id_rx2, true);
	if (ret) {
		goto fail;
	}


	printf("TEST PASS: %s()\n", __func__);
	goto done;

fail:
	printf("TEST FAIL: %s()\n", __func__);
	goto done;

done:
	testcase_common_deinit(__func__);
	return ret;
}

int test_stream_ssmd(void) {
	int ret = 0;

	ret = testcase_common_init(__func__);
	if (ret) {
		goto fail;
	}

	ret = setup_device_rx();
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream();
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream_open_prepare_start_with_device_rx();
	if (ret) {
		goto fail;
	}

	ret = setup_device_rx2();
	if (ret) {
		goto fail;
	}

	ret = agm_session_aif_connect(session_id_rx1, aif_id_rx2, true);
	if (ret) {
		goto fail;
	}

	printf("TEST PASS: %s()\n", __func__);
	goto done;

fail:
	printf("TEST FAIL: %s()\n", __func__);
	goto done;

done:
	testcase_common_deinit(__func__);
	return ret;
}

int test_stream_ssmd_teardown_first_device(void) {
	int ret = 0;

	ret = testcase_common_init(__func__);
	if (ret) {
		goto fail;
	}

	ret = setup_device_rx();
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream();
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream_open_prepare_start_with_device_rx();
	if (ret) {
		goto fail;
	}

	ret = setup_device_rx2();
	if (ret) {
		goto fail;
	}

	ret = agm_session_aif_connect(session_id_rx1, aif_id_rx2, true);
	if (ret) {
		goto fail;
	}

	ret = agm_session_aif_connect(session_id_rx1, aif_id_rx1, false);
	if (ret) {
		goto fail;
	}

	printf("TEST PASS: %s()\n", __func__);
	goto done;

fail:
	printf("TEST FAIL: %s()\n", __func__);
	goto done;

done:
	testcase_common_deinit(__func__);
	return ret;
}

int test_stream_ssmd_teardown_both_devices_resetup_firstdevice(void) {
	int ret = 0;

	ret = testcase_common_init(__func__);
	if (ret) {
		goto fail;
	}

	ret = setup_device_rx();
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream();
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream_open_prepare_start_with_device_rx();
	if (ret) {
		goto fail;
	}

	ret = setup_device_rx2();
	if (ret) {
		goto fail;
	}

	ret = agm_session_aif_connect(session_id_rx1, aif_id_rx2, true);
	if (ret) {
		goto fail;
	}

	ret = agm_session_aif_connect(session_id_rx1, aif_id_rx1, false);
	if (ret) {
		goto fail;
	}

	ret = agm_session_aif_connect(session_id_rx1, aif_id_rx2, false);
	if (ret) {
		goto fail;
	}

	ret = agm_session_aif_connect(session_id_rx1, aif_id_rx1, true);
	if (ret) {
		goto fail;
	}

	printf("TEST PASS: %s()\n", __func__);
	goto done;

fail:
	printf("TEST FAIL: %s()\n", __func__);
	goto done;

done:
	testcase_common_deinit(__func__);
	return ret;
}

int test_stream_ssmd_deviceswitch_second_device(void) {
	int ret = 0;

	ret = testcase_common_init(__func__);
	if (ret) {
		goto fail;
	}

	ret = setup_device_rx();
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream();
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream_open_prepare_start_with_device_rx();
	if (ret) {
		goto fail;
	}

	ret = setup_device_rx2();
	if (ret) {
		goto fail;
	}

	ret = agm_session_aif_connect(session_id_rx1, aif_id_rx2, true);
	if (ret) {
		goto fail;
	}

	//tear down second device
	ret = agm_session_aif_connect(session_id_rx1, aif_id_rx2, false);
	if (ret) {
		goto fail;
	}

	ret = setup_device_rx3();
	if (ret) {
		goto fail;
	}

	ret = agm_session_aif_connect(session_id_rx1, aif_id_rx3, true);
	if (ret) {
		goto fail;
	}

	printf("TEST PASS: %s()\n", __func__);
	goto done;

fail:
	printf("TEST FAIL: %s()\n", __func__);
	goto done;

done:
	testcase_common_deinit(__func__);
	return ret;
}

int test_stream_deint_with_mssd(void) {
	int ret = 0;

	ret = testcase_common_init(__func__);
	if (ret) {
		goto fail;
	}

	ret = setup_device_rx();
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream();
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream_open_prepare_start_with_device_rx();
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream_2();
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream_2_open_prepare_start_with_device_rx();
	if (ret) {
		goto fail;
	}

	ret = agm_deinit();
	if (ret) {
		goto fail;
	}

	printf("TEST PASS: %s()\n", __func__);
	goto done;

fail:
	printf("TEST FAIL: %s()\n", __func__);
	goto done;

done:
	testcase_common_deinit(__func__);
	return ret;
}

int test_capture_sess_loopback()
{
	int ret = 0;

	ret = testcase_common_init(__func__);
	if (ret) {
		goto fail;
	}

	ret = setup_device_rx();
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream();
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream_open_prepare_start_with_device_rx();
	if (ret) {
		goto fail;
	}

	ret = setup_device_tx();
	if (ret) {
		goto fail;
	}

	ret = setup_capture_stream();
	if (ret) {
		goto fail;
	}

	ret = agm_session_set_loopback(session_id_tx1, session_id_rx1, true);
	if (ret) {
		goto fail;
	}

	ret = setup_capture_stream_open_prepare_start_with_device_tx();
	if (ret) {
		goto fail;
	}

	ret = agm_session_set_loopback(session_id_tx1, session_id_rx1, false);
	if (ret) {
		goto fail;
	}

	ret = agm_session_set_loopback(session_id_tx1, session_id_rx1, true);
	if (ret) {
		goto fail;
	}

	ret = agm_deinit();
	if (ret) {
		goto fail;
	}

	printf("TEST PASS: %s()\n", __func__);
	goto done;

fail:
	printf("TEST FAIL: %s()\n", __func__);
	goto done;

done:
	testcase_common_deinit(__func__);
	return ret;
}


int test_capture_sess_loopback2()
{
	int ret = 0;

	ret = testcase_common_init(__func__);
	if (ret) {
		goto fail;
	}

	ret = agm_session_set_loopback(session_id_tx1, session_id_rx1, true);
	if (ret) {
		goto fail;
	}

	ret = setup_device_rx();
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream();
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream_open_prepare_start_with_device_rx();
	if (ret) {
		goto fail;
	}

	ret = setup_device_tx();
	if (ret) {
		goto fail;
	}

	ret = setup_capture_stream();
	if (ret) {
		goto fail;
	}

	ret = setup_capture_stream_open_prepare_start_with_device_tx();
	if (ret) {
		goto fail;
	}

	ret = agm_session_set_loopback(session_id_tx1, session_id_rx1, false);
	if (ret) {
		goto fail;
	}

	ret = agm_session_set_loopback(session_id_tx1, session_id_rx1, true);
	if (ret) {
		goto fail;
	}

	ret = agm_deinit();
	if (ret) {
		goto fail;
	}

	printf("TEST PASS: %s()\n", __func__);
	goto done;

fail:
	printf("TEST FAIL: %s()\n", __func__);
	goto done;

done:
	testcase_common_deinit(__func__);
	return ret;
}

int test_stream_set_ecref(void)
{
	int ret = 0;

	ret = testcase_common_init(__func__);
	if (ret) {
		goto fail;
	}

	ret = agm_session_set_ec_ref(session_id_tx1, aif_id_rx1, true);
	if (ret) {
		goto fail;
	}

	ret = setup_device_rx();
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream();
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream_open_prepare_start_with_device_rx();
	if (ret) {
		goto fail;
	}

	ret = setup_device_tx();
	if (ret) {
		goto fail;
	}

	ret = setup_capture_stream();
	if (ret) {
		goto fail;
	}

	ret = agm_session_set_ec_ref(session_id_tx1, aif_id_rx1, false);
	if (ret) {
		goto fail;
	}

	ret = agm_session_set_ec_ref(session_id_tx1, 0, true);
	if (ret) {
		goto fail;
	}


	ret = agm_deinit();
	if (ret) {
		goto fail;
	}

	printf("TEST PASS: %s()\n", __func__);
	goto done;

fail:
	printf("TEST FAIL: %s()\n", __func__);
	goto done;

done:
	testcase_common_deinit(__func__);
	return ret;
}

int test_stream_setparams(void)
{
	int ret = 0;
	char params[512] = {0};

	ret = testcase_common_init(__func__);
	if (ret) {
		goto fail;
	}

	ret = setup_device_rx();
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream();
	if (ret) {
		goto fail;
	}

	// stream + aif params
	ret = agm_session_aif_set_params(session_id_rx1, aif_id_rx1, params, sizeof(params));
	if (ret) {
		goto fail;
	}

	// stream params
	ret = agm_session_set_params(session_id_rx1, params, sizeof(params));
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream_open_prepare_start_with_device_rx();
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream_stop_close();
	if (ret) {
		goto fail;
	}

	printf("TEST PASS: %s()\n", __func__);
	goto done;

fail:
	printf("TEST FAIL: %s()\n", __func__);
	goto done;

done:
	testcase_common_deinit(__func__);
	return ret;

}

int test_stream_setparams_with_tags()
{
	int ret = 0;

	ret = testcase_common_init(__func__);
	if (ret) {
		goto fail;
	}

	ret = setup_device_rx();
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream();
	if (ret) {
		goto fail;
	}

	ret = agm_set_params_with_tag(session_id_rx1, aif_id_rx1, &tag_config);
	if (!ret) {
		goto fail;
	}

	ret = setup_playback_stream_open_prepare_start_with_device_rx();
	if (ret) {
		goto fail;
	}

	ret = agm_set_params_with_tag(session_id_rx1, aif_id_rx1, &tag_config);
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream_stop_close();
	if (ret) {
		goto fail;
	}

	printf("TEST PASS: %s()\n", __func__);
	goto done;

fail:
	printf("TEST FAIL: %s()\n", __func__);
	goto done;

done:
	testcase_common_deinit(__func__);
	return ret;
}

int test_stream_set_cal()
{
	int ret = 0;

	ret = testcase_common_init(__func__);
	if (ret) {
		goto fail;
	}

	ret = setup_device_rx();
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream();
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream_open_prepare_start_with_device_rx();
	if (ret) {
		goto fail;
	}

	ret = agm_session_aif_set_cal(session_id_rx1, aif_id_rx1, &cal);
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream_stop_close(sess_handle_rx1);
	if (ret) {
		goto fail;
	}

	printf("TEST PASS: %s()\n", __func__);
	goto done;

fail:
	printf("TEST FAIL: %s()\n", __func__);
	goto done;

done:
	testcase_common_deinit(__func__);
	return ret;
}

void agmevent_cb(uint32_t session_id, struct agm_event_cb_params *event_params, void *client_data)
{
	uint32_t cookie = (uint32_t) client_data;
	printf("%s session_id:%d, client_data:%d\n", __func__, session_id, cookie);
	printf("%s: source_module_id:%d, event_id:%d, event_payload_size:%d\n",
			__func__,event_params->source_module_id, event_params->event_id, event_params->event_payload_size);
}

int test_event_registration_and_notification()
{
	int ret = 0;
	struct agm_event_reg_cfg evt_cfg;

	ret = testcase_common_init(__func__);
	if (ret) {
		goto fail;
	}

	ret = agm_session_register_cb(session_id_rx1, agmevent_cb, AGM_EVENT_MODULE, (void *) session_id_rx1);
	if(ret) {
		goto fail;
	}

	ret = agm_session_register_cb(session_id_rx1, agmevent_cb, AGM_EVENT_MODULE, (void *) session_id_rx1+1);
	if(ret) {
		goto fail;
	}

	ret = setup_device_rx();
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream();
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream_open_prepare_start_with_device_rx();
	if (ret) {
		goto fail;
	}

	ret = agm_session_register_for_events(session_id_rx1, &evt_cfg);
	if (ret) {
		goto fail;
	}

	sleep(3);

	ret = setup_playback_stream_stop_close(sess_handle_rx1);
	if (ret) {
		goto fail;
	}

	printf("TEST PASS: %s()\n", __func__);
	goto done;

	fail:
	printf("TEST FAIL: %s()\n", __func__);
	goto done;

	done:
	//testcase_common_deinit(__func__);
	return ret;
}

int test_get_tagged_module_info()
{
	int ret = 0;
	char buf[512];
	size_t size = 0;

	ret = testcase_common_init(__func__);
	if (ret) {
		goto fail;
	}

	ret = setup_device_rx();
	if (ret) {
		goto fail;
	}

	ret = setup_playback_stream();
	if (ret) {
		goto fail;
	}

	// Module info list
	ret = agm_session_aif_get_tag_module_info(session_id_rx1, aif_id_rx1, NULL, &size);
	if (ret) {
		goto fail;
	}

	ret = agm_session_aif_get_tag_module_info(session_id_rx1, aif_id_rx1, buf, &size);
	if (ret) {
		goto fail;
	}

	printf("TEST PASS: %s()\n", __func__);
	goto done;

	fail:
	printf("TEST FAIL: %s()\n", __func__);
	goto done;

	done:
	//testcase_common_deinit(__func__);
	return ret;
}

int main() {
	int ret = 0;
	int i = 0;

	testcase testcases[] = {
			    test_device_get_aif_list,
				test_stream_sssd_with_buf_writes,
				test_stream_sssd_deviceswitch,
				test_stream_ssmd,
				test_stream_ssmd_teardown_first_device,
				test_stream_ssmd_deviceswitch_second_device,
				test_stream_ssmd_teardown_both_devices_resetup_firstdevice,
				test_stream_pause_resume,
				test_capture_sess_loopback,
				test_capture_sess_loopback2,
				test_stream_setparams,
				test_stream_setparams_with_tags,
				test_stream_set_cal,
				test_stream_set_ecref,
				test_get_tagged_module_info,
				test_event_registration_and_notification,
				//adverserial test cases
				test_stream_open_without_aif_connected,
				test_stream_open_with_same_aif_twice,
				test_stream_deint_with_mssd,

	};

	int testcount  = sizeof(testcases)/sizeof(testcase);
	int failed_count = 0;

	for (i = 0; i < testcount; i++) {
		printf("************* Start TestCase:%d*************\n", i+1);
		ret = testcases[i]();
		if (ret) {
			printf("Failed @ testcase no :%d\n", i+1);
			failed_count++;
		}
		printf("************* End TestCase:%d*************\n", i+1);
		printf("\n\n");
	}
	
	printf("\n\n");
	printf("*************TEST REPORT*************\n");
	printf("RAN:           %d/%d\n", i, testcount);
	printf("SUCCESSESFULL: %d\n", testcount- failed_count);
	printf("FAILED:        %d\n", failed_count);
	printf("*************************************\n\n\n\n");
	return 0;
}
