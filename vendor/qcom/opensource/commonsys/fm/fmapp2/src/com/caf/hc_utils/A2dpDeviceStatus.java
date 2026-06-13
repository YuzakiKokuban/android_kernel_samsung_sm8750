/*
 * Copyright (c) 2011-2012, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *        * Redistributions of source code must retain the above copyright
 *            notice, this list of conditions and the following disclaimer.
 *        * Redistributions in binary form must reproduce the above copyright
 *            notice, this list of conditions and the following disclaimer in the
 *            documentation and/or other materials provided with the distribution.
 *        * Neither the name of The Linux Foundation nor
 *            the names of its contributors may be used to endorse or promote
 *            products derived from this software without specific prior written
 *            permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.    IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

package com.caf.utils;

import android.bluetooth.BluetoothA2dp;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothProfile;
import android.content.Intent;
import android.content.Context;

public class A2dpDeviceStatus {
    private BluetoothAdapter mAdapter;
    private BluetoothA2dp mA2dp = null;
    public String getActionSinkStateChangedString (){
        return BluetoothA2dp.ACTION_CONNECTION_STATE_CHANGED;
    }
    public String getActionPlayStateChangedString (){
        return BluetoothA2dp.ACTION_PLAYING_STATE_CHANGED;
    }
    public boolean  isA2dpStateChange( String action) {
        if(action.equals(BluetoothA2dp.ACTION_CONNECTION_STATE_CHANGED) ) {
           return true;
        }
        return false;
    }
    public boolean  isA2dpPlayStateChange( String action) {
        if(action.equals(BluetoothA2dp.ACTION_PLAYING_STATE_CHANGED) ) {
           return true;
        }
        return false;
    }
    public boolean isConnected(Intent intent) {
        boolean isConnected = false;
        int state = intent.getIntExtra(BluetoothA2dp.EXTRA_STATE,
                                BluetoothA2dp.STATE_DISCONNECTED);
        if((state == BluetoothA2dp.STATE_CONNECTED) || (state == BluetoothProfile.STATE_CONNECTED)
           || (state ==  BluetoothA2dp.STATE_CONNECTING)) {
            isConnected = true;
        }
        return isConnected;
    }
    public boolean isDisconnected(Intent intent) {
        boolean isDisconnected = false;
        int state = intent.getIntExtra(BluetoothA2dp.EXTRA_STATE,
                                BluetoothA2dp.STATE_CONNECTED);
        if((state == BluetoothA2dp.STATE_DISCONNECTED)) {
            isDisconnected = true;
        }
        return isDisconnected;
    }
    public boolean isPlaying(Intent intent) {
        boolean isPlaying = false;
        int state = intent.getIntExtra(BluetoothA2dp.EXTRA_STATE,
                                BluetoothA2dp.STATE_DISCONNECTED);
        if(state == BluetoothA2dp.STATE_PLAYING ) {
            isPlaying = true;
        }
        return isPlaying;
    }
    public boolean isDeviceAvailable() {
        if(null != mA2dp) {
            if( 0 != mA2dp.getConnectedDevices().size() ) {
                return true;
            }
        }
        return false;
    }

    public A2dpDeviceStatus(Context mContext) {
        mAdapter = BluetoothAdapter.getDefaultAdapter();
        if(mAdapter !=  null) {
            mAdapter.getProfileProxy(mContext, mProfileListener,
                    BluetoothProfile.A2DP);
        }
    }
    private BluetoothProfile.ServiceListener mProfileListener =
        new BluetoothProfile.ServiceListener() {
    public void onServiceConnected(int profile, BluetoothProfile proxy) {
        if (profile == BluetoothProfile.A2DP) {
            mA2dp = (BluetoothA2dp) proxy;
        }
    }
    public void onServiceDisconnected(int profile) {
        if (profile == BluetoothProfile.A2DP) {
            mA2dp = null;
        }
    }
};
}

