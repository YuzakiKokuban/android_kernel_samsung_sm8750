/*
 * Copyright (c) 2009,2012-2014, The Linux Foundation. All rights reserved.
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

import android.content.Context;
import android.os.Bundle;
import android.telephony.TelephonyManager;
import android.telephony.PhoneStateListener;
import android.util.Log;
import android.os.SystemProperties;
import java.util.Arrays;
import java.lang.Runnable;
import android.content.BroadcastReceiver;
import android.content.Intent;
import android.net.wifi.WifiManager;
import android.content.IntentFilter;
import android.bluetooth.BluetoothAdapter;

/**
 * This class contains all interfaces and types needed to
 * Control the FM receiver.
 *    @hide
 */
public class FmReceiver extends FmTransceiver
{

   public static int mSearchState = subSrchLevel_NoSearch;
   private IntentFilter mIntentFilter;
   private IntentFilter mBtIntentFilter;

   static final int STD_BUF_SIZE = 256;
   static final int GRP_3A = 64;
   static final int ENABLE_LPF = 1;
   static final int DISABLE_LPF = 0;
   private static final String TAG = "FMRadio";


   private static int  mEnableLpfGprs = 0x1;
   private static int  mEnableLpfEdge = 0x2;
   private static int  mEnableLpfUmts = 0x4;
   private static int  mEnableLpfCdma = 0x8;
   private static int  mEnableLpfEvdo0 = 0x10;
   private static int  mEnableLpfEvdoA = 0x20;
   private static int  mEnableLpf1xRtt = 0x40;
   private static int  mEnableLpfHsdpa = 0x80;
   private static int  mEnableLpfHsupa = 0x100;
   private static int  mEnableLpfHspa = 0x200;
   private static int  mEnableLpfIden = 0x400;
   private static int  mEnableLpfEvdoB = 0x800;
   private static int  mEnableLpfLte = 0x1000;
   private static int  mEnableLpfEhrpd = 0x2000;
   private static int  mEnableLpfHspap = 0x4000;
   private static int  mEnableLpfGsm = 0x8000;
   private static int  mEnableLpfScdma = 0x10000;
   private static int  mEnableLpfIwlan = 0x20000;
   private static int  mEnableLpfLteCa = 0x40000;

   private static int  mIsBtLpfEnabled = 0x01;
   private static int  mIsWlanLpfEnabled = 0x2;
   /**
   * Search (seek/scan/searchlist) by decrementing the frequency
   *
   * @see #FM_RX_SEARCHDIR_UP
   * @see #searchStations(int, int, int)
   * @see #searchStations(int, int, int, int, int)
   * @see #searchStationList
   */
   public static final int FM_RX_SEARCHDIR_DOWN=0;
   /**
   * Search (seek/scan/searchlist) by inrementing the frequency
   *
   * @see #FM_RX_SEARCHDIR_DOWN
   * @see #searchStations(int, int, int)
   * @see #searchStations(int, int, int, int, int)
   * @see #searchStationList
   */
   public static final int FM_RX_SEARCHDIR_UP=1;

   /**
   * Scan dwell (Preview) duration = 0 seconds
   *
   * @see #searchStations(int, int, int)
   * @see #searchStations(int, int, int, int, int)
   */
   public static final int FM_RX_DWELL_PERIOD_0S=0;
  /**
   * Scan dwell (Preview) duration = 1 second
   *
   * @see #searchStations(int, int, int)
   * @see #searchStations(int, int, int, int, int)
   */
   public static final int FM_RX_DWELL_PERIOD_1S=1;
   /**
   * Scan dwell (Preview) duration = 2 seconds
   *
   * @see #searchStations(int, int, int)
   * @see #searchStations(int, int, int, int, int)
   */
   public static final int FM_RX_DWELL_PERIOD_2S=2;
   /**
   * Scan dwell (Preview) duration = 3 seconds
   *
   * @see #searchStations(int, int, int)
   * @see #searchStations(int, int, int, int, int)
   */
   public static final int FM_RX_DWELL_PERIOD_3S=3;
   /**
   * Scan dwell (Preview) duration = 4 seconds
   *
   * @see #searchStations(int, int, int)
   * @see #searchStations(int, int, int, int, int)
   */
   public static final int FM_RX_DWELL_PERIOD_4S=4;
   /**
    * Scan dwell (Preview) duration = 5 seconds
    *
    * @see #searchStations(int, int, int)
    * @see #searchStations(int, int, int, int, int)
    */
   public static final int FM_RX_DWELL_PERIOD_5S=5;
   /**
    * Scan dwell (Preview) duration = 6 seconds
    *
    * @see #searchStations(int, int, int)
    * @see #searchStations(int, int, int, int, int)
    */
   public static final int FM_RX_DWELL_PERIOD_6S=6;
   /**
    * Scan dwell (Preview) duration = 7 second
    *
    * @see #searchStations(int, int, int)
    * @see #searchStations(int, int, int, int, int)
    */
   public static final int FM_RX_DWELL_PERIOD_7S=7;


   /**
   * Basic Seek Mode Option
   *
   * @see #searchStations(int, int, int)
   */
   public static final int FM_RX_SRCH_MODE_SEEK        =0;
   /**
   * Basic Scan Mode Option
   *
   * @see #searchStations(int, int, int)
   */
   public static final int FM_RX_SRCH_MODE_SCAN        =1;

   /**
   * Search list mode Options to search for Strong stations
   *
   * @see #searchStationList
   */
   public static final int FM_RX_SRCHLIST_MODE_STRONG  =2;
   /**
   * Search list mode Options to search for Weak stations
   *
   * @see #searchStationList
   */
   public static final int FM_RX_SRCHLIST_MODE_WEAK    =3;

   /**
   * Seek by Program Type
   *
   * @see #searchStations(int, int, int, int, int)
   */
   public static final int FM_RX_SRCHRDS_MODE_SEEK_PTY =4;
   /**
   * Scan by Program Type
   *
   * @see #searchStations(int, int, int, int, int)
   */
   public static final int FM_RX_SRCHRDS_MODE_SCAN_PTY =5;
   /**
   * Seek by Program identification
   *
   * @see #searchStations(int, int, int, int, int)
   */
   public static final int FM_RX_SRCHRDS_MODE_SEEK_PI  =6;
   /**
   * Seek Alternate Frequency for the same station
   *
   * @see #searchStations(int, int, int, int, int)
   */
   public static final int FM_RX_SRCHRDS_MODE_SEEK_AF  =7;
   /**
   * Search list mode Options to search for Strongest stations
   *
   * @see #searchStations(int, int, int, int, int)
   */
   public static final int FM_RX_SRCHLIST_MODE_STRONGEST  =8;
   /**
   * Search list mode Options to search for Weakest stations
   *
   * @see #searchStations(int, int, int, int, int)
   */
   public static final int FM_RX_SRCHLIST_MODE_WEAKEST  =9;

   /**
   * Maximum number of stations the SearchStationList can
   * support
   *
   * @see #searchStationList
   */
   public static final int FM_RX_SRCHLIST_MAX_STATIONS =12;

   /**
    *  Argument option for setMuteMode to unmute FM
    *
    *  @see #setMuteMode
    */
   public static final int FM_RX_UNMUTE     =0;
   /**
    *  Argument option for setMuteMode to Mute FM
    *
    *  @see #setMuteMode
    */
   public static final int FM_RX_MUTE       =1;

   /**
    *  Argument option for setStereoMode to set FM to Stereo
    *  Mode.
    *
    *  @see #setStereoMode
    */
   public static final int FM_RX_AUDIO_MODE_STEREO    =0;
   /**
    *  Argument option for setStereoMode to set FM to "Force
    *  Mono" Mode.
    *
    *  @see #setStereoMode
    */
   public static final int FM_RX_AUDIO_MODE_MONO      =1;

   /**
    *  Signal Strength
    *
    *  @see #setSignalThreshold
    *  @see #getSignalThreshold
    */
   public static final int FM_RX_SIGNAL_STRENGTH_VERY_WEAK  =0;
   public static final int FM_RX_SIGNAL_STRENGTH_WEAK       =1;
   public static final int FM_RX_SIGNAL_STRENGTH_STRONG     =2;
   public static final int FM_RX_SIGNAL_STRENGTH_VERY_STRONG=3;

   /**
    * Power settings
    *
    * @see #setPowerMode
    * @see #getPowerMode
    */
   public static final int FM_RX_NORMAL_POWER_MODE   =0;
   public static final int FM_RX_LOW_POWER_MODE      =1;



   /**
    * RDS Processing Options
    *
    * @see #registerRdsGroupProcessing
    * @see #getPSInfo
    * @see #getRTInfo
    * @see #getAFInfo
    */
   public static final int FM_RX_RDS_GRP_RT_EBL         =1;
   public static final int FM_RX_RDS_GRP_PS_EBL         =2;
   public static final int FM_RX_RDS_GRP_PS_SIMPLE_EBL  =4;
   public static final int FM_RX_RDS_GRP_AF_EBL         =8;
   public static final int FM_RX_RDS_GRP_ECC_EBL        =32;
   public static final int FM_RX_RDS_GRP_PTYN_EBL       =64;
   public static final int FM_RX_RDS_GRP_RT_PLUS_EBL    =128;


   private static final int V4L2_CID_PRIVATE_BASE = 0x8000000;
   private static final int V4L2_CID_PRIVATE_TAVARUA_SIGNAL_TH = V4L2_CID_PRIVATE_BASE + 8;
   private static final int V4L2_CTRL_CLASS_USER = 0x00980000;
   private static final int V4L2_CID_PRIVATE_IRIS_GET_SPUR_TBL = (V4L2_CTRL_CLASS_USER + 0x92E);


   private static final int TAVARUA_BUF_SRCH_LIST=0;
   private static final int TAVARUA_BUF_EVENTS=1;
   private static final int TAVARUA_BUF_RT_RDS=2;
   private static final int TAVARUA_BUF_PS_RDS=3;
   private static final int TAVARUA_BUF_RAW_RDS=4;
   private static final int TAVARUA_BUF_AF_LIST=5;
   private static final int TAVARUA_BUF_MAX=6;

   public static FmRxEvCallbacksAdaptor mCallback;
   static FmRxEvCallbacks callback;
   static FmReceiverJNI mFmReceiverJNI;
  /**
    *  Internal Constants for Signal thresholds
    *
    *  @see #setSignalThreshold
    *  @see #getSignalThreshold
    */
   private static final int FM_RX_RSSI_LEVEL_VERY_WEAK   = -105;
   private static final int FM_RX_RSSI_LEVEL_WEAK        = -100;
   private static final int FM_RX_RSSI_LEVEL_STRONG      = -96;
   private static final int FM_RX_RSSI_LEVEL_VERY_STRONG = -90;

   /**
     * BUF_TYPE
     */
   private static final int BUF_ERT = 12;
   private static final int BUF_RTPLUS = 11;

   private static final int LEN_IND = 0;
   private static final int RT_OR_ERT_IND = 1;
   private static final int ENCODE_TYPE_IND = 1;
   private static final int ERT_DIR_IND = 2;

  /**
    * Search Algo type
    */
   private static final int SEARCH_MPXDCC = 0;
   private static final int SEARCH_SINR_INT = 1;

   /**
    * Fm Coex property type
    */
   private static final int WAN_RATCONF = 0;
   private static final int BTWLAN_LPFENABLER = 1;


   public boolean isSmdTransportLayer() {
       String chip = getSocName();
       if (chip.equals("pronto"))
           return true;
       else
           return false;
   }

   public static boolean isRomeChip() {
       String chip = FmReceiverJNI.getSocNameNative();
       if (chip.equals("rome"))
           return true;
       else
           return false;
   }

   public PhoneStateListener  mDataConnectionStateListener = new PhoneStateListener(){
        public void onDataConnectionStateChanged(int state, int networkType) {
              Log.d (TAG, "state: " + Integer.toString(state) +  " networkType: " + Integer.toString(networkType));
              if (state == TelephonyManager.DATA_CONNECTED) {
                  FMcontrolLowPassFilter(state, networkType, ENABLE_LPF);
              } else if (state == TelephonyManager.DATA_DISCONNECTED) {
                  FMcontrolLowPassFilter(state, networkType, DISABLE_LPF);
              }
       }
   };

   /* Register for wan state changes to support wan-fm concurrency */
   public void registerDataConnectionStateListener(Context mContext) {
       Log.d (TAG, "registerDataConnectionStateListener");
       TelephonyManager tm = (TelephonyManager) mContext.getSystemService(Context.TELEPHONY_SERVICE);
       tm.listen(mDataConnectionStateListener, PhoneStateListener.LISTEN_DATA_CONNECTION_STATE);
   }

