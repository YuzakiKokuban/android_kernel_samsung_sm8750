/*
 * Implements HIDL communication channel for fst manager
 *
 * Copyright (c) 2019, The Linux Foundation. All rights reserved.
 * Copyright (c) 2004-2016, Jouni Malinen <j@w1.fi>
 * Copyright (c) 2004-2016, Roshan Pius <rpius@google.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <vendor/qti/hardware/fstman/1.0/IFstManager.h>
#include <vendor/qti/hardware/fstman/1.0/types.h>
#include <hwbinder/IPCThreadState.h>
#include <hwbinder/ProcessState.h>
#include <cutils/properties.h>
#include <hidl/HidlTransportSupport.h>

#define FST_MGR_COMPONENT "HIDL"
#include "fst_manager.h"
#include "fst_cfgmgr.h"
#include "hidl_manager.h"

extern "C"
{
#include "utils/common.h"
#include "utils/eloop.h"
#include "utils/os.h"
}
#include "fst_hidl.h"

using android::hardware::hidl_string;
using namespace android;
using android::hardware::configureRpcThreadpool;
using android::hardware::handleTransportPoll;
using android::hardware::setupTransportPolling;
using vendor::qti::hardware::fstman::V1_0::implementation::HidlManager;
using FstManagerStatus = ::vendor::qti::hardware::fstman::V1_0::FstManagerStatus;
using FstManagerStatusCode = ::vendor::qti::hardware::fstman::V1_0::FstManagerStatusCode;
using IFstManager = ::vendor::qti::hardware::fstman::V1_0::IFstManager;

struct fst_hidl_priv {
	int hidl_fd;
};

static void fst_hidl_sock_handler(
    int sock, void * /* eloop_ctx */, void * /* sock_ctx */)
{
	handleTransportPoll(sock);
}

void *fst_hidl_init(void)
{
	struct fst_hidl_priv *priv;
	HidlManager *hidl_manager;
	struct fst_group_info *groups = NULL;
	int nof_groups, i;

	priv = (fst_hidl_priv *)os_zalloc(sizeof(*priv));
	if (!priv) {
		fst_mgr_printf(MSG_ERROR, "failed to allocate hidl area");
		return NULL;
	}

	fst_mgr_printf(MSG_INFO, "initializing hidl control");

	configureRpcThreadpool(1, true /* callerWillJoin */);
	priv->hidl_fd = setupTransportPolling();
	if (priv->hidl_fd < 0)
		goto err;

	fst_mgr_printf(MSG_INFO, "Processing hidl events on FD %d", priv->hidl_fd);
	// Look for read events from the hidl socket in the eloop.
	if (eloop_register_read_sock(
		priv->hidl_fd, fst_hidl_sock_handler, NULL, priv) < 0)
		goto err;

	hidl_manager = HidlManager::getInstance();
	if (!hidl_manager)
		goto err;
	hidl_manager->registerHidlService();

	nof_groups = fst_cfgmgr_get_groups(&groups);

	if (nof_groups > 0) {
		for (i = 0; i < nof_groups; i++)
			hidl_manager->registerGroup(groups[i].id);
		fst_free(groups);
	}

	return priv;
err:
	fst_hidl_deinit(priv);
	return NULL;
}

void fst_hidl_deinit(void *handle)
{
	HidlManager *hidl_manager;
	struct fst_group_info *groups = NULL;
	int nof_groups, i;

	if (!handle)
		return;
	hidl_manager = HidlManager::getInstance();
	if (!hidl_manager)
		return;

	struct fst_hidl_priv *priv = (struct fst_hidl_priv *)handle;
	fst_mgr_printf(MSG_INFO, "Deiniting hidl control");

	nof_groups = fst_cfgmgr_get_groups(&groups);

	if (nof_groups > 0) {
		for (i = 0; i < nof_groups; i++)
			hidl_manager->unregisterGroup(groups[i].id);
		fst_free(groups);
	}

	HidlManager::destroyInstance();
	eloop_unregister_read_sock(priv->hidl_fd);
	os_free(priv);
}
