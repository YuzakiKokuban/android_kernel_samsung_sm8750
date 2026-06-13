/*
 * Copyright (c) 2009-2012, The Linux Foundation. All rights reserved.
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

package qcom.fmradio;
import android.util.Log;
import java.util.Arrays;
import java.lang.Runnable;
import qcom.fmradio.FmReceiver;

class FmReceiverJNI {
    /**
     * General success
     */
    static final int FM_JNI_SUCCESS = 0;

    /**
     * General failure
     */
    static final int FM_JNI_FAILURE = -1;

    /**
     * native method: Open device
     * @return The file descriptor of the device
     *
     */
    private static final String TAG = "FmReceiverJNI";

    static {
        Log.d(TAG, "classinit native called");
        classInitNative();
    }
    static native void classInitNative();
    static native void initNative();
    static native void cleanupNative();

    private FmRxEvCallbacks mCallback;
    static private final int STD_BUF_SIZE = 256;
    static private byte[] mRdsBuffer = new byte[STD_BUF_SIZE];

    public static  byte[]  getPsBuffer(byte[] buff) {
        Log.d(TAG, "getPsBuffer enter");
        buff = Arrays.copyOf(mRdsBuffer, mRdsBuffer.length);
        Log.d(TAG, "getPsBuffer exit");
        return buff;
    }

    public void AflistCallback(byte[] aflist) {
        Log.e(TAG, "AflistCallback enter " );
        if (aflist == null) {
            Log.e(TAG, "aflist null return  ");
            return;
        }
        mRdsBuffer = Arrays.copyOf(aflist, aflist.length);
        FmReceiver.mCallback.FmRxEvRdsAfInfo();
        Log.e(TAG, "AflistCallback exit " );
    }

    public void getSigThCallback(int val, int status)
    {
        Log.d(TAG, "get Signal Threshold callback");

        FmReceiver.mCallback.FmRxEvGetSignalThreshold(val, status);
    }

    public void getChDetThCallback(int val, int status)
    {
        FmReceiver.mCallback.FmRxEvGetChDetThreshold(val, status);
    }

    public void setChDetThCallback(int status)
    {
        FmReceiver.mCallback.FmRxEvSetChDetThreshold(status);
    }

    public void DefDataRdCallback(int val, int status)
    {
        FmReceiver.mCallback.FmRxEvDefDataRead(val, status);
    }

    public void DefDataWrtCallback(int status)
    {
        FmReceiver.mCallback.FmRxEvDefDataWrite(status);
    }

    public void getBlendCallback(int val, int status)
    {
        FmReceiver.mCallback.FmRxEvGetBlend(val, status);
    }

    public void setBlendCallback(int status)
    {
        FmReceiver.mCallback.FmRxEvSetBlend(status);
    }

    public void getStnParamCallback(int val, int status)
    {
        FmReceiver.mCallback.FmRxGetStationParam(val, status);
    }

    public void getStnDbgParamCallback(int val, int status)
    {
        FmReceiver.mCallback.FmRxGetStationDbgParam(val, status);
    }

    public void enableSlimbusCallback(int status)
    {
        Log.d(TAG, "++enableSlimbusCallback" );
        FmReceiver.mCallback.FmRxEvEnableSlimbus(status);
        Log.d(TAG, "--enableSlimbusCallback" );
    }

	public void enableSoftMuteCallback(int status)
    {
        Log.d(TAG, "++enableSoftMuteCallback" );
        FmReceiver.mCallback.FmRxEvEnableSoftMute(status);
        Log.d(TAG, "--enableSoftMuteCallback" );
    }

    public void RtPlusCallback(byte[] rtplus) {
        Log.d(TAG, "RtPlusCallback enter " );
        if (rtplus == null) {
            Log.e(TAG, "psInfo null return  ");
            return;
        }
        mRdsBuffer = Arrays.copyOf(rtplus, rtplus.length);
        FmReceiver.mCallback.FmRxEvRTPlus();
        Log.d(TAG, "RtPlusCallback exit " );
    }

    public void RtCallback(byte[] rt) {
        Log.d(TAG, "RtCallback enter " );
        if (rt == null) {
            Log.e(TAG, "psInfo null return  ");
            return;
        }
        mRdsBuffer = Arrays.copyOf(rt, rt.length);
        FmReceiver.mCallback.FmRxEvRdsRtInfo();
        Log.d(TAG, "RtCallback exit " );
    }

    public void ErtCallback(byte[] ert) {
        Log.d(TAG, "ErtCallback enter " );
        if (ert == null) {
            Log.e(TAG, "ERT null return  ");
            return;
        }
        mRdsBuffer = Arrays.copyOf(ert, ert.length);
        FmReceiver.mCallback.FmRxEvERTInfo();
        Log.d(TAG, "RtCallback exit " );
    }

    public void EccCallback(byte[] ecc) {
        Log.i(TAG, "EccCallback enter " );
        if (ecc == null) {
            Log.e(TAG, "ECC null return  ");
            return;
        }
        mRdsBuffer = Arrays.copyOf(ecc, ecc.length);
        FmReceiver.mCallback.FmRxEvECCInfo();
        Log.i(TAG, "EccCallback exit " );
    }

    public void PsInfoCallback(byte[] psInfo) {
        Log.d(TAG, "PsInfoCallback enter " );
        if (psInfo == null) {
            Log.e(TAG, "psInfo null return  ");
            return;
        }
        Log.e(TAG, "length =  " +psInfo.length);
        mRdsBuffer = Arrays.copyOf(psInfo, psInfo.length);
        FmReceiver.mCallback.FmRxEvRdsPsInfo();
        Log.d(TAG, "PsInfoCallback exit");
    }

    public void enableCallback() {
        Log.d(TAG, "enableCallback enter");
        FmTransceiver.setFMPowerState(FmTransceiver.FMState_Rx_Turned_On);
        Log.v(TAG, "RxEvtList: CURRENT-STATE : FMRxStarting ---> NEW-STATE : FMRxOn");
        FmReceiver.mCallback.FmRxEvEnableReceiver();
        Log.d(TAG, "enableCallback exit");
    }

    public void tuneCallback(int freq) {
        int state;
        Log.d(TAG, "tuneCallback enter");
        state = FmReceiver.getSearchState();
        switch(state) {
        case FmTransceiver.subSrchLevel_SrchAbort:
            Log.v(TAG, "Current state is SRCH_ABORTED");
            Log.v(TAG, "Aborting on-going search command...");
            /* intentional fall through */
        case FmTransceiver.subSrchLevel_SeekInPrg :
            Log.v(TAG, "Current state is " + state);
            FmReceiver.setSearchState(FmTransceiver.subSrchLevel_SrchComplete);
            Log.v(TAG, "RxEvtList: CURRENT-STATE : Search ---> NEW-STATE : FMRxOn");
            FmReceiver.mCallback.FmRxEvSearchComplete(freq);
            break;
        default:
            if (freq > 0)
                FmReceiver.mCallback.FmRxEvRadioTuneStatus(freq);
            else
                Log.e(TAG, "get frequency command failed");
            break;
        }
        Log.d(TAG, "tuneCallback exit");
    }

    public void seekCmplCallback(int freq) {
        int state;

        Log.d(TAG, "seekCmplCallback enter");
        state = FmReceiver.getSearchState();
        switch (state) {
        case FmTransceiver.subSrchLevel_ScanInProg:
            Log.v(TAG, "Current state is " + state);
            FmReceiver.setSearchState(FmTransceiver.subSrchLevel_SrchComplete);
            Log.v(TAG, "RxEvtList: CURRENT-STATE : Search ---> NEW-STATE :FMRxOn");
            FmReceiver.mCallback.FmRxEvSearchComplete(freq);
            break;
        case FmTransceiver.subSrchLevel_SrchAbort:
            Log.v(TAG, "Current state is SRCH_ABORTED");
            Log.v(TAG, "Aborting on-going search command...");
            FmReceiver.setSearchState(FmTransceiver.subSrchLevel_SrchComplete);
            Log.v(TAG, "RxEvtList: CURRENT-STATE : Search ---> NEW-STATE : FMRxOn");
            FmReceiver.mCallback.FmRxEvSearchComplete(freq);
            break;
        }
        Log.d(TAG, "seekCmplCallback exit");
    }

    public void srchListCallback(byte[] scan_tbl) {
        int state;
        state = FmReceiver.getSearchState();
        switch (state) {
        case FmTransceiver.subSrchLevel_SrchListInProg:
            Log.v(TAG, "FmRxEventListener: Current state is AUTO_PRESET_INPROGRESS");
            FmReceiver.setSearchState(FmTransceiver.subSrchLevel_SrchComplete);
            Log.v(TAG, "RxEvtList: CURRENT-STATE : Search ---> NEW-STATE : FMRxOn");
            FmReceiver.mCallback.FmRxEvSearchListComplete();
            break;
        case FmTransceiver.subSrchLevel_SrchAbort:
            Log.v(TAG, "Current state is SRCH_ABORTED");
            Log.v(TAG, "Aborting on-going SearchList command...");
            FmReceiver.setSearchState(FmTransceiver.subSrchLevel_SrchComplete);
            Log.v(TAG, "RxEvtList: CURRENT-STATE : Search ---> NEW-STATE : FMRxOn");
            FmReceiver.mCallback.FmRxEvSearchCancelled();
            break;
       }
   }

    public void scanNxtCallback() {
        Log.d(TAG, "scanNxtCallback enter");
        FmReceiver.mCallback.FmRxEvSearchInProgress();
        Log.d(TAG, "scanNxtCallback exit");
    }

    public void stereostsCallback(boolean stereo) {
        Log.d(TAG, "stereostsCallback enter");
        FmReceiver.mCallback.FmRxEvStereoStatus (stereo);
        Log.d(TAG, "stereostsCallback exit");
    }

    public void rdsAvlStsCallback(boolean rdsAvl) {
        Log.d(TAG, "rdsAvlStsCallback enter");
        FmReceiver.mCallback.FmRxEvRdsLockStatus(rdsAvl);
        Log.d(TAG, "rdsAvlStsCallback exit");
    }

    public void disableCallback() {
        Log.d(TAG, "disableCallback enter");
        if (FmTransceiver.getFMPowerState() == FmTransceiver.subPwrLevel_FMTurning_Off) {
                 /*Set the state as FMOff */
            FmTransceiver.setFMPowerState(FmTransceiver.FMState_Turned_Off);
            Log.v(TAG, "RxEvtList: CURRENT-STATE : FMTurningOff ---> NEW-STATE : FMOff");
            FmReceiver.mCallback.FmRxEvDisableReceiver();
        } else {
            FmTransceiver.setFMPowerState(FmTransceiver.FMState_Turned_Off);
            Log.d(TAG, "Unexpected RADIO_DISABLED recvd");
            Log.v(TAG, "RxEvtList: CURRENT-STATE : FMRxOn ---> NEW-STATE : FMOff");
            FmReceiver.mCallback.FmRxEvRadioReset();
            Log.d(TAG, "disableCallback exit");
        }
    }

    public FmReceiverJNI(FmRxEvCallbacks callback) {
        mCallback = callback;
        if (mCallback == null)
            Log.e(TAG, "mCallback is null in JNI");
        Log.d(TAG, "init native called");
        initNative();
    }

    public FmReceiverJNI() {
        Log.d(TAG, "FmReceiverJNI constructor called");
    }

    /**
     * native method:
     * @param fd
     * @param control
     * @param field
     * @return
     */
    static native int audioControlNative(int fd, int control, int field);

    /**
     * native method: cancels search
     * @param fd file descriptor of device
     * @return May return
     *             {@link #FM_JNI_SUCCESS}
     *             {@link #FM_JNI_FAILURE}
     */
    static native int cancelSearchNative(int fd);

    /**
     * native method: get frequency
     * @param fd file descriptor of device
     * @return Returns frequency in int form
     */
    static native int getFreqNative(int fd);

    /**
     * native method: set frequency
     * @param fd file descriptor of device
     * @param freq freq to be set in int form
     * @return {@link #FM_JNI_SUCCESS}
     *         {@link #FM_JNI_FAILURE}
     *
     */
    static native int setFreqNative(int fd, int freq);

    /**
     * native method: get v4l2 control
     * @param fd file descriptor of device
     * @param id v4l2 id to be retrieved
     * @return Returns current value of the
     *         v4l2 control
     */
    static native int getControlNative (int fd, int id);

    /**
     * native method: set v4l2 control
     * @param fd file descriptor of device
     * @param id v4l2 control to be set
     * @param value value to be set
     * @return {@link #FM_JNI_SUCCESS}
     *         {@link #FM_JNI_FAILURE}
     */
    static native int setControlNative (int fd, int id, int value);

    /**
     * native method: start search
     * @param fd file descriptor of device
     * @param dir search direction
     * @return {@link #FM_JNI_SUCCESS}
     *         {@link #FM_JNI_FAILURE}
     */
    static native int startSearchNative (int fd, int dir);

    /**
     * native method: get RSSI value of the
     *                received signal
     * @param fd file descriptor of device
     * @return Returns signal strength in int form
     *         Signal value range from -120 to 10
     */
    static native int getRSSINative (int fd);

    /**
     * native method: set FM band
     * @param fd file descriptor of device
     * @param low lower band
     * @param high higher band
     * @return {@link #FM_JNI_SUCCESS}
     *         {@link #FM_JNI_FAILURE}
     */
    static native int setBandNative (int fd, int low, int high);

    /**
     * native method: get lower band
     * @param fd file descriptor of device
     * @return Returns lower band in int form
     */
    static native int getLowerBandNative (int fd);

    /**
     * native method: get upper band
     * @param fd file descriptor of device
     * @return Returns upper band in int form
     */
    static native int getUpperBandNative (int fd);

    /**
     * native method: force Mono/Stereo mode
     * @param fd file descriptor of device
     * @param val force mono/stereo indicator
     * @return {@link #FM_JNI_SUCCESS}
     *         {@link #FM_JNI_FAILURE}
     */
    static native int setMonoStereoNative (int fd, int val);

    /**
     * native method: get Raw RDS data
     * @param fd file descriptor of device
     * @param buff[] buffer
     * @param count number of bytes to be read
     * @return Returns number of bytes read
     */
    static native int getRawRdsNative (int fd, byte  buff[], int count);

   /**
     * native method: Sets the repeat count for Programme service
     * transmission.
     * @param fd file descriptor of device
     * @param repeatcount  number of times PS string to be transmited
     *                     repeatedly.
     * @return {@link #FM_JNI_SUCCESS}
     *         {@link #FM_JNI_FAILURE}
     */
    static native int setPSRepeatCountNative(int fd, int repeatCount);
   /**
     * native method: Sets the power level for the tramsmitter
     * transmission.
     * @param fd file descriptor of device
     * @param powLevel is the level at which transmitter operates.
     * @return {@link #FM_JNI_SUCCESS}
     *         {@link #FM_JNI_FAILURE}
     */
    static native int setTxPowerLevelNative(int fd, int powLevel);

   /**
     * native method: Configures the spur table
     * @param fd file descriptor of device
     * @return {@link #FM_JNI_SUCCESS}
     *         {@link #FM_JNI_FAILURE}
     */
    static native int configureSpurTable(int fd);

    /**
     * native method: Configures the new spur table
     * @param fd file descriptor of device
     * @return {@link #FM_JNI_SUCCESS}
     *         {@link #FM_JNI_FAILURE}
     */
    static native int setSpurDataNative(int fd, short  buff[], int len);
    static native int enableSlimbus(int fd, int val);
    static native int enableSoftMute(int fd, int val);
    static native String getSocNameNative();
    static native boolean getFmStatsPropNative();
    static native int getFmCoexPropNative(int fd, int property);
}