   /* UnRegister */
   public void unregisterDataConnectionStateListener(Context mContext) {
       Log.d (TAG, "unregisterDataConnectionStateListener: ");
       TelephonyManager tm = (TelephonyManager) mContext.getSystemService(Context.TELEPHONY_SERVICE);
       tm.listen(mDataConnectionStateListener, PhoneStateListener.LISTEN_NONE);
   }

   private final BroadcastReceiver mReceiver = new BroadcastReceiver() {

       @Override
       public void onReceive(Context context, Intent intent) {

           Log.d (TAG, "onReceive: Wifi State change intent");

           if (WifiManager.WIFI_STATE_CHANGED_ACTION.equals(intent.getAction())) {
               int newState = intent.getIntExtra(WifiManager.EXTRA_WIFI_STATE,
                       WifiManager.WIFI_STATE_UNKNOWN);
               int mBtWlanLpf = getFmWanWlanCoexProp(BTWLAN_LPFENABLER);
               if (newState == WifiManager.WIFI_STATE_ENABLED) {
                   Log.d (TAG, "enable LPF on wifi enabled " + newState);
                   if ((mBtWlanLpf & mIsWlanLpfEnabled) == mIsWlanLpfEnabled)
                       mControl.enableLPF(sFd, ENABLE_LPF);
               } else if ((mBtWlanLpf & mIsWlanLpfEnabled) == mIsWlanLpfEnabled) {
                   Log.d (TAG, "Disable LPF on wifi state other than enabled " + newState);
                   mControl.enableLPF(sFd, DISABLE_LPF);
               }
           } else {
               Log.d (TAG, "WIFI_STATE_CHANGED_ACTION failed");
           }
       }
   };

   private final BroadcastReceiver mBtReceiver = new BroadcastReceiver() {
       @Override
       public void onReceive(Context context, Intent intent) {

           Log.d (TAG, "onReceive: Bluetooth State change intent");

           if (BluetoothAdapter.ACTION_STATE_CHANGED.equals(intent.getAction())) {
               int newState = intent.getIntExtra(BluetoothAdapter.EXTRA_STATE, BluetoothAdapter.ERROR);
               int mBtWlanLpf = SystemProperties.getInt("persist.btwlan.lpfenabler", 0);
               if (newState == BluetoothAdapter.STATE_ON) {
                   Log.d (TAG, "enable LPF on BT enabled " + newState);
                   if ((mBtWlanLpf & mIsBtLpfEnabled) == mIsBtLpfEnabled)
                       mControl.enableLPF(sFd, ENABLE_LPF);
               } else if ((mBtWlanLpf & mIsBtLpfEnabled) == mIsBtLpfEnabled) {
                   Log.d (TAG, "Disable LPF on BT state other than enabled " + newState);
                   mControl.enableLPF(sFd, DISABLE_LPF);
               }
           } else {
               Log.d (TAG, "ACTION_STATE_CHANGED failed");
           }
       }
   };

   /**
    * Constructor for the receiver Object
    */
   public FmReceiver(){
      mControl = new FmRxControls();
      mRdsData = new FmRxRdsData (sFd);
   }

   /**
   *    Constructor for the receiver Object that takes path to
   *    radio and event callbacks.
   *    <p>
   *    @param devicePath FM Device path String.
   *    @param callback the callbacks to handle the events
   *                               events from the FM receiver.
   *
   */
   public FmReceiver(String devicePath,
                     FmRxEvCallbacksAdaptor callback) throws InstantiationException {
      mControl = new FmRxControls();

      Log.e(TAG, "FmReceiver constructor");
      //registerClient(callback);
      mCallback = callback;
      mFmReceiverJNI = new FmReceiverJNI(mCallback);
   }

   /*==============================================================
   FUNCTION:  enable
   ==============================================================*/
   /**
   *    Enables the FM device in Receiver Mode.
   *    <p>
   *    This is a synchronous method used to initialize the FM
   *    receiver. If already initialized this function will
   *    intialize the receiver with default settings. Only after
   *    successfully calling this function can many of the FM device
   *    interfaces be used.
   *    <p>
   *    When enabling the receiver, the client must also provide
   *    the regional settings in which the receiver will operate.
   *    These settings (included in argument configSettings) are
   *    typically used for setting up the FM receiver for operating
   *    in a particular geographical region. These settings can be
   *    changed after the FM driver is enabled through the use of
   *    the function {@link #configure}.
   *    <p>
   *    This command can only be issued by the owner of an FM
   *    receiver.  To issue this command, the client must first
   *    successfully call {@link #acquire}.
   *    <p>
   *    @param configSettings  the settings to be applied when
   *                             turning on the radio
   *    @return true if Initialization succeeded, false if
   *            Initialization failed.
   *    <p>
   *    @see #enable
   *    @see #registerClient
   *    @see #disable
   *
   */
   public boolean enable (FmConfig configSettings, Context app_context){
      boolean status = false;
      /*
       * Check for FM State.
       * If FMRx already on, then return.
      */
      int state = getFMState();

      mIntentFilter = new IntentFilter(WifiManager.WIFI_STATE_CHANGED_ACTION);
      mBtIntentFilter = new IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED);

      if (state == FMState_Rx_Turned_On || state == FMState_Srch_InProg) {
         Log.d(TAG, "enable: FM already turned On and running");
         return status;
      }else if (state == subPwrLevel_FMTurning_Off) {
         Log.v(TAG, "FM is in the process of turning off.Pls wait for sometime.");
         return status;
      }else if (state == subPwrLevel_FMRx_Starting) {
         Log.v(TAG, "FM is in the process of turning On.Pls wait for sometime.");
         return status;
      }else if ((state == FMState_Tx_Turned_On)
                || (state == subPwrLevel_FMTx_Starting)) {
         Log.v(TAG, "FM Tx is turned on or in the process of turning on.");
         return status;
      }

      setFMPowerState(subPwrLevel_FMRx_Starting);
      Log.v(TAG, "enable: CURRENT-STATE : FMOff ---> NEW-STATE : FMRxStarting");
      status = super.enable(configSettings, FmTransceiver.FM_RX);

