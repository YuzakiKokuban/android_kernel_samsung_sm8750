/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.paleventnotifier;

import vendor.qti.hardware.paleventnotifier.IPALEventNotifierCallback;

@VintfStability
interface IPALEventNotifier {

    int ipc_pal_notify_register_callback(in IPALEventNotifierCallback callback);

}
