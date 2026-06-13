/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package qti.audio.core;

/**
 * This is a simple wrapper around an 'string', putting it in a parcelable, so it
 * can be used as an 'inout' parameter, be made '@nullable', etc.
 *
 * {@hide}
 */
@JavaDerive(equals=true, toString=true)
@VintfStability
parcelable VString {
   @utf8InCpp String value;
}