      if (status == true ) {
          mRdsData = new FmRxRdsData(sFd);
          registerDataConnectionStateListener(app_context);
          app_context.registerReceiver(mReceiver, mIntentFilter);
          int mBtWlanLpf = SystemProperties.getInt("persist.btwlan.lpfenabler", 0);
          WifiManager wifiManager = (WifiManager)app_context.getSystemService(app_context.WIFI_SERVICE);
          if (wifiManager.getWifiState() == WifiManager.WIFI_STATE_ENABLED) {
              if ((mBtWlanLpf & mIsWlanLpfEnabled) == mIsWlanLpfEnabled) {
                 Log.d(TAG, "enable LPF if WIFI is already on");
                 mControl.enableLPF(sFd, ENABLE_LPF);
              }
          }
          app_context.registerReceiver(mBtReceiver, mBtIntentFilter);
          BluetoothAdapter btAdapter = BluetoothAdapter.getDefaultAdapter();
          if (btAdapter != null) {
              if ((mBtWlanLpf & mIsWlanLpfEnabled) == mIsWlanLpfEnabled) {
                  Log.d(TAG, "enable LPF if BT is already on");
                  mControl.enableLPF(sFd, ENABLE_LPF);
              }
          }
      }
      else {
         status = false;
         Log.e(TAG, "enable: Error while turning FM On");
         Log.e(TAG, "enable: CURRENT-STATE : FMRxStarting ---> NEW-STATE : FMOff");
         setFMPowerState(FMState_Turned_Off);
      }
      return status;
   }

   /*==============================================================
   FUNCTION:  reset
   ==============================================================*/
   /**
   *    Reset the FM Device.
   *    <p>
   *    This is a synchronous command used to reset the state of FM
   *    device in case of unrecoverable error. This function is
   *    expected to be used when the client receives unexpected
   *    notification of radio disabled. Once called, most
   *    functionality offered by the FM device will be disabled
   *    until the client re-enables the device again via
   *    {@link #enable}.
   *    <p>
   *    @return true if reset succeeded, false if reset failed.
   *    @see #enable
   *    @see #disable
   *    @see #registerClient
   */
   public boolean reset(){
      boolean status = false;
      int state = getFMState();

      if(state == FMState_Turned_Off) {
         Log.d(TAG, "FM already turned Off.");
         return false;
      }

      setFMPowerState(FMState_Turned_Off);
      Log.v(TAG, "reset: NEW-STATE : FMState_Turned_Off");

      return status;
   }

   /*==============================================================
   FUNCTION:  disable
   ==============================================================*/
   /**
   *    Disables the FM Device.
   *    <p>
   *    This is a synchronous command used to disable the FM
   *    device. This function is expected to be used when the
   *    client no longer requires use of the FM device. Once
   *    called, most functionality offered by the FM device will be
   *    disabled until the client re-enables the device again via
   *    {@link #enable}.
   *    <p>
   *    @return true if disabling succeeded, false if disabling
   *            failed.
   *    @see #enable
   *    @see #registerClient
   */
   public boolean disable(Context app_context){
      boolean status = false;
      /*
       * Check for FM State. If search is in progress, then cancel the search prior
       * to disabling FM.
      */
      int state = getFMState();
      switch(state) {
      case FMState_Turned_Off:
         Log.d(TAG, "FM already tuned Off.");
         return false;
      case FMState_Srch_InProg:
         Log.v(TAG, "disable: Cancelling the on going search operation prior to disabling FM");
         setSearchState(subSrchLevel_SrchAbort);
         cancelSearch();
         Log.v(TAG, "disable: Wait for the state to change from : Search ---> FMRxOn");
         try {
            /*
             *    The delay of 50ms here is very important.
             *    This delay is useful for the cleanup purpose
             *    when HS is abruptly plugged out when search
             *    is in progress.
            */
            Thread.sleep(50);
         } catch (InterruptedException e) {
            e.printStackTrace();
         }
         break;
      case subPwrLevel_FMRx_Starting:
      /*
       * If, FM is in the process of turning On, then wait for
       * the turn on operation to complete before turning off.
      */
         Log.d(TAG, "disable: FM not yet turned On...");
         try {
            Thread.sleep(100);
         } catch (InterruptedException e) {
            e.printStackTrace();
         }
         /* Check for the state of FM device */
         state = getFMState();
         if(state == subPwrLevel_FMRx_Starting) {
            Log.e(TAG, "disable: FM in bad state");
            return status;
         }
         break;
      case subPwrLevel_FMTurning_Off:
      /*
       * If, FM is in the process of turning Off, then wait for
       * the turn off operation to complete.
      */
         Log.v(TAG, "disable: FM is getting turned Off.");
            return status;
      }

      setFMPowerState(subPwrLevel_FMTurning_Off);
      Log.v(TAG, "disable: CURRENT-STATE : FMRxOn ---> NEW-STATE : FMTurningOff");
      super.disable();
      unregisterDataConnectionStateListener(app_context);
      app_context.unregisterReceiver(mBtReceiver);
      app_context.unregisterReceiver(mReceiver);
      return true;
   }

   /*==============================================================
   FUNCTION:  getSearchState
   ==============================================================*/
   /**
   *    Gets the current state of the search operation.
   *    <p>
   *    This function is expected to be used when searchStations()
   *    function wants to know whether any seek/scan/auto-select
   *    operation is already in-progress.
   *    If a seek command is issued when one is already in-progress,
   *    we cancel the on-going seek command and start a new search
   *    operation.
   *    <p>
   *    @return current state of FM Search operation:
   *                SRCH_COMPLETE
   *                SRCH_INPROGRESS
   *                SRCH_ABORTED
   */
   static int getSearchState()
   {
      return mSearchState;
   }

   /*==============================================================
   FUNCTION:  setSearchState
   ==============================================================*/
   /**
   *    Sets the current state of the search operation.
   *    <p>
   *    This function is used to set the current state of the
   *    search operation. If a seek command is issued when one
   *    is already in-progress, we cancel the on-going seek command,
   *    set the state of search operation to SRCH_ABORTED
   *    and start a new search.
   *    <p>
   *    @return none
   */
   static void setSearchState(int state)
   {
      mSearchState = state;
      switch(mSearchState) {
         case subSrchLevel_SeekInPrg:
         case subSrchLevel_ScanInProg:
         case subSrchLevel_SrchListInProg:
            setFMPowerState(FMState_Srch_InProg);
            break;
         case subSrchLevel_SrchComplete:
            /* Update the state of the FM device */
            mSearchState = subSrchLevel_NoSearch;
            setFMPowerState(FMState_Rx_Turned_On);
            break;
         case subSrchLevel_SrchAbort:
            break;
         default:
            mSearchState = subSrchLevel_NoSearch;
            break;
      }
   }

   /*==============================================================
   FUNCTION:  searchStations
   ==============================================================*/
   /**
   *   Initiates basic seek and scan operations.
   *    <p>
   *    This command is used to invoke a basic seek/scan of the FM
   *    radio band.
   *    <p>
   *    <ul>
   *    This API is used to:
   *    <li> Invoke basic seek operations ({@link
   *    #FM_RX_SRCH_MODE_SEEK})
   *    <li> Invoke basic scan operations ({@link
   *    #FM_RX_SRCH_MODE_SCAN})
   *    </ul>
   *    <p>
   *    The most basic operation performed by this function
   *    is a {@link #FM_RX_SRCH_MODE_SEEK} command. The seek
   *    process is handled incrementing or decrementing the
   *    frequency in pre-defined channel steps (defined by the
   *    channel spacing) and measuring the resulting signal level.
   *    Once a station is successfully tuned and found to meet or
   *    exceed this signal level, the seek operation will be
   *    completed and a FmRxEvSearchComplete event will be returned
   *    to the client. If no stations are found to match the search
   *    criteria, the frequency will be returned to the originally
   *    tuned station.
   *    <p>
   *    Since seek always results in a frequency being tuned, each
   *    seek operation will also return a single
   *    FmRxEvRadioTuneStatus event to the client/application
   *    layer.
   *    <p>
   *    Much like {@link #FM_RX_SRCH_MODE_SEEK}, a {@link
   *    #FM_RX_SRCH_MODE_SCAN} command can be likened to many back
   *    to back seeks with a dwell period after each successful
   *    seek. Once issued, a scan will either increment or
   *    decrement frequencies by the defined channel spacing until
   *    a station is found to meet or exceed the set search
   *    threshold. Once this station is found, and is successfully
   *    tuned, an FmRxEvRadioTuneStatus event will be returned to
   *    the client and the station will remain tuned for the
   *    specific period of time indicated by argument dwellPeriod.
   *    After that time expires, an FmRxEvSearchInProgress event
   *    will be sent to the client and a new search will begin for
   *    the next station that meets the search threshold. After
   *    scanning the entire band, or after a cancel search has been
   *    initiated by the client, an FmRxEvRadioTuneStatus event
   *    will be sent to the client. Similar to a seek command, each
   *    scan will result in at least one station being tuned, even
   *    if this is the starting frequency.
   *    <p>
   *    Each time the driver initiates a search (seek or scan) the client
   *    will be notified via an FmRxEvSearchInProgress event.
   *    Similarly, each time a search completes, the client will be notified via an
   *    FmRxEvRadioTuneStatus event.
   *    <p>
   *    Once issuing a search command, several commands from the client
   *    may be disallowed until the search is completed or cancelled.
   *    <p>
   *    The search can be canceled at any time by using API
   *    cancelSearch (). Once cancelled, each search will tune to the
   *    last tuned station and generate both FmRxEvSearchComplete and
   *    FmRxEvRadioTuneStatus events.
   *    Valid Values for argument 'mode':
   *    <ul>
   *    <li>{@link #FM_RX_SRCH_MODE_SEEK}
   *    <li>{@link #FM_RX_SRCH_MODE_SCAN}
   *    </ul>
   *    <p>
   *    Valid Values for argument 'dwellPeriod' :
   *    <ul>
   *    <li>{@link #FM_RX_DWELL_PERIOD_1S}
   *    <li>{@link #FM_RX_DWELL_PERIOD_2S}
   *    <li>{@link #FM_RX_DWELL_PERIOD_3S}
   *    <li>{@link #FM_RX_DWELL_PERIOD_4S}
   *    <li>{@link #FM_RX_DWELL_PERIOD_5S}
   *    <li>{@link #FM_RX_DWELL_PERIOD_6S}
   *    <li>{@link #FM_RX_DWELL_PERIOD_7S}
   *    </ul>
   *    <p>
   *    Valid Values for argument 'direction' :
   *    <ul>
   *    <li>{@link #FM_RX_SEARCHDIR_DOWN}
   *    <li>{@link #FM_RX_SEARCHDIR_UP}
   *    </ul>
   *    <p>
   *
   *    <p>
   *    @param mode the FM search mode.
   *    @param dwellPeriod the FM scan dwell time. Used only when
   *    mode={@link #FM_RX_SRCH_MODE_SCAN}
   *    @param direction the Search Direction.
   *   <p>
   *    @return true if Search Initiate succeeded, false if
   *            Search Initiate  failed.
   *
   *   @see #searchStations(int, int, int, int, int)
   *   @see #searchStationList
   */
   public boolean searchStations (int mode,
                                  int dwellPeriod,
                                  int direction){

      int state = getFMState();
      boolean bStatus = true;
      int re;

      /* Check current state of FM device */
      if (state == FMState_Turned_Off || state == FMState_Srch_InProg) {
          Log.d(TAG, "searchStations: Device currently busy in executing another command.");
          return false;
      }

      Log.d (TAG, "Basic search...");

      /* Validate the arguments */
      if ( (mode != FM_RX_SRCH_MODE_SEEK) &&
           (mode != FM_RX_SRCH_MODE_SCAN))
      {
         Log.d (TAG, "Invalid search mode: " + mode );
         bStatus = false;
      }
      if ( (dwellPeriod < FM_RX_DWELL_PERIOD_0S ) ||
           (dwellPeriod > FM_RX_DWELL_PERIOD_7S))
      {
         Log.d (TAG, "Invalid dwelling time: " + dwellPeriod);
         bStatus = false;
      }
      if ( (direction != FM_RX_SEARCHDIR_DOWN) &&
           (direction != FM_RX_SEARCHDIR_UP))
      {
         Log.d (TAG, "Invalid search direction: " + direction);
         bStatus = false;
      }

      if (bStatus)
      {
         Log.d (TAG, "searchStations: mode " + mode + "direction:  " + direction);

         if (mode == FM_RX_SRCH_MODE_SEEK)
             setSearchState(subSrchLevel_SeekInPrg);
         else if (mode == FM_RX_SRCH_MODE_SCAN)
             setSearchState(subSrchLevel_ScanInProg);
         Log.v(TAG, "searchStations: CURRENT-STATE : FMRxOn ---> NEW-STATE : SearchInProg");

         re = mControl.searchStations(sFd, mode, dwellPeriod, direction, 0, 0);
         if (re != 0) {
             Log.e(TAG, "search station failed");
             if (getFMState() == FMState_Srch_InProg)
                 setSearchState(subSrchLevel_SrchComplete);
             return false;
         }
         state = getFMState();
         if (state == FMState_Turned_Off) {
             Log.d(TAG, "searchStations: CURRENT-STATE : FMState_Off (unexpected)");
             return false;
         }
      }
      return bStatus;
   }

   /*==============================================================
   FUNCTION:  searchStations
   ==============================================================*/
   /**
   *    Initiates RDS based seek and scan operations.
   *
   *    <p>
   *    This command allows the client to issue seeks and scans similar
   *    to commands found in basic searchStations(mode, scanTime,
   *    direction). However, each command has an additional RDS/RBDS
   *    component which must be satisfied before a station is
   *    successfully tuned. Please see searchStations(mode,
   *    scanTime, direction) for an understanding of how seeks and
   *    scans work.
   *
   *    <p>
   *    <ul>
   *    This API is used to search stations using RDS:
   *    <li> Invokes seek based on program type ({@link
   *    #FM_RX_SRCHRDS_MODE_SEEK_PTY})
   *    <li> Invokes scan based on program type with specified dwell period
   *    ({@link #FM_RX_SRCHRDS_MODE_SCAN_PTY})
   *    <li> Invokes seek based on program identification ({@link
   *    #FM_RX_SRCHRDS_MODE_SEEK_PI})
   *    <li> Invokes seek for alternate frequency ({@link
   *    #FM_RX_SRCHRDS_MODE_SEEK_AF})
   *    </ul>
   *
   *    <p>
   *    Much like {@link #FM_RX_SRCH_MODE_SEEK} in searchStations,
   *    {@link #FM_RX_SRCHRDS_MODE_SEEK_PTY} allows the client to
   *    seek to stations which are broadcasting RDS/RBDS groups
   *    with a particular Program Type that matches the supplied
   *    Program Type (PTY). The behavior and events generated for a
   *    {@link #FM_RX_SRCHRDS_MODE_SEEK_PTY} are very similar to
   *    that of {@link #FM_RX_SRCH_MODE_SEEK}, however only
   *    stations meeting the set search signal threshold and are
   *    also broadcasting the specified RDS Program Type (PTY) will
   *    be tuned. If no matching stations can be found, the
   *    original station will be re-tuned.
   *
   *    <p>
   *    Just as {@link #FM_RX_SRCHRDS_MODE_SEEK_PTY}'s
   *    functionality matches {@link #FM_RX_SRCH_MODE_SEEK}, so
   *    does {@link #FM_RX_SRCHRDS_MODE_SCAN_PTY} match {@link
   *    #FM_RX_SRCH_MODE_SCAN}. The one of the differences between
   *    the two is that only stations meeting the set search
   *    threshold and are also broadcasting a RDS Program Type
   *    (PTY) matching tucRdsSrchPty are found and tuned. If no
   *    station is found to have the PTY as specified by argument
   *    "pty", then the original station will be re-tuned.
   *
   *    <p> {@link #FM_RX_SRCHRDS_MODE_SEEK_PI} is used the same
   *    way as {@link #FM_RX_SRCHRDS_MODE_SEEK_PTY}, but only
   *    stations which meet the set search threshold and are also
   *    broadcasting the Program Identification matching the
   *    argument "pi" are tuned.
   *
   *    <p>
   *    Lastly, {@link #FM_RX_SRCHRDS_MODE_SEEK_AF} functionality
   *    differs slightly compared to the other commands in this
   *    function. This command only seeks to stations which are
   *    known ahead of time to be Alternative Frequencies for the
   *    currently tune station. If no alternate frequencies are
   *    known, or if the Alternative Frequencies have weaker signal
   *    strength than the original frequency, the original
   *    frequency will be re-tuned.
   *
   *    <p>
   *    Each time the driver initiates an RDS-based search, the client will be
   *    notified via a FmRxEvSearchInProgress event. Similarly, each
   *    time an RDS-based search completes, the client will be notified via a
   *    FmRxEvSearchComplete event.
   *
   *    <p>
   *    Once issuing a search command, several commands from the client may be
   *    disallowed until the search is completed or canceled.
   *
   *    <p>
   *    The search can be canceled at any time by using API
   *    cancelSearch (). Once canceled, each search will tune to the
   *    last tuned station and generate both
   *    FmRxEvSearchComplete and FmRxEvRadioTuneStatus events.
   *
   *    Valid Values for argument 'mode':
   *    <ul>
   *    <li>{@link #FM_RX_SRCHRDS_MODE_SEEK_PTY}
   *    <li>{@link #FM_RX_SRCHRDS_MODE_SCAN_PTY}
   *    <li>{@link #FM_RX_SRCHRDS_MODE_SEEK_PI}
   *    <li>{@link #FM_RX_SRCHRDS_MODE_SEEK_AF}
   *    </ul>
   *    <p>
   *    Valid Values for argument 'dwellPeriod' :
   *    <ul>
   *    <li>{@link #FM_RX_DWELL_PERIOD_1S}
   *    <li>{@link #FM_RX_DWELL_PERIOD_2S}
   *    <li>{@link #FM_RX_DWELL_PERIOD_3S}
   *    <li>{@link #FM_RX_DWELL_PERIOD_4S}
   *    <li>{@link #FM_RX_DWELL_PERIOD_5S}
   *    <li>{@link #FM_RX_DWELL_PERIOD_6S}
   *    <li>{@link #FM_RX_DWELL_PERIOD_7S}
   *    </ul>
   *    <p>
   *    Valid Values for argument 'direction' :
   *    <ul>
   *    <li>{@link #FM_RX_SEARCHDIR_DOWN}
   *    <li>{@link #FM_RX_SEARCHDIR_UP}
   *    </ul>
   *    <p>
   *    @param mode the FM search mode.
   *    @param dwellPeriod the FM scan dwell time. Used only when
   *    mode={@link #FM_RX_SRCHRDS_MODE_SCAN_PTY}
   *    @param direction the Search Direction.
   *    @param pty the FM RDS search Program Type
   *    @param pi the FM RDS search Program Identification Code
   *    <p>
   *    @return true if Search Initiate succeeded, false if
   *            Search Initiate  failed.
   *
   *   @see #searchStations(int, int, int)
   *   @see #searchStationList
   */
   public boolean searchStations (int mode,
                                  int dwellPeriod,
                                  int direction,
                                  int pty,
                                  int pi) {
      boolean bStatus = true;
      int state = getFMState();
      int re;

      /* Check current state of FM device */
      if (state == FMState_Turned_Off || state == FMState_Srch_InProg) {
          Log.d(TAG, "searchStations: Device currently busy in executing another command.");
          return false;
      }

      Log.d (TAG, "RDS search...");

      /* Validate the arguments */
      if ( (mode != FM_RX_SRCHRDS_MODE_SEEK_PTY)
           && (mode != FM_RX_SRCHRDS_MODE_SCAN_PTY)
           && (mode != FM_RX_SRCHRDS_MODE_SEEK_PI)
           && (mode != FM_RX_SRCHRDS_MODE_SEEK_AF)
         )
      {
         Log.d (TAG, "Invalid search mode: " + mode );
         bStatus = false;
      }
      if ( (dwellPeriod < FM_RX_DWELL_PERIOD_1S) ||
           (dwellPeriod > FM_RX_DWELL_PERIOD_7S))
      {
         Log.d (TAG, "Invalid dwelling time: " + dwellPeriod);
         bStatus = false;
      }
      if ( (direction != FM_RX_SEARCHDIR_DOWN) &&
           (direction != FM_RX_SEARCHDIR_UP))
      {
         Log.d (TAG, "Invalid search direction: " + direction);
         bStatus = false;
      }

      if (bStatus)
      {
         Log.d (TAG, "searchStations: mode " + mode);
         Log.d (TAG, "searchStations: dwellPeriod " + dwellPeriod);
         Log.d (TAG, "searchStations: direction " + direction);
         Log.d (TAG, "searchStations: pty " + pty);
         Log.d (TAG, "searchStations: pi " + pi);
         setSearchState(subSrchLevel_ScanInProg);
         re = mControl.searchStations(sFd, mode, dwellPeriod, direction, pty, pi);
         if (re != 0) {
             Log.e(TAG, "scan station failed");
             if (getFMState() == FMState_Srch_InProg)
                 setSearchState(subSrchLevel_SrchComplete);
             bStatus = false;
         }
      }
      return bStatus;
   }

   /*==============================================================
   FUNCTION:  searchStationList
   ==============================================================*/
   /** Initiates station list search operations.
   *    <p> This method will initate a search that will generate
   *    frequency lists based on strong and weak stations found in
   *    the FM band.
   *    <p>
   *    <ul>
   *    This API is used to generate station lists which consist of:
   *    <li>strong stations (FM_RX_SRCHLIST_MODE_STRONG,FM_RX_SRCHLIST_MODE_STRONGEST)
   *    <li>weak stations   (FM_RX_SRCHLIST_MODE_WEAK, FM_RX_SRCHLIST_MODE_WEAKEST)
   *    </ul>
   *    <p>
   *    The range of frequencies scanned depends on the currently set band.
   *    The driver searches for all valid stations in the band and when complete,
   *    returns a channel list based on the client's selection. The client can
   *    choose to search for a list of the strongest stations in the band, the
   *    weakest stations in the band, or the first N strong or weak
   *    stations. By setting the maximumStations argument, the
   *    client can constrain the number of frequencies returned in
   *    the list. If user specifies argument maximumStations to be
   *    0, the search will generate the maximum number of stations
   *    possible.
   *    <p>
   *    Each time the driver initiates a list-based search, the client will be
   *    notified via an FmRxEvSearchInProgress event. Similarly, each
   *    time a list-based search completes, the client will be
   *    notified via an FmRxEvSearchListComplete event.
   *    <p>
   *    On completion of the search, the originally tuned station
   *    will be tuned and the following events will be generated:
   *    FmRxEvSearchListComplete - The search has completed.
   *    FmRxEvRadioTuneStatus - The original frequency has been
   *    re-tuned.
   *    <p>
   *    Once issuing a search command, several commands from the client may be
   *    disallowed until the search is completed or cancelled.
   *    <p>
   *    The search can be canceled at any time by using API
   *    cancelSearch (). A cancelled search is treated as a completed
   *    search and the following events will be generated:
   *    FmRxEvSearchComplete  - The search has completed.
   *    FmRxEvRadioTuneStatus - The original frequency has been re-tuned.
   *    <p>
   *    Valid Values for argument 'mode':
   *    <ul>
   *    <li>{@link #FM_RX_SRCHLIST_MODE_STRONG}
   *    <li>{@link #FM_RX_SRCHLIST_MODE_WEAK}
   *    <li>{@link #FM_RX_SRCHLIST_MODE_STRONGEST}
   *    <li>{@link #FM_RX_SRCHLIST_MODE_WEAKEST}
   *    <li>FM_RX_SRCHLIST_MODE_PTY (Will be implemented in the
   *    future)
   *    </ul>
   *    <p>
   *    Valid Values for argument 'direction' :
   *    <ul>
   *    <li>{@link #FM_RX_SEARCHDIR_DOWN}
   *    <li>{@link #FM_RX_SEARCHDIR_UP}
   *    </ul>
   *    <p>
   *    Valid Values for argument 'maximumStations' : 1-12
   *    <p>
   *    @param mode the FM search mode.
   *    @param direction the Search Direction.
   *    @param maximumStations the maximum number of stations that
   *                           can be returned from a search. This parameter is
   *                           ignored and 12 stations are returned if the
   *                           search mode is either FM_RX_SRCHLIST_MODE_STRONGEST or
   *                           FM_RX_SRCHLIST_MODE_WEAKEST
   *
   *    @param pty the FM RDS search Program Type (Not used
   *               currently)
   *   <p>
   *    @return true if Search Initiate succeeded, false if
   *            Search Initiate  failed.
   *
   *   @see #searchStations(int, int, int)
   *   @see #searchStations(int, int, int, int, int)
   */
   public boolean searchStationList (int mode,
                                     int direction,
                                     int maximumStations,
                                     int pty){

      int state = getFMState();
      boolean bStatus = true;
      int re = 0;

      /* Check current state of FM device */
      if (state == FMState_Turned_Off || state == FMState_Srch_InProg) {
          Log.d(TAG, "searchStationList: Device currently busy in executing another command.");
          return false;
      }

      Log.d (TAG, "searchStations: mode " + mode);
      Log.d (TAG, "searchStations: direction " + direction);
      Log.d (TAG, "searchStations: maximumStations " + maximumStations);
      Log.d (TAG, "searchStations: pty " + pty);

      /* Validate the arguments */
      if ( (mode != FM_RX_SRCHLIST_MODE_STRONG)
           && (mode != FM_RX_SRCHLIST_MODE_WEAK )
           && (mode != FM_RX_SRCHLIST_MODE_STRONGEST )
           && (mode != FM_RX_SRCHLIST_MODE_WEAKEST )
         )
      {
         bStatus = false;
      }
      if ( (maximumStations < 0) ||
           (maximumStations > FM_RX_SRCHLIST_MAX_STATIONS))
      {
         bStatus = false;
      }
      if ( (direction != FM_RX_SEARCHDIR_DOWN) &&
           (direction != FM_RX_SEARCHDIR_UP))
      {
         bStatus = false;
      }

      if (bStatus)
      {
         setSearchState(subSrchLevel_SrchListInProg);
         Log.v(TAG, "searchStationList: CURRENT-STATE : FMRxOn ---> NEW-STATE : SearchInProg");
         if ((mode == FM_RX_SRCHLIST_MODE_STRONGEST) || (mode == FM_RX_SRCHLIST_MODE_WEAKEST)) {
             mode = (mode == FM_RX_SRCHLIST_MODE_STRONGEST)?
                               FM_RX_SRCHLIST_MODE_STRONG: FM_RX_SRCHLIST_MODE_WEAK;
            re = mControl.searchStationList(sFd, mode, 0, direction, pty);
         }
         else
            re = mControl.searchStationList(sFd, mode, maximumStations, direction, pty);

         if (re != 0) {
             Log.e(TAG, "search station list failed");
             if (getFMState() == FMState_Srch_InProg)
                 setSearchState(subSrchLevel_SrchComplete);
             bStatus =  false;
         }
      }

      return bStatus;
   }



   /*==============================================================
   FUNCTION:  cancelSearch
   ==============================================================*/
   /**
   *  Cancels an ongoing search operation
   *  (seek, scan, searchlist, etc).
   * <p>
   * This method should be used to cancel a previously initiated
   * search (e.g. Basic Seek/Scan, RDS Seek/Scans, Search list,
   * etc...).
   * <p>
   * Once completed, this command will generate an
   * FmRxEvSearchCancelledtr event to all registered clients.
   * Following this event, the client may also receive search events related
   * to the ongoing search now being complete.
   *
   *   <p>
   *    @return true if Cancel Search initiate succeeded, false if
   *            Cancel Search initiate failed.
   *   @see #searchStations(int, int, int)
   *   @see #searchStations(int, int, int)
   *   @see #searchStationList
   */
   public boolean cancelSearch () {
      boolean status = false;
      int state = getFMState();
      /* Check current state of FM device */
      if (state == FMState_Srch_InProg) {
         Log.v(TAG, "cancelSearch: Cancelling the on going search operation");
         setSearchState(subSrchLevel_SrchAbort);
         mControl.cancelSearch(sFd);
         return true;
      } else
         Log.d(TAG, "cancelSearch: No on going search operation to cancel");
      return status;
   }

   /*==============================================================
   FUNCTION:  setMuteMode
   ==============================================================*/
   /**
   *    Allows the muting and un-muting of the audio coming
   *    from the FM receiver.
   *    <p>
   *    This is a synchronous command used to mute or un-mute the
   *    FM audio. This command mutes the audio coming from the FM
   *    device. It is important to note that this only affects the
   *    FM audio and not any other audio system being used.
   *    <p>
   *    @param mode the mute Mode setting to apply
   *    <p>
   *    @return true if setMuteMode call was placed successfully,
   *           false if setMuteMode failed.
   *
   *    @see #enable
   *    @see #registerClient
   *
   */
   public boolean setMuteMode (int mode) {
      int state = getFMState();
      /* Check current state of FM device */
      if (state == FMState_Turned_Off || state == FMState_Srch_InProg) {
          Log.d(TAG, "setMuteMode: Device currently busy in executing another command.");
          return false;
      }
      switch (mode)
      {
      case FM_RX_UNMUTE:
         mControl.muteControl(sFd, false);
         break;
      case FM_RX_MUTE:
         mControl.muteControl(sFd, true);
         break;
      default:
         break;
      }

      return true;

   }

   /*==============================================================
   FUNCTION:  setStereoMode
   ==============================================================*/
   /**
   *    Sets the mono/stereo mode of the FM device.
   *
   *    <p>
   *    This command allows the user to set the mono/stereo mode
   *    of the FM device. Using this function,
   *    the user can allow mono/stereo mixing or force the reception
   *    of mono audio only.
   *
   *    @param stereoEnable true: Enable Stereo, false: Force Mono
   *
   *   @return true if setStereoMode call was placed successfully,
   *           false if setStereoMode failed.
   */
   public boolean setStereoMode (boolean stereoEnable) {
      int state = getFMState();
      /* Check current state of FM device */
      if (state == FMState_Turned_Off || state == FMState_Srch_InProg
                             || state == subPwrLevel_FMTurning_Off) {
          Log.d(TAG, "setStereoMode: Device currently busy in executing another command.");
          return false;
      }
      int re = mControl.stereoControl(sFd, stereoEnable);

      if (re == 0)
        return true;
      return false;
   }

   /*==============================================================
   FUNCTION:  setSignalThreshold
   ==============================================================*/
   /**
   *    This function sets the threshold which the FM driver
   *    uses to determine which stations have service available.
   *
   *    <p>
   *    This information is used to determine which stations are
   *    tuned during searches and Alternative Frequency jumps, as
   *    well as at what threshold FmRxEvServiceAvailable event
   *    callback are generated.
   *    <p>
   *    This is a command used to set the threshold used by the FM driver
   *    and/or hardware to determine which stations are "good" stations.
   *    Using this function, the client can allow very weak stations,
   *    relatively weak stations, relatively strong stations, or very.
   *    strong stations to be found during searches. Additionally,
   *    this threshold will be used to determine at what threshold a
   *    FmRxEvServiceAvailable event callback is generated.
   *    <p>
   *    @param threshold the new signal threshold.
   *    @return true if setSignalThreshold call was placed
   *           successfully, false if setSignalThreshold failed.
   */
   public boolean setSignalThreshold (int threshold) {

      int state = getFMState();
      /* Check current state of FM device */
      if (state == FMState_Turned_Off || state == FMState_Srch_InProg) {
          Log.d(TAG, "setSignalThreshold: Device currently busy in executing another command.");
          return false;
      }
      boolean bStatus = true;
      int re;
      Log.d(TAG, "Signal Threshhold input: "+threshold );
      int rssiLev = 0;

      switch(threshold)
      {
      case FM_RX_SIGNAL_STRENGTH_VERY_WEAK:
         rssiLev = FM_RX_RSSI_LEVEL_VERY_WEAK;
         break;
      case FM_RX_SIGNAL_STRENGTH_WEAK:
         rssiLev = FM_RX_RSSI_LEVEL_WEAK;
         break;
      case FM_RX_SIGNAL_STRENGTH_STRONG:
         rssiLev = FM_RX_RSSI_LEVEL_STRONG;
         break;
      case FM_RX_SIGNAL_STRENGTH_VERY_STRONG:
         rssiLev = FM_RX_RSSI_LEVEL_VERY_STRONG;
         break;
      default:
         /* Should never reach here */
         bStatus = false;
         Log.d (TAG, "Invalid threshold: " + threshold );
         return bStatus;
      }

      if (bStatus) {
        re=FmReceiverJNI.setControlNative (sFd, V4L2_CID_PRIVATE_TAVARUA_SIGNAL_TH, rssiLev);

        if (re !=0)
          bStatus = false;
      }

      return bStatus;
   }

   /*==============================================================
   FUNCTION:  getStationParameters
   ==============================================================*
   /**
   *     Returns various Paramaters related to the currently
   *    tuned station.
   *
   *    <p>
   *    This is method retreives various parameters and statistics
   *    related to the currently tuned station. Included in these
   *    statistics are the currently tuned frequency, the RDS/RBDS
   *    sync status, the RSSI level, current mute settings and the
   *    stereo/mono status.
   *
   *    <p>
   *    Once completed, this command will generate an asynchronous
   *    FmRxEvStationParameters event to the registered client.
   *    This event will contain the station parameters.
   *
   *    <p>
   *    @return      FmStationParameters: Object that contains
   *                    all the station parameters
   public FmStationParameters getStationParameters () {
      return mStationParameters;
   }

   */

   /*==============================================================
   FUNCTION:  getTunedFrequency
   ==============================================================*/
   /**
   *    Get the Frequency of the Tuned Station
   *
   *    @return frequencyKHz: Tuned Station Frequency (in kHz)
   *                          (Example: 96500 = 96.5Mhz)
   *            ERROR       : If device is currently executing another command
   */
   public int getTunedFrequency () {

      int state = getFMState();
      /* Check current state of FM device */
      if (state == FMState_Turned_Off || state == FMState_Srch_InProg) {
          Log.d(TAG, "getTunedFrequency: Device currently busy in executing another command.");
          return ERROR;
      }

      int frequency = FmReceiverJNI.getFreqNative(sFd);

      Log.d(TAG, "getFrequency: "+frequency);

      return frequency;
   }

   /*==============================================================
   FUNCTION:  getPSInfo
   ==============================================================*/
   /**
   *    Returns the current RDS/RBDS Program Service
   *            Information.
   *    <p>
   *    This is a command which returns the last complete RDS/RBDS
   *    Program Service information for the currently tuned station.
   *    To use this command, the client must first register for
   *    Program Service info by receiving either the
   *    FM_RX_RDS_GRP_PS_EBL or FM_RX_RDS_GRP_PS_SIMPLE_EBL event.
   *    Under normal operating mode, this information will
   *    automatically be sent to the client. However, if the client
   *    requires this information be sent again, this function can be
   *    used.
   *
   *    Typicaly this method needs to be called when "FmRxEvRdsPsInfo"
   *    callback is invoked.
   *
   *    <p>
   *    @return  the RDS data including the Program Service
   *             Information
   *
   */
   public FmRxRdsData  getPSInfo() {

      byte [] buff = new byte[STD_BUF_SIZE];
      int piLower = 0;
      int piHigher = 0;

      buff = FmReceiverJNI.getPsBuffer(buff);

      /* byte is signed ;(
      *  knock down signed bits
      */
      piLower = buff[3] & 0xFF;
      piHigher = buff[2] & 0xFF;
      int pi = ((piHigher << 8) | piLower);
      Log.d (TAG, "PI= " + pi);
      mRdsData.setPrgmId (pi);
      mRdsData.setPrgmType ( (int)( buff[1] & 0x1F));
      int numOfPs = (int)(buff[0] & 0x0F);
      Log.d (TAG, "numofpsI= " + numOfPs);
      try
      {

     String rdsStr = new String(buff, 5, numOfPs*8 );
         mRdsData.setPrgmServices (rdsStr);

      } catch (StringIndexOutOfBoundsException x)
      {
         Log.d (TAG, "Number of PS names " + numOfPs);
      }
      return mRdsData;
   }

   /*==============================================================
   FUNCTION:  getRTInfo
   ==============================================================*/
   /**
   *    Returns the current RDS/RBDS RadioText Information.
   *
   *    <p>
   *    This is a command which returns the last complete RadioText information
   *    for the currently tuned station. For this command to return meaningful
   *    information, the client must first register for RadioText events by registerring
   *    the FM_RX_RDS_GRP_RT_EBL callback function. Under normal operating mode, this information
   *    will automatically be sent to the client. However, if the client requires
   *    this information be sent again, this function can be used.
   *
   *    <p>
   *    Typicaly this method needs to be called when
   *    "FmRxEvRdsRtInfo" callback is invoked.
   *
   *    <p>
   *    @return  the RDS data including the Radio Text Information
   */
   public FmRxRdsData getRTInfo () {

      byte [] buff = new byte[STD_BUF_SIZE];
      int piLower = 0;
      int piHigher = 0;

      buff = FmReceiverJNI.getPsBuffer(buff);

      String rdsStr = new String(buff);
      /* byte is signed ;(
      *  knock down signed bit
      */
      piLower = buff[3] & 0xFF;
      piHigher = buff[2] & 0xFF;
      int pi = ((piHigher << 8) | piLower);
      mRdsData.setPrgmId (pi);
      mRdsData.setPrgmType ( (int)( buff[1] & 0x1F));
      try
      {
         rdsStr = rdsStr.substring(5, (int) buff[0]+ 5);
         mRdsData.setRadioText (rdsStr);

      } catch (StringIndexOutOfBoundsException x)
      {
         Log.d (TAG, "StringIndexOutOfBoundsException ...");
      }
      return mRdsData;
   }

   public FmRxRdsData getRTPlusInfo() {
      byte []rt_plus = new byte[STD_BUF_SIZE];
      int bytes_read;
      String rt = "";
      int rt_len;
      int i, j = 2;
      byte tag_code, tag_len, tag_start_pos;
      rt_plus = FmReceiverJNI.getPsBuffer(rt_plus);

      bytes_read = rt_plus[0];
      if (bytes_read > 0) {
          if (rt_plus[RT_OR_ERT_IND] == 0)
              rt = mRdsData.getRadioText();
          else
              rt = mRdsData.getERadioText();
          if ((rt != "") && (rt != null)) {
              rt_len = rt.length();
              mRdsData.setTagNums(0);
              for (i = 1; (i <= 2) && (j < rt_plus[LEN_IND]); i++) {
                  tag_code = rt_plus[j++];
                  tag_start_pos = rt_plus[j++];
                  tag_len = rt_plus[j++];
                  if (((tag_len + tag_start_pos) <= rt_len) && (tag_code > 0)) {
                      mRdsData.setTagValue(rt.substring(tag_start_pos,
                                            (tag_len + tag_start_pos)), i);
                      mRdsData.setTagCode(tag_code, i);
                  }
              }
          } else {
              mRdsData.setTagNums(0);
          }
      } else {
              mRdsData.setTagNums(0);
      }
      return mRdsData;
   }

   public FmRxRdsData getERTInfo() {
      byte [] raw_ert = new byte[STD_BUF_SIZE];
      byte [] ert_text;
      int i;
      String s = "";
      String encoding_type = "UCS-2";
      int bytes_read;

      raw_ert = FmReceiverJNI.getPsBuffer(raw_ert);

      bytes_read = raw_ert[0];
      if (bytes_read > 0) {
          ert_text = new byte[raw_ert[LEN_IND]];
          for(i = 3; (i - 3) < raw_ert[LEN_IND]; i++) {
              ert_text[i - 3] = raw_ert[i];
          }
          if (raw_ert[ENCODE_TYPE_IND] == 1)
              encoding_type = "UTF-8";
          try {
               s = new String (ert_text, encoding_type);
          } catch (Exception e) {
               e.printStackTrace();
          }
          mRdsData.setERadioText(s);
          if (raw_ert[ERT_DIR_IND] == 0)
              mRdsData.setFormatDir(false);
          else
              mRdsData.setFormatDir(true);
          Log.d(TAG, "eRT: " + s + "dir: " +raw_ert[ERT_DIR_IND]);
      }
      return mRdsData;
   }

   public FmRxRdsData getECCInfo() {
      byte [] raw_ecc = new byte[STD_BUF_SIZE];
      int ecc_code =0;
      int bytes_read;

      raw_ecc = FmReceiverJNI.getPsBuffer(raw_ecc);
      bytes_read = raw_ecc[0];
      Log.d (TAG, "bytes_read = " + bytes_read);
      if (bytes_read > 0) {
          ecc_code =  raw_ecc[9] & 0xFF;
          mRdsData.setECountryCode(ecc_code);
          Log.d(TAG, "ECC code: " + ecc_code );
      }
      return mRdsData;
   }
   /*==============================================================
   FUNCTION:  getAFInfo
   ==============================================================*/
   /**
   *   Returns the current RDS/RBDS Alternative Frequency
   *          Information.
   *
   *    <p>
   *    This is a command which returns the last known Alternative Frequency
   *    information for the currently tuned station. For this command to return
   *    meaningful information, the client must first register for Alternative
   *    Frequency events by registering an FM_RX_RDS_GRP_AF_EBL call back function.
   *    Under normal operating mode, this information will automatically be
   *    sent to the client. However, if the client requires this information
   *    be sent again, this function can be used.
   *
   *    <p>
   *    Typicaly this method needs to be called when
   *    "FmRxEvRdsAfInfo" callback is invoked.
   *
   *    @return  the RDS data including the AF Information
   */
   public int[] getAFInfo() {

      byte [] buff = new byte[STD_BUF_SIZE];
      int  [] AfList = new int [50];
      int lowerBand, i;
      int tunedFreq, PI, size_AFLIST;

      buff = FmReceiverJNI.getPsBuffer(buff);

      tunedFreq = (buff[0] & 0xFF) |
                  ((buff[1] & 0xFF) << 8) |
                  ((buff[2] & 0xFF) << 16) |
                  ((buff[3] & 0xFF) << 24) ;
      Log.d(TAG, "tunedFreq = " +tunedFreq);
       PI = (buff[4] & 0xFF) |
           ((buff[5] & 0xFF) << 8);
      Log.d(TAG, "PI: " + PI);
       size_AFLIST = buff[6] & 0xFF;
      Log.d(TAG, "size_AFLIST : " +size_AFLIST);

      for (i = 0;i < size_AFLIST;i++) {
            AfList[i] = (buff[6 + i * 4 + 1] & 0xFF) |
                       ((buff[6 + i * 4 + 2] & 0xFF) << 8) |
                       ((buff[6 + i * 4 + 3] & 0xFF) << 16) |
                       ((buff[6 + i * 4 + 4] & 0xFF) << 24) ;
            Log.d(TAG, "AF: " + AfList[i]);
      }

      return AfList;
   }

   /*==============================================================
   FUNCTION:  setPowerMode
   ==============================================================*/
   /**
   *    Puts the driver into or out of low power mode.
   *
   *    <p>
   *    This is an synchronous command which can put the FM
   *    device and driver into and out of low power mode. Low power mode
   *    should be used when the receiver is tuned to a station and only
   *    the FM audio is required. The typical scenario for low power mode
   *    is when the FM application is no longer visible.
   *
   *    <p>
   *    While in low power mode, all normal FM and RDS indications from
   *    the FM driver will be suppressed. By disabling these indications,
   *    low power mode can result in fewer interruptions and this may lead
   *    to a power savings.
   *
   *    <p>
   *    @param powerMode the new driver operating mode.
   *
   *    @return true if setPowerMode succeeded, false if
   *            setPowerMode failed.
   */
   public boolean setPowerMode(int powerMode){

      int re;

      if (powerMode == FM_RX_LOW_POWER_MODE) {
        re = mControl.setLowPwrMode (sFd, true);
      }
      else {
        re = mControl.setLowPwrMode (sFd, false);
      }

      if (re == 0)
         return true;
      return false;
   }

   /*==============================================================
  FUNCTION:  getPowerMode
  ==============================================================*/
   /**
   *    Get FM device low power mode.
   *    <p>
   *    This is an synchronous method that will read the power mode
   *    of the FM device and driver.
   *    <p>
   *       @return true if the FM Device is in Low power mode and
   *               false if the FM Device in Normal power mode.
   *
   *    @see #setPowerMode
   */
   public int getPowerMode(){

      return  mControl.getPwrMode (sFd);

   }

   /*==============================================================
   FUNCTION:  getRssiLimit
   ==============================================================*/
   /**
   *    Returns the RSSI thresholds for the FM driver.
   *
   *    <p>
   *    This method returns the RSSI thresholds for the FM driver.
   *    This function returns a structure containing the minimum RSSI needed
   *    for reception and the minimum RSSI value where reception is perfect.
   *    The minimum RSSI value for reception is the recommended threshold where
   *    an average user would consider the station listenable. Similarly,
   *    the minimum RSSI threshold for perfect reception is the point where
   *    reception quality will improve only marginally even if the RSSI level
   *    improves greatly.
   *
   *    <p>
   *    These settings should only be used as a guide for describing
   *    the RSSI values returned by the FM driver. Used in conjunction
   *    with getRssiInfo, the client can use the values from this
   *    function to give meaning to the RSSI levels returned by the driver.
   *
   *    <p>
   *       @return the RSSI level
   */
   public int[] getRssiLimit () {

      int[] rssiLimits = {0, 100};

      return rssiLimits;
   }

   /*==============================================================
   FUNCTION:  getSignalThreshold
   ==============================================================*/
   /**
   *   This function returns:
   *          currently set signal threshold - if API invocation
   *                                           is successful
   *          ERROR                          - if device is currently
   *                                           executing another command
   *    <p>
   *    This value used by the FM driver/hardware to determine which
   *    stations are tuned during searches and Alternative Frequency jumps.
   *    Additionally, this level is used to determine at what
   *    threshold FmRxEvServiceAvailable are generated.
   *
   *    <p>
   *    This is a command used to return the currently set signal
   *    threshold used by the FM driver and/or hardware. This
   *    value is used to determine. which stations are tuned
   *    during searches and Alternative Frequency jumps as well as
   *    when Service available events are generated.
   *
   *    <p>
   *    Once completed, this command will generate an asynchronous
   *    FmRxEvGetSignalThreshold event to the registered client.
   *    This event will contain the current signal threshold
   *    level.
   *
   *    <p>
   *    @return the signal threshold
   */
   public int getSignalThreshold () {
      int state = getFMState();
      /* Check current state of FM device */
      if (state == FMState_Turned_Off || state == FMState_Srch_InProg) {
          Log.d(TAG, "getSignalThreshold: Device currently busy in executing another command.");
          return ERROR;
      }
     int threshold = FM_RX_SIGNAL_STRENGTH_VERY_WEAK, signalStrength;
     int rmssiThreshold = FmReceiverJNI.getControlNative (sFd, V4L2_CID_PRIVATE_TAVARUA_SIGNAL_TH);
     Log.d(TAG, "Signal Threshhold: "+rmssiThreshold );

     if ( (FM_RX_RSSI_LEVEL_VERY_WEAK < rmssiThreshold) && (rmssiThreshold <= FM_RX_RSSI_LEVEL_WEAK) )
     {
       signalStrength = FM_RX_RSSI_LEVEL_WEAK;
     }
     else if ( (FM_RX_RSSI_LEVEL_WEAK < rmssiThreshold) && (rmssiThreshold  <= FM_RX_RSSI_LEVEL_STRONG))
     {
       signalStrength = FM_RX_RSSI_LEVEL_STRONG;
     }
     else if ((FM_RX_RSSI_LEVEL_STRONG < rmssiThreshold))
     {
       signalStrength = FM_RX_RSSI_LEVEL_VERY_STRONG;
     }
     else
     {
       signalStrength = FM_RX_RSSI_LEVEL_VERY_WEAK;
     }

     switch(signalStrength)
     {
     case FM_RX_RSSI_LEVEL_VERY_WEAK:
        threshold = FM_RX_SIGNAL_STRENGTH_VERY_WEAK;
        break;
     case FM_RX_RSSI_LEVEL_WEAK:
        threshold = FM_RX_SIGNAL_STRENGTH_WEAK;
        break;
     case FM_RX_RSSI_LEVEL_STRONG:
        threshold = FM_RX_SIGNAL_STRENGTH_STRONG;
        break;
     case FM_RX_RSSI_LEVEL_VERY_STRONG:
        threshold = FM_RX_SIGNAL_STRENGTH_VERY_STRONG;
        break;
     default:
        /* Should never reach here */
        break;
     }

     return threshold;
   }

   public int getAFJumpRmssiTh() {
      int state = getFMState();
      /* Check current state of FM device */
      if ((state == FMState_Turned_Off) || (state == FMState_Srch_InProg)) {
          Log.d(TAG, "getAFJumpThreshold: Device currently busy in executing another command.");
          return ERROR;
      }
      return mControl.getAFJumpRmssiTh(sFd);
   }

   public boolean setAFJumpRmssiTh(int th) {
      int state = getFMState();
      /* Check current state of FM device */
      if ((state == FMState_Turned_Off) || (state == FMState_Srch_InProg)) {
          Log.d(TAG, "setAFJumpThreshold: Device currently busy in executing another command.");
          return false;
      }
      return mControl.setAFJumpRmssiTh(sFd, th);
   }

   public int getAFJumpRmssiSamples() {
      int state = getFMState();
      /* Check current state of FM device */
      if ((state == FMState_Turned_Off) || (state == FMState_Srch_InProg)) {
          Log.d(TAG, "getAFJumpRmssiSamples: Device currently busy in executing another command.");
          return ERROR;
      }
      return mControl.getAFJumpRmssiSamples(sFd);
   }

   public boolean setAFJumpRmssiSamples(int samples) {
      int state = getFMState();
      /* Check current state of FM device */
      if ((state == FMState_Turned_Off) || (state == FMState_Srch_InProg)) {
          Log.d(TAG, "setAFJumpRmssiSamples: Device currently busy in executing another command.");
          return false;
      }
      return mControl.setAFJumpRmssiSamples(sFd, samples);
   }

   public int getGdChRmssiTh() {
      int state = getFMState();
      /* Check current state of FM device */
      if ((state == FMState_Turned_Off) || (state == FMState_Srch_InProg)) {
          Log.d(TAG, "getGdChRmssiTh: Device currently busy in executing another command.");
          return ERROR;
      }
      return mControl.getGdChRmssiTh(sFd);
   }

   public boolean setGdChRmssiTh(int th) {
      int state = getFMState();
      /* Check current state of FM device */
      if ((state == FMState_Turned_Off) || (state == FMState_Srch_InProg)) {
          Log.d(TAG, "setGdChRmssiTh: Device currently busy in executing another command.");
          return false;
      }
      return mControl.setGdChRmssiTh(sFd, th);
   }

   public int getSearchAlgoType() {
      int state = getFMState();
      if ((state == FMState_Turned_Off) || (state == FMState_Srch_InProg)) {
          Log.d(TAG, "getSearchAlgoType: Device currently busy in executing another command.");
          return Integer.MAX_VALUE;
      }
      return mControl.getSearchAlgoType(sFd);
   }

   public boolean setSearchAlgoType(int searchType) {
      int state = getFMState();
      if ((state == FMState_Turned_Off) || (state == FMState_Srch_InProg)) {
          Log.d(TAG, "setSearchAlgoType: Device currently busy in executing another command.");
          return false;
      }
      if((searchType != SEARCH_MPXDCC) && (searchType != SEARCH_SINR_INT)) {
          Log.d(TAG, "Search Algo is invalid");
          return false;
      }else {
          return mControl.setSearchAlgoType(sFd, searchType);
      }
   }

   public int getSinrFirstStage() {
      int state = getFMState();
      if ((state == FMState_Turned_Off) || (state == FMState_Srch_InProg)) {
          Log.d(TAG, "getSinrFirstStage: Device currently busy in executing another command.");
          return Integer.MAX_VALUE;
      }
      return mControl.getSinrFirstStage(sFd);
   }

   public boolean setSinrFirstStage(int sinr) {
      int state = getFMState();
      if ((state == FMState_Turned_Off) || (state == FMState_Srch_InProg)) {
          Log.d(TAG, "setSinrFirstStage: Device currently busy in executing another command.");
          return false;
      }
      return mControl.setSinrFirstStage(sFd, sinr);
   }

   public int getRmssiFirstStage() {
      int state = getFMState();
      if ((state == FMState_Turned_Off) || (state == FMState_Srch_InProg)) {
          Log.d(TAG, "getRmssiFirstStage: Device currently busy in executing another command.");
          return Integer.MAX_VALUE;
      }
      return mControl.getRmssiFirstStage(sFd);
   }

   public boolean setRmssiFirstStage(int rmssi) {
      int state = getFMState();
      if ((state == FMState_Turned_Off) || (state == FMState_Srch_InProg)) {
          Log.d(TAG, "setRmssiFirstStage: Device currently busy in executing another command.");
          return false;
      }
      return mControl.setRmssiFirstStage(sFd, rmssi);
   }

   public int getCFOMeanTh() {
      int state = getFMState();
      if ((state == FMState_Turned_Off) || (state == FMState_Srch_InProg)) {
          Log.d(TAG, "getCF0Th12: Device currently busy in executing another command.");
          return Integer.MAX_VALUE;
      }
      return mControl.getCFOMeanTh(sFd);
   }

   public boolean setCFOMeanTh(int th) {
      int state = getFMState();
      if ((state == FMState_Turned_Off) || (state == FMState_Srch_InProg)) {
          Log.d(TAG, "setRmssiFirstStage: Device currently busy in executing another command.");
          return false;
      }
      return mControl.setCFOMeanTh(sFd, th);
   }

   public boolean setPSRxRepeatCount(int count) {
      int state = getFMState();
      /* Check current state of FM device */
      if (state == FMState_Turned_Off){
          Log.d(TAG, "setRxRepeatcount failed");
          return false;
      }
      return mControl.setPSRxRepeatCount(sFd, count);
   }

   public boolean getPSRxRepeatCount() {
      int state = getFMState();
      /* Check current state of FM device */
      if (state == FMState_Turned_Off){
          Log.d(TAG, "setRxRepeatcount failed");
          return false;
      }
      return mControl.getPSRxRepeatCount(sFd);
   }

   public byte getBlendSinr() {
      int state = getFMState();
      if ((state == FMState_Turned_Off) || (state == FMState_Srch_InProg)) {
          Log.d(TAG, "getBlendSinr: Device currently busy in executing another command.");
          return Byte.MAX_VALUE;
      }
      return mControl.getBlendSinr(sFd);
   }

   public boolean setBlendSinr(int sinrHi) {
      int state = getFMState();
      if ((state == FMState_Turned_Off) || (state == FMState_Srch_InProg)) {
          Log.d(TAG, "setBlendSinr: Device currently busy in executing another command.");
          return false;
      }
      return mControl.setBlendSinr(sFd, sinrHi);
   }

   public byte getBlendRmssi() {
      int state = getFMState();
      if ((state == FMState_Turned_Off) || (state == FMState_Srch_InProg)) {
          Log.d(TAG, "getBlendRmssi: Device currently busy in executing another command.");
          return Byte.MAX_VALUE;
      }
      return mControl.getBlendRmssi(sFd);
   }

   public boolean setBlendRmssi(int rmssiHi) {
      int state = getFMState();
      if ((state == FMState_Turned_Off) || (state == FMState_Srch_InProg)) {
          Log.d(TAG, "setBlendRmssi: Device currently busy in executing another command.");
          return false;
      }
      return mControl.setBlendRmssi(sFd, rmssiHi);
   }

   /*==============================================================
   FUNCTION:  setRdsGroupOptions
   ==============================================================*/
   /**
   *
   *    This function enables or disables various RDS/RBDS
   *    group filtering and buffering features.
   *
   *    <p>
   *    Included in these features are the RDS group enable mask, RDS/RBDS group
   *    change filter, and the RDS/RBDS group buffer size.
   *    <p>
   *    This is a function used to set or unset various Rx RDS/RBDS group filtering
   *    and buffering options in the FM driver.
   *    <p>
   *    Included in these options is the ability for the client to select
   *    which RDS/RBDS groups should be sent to the client. By default, all
   *    RDS/RBDS groups are filtered out before reaching the client. To allow one
   *    or more specific groups to be received, the client must set one or mors bits
   *    within the argument enRdsGrpsMask bitmask. Each bit in this
   *    mask corresponds to a specific RDS/RBDS group type. Once a
   *    group is enabled, and when a buffer holding those groups
   *    reaches the threshold defined by argument rdsBuffSize, the
   *    group or groups will be sent to the client as a
   *    FmRxEvRdsGroupData callback.
   *
   *    <p>
   *    Additionally, this function also allows the client to enable or
   *    disable the RDS/RBDS group change filter. This filter allows the client
   *    to prevent duplicate groups of the same group type from being received.
   *    This filter only applies to consecutive groups, so
   *    identical groups received in different order will not be
   *    filtered out.
   *
   *    <p>
   *    @param enRdsGrpsMask the bitMask that enables the RT/PS/AF.
   *
   *    @param rdsBuffSize the number of RDS/RBDS groups the FM
   *                        driver should buffer  before sending to
   *                        the client.
   *
   *    @param enRdsChangeFilter the Flag used to determine whether
   *                              the RDS/RBDS change filter
   *                              should be enabled.
   *
   *    @return true if the command was placed successfully, false
   *            if command failed.
   *
   */
   public boolean setRdsGroupOptions (int enRdsGrpsMask,
                                      int rdsBuffSize,
                                      boolean enRdsChangeFilter)
   {

      int state = getFMState();
      /* Check current state of FM device */
      if (state == FMState_Turned_Off || state == FMState_Srch_InProg) {
          Log.d(TAG, "setRdsGroupOptions: Device currently busy in executing another command.");
          return false;
      }
      // Enable RDS
      int re = mRdsData.rdsOn(true);

      if (re != 0)
        return false;

      re = mRdsData.rdsGrpOptions (enRdsGrpsMask, rdsBuffSize, enRdsChangeFilter);

      if (re ==0)
        return true;

      return false;

   }

   public boolean setRawRdsGrpMask()
   {
      return super.setRDSGrpMask(GRP_3A);
   }
   /*==============================================================
   FUNCTION:  registerRdsGroupProcessing
   ==============================================================*/
   /**
   *
   *    This function enables or disables RDS/RBDS group processing features.
   *
   *    <p>
   *    Included in these features is the ability for the FM driver
   *    to return Program Service, RadioText, and Alternative
   *    Frequency information.
   *
   *    <p>
   *    These options free the client from the burden of collecting a continuous
   *    stream of RDS/RBDS groups and processing them. By setting the
   *    FM_RX_RDS_GRP_RT_EBL bit in argument fmGrpsToProc, the FM
   *    hardware or driver will collect RDS/RBDS 2A/2B groups and
   *    return complete RadioText strings and information in the
   *    form of a FmRxEvRdsRtInfo event. This event will be
   *    generated only when the RadioText information changes.
   *
   *    <p>
   *    Similarly, by setting either the FM_RX_RDS_GRP_PS_EBL or
   *    FM_RX_RDS_GRP_PS_SIMPLE_EBL bit in argument fmGrpsToProc,
   *    the FM hardware or driver will collect RDS/RBDS 0A/0B
   *    groups and return Program Service information in the form
   *    of a FmRxEvRdsPsInfo event. This event will be generated
   *    whenever the Program Service information changes. This
   *    event will include one or more collected Program Service
   *    strings which can be continuously displayed by the client.
   *
   *    <p>
   *    Additionally, by setting the FM_RX_RDS_GRP_AF_EBL bit in
   *    argument FmGrpsToProc, the FM hardware or driver will
   *    collect RDS/RBDS 0A/0B groups and return Alternative
   *    Frequency information in the form of a FmRxEvRdsAfInfo
   *    event. This event will be generated when the Alternative
   *    Frequency information changes and will include an up to
   *    date list of all known Alternative Frequencies.
   *
   *    <p>
   *    Lastly, by setting the FM_RX_RDS_GRP_AF_JUMP_EBL bit in
   *    argument FmGrpsToProc, the FM hardware or driver will
   *    collect RDS/RBDS 0A/0B groups and automatically tune to a
   *    stronger alternative frequency when the signal level falls
   *    below the search threshold.
   *
   *    @param fmGrpsToProc the bitMask that enables the RT/PS/AF.
   *
   *    @return true if the command was placed successfully, false
   *            if command failed.
   *
   */
   public boolean registerRdsGroupProcessing (int fmGrpsToProc){

      if (mRdsData == null)
         return false;

      int state = getFMState();
      /* Check current state of FM device */
      if (state == FMState_Turned_Off || state == FMState_Srch_InProg) {
          Log.d(TAG, "registerRdsGroupProcessing: Device currently busy in executing another command.");
          return false;
      }

      // Enable RDS
      int re = mRdsData.rdsOn(true);

      if (re != 0)
        return false;

      re = mRdsData.rdsOptions (fmGrpsToProc);

      if (re ==0)
        return true;

      return false;
   }


   /*==============================================================
   FUNCTION:  enableAFjump
   ==============================================================*/
   /**
   *    Enables automatic jump to alternative frequency
   *
   *    <p>
   *    This method enables automatic seeking to stations which are
   *    known ahead of time to be Alternative Frequencies for the
   *    currently tuned station. If no alternate frequencies are
   *    known, or if the Alternative Frequencies have weaker signal
   *    strength than the original frequency, the original frequency
   *    will be re-tuned.
   *
   *    <p>
   *    @return     true if successful false otherwise.
   */
   public boolean enableAFjump (boolean enable) {

      int state = getFMState();
      /* Check current state of FM device */
      if (state == FMState_Turned_Off || state == FMState_Srch_InProg) {
          Log.d(TAG, "enableAFjump: Device currently busy in executing another command.");
          return false;
      }
      // Enable RDS
      int re = mRdsData.rdsOn(true);

      if (re != 0)
        return false;

      re = mRdsData.enableAFjump(enable);

      if (re == 0)
        return true;

      return false;
   }

   /*==============================================================
   FUNCTION:  getRssi
   ==============================================================*/
   /**
   *    Returns the signal strength of the currently tuned station
   *
   *    <p>
   *    This method returns the signal strength of the currently
   *    tuned station.
   *
   *    <p>
   *    @return    RSSI of currently tuned station
   */
   public int getRssi()
   {

       int rssi = FmReceiverJNI.getRSSINative (sFd);

       return rssi;
   }

   /*==============================================================
   FUNCTION:  getIoverc
   ==============================================================*/
   /**
   *    Returns the Estimated Interference Over Carrier of the currently tuned station
   *
   *    <p>
   *    This method returns the Estimated Interference Over Carrier of the currently
   *    tuned station.
   *
   *    <p>
   *    @return    IOVERC of currently tuned station on Success.
   *           -1 on failure to retrieve the current IoverC.
   */
   public int getIoverc()
   {
      int re;
      re = mControl.IovercControl(sFd);
      return re;
   }

   /*==============================================================
   FUNCTION:  getIntDet
   ==============================================================*/
   /**
   *    Returns the IntDet the currently tuned station
   *
   *    <p>
   *    This method returns the IntDet of the currently
   *    tuned station.
   *
   *    <p>
   *    @return    IntDet of currently tuned station.
   *           -1 on failure to retrieve the current IntDet
   */
   public int getIntDet()
   {
      int re;

      re = mControl.IntDet(sFd);
      return re;
   }


   /*==============================================================
   FUNCTION:  getMpxDcc
   ==============================================================*/
   /**
   *    Returns the MPX_DCC of the currently tuned station
   *
   *    <p>
   *    This method returns the MPX_DCC of the currently
   *    tuned station.
   *
   *    <p>
   *    @return    MPX_DCC value of currently tuned station.
   *               -1 on failure to retrieve the current MPX_DCC
   */
   public int getMpxDcc()
   {
      int re;

      re = mControl.Mpx_Dcc(sFd);
      return re;
   }
/*==============================================================
   FUNCTION:  setHiLoInj
   ==============================================================*/
   /**
   *    Sets the Hi-Lo injection
   *
   *    <p>
   *    This method sets the hi-low injection.
   *
   *    <p>
   */
   public void setHiLoInj(int inj)
   {
      int re =  mControl.setHiLoInj(sFd, inj);
   }

/*==============================================================
   FUNCTION:  getRmssiDelta
   ==============================================================*/
   /**
   *    Gets the value of currently set RMSSI Delta
   *
   *    <p>
   *    This method gets the currently set RMSSI Delta value.
   *
   *    <p>
   */
   public int getRmssiDelta()
   {
      int re =  mControl.getRmssiDelta(sFd);
      Log.d (TAG, "The value of RMSSI Delta is " + re);
      return re;
   }

/*==============================================================
   FUNCTION:  setRmssiDel
   ==============================================================*/
   /**
   *    Sets the RMSSI Delta
   *
   *    <p>
   *    This method sets the RMSSI Delta.
   *
   *    <p>
   */
   public void setRmssiDel(int delta)
   {
      int re =  mControl.setRmssiDel(sFd, delta);
   }

   /*==============================================================
   FUNCTION:  getRawRDS
   ==============================================================*/
   /**
   *    Returns array of Raw RDS data
   *
   *    <p>
   *    This is a non-blocking call and it returns raw RDS data.
   *    The data is packed in groups of three bytes. The lsb and
   *    msb bytes contain RDS block while the third byte contains
   *    Block description. This call is wrapper around V4L2 read
   *    system call. The FM driver collects RDS/RBDS groups to meet
   *    the threshold described by setRdsGroupOptions() method.
   *    The call returns when one or more groups have been enabled
   *    by setRdsGroupOptions() method.
   *
   *    @param numBlocks Number of blocks of RDS data
   *
   *    <p>
   *    @return    byte[]
   */

   public byte[] getRawRDS (int numBlocks)
   {

        byte[] rawRds = new byte [numBlocks*3];
        int re;

        re = FmReceiverJNI.getRawRdsNative (sFd, rawRds, numBlocks*3);

        if (re == (numBlocks*3))
            return rawRds;

        if (re <= 0)
          return null;

        byte[] buff = new byte [re];

        System.arraycopy (rawRds, 0, buff, 0 , re);

        return buff;

   }

   /*
    * getFMState() returns:
    *     '0' if FM State  is OFF
    *     '1' if FM Rx     is On
    *     '2' if FM Tx     is On
    *     '3' if FM device is Searching
   */
   public int getFMState()
   {
      /* Current State of FM device */
      int currFMState = FmTransceiver.getFMPowerState();
      return currFMState;

   }
/*==============================================================
   FUNCTION:  setOnChannelThreshold
   ==============================================================*/
   /**
   *    Sets the On channel threshold value
   *
   *    <p>
   *    This method sets the On channel threshold value.
   *
   *    <p>
   */
   public boolean setOnChannelThreshold(int data)
   {
      int re =  mControl.setOnChannelThreshold(sFd, data);
      if (re < 0)
          return false;
      else
          return true;
   }

/*==============================================================
   FUNCTION:  getOnChannelThreshold
   ==============================================================*/
   /**
   *    Gets the On channel threshold value
   *
   *    <p>
   *    This method gets the currently set On channel threshold value.
   *
   *    <p>
   */
   public boolean getOnChannelThreshold()
   {
       int re = mControl.getOnChannelThreshold(sFd);

       if (re != 0)
           return false;
       else
           return true;
   }

/*==============================================================
   FUNCTION:  setOffChannelThreshold
   ==============================================================*/
   /**
   *    Sets the Off channel threshold value
   *
   *    <p>
   *    This method sets the Off channel threshold value.
   *
   *    <p>
   */
   public boolean setOffChannelThreshold(int data)
   {
      int re =  mControl.setOffChannelThreshold(sFd, data);
      if (re < 0)
          return false;
      else
          return true;
   }
/*==============================================================
   FUNCTION:  getOffChannelThreshold
   ==============================================================*/
   /**
   *    Gets the Off channel threshold value
   *
   *    <p>
   *    This method gets the currently set Off channel threshold value.
   *
   *    <p>
   */
   public boolean  getOffChannelThreshold()
   {
       int re = mControl.getOffChannelThreshold(sFd);

       if (re != 0)
           return false;
       else
           return true;
   }
/*===============================================================
   FUNCTION:  getSINR
   ==============================================================*/
   /**
   *    Gets the SINR value of currently tuned station
   *
   *    <p>
   *    This method gets the SINR value for  currently tuned station.
   *
   *    <p>
   */
   public int getSINR()
   {
      int re =  mControl.getSINR(sFd);
      Log.d (TAG, "The value of SINR is " + re);
      return re;
   }

/*==============================================================
   FUNCTION:  setSINRThreshold
   ==============================================================*/
   /**
   *    Sets the SINR threshold value
   *
   *    <p>
   *    This method sets the SINR threshold value.
   *
   *    <p>
   */
   public boolean setSINRThreshold(int data)
   {
      int re =  mControl.setSINRThreshold(sFd, data);
      if (re < 0)
          return false;
      else
          return true;
   }

/*==============================================================
   FUNCTION:  getSINRThreshold
   ==============================================================*/
   /**
   *    Gets the SINR threshold value
   *
   *    <p>
   *    This method gets the currently set SINR threshold value.
   *
   *    <p>
   */
   public int getSINRThreshold()
   {
      return mControl.getSINRThreshold(sFd);
   }

/*==============================================================
   FUNCTION:  setRssiThreshold
   ==============================================================*/
   /**
   *    Sets the RSSI threshold value
   *
   *    <p>
   *    This method sets the RSSI threshold value.
   *
   *    <p>
   */
   public boolean setRssiThreshold(int data)
   {
      int re =  mControl.setRssiThreshold(sFd, data);
      if (re < 0)
          return false;
      else
          return true;
   }

/*==============================================================
   FUNCTION:  getRssiThreshold
   ==============================================================*/
   /**
   *    Gets the Rssi threshold value
   *
   *    <p>
   *    This method gets the currently set Rssi threshold value.
   *
   *    <p>
   */
   public int getRssiThreshold()
   {
      return mControl.getRssiThreshold(sFd);
   }

/*==============================================================
   FUNCTION:  setAfJumpRssiThreshold
   ==============================================================*/
   /**
   *    Sets the Af jump RSSI threshold value
   *
   *    <p>
   *    This method sets the AF jump RSSI threshold value.
   *
   *    <p>
   */
   public boolean setAfJumpRssiThreshold(int data)
   {
      int re =  mControl.setAfJumpRssiThreshold(sFd, data);
      if (re < 0)
          return false;
      else
          return true;
   }

/*==============================================================
   FUNCTION:  getAfJumpRssiThreshold
   ==============================================================*/
   /**
   *    Gets the Af jump RSSI threshold value
   *
   *    <p>
   *    This method gets the currently set AF jump RSSI threshold value.
   *
   *    <p>
   */
   public int getAfJumpRssiThreshold()
   {
      return mControl.getAfJumpRssiThreshold(sFd);
   }

/*==============================================================
   FUNCTION: setRdsFifoCnt
   ==============================================================*/
   /**
   *    Sets the RDS FIFO count value
   *
   *    <p>
   *    This method sets the RDS FIFO count value.
   *
   *    <p>
   */
   public boolean setRdsFifoCnt(int data)
   {
      int re =  mControl.setRdsFifoCnt(sFd, data);
      if (re < 0)
          return false;
      else
          return true;
   }

/*==============================================================
   FUNCTION:  getRdsFifoCnt
   ==============================================================*/
   /**
   *    Gets the RDS FIFO count value
   *
   *    <p>
   *    This method gets the currently set RDS FIFO count value.
   *
   *    <p>
   */
   public int getRdsFifoCnt()
   {
      return mControl.getRdsFifoCnt(sFd);
   }

/*==============================================================
   FUNCTION:  setSINRsamples
   ==============================================================*/
   /**
   *    Sets the SINR samples
   *
   *    <p>
   *    This method sets the number of SINR samples to calculate the SINR value.
   *
   *    <p>
   */
   public boolean setSINRsamples(int data)
   {
      int re =  mControl.setSINRsamples(sFd, data);
      if (re < 0)
          return false;
      else
          return true;
   }

/*==============================================================
   FUNCTION:  getSINRsamples
   ==============================================================*/
   /**
   *    Gets the SINR samples value
   *
   *    <p>
   *    This method gets the number of currently set SINR samples.
   *
   *    <p>
   */
   public int getSINRsamples()
   {
      return mControl.getSINRsamples(sFd);
   }

   public int updateSpurFreq(int freq, int rmssi, boolean enable)
   {
       return mControl.updateSpurTable(sFd, freq, rmssi, enable);
   }

   public int configureSpurTable()
   {
       return mControl.configureSpurTable(sFd);
   }

   public static int getSpurConfiguration(int freq)
   {
       int retval;

       retval = FmReceiverJNI.setControlNative(sFd, V4L2_CID_PRIVATE_IRIS_GET_SPUR_TBL, freq);

        if (retval !=0)
            Log.d(TAG, "Failed/No Spurs for " +freq);
       return retval;
   }

   public void FMcontrolLowPassFilter(int state, int net_type, int enable) {
       int RatConf = getFmWanWlanCoexProp(WAN_RATCONF);
       Log.v (TAG, "FMcontrolLowPassFilter " + RatConf);
       switch (net_type)
       {

           case TelephonyManager.NETWORK_TYPE_GPRS:
               if ((mEnableLpfGprs  & RatConf) == mEnableLpfGprs) {
                   Log.v (TAG, "set LPF for net_type: " + Integer.toString(net_type));
                   mControl.enableLPF(sFd, enable);
               }
               break;
           case TelephonyManager.NETWORK_TYPE_EDGE:
               if ((mEnableLpfEdge  & RatConf) == mEnableLpfEdge) {
                   Log.v (TAG, "set LPF for net_type: " + Integer.toString(net_type));
                   mControl.enableLPF(sFd, enable);
               }
               break;
           case TelephonyManager.NETWORK_TYPE_UMTS:
               if ((mEnableLpfUmts  & RatConf) == mEnableLpfUmts ) {
                   Log.v (TAG, "set LPF for net_type: " + Integer.toString(net_type));
                   Log.v (TAG,  "enable:" + enable);
                   mControl.enableLPF(sFd, enable);
               }
               break;
           case TelephonyManager.NETWORK_TYPE_CDMA:
               if ((mEnableLpfCdma & RatConf) == mEnableLpfCdma) {
                   Log.d (TAG, "enabling LPF for net_type: " + Integer.toString(net_type));
                   mControl.enableLPF(sFd, enable);
               }
               break;
           case TelephonyManager.NETWORK_TYPE_EVDO_0:
               if ((mEnableLpfEvdo0  & RatConf) == mEnableLpfEvdo0 ) {
                   Log.d (TAG, "enabling LPF for net_type: " + Integer.toString(net_type));
                   mControl.enableLPF(sFd, enable);
               }
               break;
           case TelephonyManager.NETWORK_TYPE_EVDO_A:
               if ((mEnableLpfEvdoA  & RatConf) == mEnableLpfEvdoA ) {
                   Log.d (TAG, "enabling LPF for net_type: " + Integer.toString(net_type));
                   mControl.enableLPF(sFd, enable);
               }
               break;
           case TelephonyManager.NETWORK_TYPE_1xRTT:
               if ((mEnableLpf1xRtt  & RatConf) == mEnableLpf1xRtt ) {
                   Log.d (TAG, "enabling LPF for net_type: " + Integer.toString(net_type));
                   mControl.enableLPF(sFd, enable);
               }
               break;
           case TelephonyManager.NETWORK_TYPE_HSDPA:
               if ((mEnableLpfHsdpa  & RatConf) == mEnableLpfHsdpa ) {
                   Log.d (TAG, "enabling LPF for net_type: " + Integer.toString(net_type));
                   mControl.enableLPF(sFd, enable);
               }
               break;
           case TelephonyManager.NETWORK_TYPE_HSUPA:
               if ((mEnableLpfHsupa & RatConf) == mEnableLpfHsupa) {
                   Log.d (TAG, "enabling LPF for net_type: " + Integer.toString(net_type));
                   mControl.enableLPF(sFd, enable);
               }
               break;
           case TelephonyManager.NETWORK_TYPE_HSPA:
               if ((mEnableLpfHspa  & RatConf) == mEnableLpfHspa ) {
                   Log.d (TAG, "enabling LPF for net_type: " + Integer.toString(net_type));
                   mControl.enableLPF(sFd, enable);
               }
               break;
           case TelephonyManager.NETWORK_TYPE_IDEN:
               if ((mEnableLpfIden  & RatConf) == mEnableLpfIden ) {
                   Log.d (TAG, "enabling LPF for net_type: " + Integer.toString(net_type));
                   mControl.enableLPF(sFd, enable);
               }
               break;
           case TelephonyManager.NETWORK_TYPE_EVDO_B:
               if ((mEnableLpfEvdoB  & RatConf) == mEnableLpfEvdoB ) {
                   Log.d (TAG, "enabling LPF for net_type: " + Integer.toString(net_type));
                   mControl.enableLPF(sFd, enable);
               }
               break;
           case TelephonyManager.NETWORK_TYPE_LTE:
               if ((mEnableLpfLte  & RatConf) == mEnableLpfLte ) {
                   Log.d (TAG, "enabling LPF for net_type: " + Integer.toString(net_type));
                   mControl.enableLPF(sFd, enable);
               }
               break;
           case TelephonyManager.NETWORK_TYPE_EHRPD:
               if ((mEnableLpfEhrpd  & RatConf) == mEnableLpfEhrpd ) {
                   Log.d (TAG, "enabling LPF for net_type: " + Integer.toString(net_type));
                   mControl.enableLPF(sFd, enable);
               }
               break;
           case TelephonyManager.NETWORK_TYPE_HSPAP:
               if ((mEnableLpfHspap  & RatConf) == mEnableLpfHspap ) {
                   Log.d (TAG, "enabling LPF for net_type: " + Integer.toString(net_type));
                   mControl.enableLPF(sFd, enable);
               }
               break;
           case TelephonyManager.NETWORK_TYPE_GSM:
               if ((mEnableLpfGsm & RatConf) == mEnableLpfGsm ) {
                   Log.d (TAG, "enabling LPF for net_type: " + Integer.toString(net_type));
                   mControl.enableLPF(sFd, enable);
               }
               break;
           case TelephonyManager.NETWORK_TYPE_TD_SCDMA:
               if ((mEnableLpfScdma & RatConf) == mEnableLpfScdma ) {
                   Log.d (TAG, "enabling LPF for net_type: " + Integer.toString(net_type));
                   mControl.enableLPF(sFd, enable);
               }
               break;
          case TelephonyManager.NETWORK_TYPE_IWLAN:
               if ((mEnableLpfIwlan  & RatConf) == mEnableLpfIwlan ) {
                   Log.d (TAG, "enabling LPF for net_type: " + Integer.toString(net_type));
                   mControl.enableLPF(sFd, enable);
               }
               break;
          case TelephonyManager.NETWORK_TYPE_LTE_CA:
               if ((mEnableLpfLteCa  & RatConf) == mEnableLpfLteCa ) {
                   Log.d (TAG, "enabling LPF for net_type: " + Integer.toString(net_type));
                   mControl.enableLPF(sFd, enable);
               }
               break;
           default:
               Log.d (TAG, "net_type " + Integer.toString(net_type) + " doesn't need LPF enabling");
               break;
       }
   }
   public void EnableSlimbus(int enable) {
       Log.d(TAG, "EnableSlimbus :enable =" + enable);
       mControl.enableSlimbus(sFd, enable);
   }
   public void EnableSoftMute(int enable) {
       Log.d(TAG, "enableSoftMute :enable =" + enable);
       mControl.enableSoftMute(sFd, enable);
   }

   public String getSocName() {
     return FmReceiverJNI.getSocNameNative();
   }

   public boolean getFmStatsProp() {
     return FmReceiverJNI.getFmStatsPropNative();
   }

   public int getFmWanWlanCoexProp(int property) {
     return FmReceiverJNI.getFmCoexPropNative(sFd, property);
   }
}
