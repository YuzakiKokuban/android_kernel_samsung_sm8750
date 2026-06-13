/*
 * Copyright (c) 2009-2016, The Linux Foundation. All rights reserved.
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

package com.caf.fmradio;

import java.io.File;
import java.util.*;
import java.io.IOException;
import java.lang.ref.WeakReference;

import android.app.AlarmManager;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ServiceInfo;
import android.app.IntentService;
import android.os.UserHandle;
import android.content.BroadcastReceiver;
import android.media.AudioManager;
import android.media.AudioManager.OnAudioFocusChangeListener;
import android.media.AudioSystem;
import android.media.MediaRecorder;
import android.media.AudioDevicePort;
import android.media.AudioDevicePortConfig;
import android.media.AudioFormat;
import android.media.AudioManager.OnAudioPortUpdateListener;
import android.media.AudioMixPort;
import android.media.AudioPatch;
import android.media.AudioPort;
import android.media.AudioPortConfig;
import android.media.AudioRecord;
import android.media.AudioTrack;
import android.media.AudioDeviceInfo;
import android.media.AudioAttributes;
import android.media.AudioFocusRequest;

import android.os.Environment;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.os.RemoteException;
import android.telephony.TelephonyCallback;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.widget.RemoteViews;
import android.widget.Toast;
import android.view.KeyEvent;
import android.os.SystemProperties;

import qcom.fmradio.FmReceiver;
import qcom.fmradio.FmRxEvCallbacksAdaptor;
import qcom.fmradio.FmRxRdsData;
import qcom.fmradio.FmConfig;
import android.net.Uri;
import android.content.res.Resources;
import java.util.Date;
import java.text.SimpleDateFormat;
import android.provider.MediaStore;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.database.Cursor;
import com.caf.utils.A2dpDeviceStatus;
import android.content.ComponentName;
import android.os.StatFs;
import android.os.SystemClock;
import android.os.Process;
import android.app.ActivityManager;
import android.app.ActivityManager.RunningAppProcessInfo;
import android.media.session.MediaSession;
import android.media.AudioRouting;
import android.bluetooth.BluetoothA2dp;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.os.IBinder.DeathRecipient;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import java.lang.Math;


/**
 * Provides "background" FM Radio (that uses the hardware) capabilities,
 * allowing the user to switch between activities without stopping playback.
 */
public class FMRadioService extends Service
{
   private static final int FMRADIOSERVICE_STATUS = 101;
   private static final String FMRADIO_DEVICE_FD_STRING = "/dev/radio0";
   private static final String FMRADIO_NOTIFICATION_CHANNEL = "fmradio_notification_channel";
   private static final String LOGTAG = "FMService";//FMRadio.LOGTAG;

   private FmReceiver mReceiver;
   private final Object mReceiverLock = new Object();
   private BroadcastReceiver mHeadsetReceiver = null;
   private BroadcastReceiver mSdcardUnmountReceiver = null;
   private BroadcastReceiver mMusicCommandListener = null;
   private BroadcastReceiver mSleepExpiredListener = null;
   private boolean mSleepActive = false;
   private BroadcastReceiver mRecordTimeoutListener = null;
   private BroadcastReceiver mDelayedServiceStopListener = null;
   private BroadcastReceiver mAudioBecomeNoisyListener = null;
   private SharedPreferences mPref = null;
   private Editor mEditor = null;
   private BroadcastReceiver mFmMediaButtonListener;
   private BroadcastReceiver mAirplaneModeChanged;
   private BroadcastReceiver mRegisterUserSwitched;
   private IFMRadioServiceCallbacks mCallbacks;
   private static FmSharedPreferences mPrefs;
   private boolean mHeadsetPlugged = false;
   private boolean mInternalAntennaAvailable = false;
   private WakeLock mWakeLock;
   private int mServiceStartId = -1;
   private boolean mServiceInUse = false;
   private static boolean mMuted = false;
   private static boolean mResumeAfterCall = false;
   private static int mAudioDevice = AudioDeviceInfo.TYPE_WIRED_HEADPHONES;
   MediaRecorder mRecorder = null;
   MediaRecorder mA2dp = null;
   private boolean mFMOn = false;
   private boolean mFmRecordingOn = false;
   private static boolean mRtPlusSupport = false;
   private boolean mSpeakerPhoneOn = false;
   private int mCallStatus = 0;
   private BroadcastReceiver mScreenOnOffReceiver = null;
   final Handler mHandler = new Handler();
   private boolean misAnalogPathEnabled = false;
   private boolean mA2dpDisconnected = false;
   private boolean mA2dpConnected = false;

   private boolean mFmStats = false;
   //Install the death receipient
   private IBinder.DeathRecipient mDeathRecipient;
   private FMDeathRecipient mFMdr;

   //PhoneStateListener instances corresponding to each

   private FmRxRdsData mFMRxRDSData=null;
   // interval after which we stop the service when idle
   private static final int IDLE_DELAY = 60000;
   private File mA2DPSampleFile = null;
   //Track FM playback for reenter App usecases
   private boolean mPlaybackInProgress = false;
   private boolean mStoppedOnFocusLoss = true;
   private boolean mStoppedOnFactoryReset = false;
   private File mSampleFile = null;
   long mSampleStart = 0;
   int mSampleLength = 0;
   // Messages handled in FM Service
   private static final int FM_STOP =1;
   private static final int RESET_NOTCH_FILTER =2;
   private static final int STOPSERVICE_ONSLEEP = 3;
   private static final int STOPRECORD_ONTIMEOUT = 4;
   private static final int FOCUSCHANGE = 5;
   //Track notch filter settings
   private boolean mNotchFilterSet = false;
   public static final int STOP_SERVICE = 0;
   public static final int STOP_RECORD = 1;
   // A2dp Device Status will be queried through this class
   A2dpDeviceStatus mA2dpDeviceState = null;
   private boolean mA2dpDeviceSupportInHal = false;
   //on shutdown not to send start Intent to AudioManager
   private boolean mAppShutdown = false;
   private AudioManager mAudioManager;
   public static final long UNAVAILABLE = -1L;
   public static final long PREPARING = -2L;
   public static final long UNKNOWN_SIZE = -3L;
   public static final long LOW_STORAGE_THRESHOLD = 50000000;
   private long mStorageSpace;
   private static final String IOBUSY_UNVOTE = "com.android.server.CpuGovernorService.action.IOBUSY_UNVOTE";
   private static final String SLEEP_EXPIRED_ACTION = "com.caf.fmradio.SLEEP_EXPIRED";
   private static final String RECORD_EXPIRED_ACTION = "com.caf.fmradio.RECORD_TIMEOUT";
   private static final String SERVICE_DELAYED_STOP_ACTION = "com.caf.fmradio.SERVICE_STOP";
   public static final String ACTION_FM =
               "codeaurora.intent.action.FM";
   public static final String ACTION_FM_RECORDING =
           "codeaurora.intent.action.FM_Recording";
   public static final String ACTION_FM_RECORDING_STATUS =
           "codeaurora.intent.action.FM.Recording.Status";
   private BroadcastReceiver mFmRecordingStatus  = null;
   private Thread mRecordServiceCheckThread = null;
   private MediaSession mSession;
   private boolean mIsSSRInProgress = false;
   private boolean mIsSSRInProgressFromActivity = false;
   private int mKeyActionDownCount = 0;

   private AudioTrack mAudioTrack = null;
   private static final int AUDIO_FRAMES_COUNT_TO_IGNORE = 3;
   private Object mEventWaitLock = new Object();
   private boolean mIsFMDeviceLoopbackActive = false;
   private File mStoragePath = null;
   private static final int FM_OFF_FROM_APPLICATION = 1;
   private static final int FM_OFF_FROM_ANTENNA = 2;
   private static final int RADIO_TIMEOUT = 1500;
   private static final int FW_TIMEOUT = 200;
   private static final int DISABLE_SLIMBUS_DATA_PORT = 0;
   private static final int ENABLE_SLIMBUS_DATA_PORT = 1;
   private static final int DISABLE_SOFT_MUTE = 0;
   private static final int ENABLE_SOFT_MUTE = 1;
   private static final int DEFAULT_VOLUME_INDEX = 6;
   private static Object mNotchFilterLock = new Object();
   private static Object mNotificationLock = new Object();

   private boolean mEventReceived = false;
   private boolean isfmOffFromApplication = false;
   private AudioFocusRequest mGainFocusReq;
   private PhoneStateCallback mPhoneStateCallback;

   public FMRadioService() {
   }

   @Override
   public void onCreate() {
      super.onCreate();

      mPref = getApplicationContext().getSharedPreferences("SlimbusPref", MODE_PRIVATE);
      mEditor = mPref.edit();
      mPrefs = new FmSharedPreferences(this);
      mCallbacks = null;
      TelephonyManager tmgr = (TelephonyManager) getSystemService(Context.TELEPHONY_SERVICE);
      if (mPhoneStateCallback == null) {
         mPhoneStateCallback = new PhoneStateCallback();
      }
      tmgr.registerTelephonyCallback(getApplicationContext().getMainExecutor(), mPhoneStateCallback);
      PowerManager pm = (PowerManager)getSystemService(Context.POWER_SERVICE);
      mWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, this.getClass().getName());
      mWakeLock.setReferenceCounted(false);
      /* Register for Screen On/off broadcast notifications */
      mA2dpDeviceState = new A2dpDeviceStatus(getApplicationContext());
      registerScreenOnOffListener();
      registerHeadsetListener();
      registerSleepExpired();
      registerRecordTimeout();
      registerDelayedServiceStop();
      registerExternalStorageListener();
      registerAirplaneModeStatusChanged();
      registerUserSwitch();
      registerAudioBecomeNoisy();

      mSession = new MediaSession(getApplicationContext(), this.getClass().getName());
      mSession.setCallback(mSessionCallback);
      mSession.setFlags(MediaSession.FLAG_EXCLUSIVE_GLOBAL_PRIORITY);

      // Register for pause commands from other apps to stop FM
      registerMusicServiceCommandReceiver();

      // If the service was idle, but got killed before it stopped itself, the
      // system will relaunch it. Make sure it gets stopped again in that case.
      setAlarmDelayedServiceStop();
      /* Query to check is a2dp supported in Hal */
      AudioManager audioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
      String valueStr = audioManager.getParameters("isA2dpDeviceSupported");
      mA2dpDeviceSupportInHal = valueStr.contains("=true");
      Log.d(LOGTAG, " is A2DP device Supported In HAL"+mA2dpDeviceSupportInHal);

      mGainFocusReq = requestAudioFocus();
      AudioManager mAudioManager =
          (AudioManager) getSystemService(Context.AUDIO_SERVICE);
      AudioDeviceInfo[] deviceList = mAudioManager.getDevices(AudioManager.GET_DEVICES_OUTPUTS);
      for (int index = 0; index < deviceList.length; index++) {
          if ((deviceList[index].getType() == AudioDeviceInfo.TYPE_WIRED_HEADSET ) ||
                   (deviceList[index].getType() == AudioDeviceInfo.TYPE_WIRED_HEADPHONES )){
              mHeadsetPlugged = true;
              break;
          }
      }
   }

   @Override
   public void onDestroy() {
      Log.d(LOGTAG, "onDestroy");
      if (isFmOn())
      {
         Log.e(LOGTAG, "FMRadio Service being destroyed");
          /* Since the service is closing, disable the receiver */
          fmOff();
      }

      // make sure there aren't any other messages coming
      mDelayedStopHandler.removeCallbacksAndMessages(null);
      cancelAlarms();
      //release the audio focus listener
      AudioManager audioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
      audioManager.abandonAudioFocusRequest(mGainFocusReq);
      mGainFocusReq = null;
      /* Remove the Screen On/off listener */
      if (mScreenOnOffReceiver != null) {
          unregisterReceiver(mScreenOnOffReceiver);
          mScreenOnOffReceiver = null;
      }
      /* Unregister the headset Broadcase receiver */
      if (mHeadsetReceiver != null) {
          unregisterReceiver(mHeadsetReceiver);
          mHeadsetReceiver = null;
      }
      if( mMusicCommandListener != null ) {
          unregisterReceiver(mMusicCommandListener);
          mMusicCommandListener = null;
      }
      if( mFmMediaButtonListener != null ) {
          unregisterReceiver(mFmMediaButtonListener);
          mFmMediaButtonListener = null;
      }
      if (mAudioBecomeNoisyListener != null) {
          unregisterReceiver(mAudioBecomeNoisyListener);
          mAudioBecomeNoisyListener = null;
      }
      if (mSleepExpiredListener != null ) {
          unregisterReceiver(mSleepExpiredListener);
          mSleepExpiredListener = null;
      }
      if (mRecordTimeoutListener != null) {
          unregisterReceiver(mRecordTimeoutListener);
          mRecordTimeoutListener = null;
      }
      if (mDelayedServiceStopListener != null) {
          unregisterReceiver(mDelayedServiceStopListener);
          mDelayedServiceStopListener = null;
      }
      if (mFmRecordingStatus != null ) {
          unregisterReceiver(mFmRecordingStatus);
          mFmRecordingStatus = null;
      }
      if (mAirplaneModeChanged != null) {
          unregisterReceiver(mAirplaneModeChanged);
          mAirplaneModeChanged = null;
      }
      if( mSdcardUnmountReceiver != null ) {
          unregisterReceiver(mSdcardUnmountReceiver);
          mSdcardUnmountReceiver = null;
      }
      if (mRegisterUserSwitched != null) {
          unregisterReceiver(mRegisterUserSwitched);
          mRegisterUserSwitched = null;
      }

      TelephonyManager tmgr = (TelephonyManager) getSystemService(Context.TELEPHONY_SERVICE);
      tmgr.unregisterTelephonyCallback(mPhoneStateCallback);

      Log.d(LOGTAG, "onDestroy: unbindFromService completed");

      //unregisterReceiver(mIntentReceiver);
      mWakeLock.release();
      super.onDestroy();
   }

    private void setFMVolume(int mCurrentVolumeIndex) {
       AudioManager audioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
       float decibels = audioManager.getStreamVolumeDb(AudioManager.STREAM_MUSIC,
                                     mCurrentVolumeIndex,mAudioDevice);
       /* DbToAmpl */
       float volume =  (float)Math.exp(decibels * 0.115129f);
       Log.d(LOGTAG, "setFMVolume mCurrentVolumeIndex = "+
                     mCurrentVolumeIndex + " volume = " + volume);
       String keyValPairs = new String("fm_volume="+volume);
       Log.d(LOGTAG, "keyValPairs = "+keyValPairs);
       audioManager.setParameters(keyValPairs);
       try
       {
           if (mCallbacks != null)
           {
             if (isMuted() == true) {
               unMute();
               mCallbacks.onMute(false);
             }
           }
       }
       catch (RemoteException e)
       {
          e.printStackTrace();
       }
    }
    private boolean configureFMDeviceLoopback(boolean enable) {
        Log.d(LOGTAG, "configureFMDeviceLoopback enable = " + enable +
               " DeviceLoopbackActive = " + mIsFMDeviceLoopbackActive +
               " mStoppedOnFocusLoss = "+mStoppedOnFocusLoss);
        int mAudioDeviceType;

        AudioManager audioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        if(enable) {
            if(mIsFMDeviceLoopbackActive || mStoppedOnFocusLoss) {
                Log.d(LOGTAG, "either FM does not have audio focus"+
                  "or Already devcie loop back is acive, not enabling audio");
                return false;
            }
            String status = audioManager.getParameters("fm_status");
            Log.d(LOGTAG," FM hardwareLoopback Status = " + status);
            if (status.contains("1")) {
               /* This case usually happens, when FM is force killed through settings app
                * and we don't get chance to disable Hardware LoopBack.
                * Hardware LoopBack will be running,disable it first and enable again
                * using routing set param to audio */
               Log.d(LOGTAG," FM HardwareLoopBack Active, disable it first and enable again");
               mAudioDeviceType =
                  AudioDeviceInfo.TYPE_WIRED_HEADPHONES | AudioSystem.DEVICE_OUT_FM;
               String keyValPairs = new String("fm_routing="+mAudioDeviceType);
               Log.d(LOGTAG, "keyValPairs = "+keyValPairs);
               audioManager.setParameters(keyValPairs);
            }
            mIsFMDeviceLoopbackActive = true;
            /*or with DEVICE_OUT_FM to support backward compatiblity*/
            mAudioDeviceType = mAudioDevice | AudioSystem.DEVICE_OUT_FM;
            int mCurrentVolumeIndex = audioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
            setFMVolume(mCurrentVolumeIndex);
        } else if (mIsFMDeviceLoopbackActive == false) {
            Log.d(LOGTAG, "no devcie loop back is active, not disabling the audio");
            return false;
        } else {
            /*disabling the fm audio*/
            mIsFMDeviceLoopbackActive = false;
            mAudioDeviceType = mAudioDevice;
        }
        String keyValPairs = new String("handle_fm="+mAudioDeviceType);
        Log.d(LOGTAG, "keyValPairs = "+keyValPairs);
        audioManager.setParameters(keyValPairs);

        return true;
    }

    private void setCurrentFMVolume() {
        if(isFmOn()) {
            AudioManager maudioManager =
                    (AudioManager) getSystemService(Context.AUDIO_SERVICE);
            int mCurrentVolumeIndex =
                    maudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
            setFMVolume(mCurrentVolumeIndex);
        }
    }

    /**
      * Registers an intent to listen for ACTION_MEDIA_UNMOUNTED notifications.
      * The intent will call closeExternalStorageFiles() if the external media
      * is going to be ejected, so applications can clean up.
      */
     public void registerExternalStorageListener() {
         if (mSdcardUnmountReceiver == null) {
             mSdcardUnmountReceiver = new BroadcastReceiver() {
                 @Override
                 public void onReceive(Context context, Intent intent) {
                     String action = intent.getAction();
                     if ((action.equals(Intent.ACTION_MEDIA_UNMOUNTED))
                           || (action.equals(Intent.ACTION_MEDIA_EJECT))) {
                         Log.d(LOGTAG, "ACTION_MEDIA_UNMOUNTED Intent received");
                         if (mFmRecordingOn == true) {
                             if (mStoragePath == null) {
                                 Log.d(LOGTAG, "Storage path is null, doing nothing");
                                 return;
                             }
                             try {
                                 String state = Environment.getExternalStorageState(mStoragePath);
                                 if (!Environment.MEDIA_MOUNTED.equals(state)) {
                                     Log.d(LOGTAG, "Recording storage is not mounted, stop recording");
                                     stopRecording();
                                 }
                             } catch (Exception e) {
                                  e.printStackTrace();
                             }
                         }
                     }
                 }
             };
             IntentFilter iFilter = new IntentFilter();
             iFilter.addAction(Intent.ACTION_MEDIA_UNMOUNTED);
             iFilter.addAction(Intent.ACTION_MEDIA_EJECT);
             iFilter.addDataScheme("file");
             registerReceiver(mSdcardUnmountReceiver, iFilter, Context.RECEIVER_EXPORTED);
         }
     }

   public void registerAirplaneModeStatusChanged() {
       if (mAirplaneModeChanged == null) {
           mAirplaneModeChanged = new BroadcastReceiver() {
               @Override
               public void onReceive(Context context, Intent intent) {
                   String action = intent.getAction();
                   if (action.equals(Intent.ACTION_AIRPLANE_MODE_CHANGED)) {
                       Log.d(LOGTAG, "ACTION_AIRPLANE_MODE_CHANGED received");
                       boolean state = intent.getBooleanExtra("state", false);
                       if (state == true) {
                           fmOff();
                           try {
                               if ((mServiceInUse) && (mCallbacks != null) ) {
                                   mCallbacks.onDisabled();
                               }
                           } catch (RemoteException e) {
                               e.printStackTrace();
                           }
                       }
                   }
               }
           };
           IntentFilter iFilter = new IntentFilter();
           iFilter.addAction(Intent.ACTION_AIRPLANE_MODE_CHANGED);
           registerReceiver(mAirplaneModeChanged, iFilter, Context.RECEIVER_EXPORTED);
       }
   }

    public void registerUserSwitch(){
        if (mRegisterUserSwitched == null) {
            mRegisterUserSwitched = new BroadcastReceiver() {
                @Override
                public void onReceive(Context context, Intent intent) {
                    String action = intent.getAction();
                    Log.d(LOGTAG, "on receive UserSwitched " + action);
                    if (action.equals(Intent.ACTION_USER_SWITCHED)) {
                        Log.d(LOGTAG, "ACTION_USER_SWITCHED Intent received");
                        int userId = intent.getIntExtra(Intent.EXTRA_USER_HANDLE, 0);
                        Log.d(LOGTAG, "ACTION_USER_SWITCHED, user ID:" + userId);
                        if (isFmOn()){
                            fmOff();
                            try {
                                if ((mServiceInUse) && (mCallbacks != null) ) {
                                    mCallbacks.onDisabled();
                                }
                            } catch (RemoteException e) {
                                 e.printStackTrace();
                            }
                        }
                            stop();
                            android.os.Process.killProcess(android.os.Process.myPid());
                            System.exit(0);
                    }
                }
            };
            IntentFilter iFilter = new IntentFilter();
            iFilter.addAction(Intent.ACTION_USER_SWITCHED);
            registerReceiver(mRegisterUserSwitched, iFilter, Context.RECEIVER_EXPORTED);
       }
   }

     /**
     * Registers an intent to listen for ACTION_HEADSET_PLUG
     * notifications. This intent is called to know if the headset
     * was plugged in/out
     */
    public void registerHeadsetListener() {
        if (mHeadsetReceiver == null) {
            mHeadsetReceiver = new BroadcastReceiver() {
                @Override
                public void onReceive(Context context, Intent intent) {
                    String action = intent.getAction();
                    Log.d(LOGTAG, "on receive HeadsetListener " + action);
                    if (action.equals(Intent.ACTION_HEADSET_PLUG)) {
                       Log.d(LOGTAG, "ACTION_HEADSET_PLUG Intent received");
                       // Listen for ACTION_HEADSET_PLUG broadcasts.
                       Log.d(LOGTAG, "mReceiver: ACTION_HEADSET_PLUG");
                       Log.d(LOGTAG, "==> intent: " + intent);
                       Log.d(LOGTAG, "    state: " + intent.getIntExtra("state", 0));
                       Log.d(LOGTAG, "    name: " + intent.getStringExtra("name"));
                       mHeadsetPlugged = (intent.getIntExtra("state", 0) == 1);
                       // if headset is plugged out it is required to disable
                       // in minimal duration to avoid race conditions with
                       // audio policy manager switch audio to speaker.
                       if (isfmOffFromApplication) {
                           Log.d(LOGTAG, "fm is off from Application, bail out");
                           return;
                       }
                       mHandler.removeCallbacks(mHeadsetPluginHandler);
                       mHandler.post(mHeadsetPluginHandler);
                    } else if(mA2dpDeviceState.isA2dpStateChange(action) &&
                             (mA2dpDeviceState.isConnected(intent) ||
                              mA2dpDeviceState.isDisconnected(intent))) {
                        boolean  bA2dpConnected =
                        mA2dpDeviceState.isConnected(intent);
                        Log.d(LOGTAG, "bA2dpConnected: " + bA2dpConnected);

                        //mSpeakerPhoneOn = bA2dpConnected;
                        mA2dpConnected = bA2dpConnected;
                        mA2dpDisconnected = !bA2dpConnected;
                    } else if (action.equals("HDMI_CONNECTED")) {
                        //FM should be off when HDMI is connected.
                        fmOff();
                        try
                        {
                            /* Notify the UI/Activity, only if the service is "bound"
                               by an activity and if Callbacks are registered
                             */
                            if((mServiceInUse) && (mCallbacks != null) )
                            {
                                mCallbacks.onDisabled();
                            }
                        } catch (RemoteException e)
                        {
                            e.printStackTrace();
                        }
                    } else if( action.equals(Intent.ACTION_SHUTDOWN)) {
                        mAppShutdown = true;
                        if (isFmRecordingOn()) {
                            stopRecording();
                        }
                    } else if( action.equals(AudioManager.VOLUME_CHANGED_ACTION)) {
                        if(!isFmOn()) {
                            Log.d(LOGTAG, "FM is Turned off ,not applying the changed volume");
                            return;
                        }
                        int streamType =
                              intent.getIntExtra(AudioManager.EXTRA_VOLUME_STREAM_TYPE, -1);
                        if (streamType == AudioManager.STREAM_MUSIC) {
                            int mCurrentVolumeIndex =
                              intent.getIntExtra(AudioManager.EXTRA_VOLUME_STREAM_VALUE, -1);
                            setFMVolume(mCurrentVolumeIndex);
                        }
                    } else if (action.equals(BluetoothA2dp.ACTION_ACTIVE_DEVICE_CHANGED)) {
                            mHandler.removeCallbacks(mFmVolumeHandler);
                            mHandler.post(mFmVolumeHandler);

                    } else if (action.equals(BluetoothAdapter.ACTION_STATE_CHANGED)) {
                        int state =
                           intent.getIntExtra(BluetoothAdapter.EXTRA_STATE, BluetoothAdapter.ERROR);
                        Log.d(LOGTAG, "ACTION_STATE_CHANGED state :"+ state);
                        if (state == BluetoothAdapter.STATE_OFF) {
                            mHandler.removeCallbacks(mFmVolumeHandler);
                            mHandler.post(mFmVolumeHandler);
                        }
                    }
                }
            };
            AudioManager am = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
            IntentFilter iFilter = new IntentFilter();
            iFilter.addAction(Intent.ACTION_HEADSET_PLUG);
            iFilter.addAction(mA2dpDeviceState.getActionSinkStateChangedString());
            iFilter.addAction("HDMI_CONNECTED");
            iFilter.addAction(Intent.ACTION_SHUTDOWN);
            iFilter.addAction(AudioManager.VOLUME_CHANGED_ACTION);
            iFilter.addAction(BluetoothAdapter.ACTION_STATE_CHANGED);
            iFilter.addAction(BluetoothA2dp.ACTION_ACTIVE_DEVICE_CHANGED);
            iFilter.addCategory(Intent.CATEGORY_DEFAULT);
            registerReceiver(mHeadsetReceiver, iFilter, Context.RECEIVER_EXPORTED);
        }
    }

    public void registerAudioBecomeNoisy() {
        if (mAudioBecomeNoisyListener == null) {
            mAudioBecomeNoisyListener = new BroadcastReceiver() {
                @Override
                public void onReceive(Context context, Intent intent) {
                    String intentAction = intent.getAction();
                    Log.d(LOGTAG, "intent received " + intentAction);
                    if ((intentAction != null) &&
                          intentAction.equals(AudioManager.ACTION_AUDIO_BECOMING_NOISY)) {
                        if (isFmOn())
                        {
                            Log.d(LOGTAG, "AUDIO_BECOMING_NOISY INTENT: mute FM Audio");
                            mute();
                        }
                    }
                }
            };
            IntentFilter intentFilter = new IntentFilter(AudioManager.ACTION_AUDIO_BECOMING_NOISY);
            registerReceiver(mAudioBecomeNoisyListener, intentFilter, Context.RECEIVER_EXPORTED);
        }
    }

    // TODO: Check if this is needed with latest Android versions?
    public void registerMusicServiceCommandReceiver() {
        if (mMusicCommandListener == null) {
            mMusicCommandListener = new BroadcastReceiver() {
                @Override
                public void onReceive(Context context, Intent intent) {
                    String action = intent.getAction();

                    if (action.equals("com.android.music.musicservicecommand")) {
                        String cmd = intent.getStringExtra("command");
                        Log.d(LOGTAG, "Music Service command : "+cmd+ " received");
                        if (cmd != null && cmd.equals("pause")) {
                            if (isFmOn()) {
                                AudioManager audioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
                                audioManager.abandonAudioFocusRequest(mGainFocusReq);
                                mDelayedStopHandler.obtainMessage(FOCUSCHANGE, AudioManager.AUDIOFOCUS_LOSS, 0).sendToTarget();

                                if (isOrderedBroadcast()) {
                                    abortBroadcast();
                                }
                            }
                        }
                    }
                }
            };
            IntentFilter commandFilter = new IntentFilter();
            commandFilter.addAction("com.android.music.musicservicecommand");
            registerReceiver(mMusicCommandListener, commandFilter, Context.RECEIVER_EXPORTED);
        }
    }
    public void registerSleepExpired() {
        if (mSleepExpiredListener == null) {
            mSleepExpiredListener = new BroadcastReceiver() {
                @Override
                public void onReceive(Context context, Intent intent) {
                    Log.d(LOGTAG, "registerSleepExpired");
                    mWakeLock.acquire(10 * 1000);
                    fmOff();
                }
            };
            IntentFilter intentFilter = new IntentFilter(SLEEP_EXPIRED_ACTION);
            registerReceiver(mSleepExpiredListener, intentFilter, Context.RECEIVER_EXPORTED);
        }
    }
    public void registerRecordTimeout() {
        if (mRecordTimeoutListener == null) {
            mRecordTimeoutListener = new BroadcastReceiver() {
                @Override
                public void onReceive(Context context, Intent intent) {
                    Log.d(LOGTAG, "registerRecordTimeout");
                    mWakeLock.acquire(5 * 1000);
                    stopRecording();
                }
            };
            IntentFilter intentFilter = new IntentFilter(RECORD_EXPIRED_ACTION);
            registerReceiver(mRecordTimeoutListener, intentFilter, Context.RECEIVER_EXPORTED);
        }
    }
    public void registerDelayedServiceStop() {
        if (mDelayedServiceStopListener == null) {
            mDelayedServiceStopListener = new BroadcastReceiver() {
                @Override
                public void onReceive(Context context, Intent intent) {
                    Log.d(LOGTAG, "registerDelayedServiceStop");
                    mWakeLock.acquire(5 * 1000);
                    if (isFmOn() || mServiceInUse) {
                        return;
                    }
                    stopSelf(mServiceStartId);
                }
            };
            IntentFilter intentFilter = new IntentFilter(SERVICE_DELAYED_STOP_ACTION);
            registerReceiver(mDelayedServiceStopListener, intentFilter, Context.RECEIVER_EXPORTED);
        }
    }

    final Runnable    mFmVolumeHandler = new Runnable() {
        public void run() {
            try {
                Thread.sleep(1500);
            } catch (Exception ex) {
                Log.d( LOGTAG, "RunningThread InterruptedException");
                return;
            }
            setCurrentFMVolume();
        }
    };

    final Runnable    mHeadsetPluginHandler = new Runnable() {
        public void run() {
            /* Update the UI based on the state change of the headset/antenna*/
            if(!isAntennaAvailable())
            {
                mSpeakerPhoneOn = false;
                if (!isFmOn())
                    return;
                /* Disable FM and let the UI know */
                fmOff(FM_OFF_FROM_ANTENNA);
                try
                {
                    /* Notify the UI/Activity, only if the service is "bound"
                  by an activity and if Callbacks are registered
                     */
                    if((mServiceInUse) && (mCallbacks != null) )
                    {
                        mCallbacks.onDisabled();
                    }
                } catch (RemoteException e)
                {
                    e.printStackTrace();
                }
            }
            else
            {
                /* headset is plugged back in,
               So turn on FM if:
               - FM is not already ON.
               - If the FM UI/Activity is in the foreground
                 (the service is "bound" by an activity
                  and if Callbacks are registered)
                 */
                if ((!isFmOn()) && (mServiceInUse)
                        && (mCallbacks != null))
                {
                    if( true != fmOn() ) {
                        return;
                    }
                    try
                    {
                        mCallbacks.onEnabled();
                    } catch (RemoteException e)
                    {
                        e.printStackTrace();
                    }
                }
            }
        }
    };


   @Override
   public IBinder onBind(Intent intent) {
      mDelayedStopHandler.removeCallbacksAndMessages(null);
      cancelAlarms();
      mServiceInUse = true;
      /* Application/UI is attached, so get out of lower power mode */
      setLowPowerMode(false);
      Log.d(LOGTAG, "onBind ");
      //todo check for the mBinder validity
      mFMdr = new FMDeathRecipient(this, mBinder);
      try {
              mBinder.linkToDeath(mFMdr, 0);
              Log.d(LOGTAG, "onBind mBinder linked to death" + mBinder);
          } catch (RemoteException e) {
              Log.e(LOGTAG,"LinktoDeath Exception: "+e + "FM DR =" + mFMdr);
          }
      return mBinder;
   }

   @Override
   public void onRebind(Intent intent) {
      Log.d(LOGTAG, "onRebind");
      mDelayedStopHandler.removeCallbacksAndMessages(null);
      cancelAlarmDealyedServiceStop();
      mServiceInUse = true;
      /* Application/UI is attached, so get out of lower power mode */
      if (isFmOn()) {
          setLowPowerMode(false);
          if(false == mPlaybackInProgress) {
              startFM();
              enableSlimbus(ENABLE_SLIMBUS_DATA_PORT);
          }
      }
   }

   @Override
   public int onStartCommand(Intent intent, int flags, int startId) {
      Log.d(LOGTAG, "onStart");
      mServiceStartId = startId;
      // make sure the service will shut down on its own if it was
      // just started but not bound to and nothing is playing
      mDelayedStopHandler.removeCallbacksAndMessages(null);
      cancelAlarmDealyedServiceStop();
      setAlarmDelayedServiceStop();

      return START_STICKY;
   }

   @Override
   public boolean onUnbind(Intent intent) {
       mServiceInUse = false;
       Log.d(LOGTAG, "onUnbind");
       /* Application/UI is not attached, so go into lower power mode */
       unregisterCallbacks();
       setLowPowerMode(true);
       try{
           Log.d(LOGTAG,"Unlinking FM Death receipient");
           mBinder.unlinkToDeath(mFMdr,0);
       }
       catch(NoSuchElementException e){
           Log.e(LOGTAG,"No death recipient registered"+e);
       }
       return true;
  }

   private String getProcessName() {
      int id = Process.myPid();
      String myProcessName = this.getPackageName();

      ActivityManager actvityManager =
              (ActivityManager)this.getSystemService(this.ACTIVITY_SERVICE);
      List<RunningAppProcessInfo> procInfos =
              actvityManager.getRunningAppProcesses();

      for(RunningAppProcessInfo procInfo : procInfos) {
         if (id == procInfo.pid) {
              myProcessName = procInfo.processName;
         }
      }
      procInfos.clear();
      return myProcessName;
   }

   private void sendRecordServiceIntent(int action) {
       Intent intent = new Intent(ACTION_FM);
       intent.putExtra("state", action);
       intent.addFlags(Intent.FLAG_RECEIVER_FOREGROUND);
       Log.d(LOGTAG, "Sending Recording intent for = " +action);
       getApplicationContext().sendBroadcast(intent);
   }

   private void toggleFM() {
       Log.d(LOGTAG, "Toggle FM");
       if (isFmOn()){
           fmOff();
           try {
                if ((mServiceInUse) && (mCallbacks != null) ) {
                     mCallbacks.onDisabled();
                }
           } catch (RemoteException e) {
                e.printStackTrace();
           }
       } else if(isAntennaAvailable() && mServiceInUse ) {
           fmOn();
           try {
                if (mCallbacks != null ) {
                    mCallbacks.onEnabled();
                }
           } catch (RemoteException e) {
                e.printStackTrace();
           }
       }
   }

   private final MediaSession.Callback mSessionCallback = new MediaSession.Callback() {
        @Override
        public boolean onMediaButtonEvent(Intent intent) {
            KeyEvent event = (KeyEvent) intent.getParcelableExtra(Intent.EXTRA_KEY_EVENT);
            Log.d(LOGTAG, "SessionCallback.onMediaButton()...  event = " +event);
            int key_action = event.getAction();
            if ((event != null) && ((event.getKeyCode() == KeyEvent.KEYCODE_HEADSETHOOK)
                                    || (event.getKeyCode() == KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE))) {
                if (key_action == KeyEvent.ACTION_DOWN) {
                    mKeyActionDownCount++;
                }
                if ((mKeyActionDownCount == 1) && (key_action == KeyEvent.ACTION_UP)) {
                    Log.d(LOGTAG, "SessionCallback: HEADSETHOOK/MEDIA_PLAY_PAUSE short press");
                    mKeyActionDownCount = 0;
                    toggleFM();
                } else if ((mKeyActionDownCount == 2) && (key_action == KeyEvent.ACTION_DOWN)) {
                    Log.d(LOGTAG, "SessionCallback: HEADSETHOOK/MEDIA_PLAY_PAUSE long press");
                    if (isFmOn() && getResources()
                            .getBoolean(R.bool.def_headset_next_enabled)) {
                        try {
                            if ((mServiceInUse) && (mCallbacks != null))
                                mCallbacks.onSeekNextStation();
                        }catch (RemoteException e) {
                        }
                    }
                    mKeyActionDownCount = 0;
                }
                return true;
            } else if((event != null) && (event.getKeyCode() == KeyEvent.KEYCODE_MEDIA_PLAY)
                       && (key_action == KeyEvent.ACTION_DOWN)) {
                Log.d(LOGTAG, "SessionCallback: MEDIA_PLAY");
                if (isAntennaAvailable() && mServiceInUse) {
                    if (isFmOn()){
                        //FM should be off when Headset hook pressed.
                        fmOff();
                        try {
                            /* Notify the UI/Activity, only if the service is "bound"
                             * by an activity and if Callbacks are registered
                             * */
                            if ((mServiceInUse) && (mCallbacks != null) ) {
                                mCallbacks.onDisabled();
                            }
                        } catch (RemoteException e) {
                            e.printStackTrace();
                        }
                    } else {
                        fmOn();
                        try {
                            if (mCallbacks != null ) {
                                mCallbacks.onEnabled();
                            }
                        } catch (RemoteException e) {
                            e.printStackTrace();
                        }
                    }
                    return true;
                }
            } else if ((event != null) && ((event.getKeyCode() == KeyEvent.KEYCODE_MEDIA_PAUSE) ||
                                           (event.getKeyCode() == KeyEvent.KEYCODE_MEDIA_STOP))
                                       && (key_action == KeyEvent.ACTION_DOWN)) {
                Log.d(LOGTAG, "SessionCallback: MEDIA_PAUSE");
                if (isFmOn()){
                    //FM should be off when Headset hook pressed.
                    fmOff();
                    try {
                        /* Notify the UI/Activity, only if the service is "bound"
                           by an activity and if Callbacks are registered
                        */
                        if ((mServiceInUse) && (mCallbacks != null) ) {
                             mCallbacks.onDisabled();
                        }
                    } catch (RemoteException e) {
                        e.printStackTrace();
                    }
                    return true;
                }
            }
            return false;
        }
   };

   private AudioFocusRequest requestAudioFocus() {
       AudioAttributes playbackAttr = new AudioAttributes.Builder()
       .setUsage(AudioAttributes.USAGE_MEDIA)
       .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC)
       .build();
       AudioFocusRequest focusRequest =
           new AudioFocusRequest.Builder(AudioManager.AUDIOFOCUS_GAIN)
       .setAudioAttributes(playbackAttr)
       .setAcceptsDelayedFocusGain(true)
       .setWillPauseWhenDucked(true)
       .setOnAudioFocusChangeListener(this::onAudioFocusChange)
       .build();

       return focusRequest;
   }

   private void startFM() {
       Log.d(LOGTAG, "In startFM");
       if(true == mAppShutdown) { // not to send intent to AudioManager in Shutdown
           return;
       }
       if (isCallActive()) { // when Call is active never let audio playback
           mResumeAfterCall = true;
           return;
       }
       mResumeAfterCall = false;
       if ( true == mPlaybackInProgress ) // no need to resend event
           return;

       /* If audio focus lost while SSR in progress, don't request for Audio focus */
       if ( (true == mIsSSRInProgress || true == mIsSSRInProgressFromActivity) &&
             true == mStoppedOnFocusLoss) {
           Log.d(LOGTAG, "Audio focus lost while SSR in progress, returning");
           return;
       }

       if (mStoppedOnFocusLoss) {
           for(int i = 0; i < 4; i++)
           {
              AudioManager audioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
              int granted = audioManager.requestAudioFocus(mGainFocusReq);
              if (granted == AudioManager.AUDIOFOCUS_REQUEST_GRANTED) {
                  Log.d(LOGTAG, "audio focuss granted");
                  break;
              } else {
                  Log.d(LOGTAG, "audio focuss couldnot granted retry after some time");
                  /*in case of call and fm concurency case focus is abandon
                  ** after on call state callback, need to retry to get focus*/
                  try {
                      Thread.sleep(200);
                  } catch (Exception ex) {
                      Log.d( LOGTAG, "RunningThread InterruptedException");
                      return;
                  }
              }
           }
       }
       mSession.setActive(true);

       Log.d(LOGTAG,"FM registering for MediaButtonReceiver");
       mAudioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
       ComponentName fmRadio = new ComponentName(this.getPackageName(),
                                  FMMediaButtonIntentReceiver.class.getName());
       Intent mediaButtonIntent =
               new Intent(Intent.ACTION_MEDIA_BUTTON).setComponent(fmRadio);
       PendingIntent pi = PendingIntent.getBroadcast(getApplicationContext(), 0,
                          mediaButtonIntent, PendingIntent.FLAG_IMMUTABLE);
       mSession.setMediaButtonReceiver(pi);

       mStoppedOnFocusLoss = false;
       mPlaybackInProgress = true;

       if (mStoppedOnFactoryReset) {
           mStoppedOnFactoryReset = false;
           mSpeakerPhoneOn = false;
       // In FM stop, the audio route is set to default audio device
       }
       String temp = mSpeakerPhoneOn ? "Speaker" : "WiredHeadset";
       Log.d(LOGTAG, "Route audio to " + temp);
       if (mSpeakerPhoneOn) {
           mAudioDevice = AudioDeviceInfo.TYPE_BUILTIN_SPEAKER;
       } else {
           mAudioDevice = AudioDeviceInfo.TYPE_WIRED_HEADPHONES;
       }
       configureFMDeviceLoopback(true);
       try {
           if ((mServiceInUse) && (mCallbacks != null))
               mCallbacks.onFmAudioPathStarted();
       } catch(RemoteException e) {
           e.printStackTrace();
       }
   }

   private void stopFM() {
       Log.d(LOGTAG, "In stopFM");
       configureFMDeviceLoopback(false);
       mPlaybackInProgress = false;
       try {
           if ((mServiceInUse) && (mCallbacks != null))
               mCallbacks.onFmAudioPathStopped();
       } catch(RemoteException e) {
           e.printStackTrace();
       }
   }

   private void resetFM(){
       Log.d(LOGTAG, "resetFM");
       mPlaybackInProgress = false;
       configureFMDeviceLoopback(false);
   }

   private boolean getRecordServiceStatus() {
       boolean status = false;
       ActivityManager actvityManager =
                (ActivityManager)this.getSystemService(this.ACTIVITY_SERVICE);
       List<RunningAppProcessInfo> procInfos =
                                    actvityManager.getRunningAppProcesses();
       for(RunningAppProcessInfo procInfo : procInfos) {
           if (procInfo.processName.equals("com.codeaurora.fmrecording")) {
               status = true;
               break;
           }
       }
       procInfos.clear();
       return status;
   }

    private File createTempFile(String prefix, String suffix, File directory)
            throws IOException {
        // Force a prefix null check first
        if (prefix.length() < 3) {
            throw new IllegalArgumentException("prefix must be at least 3 characters");
        }
        if (suffix == null) {
            suffix = ".tmp";
        }
        File tmpDirFile = directory;
        if (tmpDirFile == null) {
            String tmpDir = System.getProperty("java.io.tmpdir", ".");
            tmpDirFile = new File(tmpDir);
        }

        String nameFormat = getResources().getString(R.string.def_save_name_format);
        SimpleDateFormat df = new SimpleDateFormat(nameFormat);
        String currentTime = df.format(System.currentTimeMillis());

        File result;
        do {
            result = new File(tmpDirFile, prefix + currentTime + suffix);
        } while (!result.createNewFile());
        return result;
   }

   public boolean startRecording() {
      int mRecordDuration = -1;

      Log.d(LOGTAG, "In startRecording of Recorder");

       stopRecording();

       if (!Environment.MEDIA_MOUNTED.equals(Environment.getExternalStorageState())) {
            Log.e(LOGTAG, "startRecording, no external storage available");
            return false;
       }

        if (!updateAndShowStorageHint())
            return false;
        long maxFileSize = mStorageSpace - LOW_STORAGE_THRESHOLD;
        if(FmSharedPreferences.getRecordDuration() !=
            FmSharedPreferences.RECORD_DUR_INDEX_3_VAL) {
            mRecordDuration = (FmSharedPreferences.getRecordDuration() * 60 * 1000);
         }

        mRecorder = new MediaRecorder();
        try {
              mRecorder.setMaxFileSize(maxFileSize);
              if (mRecordDuration >= 0)
                  mRecorder.setMaxDuration(mRecordDuration);
        } catch (RuntimeException exception) {

        }

        mStoragePath = Environment.getExternalStorageDirectory();
        Log.d(LOGTAG, "mStoragePath " + mStoragePath);
        if (null == mStoragePath) {
            Log.e(LOGTAG, "External Storage Directory is null");
            return false;
        }

        mSampleFile = null;
        File sampleDir = null;
        if (!"".equals(getResources().getString(R.string.def_fmRecord_savePath))) {
            String fmRecordSavePath = getResources().getString(R.string.def_fmRecord_savePath);
            sampleDir = new File(Environment.getExternalStorageDirectory().toString()
                    + fmRecordSavePath);
        } else {
            sampleDir = new File(Environment.getExternalStorageDirectory().getAbsolutePath()
                    + "/FMRecording");
        }

        if(!(sampleDir.mkdirs() || sampleDir.isDirectory()))
            return false;
        try {
            if (getResources().getBoolean(R.bool.def_save_name_format_enabled)) {
                String suffix = getResources().getString(R.string.def_save_name_suffix);
                suffix = "".equals(suffix) ? ".3gpp" : suffix;
                String prefix = getResources().getString(R.string.def_save_name_prefix) + '-';
                mSampleFile = createTempFile(prefix, suffix, sampleDir);
            } else {
                mSampleFile = File.createTempFile("FMRecording", ".3gpp", sampleDir);
            }
        } catch (IOException e) {
            Log.e(LOGTAG, "Not able to access SD Card");
            Toast.makeText(this, "Not able to access SD Card", Toast.LENGTH_SHORT).show();
            return false;
        }

        try {
             Log.d(LOGTAG, "AudioSource.RADIO_TUNER" +MediaRecorder.AudioSource.RADIO_TUNER);
             mRecorder.setAudioSource(MediaRecorder.AudioSource.RADIO_TUNER);
             mRecorder.setOutputFormat(MediaRecorder.OutputFormat.THREE_GPP);
             mRecorder.setAudioEncoder(MediaRecorder.AudioEncoder.AAC);
             final int samplingRate = 44100;
             mRecorder.setAudioSamplingRate(samplingRate);
             final int bitRate = 128000;
             mRecorder.setAudioEncodingBitRate(bitRate);
             final int audiochannels = 2;
             mRecorder.setAudioChannels(audiochannels);
        } catch (RuntimeException exception) {
             mRecorder.reset();
             mRecorder.release();
             mRecorder = null;
             return false;
        }
        mRecorder.setOutputFile(mSampleFile.getAbsolutePath());
        try {
             mRecorder.prepare();
             Log.d(LOGTAG, "start");
             mRecorder.start();
        } catch (IOException e) {
             mRecorder.reset();
             mRecorder.release();
             mRecorder = null;
             return false;
        } catch (RuntimeException e) {
             mRecorder.reset();
             mRecorder.release();
             mRecorder = null;
             return false;
        }
        mFmRecordingOn = true;
        Log.d(LOGTAG, "mSampleFile.getAbsolutePath() " +mSampleFile.getAbsolutePath());
        mRecorder.setOnInfoListener(new MediaRecorder.OnInfoListener() {
             public void onInfo(MediaRecorder mr, int what, int extra) {
                 if ((what == MediaRecorder.MEDIA_RECORDER_INFO_MAX_DURATION_REACHED) ||
                     (what == MediaRecorder.MEDIA_RECORDER_INFO_MAX_FILESIZE_REACHED)) {
                     if (mFmRecordingOn) {
                         Log.d(LOGTAG, "Maximum file size/duration reached, stop the recording");
                         stopRecording();
                     }
                     if (what == MediaRecorder.MEDIA_RECORDER_INFO_MAX_FILESIZE_REACHED)
                         // Show the toast.
                         Toast.makeText(FMRadioService.this, R.string.FMRecording_reach_size_limit,
                                        Toast.LENGTH_LONG).show();
                 }
             }
             // from MediaRecorder.OnErrorListener
             public void onError(MediaRecorder mr, int what, int extra) {
                 Log.e(LOGTAG, "MediaRecorder error. what=" + what + ". extra=" + extra);
                 if (what == MediaRecorder.MEDIA_RECORDER_ERROR_UNKNOWN) {
                     // We may have run out of space on the sdcard.
                     if (mFmRecordingOn) {
                         stopRecording();
                     }
                     updateAndShowStorageHint();
                 }
             }
        });

        mSampleStart = SystemClock.elapsedRealtime();
        Log.d(LOGTAG, "Sample start time: " +mSampleStart);
        return true;
  }

   public void stopRecording() {
       Log.d(LOGTAG, "Enter stopRecord");
       mFmRecordingOn = false;
       if (mRecorder == null)
           return;
       try {
             mRecorder.stop();
       } catch(Exception e) {
             e.printStackTrace();
       } finally {
             Log.d(LOGTAG, "reset and release of mRecorder");
             mRecorder.reset();
             mRecorder.release();
             mRecorder = null;
       }
       mSampleLength = (int)(SystemClock.elapsedRealtime() - mSampleStart);
       Log.d(LOGTAG, "Sample length is " + mSampleLength);

       if (mSampleLength == 0)
           return;
       String state = Environment.getExternalStorageState(mStoragePath);
       Log.d(LOGTAG, "storage state is " + state);

       if (Environment.MEDIA_MOUNTED.equals(state)) {
          try {
               this.addToMediaDB(mSampleFile);
               Toast.makeText(this,getString(R.string.save_record_file,
                              mSampleFile.getAbsolutePath( )),
                              Toast.LENGTH_LONG).show();
          } catch(Exception e) {
               e.printStackTrace();
          }
       } else {
           Log.e(LOGTAG, "SD card must have removed during recording. ");
           Toast.makeText(this, "Recording aborted", Toast.LENGTH_SHORT).show();
       }
       try {
           if((mServiceInUse) && (mCallbacks != null) ) {
               mCallbacks.onRecordingStopped();
           }
       } catch (RemoteException e) {
           e.printStackTrace();
       }
       return;
   }

   /*
    * Adds file and returns content uri.
    */
   private Uri addToMediaDB(File file) {
       Log.d(LOGTAG, "In addToMediaDB");
       Resources res = getResources();
       ContentValues cv = new ContentValues();
       long current = System.currentTimeMillis();
       long modDate = file.lastModified();
       Date date = new Date(current);
       SimpleDateFormat formatter = new SimpleDateFormat(
               res.getString(R.string.audio_db_title_format));
       String title = formatter.format(date);

       // Lets label the recorded audio file as NON-MUSIC so that the file
       // won't be displayed automatically, except for in the playlist.
       cv.put(MediaStore.Audio.Media.DURATION, mSampleLength);
       cv.put(MediaStore.Audio.Media.TITLE, title);
       cv.put(MediaStore.Audio.Media.DATA, file.getAbsolutePath());
       cv.put(MediaStore.Audio.Media.DATE_ADDED, (int) (current / 1000));
       cv.put(MediaStore.Audio.Media.DATE_MODIFIED, (int) (modDate / 1000));
       cv.put(MediaStore.Audio.Media.MIME_TYPE, "audio/mpeg");
       cv.put(MediaStore.Audio.Media.ARTIST,
               res.getString(R.string.audio_db_artist_name));
       cv.put(MediaStore.Audio.Media.ALBUM,
               res.getString(R.string.audio_db_album_name));
       Log.d(LOGTAG, "Inserting audio record: " + cv.toString());
       ContentResolver resolver = getContentResolver();
       Uri base = MediaStore.Audio.Media.EXTERNAL_CONTENT_URI;
       Log.d(LOGTAG, "ContentURI: " + base);
       Uri result = resolver.insert(base, cv);
       if (result == null) {
           Toast.makeText(this, "Unable to save recorded audio", Toast.LENGTH_SHORT).show();
           return null;
       }
       if (getPlaylistId(res) == -1) {
           createPlaylist(res, resolver);
       }
       int audioId = Integer.valueOf(result.getLastPathSegment());
       addToPlaylist(resolver, audioId, getPlaylistId(res));

       // Notify those applications such as Music listening to the
       // scanner events that a recorded audio file just created.
       sendBroadcast(new Intent(Intent.ACTION_MEDIA_SCANNER_SCAN_FILE, result));
       return result;
   }

   private int getPlaylistId(Resources res) {
       Uri uri = MediaStore.Audio.Playlists.getContentUri("external");
       final String[] ids = new String[] { MediaStore.Audio.Playlists._ID };
       final String where = MediaStore.Audio.Playlists.NAME + "=?";
       final String[] args = new String[] { res.getString(R.string.audio_db_playlist_name) };
       Cursor cursor = query(uri, ids, where, args, null);
       if (cursor == null) {
           Log.v(LOGTAG, "query returns null");
       }
       int id = -1;
       if (cursor != null) {
           cursor.moveToFirst();
           if (!cursor.isAfterLast()) {
               id = cursor.getInt(0);
           }
           cursor.close();
       }
       return id;
   }

   private Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs, String sortOrder) {
       try {
           ContentResolver resolver = getContentResolver();
           if (resolver == null) {
               return null;
           }
           return resolver.query(uri, projection, selection, selectionArgs, sortOrder);
        } catch (UnsupportedOperationException ex) {
           return null;
       }
   }

   private Uri createPlaylist(Resources res, ContentResolver resolver) {
       ContentValues cv = new ContentValues();
       cv.put(MediaStore.Audio.Playlists.NAME, res.getString(R.string.audio_db_playlist_name));
       Uri uri = resolver.insert(MediaStore.Audio.Playlists.getContentUri("external"), cv);
       if (uri == null) {
           Toast.makeText(this, "Unable to save recorded audio", Toast.LENGTH_SHORT).show();
       }
       return uri;
   }

   private void addToPlaylist(ContentResolver resolver, int audioId, long playlistId) {
       Uri uri = MediaStore.Audio.Playlists.Members.getContentUri("external", playlistId);

       ContentValues values = new ContentValues();
       values.put(MediaStore.Audio.Playlists.Members.PLAY_ORDER, audioId);
       values.put(MediaStore.Audio.Playlists.Members.AUDIO_ID, audioId);
       try {
           resolver.insert(uri, values);
       } catch (Exception exception) {
           exception.printStackTrace();
       }
   }

    private void resumeAfterCall() {
        if (getCallState() != TelephonyManager.CALL_STATE_IDLE)
            return;

        // start playing again
        if (!mResumeAfterCall)
            return;

        // resume playback only if FM Radio was playing
        // when the call was answered
        if (isAntennaAvailable() && (!isFmOn()) && mServiceInUse) {
            Log.d(LOGTAG, "Resuming after call:");
            if(!fmOn()) {
                return;
            }
            mResumeAfterCall = false;
            if (mCallbacks != null) {
                try {
                    mCallbacks.onEnabled();
                } catch (RemoteException e) {
                    e.printStackTrace();
                }
            }
        }
    }

   private void fmActionOnCallState( int state ) {
   //if Call Status is non IDLE we need to Mute FM as well stop recording if
   //any. Similarly once call is ended FM should be unmuted.
       AudioManager audioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
       mCallStatus = state;
       int granted = AudioManager.AUDIOFOCUS_REQUEST_FAILED, count = 0;

       if((TelephonyManager.CALL_STATE_OFFHOOK == state)||
          (TelephonyManager.CALL_STATE_RINGING == state)) {
           boolean bTempSpeaker = mSpeakerPhoneOn ; //need to restore SpeakerPhone
           boolean bTempMute = mMuted;// need to restore Mute status
           int bTempCall = mCallStatus;//need to restore call status
           if (mSession != null && mSession.isActive()) {
               Log.d(LOGTAG, "onCallStateChanged: State - " + state
                       + " Session is Active: " + mSession.isActive() );
               mSession.setActive(false);
           }
           if (isFmOn() && fmOff()) {
               if((mServiceInUse) && (mCallbacks != null)) {
                   try {
                        mCallbacks.onDisabled();
                   } catch (RemoteException e) {
                        e.printStackTrace();
                   }
               }
               mResumeAfterCall = true;
               mSpeakerPhoneOn = bTempSpeaker;
               mCallStatus = bTempCall;
               mMuted = bTempMute;
           } else if (!mResumeAfterCall) {
               mResumeAfterCall = false;
               mSpeakerPhoneOn = bTempSpeaker;
               mCallStatus = bTempCall;
               mMuted = bTempMute;
           }
       } else if (TelephonyManager.CALL_STATE_IDLE == state) {
           /* do not resume FM after call when fm stopped on focus loss */
           if(mStoppedOnFocusLoss == false) {
               resumeAfterCall();
           }
       }
   }

    /* Handle Phone Call + FM Concurrency */
   private class PhoneStateCallback extends TelephonyCallback implements
            TelephonyCallback.CallStateListener,
            TelephonyCallback.DataActivityListener {
      @Override
      public void onCallStateChanged(int state){
          Log.d(LOGTAG, "onCallStateChanged: State - " + state );
          fmActionOnCallState(state );
      }

      @Override
      public void onDataActivity (int direction) {
          Log.d(LOGTAG, "onDataActivity - " + direction );
          if (direction == TelephonyManager.DATA_ACTIVITY_NONE ||
              direction == TelephonyManager.DATA_ACTIVITY_DORMANT) {
                 if (mReceiver != null) {
                       Message msg = mDelayedStopHandler.obtainMessage(RESET_NOTCH_FILTER);
                       mDelayedStopHandler.sendMessageDelayed(msg, 10000);
                 }
         } else {
               if (mReceiver != null) {
                   synchronized (mNotchFilterLock) {
                       if (true == mNotchFilterSet) {
                           mDelayedStopHandler.removeMessages(RESET_NOTCH_FILTER);
                       } else {
                           mReceiver.setNotchFilter(true);
                           mNotchFilterSet = true;
                       }
                   }
               }
         }
      }
 };

   private Handler mDelayedStopHandler = new Handler() {
      @Override
      public void handleMessage(Message msg) {
          switch (msg.what) {
          case FM_STOP:
              // Check again to make sure nothing is playing right now
              if (isFmOn() || mServiceInUse)
              {
                   return;
              }
              Log.d(LOGTAG, "mDelayedStopHandler: stopSelf");
              stopSelf(mServiceStartId);
              break;
          case RESET_NOTCH_FILTER:
              synchronized (mNotchFilterLock) {
                  if (false == mNotchFilterSet)
                      break;
                  if (mReceiver != null) {
                      mReceiver.setNotchFilter(false);
                      mNotchFilterSet = false;
                  }
              }
              break;
          case STOPSERVICE_ONSLEEP:
              fmOff();
              break;
          case STOPRECORD_ONTIMEOUT:
              stopRecording();
              break;
          case FOCUSCHANGE:
              if( !isFmOn() && !mResumeAfterCall) {
                  Log.v(LOGTAG, "FM is not running, not handling change");
                  return;
              }
              switch (msg.arg1) {
                  case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK:
                      Log.v(LOGTAG, "AudioFocus: received AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK");
                      mAudioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
                      int mCurrentVolumeIndex =
                                  mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
                      Log.d(LOGTAG, "Current Volume Index = "+ mCurrentVolumeIndex);
                      /* lower fm volume only if fm audio playing above default volume index */
                      if (mCurrentVolumeIndex > DEFAULT_VOLUME_INDEX) {
                          setFMVolume(DEFAULT_VOLUME_INDEX);
                      }
                      break;
                  case AudioManager.AUDIOFOCUS_LOSS:
                      Log.v(LOGTAG, "AudioFocus: received AUDIOFOCUS_LOSS mspeakerphone= " +
                               mSpeakerPhoneOn);

                      if (mSession.isActive()) {
                          mSession.setActive(false);
                      }
                      //intentional fall through.
                  case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT:
                      Log.v(LOGTAG, "AudioFocus: received AUDIOFOCUS_LOSS_TRANSIENT");

                      enableSlimbus(DISABLE_SLIMBUS_DATA_PORT);

                      if (true == mPlaybackInProgress) {
                          stopFM();
                      }

                      if (true == isFmRecordingOn())
                          stopRecording();

                      mStoppedOnFocusLoss = true;
                      break;
                  case AudioManager.AUDIOFOCUS_GAIN:
                      Log.v(LOGTAG, "AudioFocus: received AUDIOFOCUS_GAIN mPlaybackinprogress =" +
                           mPlaybackInProgress);
                      mStoppedOnFocusLoss = false;
                      if (mResumeAfterCall) {
                          Log.v(LOGTAG, "resumeAfterCall");
                          resumeAfterCall();
                          break;
                      }

                      if(false == mPlaybackInProgress) {
                          startFM();
                          enableSlimbus(ENABLE_SLIMBUS_DATA_PORT);

                      } else {
                          /* This case usually happens, when FM volume is lowered down and Playback
                           * In Progress on AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK recived. Need
                           * to restore volume level back when AUDIOFOCUS_GAIN received
                           */
                          setCurrentFMVolume();
                      }
                      if (!mSession.isActive()) {
                          mSession.setActive(true);
                      }
                      break;
                  default:
                      Log.e(LOGTAG, "Unknown audio focus change code"+msg.arg1);
              }
              break;
          }
      }
   };


     /**
     * Registers an intent to listen for
     * ACTION_SCREEN_ON/ACTION_SCREEN_OFF notifications. This intent
     * is called to know iwhen the screen is turned on/off.
     */
    public void registerScreenOnOffListener() {
        if (mScreenOnOffReceiver == null) {
            mScreenOnOffReceiver = new BroadcastReceiver() {
                @Override
                public void onReceive(Context context, Intent intent) {
                    String action = intent.getAction();
                    if (action.equals(Intent.ACTION_SCREEN_ON)) {
                       Log.d(LOGTAG, "ACTION_SCREEN_ON Intent received");
                       //Screen turned on, set FM module into normal power mode
                       mHandler.post(mScreenOnHandler);
                    }
                    else if (action.equals(Intent.ACTION_SCREEN_OFF)) {
                       Log.d(LOGTAG, "ACTION_SCREEN_OFF Intent received");
                       //Screen turned on, set FM module into low power mode
                       mHandler.post(mScreenOffHandler);
                    }
                }
            };
            IntentFilter iFilter = new IntentFilter();
            iFilter.addAction(Intent.ACTION_SCREEN_ON);
            iFilter.addAction(Intent.ACTION_SCREEN_OFF);
            registerReceiver(mScreenOnOffReceiver, iFilter, Context.RECEIVER_EXPORTED);
        }
    }

    /* Handle all the Screen On actions:
       Set FM Power mode to Normal
     */
    final Runnable    mScreenOnHandler = new Runnable() {
       public void run() {
          setLowPowerMode(false);
       }
    };
    /* Handle all the Screen Off actions:
       Set FM Power mode to Low Power
       This will reduce all the interrupts coming up from the SoC, saving power
     */
    final Runnable    mScreenOffHandler = new Runnable() {
       public void run() {
          setLowPowerMode(true);
       }
    };

   /* Show the FM Notification */
   public void startNotification() {
      Log.d(LOGTAG,"startNotification");

      synchronized (mNotificationLock) {
          RemoteViews views = new RemoteViews(getPackageName(), R.layout.statusbar);
          views.setImageViewResource(R.id.icon, R.drawable.stat_notify_fm);
          if (isFmOn())
          {
              views.setTextViewText(R.id.frequency, getTunedFrequencyString());
          } else {
             views.setTextViewText(R.id.frequency, "");
          }

          Context context = getApplicationContext();
          Notification notification;
          NotificationManager notificationManager =
              (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
          NotificationChannel notificationChannel =
              new NotificationChannel(FMRADIO_NOTIFICATION_CHANNEL,
              context.getString(R.string.app_name),
              NotificationManager.IMPORTANCE_LOW);

          notificationManager.createNotificationChannel(notificationChannel);

          notification = new Notification.Builder(context, FMRADIO_NOTIFICATION_CHANNEL)
            .setCustomContentView(views)
            .setSmallIcon(R.drawable.stat_notify_fm)
            .setContentIntent(PendingIntent.getActivity(this,
                0, new Intent("com.caf.fmradio.FMRADIO_ACTIVITY"), PendingIntent.FLAG_IMMUTABLE))
            .setOngoing(true)
            .build();

          startForeground(FMRADIOSERVICE_STATUS, notification,
                ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PLAYBACK);
          mFMOn = true;
      }
   }

      /* hide the FM Notification */
   public void stopNotification() {
      Log.d(LOGTAG,"stopNotification");

      synchronized (mNotificationLock) {
          Context context = getApplicationContext();
          NotificationManager notificationManager =
              (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
          if((notificationManager != null)
            && (notificationManager.getNotificationChannel(FMRADIO_NOTIFICATION_CHANNEL) != null)) {
             try {
               notificationManager.deleteNotificationChannel(FMRADIO_NOTIFICATION_CHANNEL);
             } catch (Exception e) {
               Log.e(LOGTAG,"exception raised from deleteNotificationChannel");
             }
          }
      }
   }

   private void stop() {
      Log.d(LOGTAG,"in stop");

      if (!mServiceInUse) {
          Log.d(LOGTAG,"release media session");
          mSession.release();
      }
      if (mSession.isActive()) {
          mSession.setActive(false);
          Log.d(LOGTAG,"mSession is not active");
      }
      gotoIdleState();
      mFMOn = false;
   }

   private void gotoIdleState() {
      mDelayedStopHandler.removeCallbacksAndMessages(null);
      cancelAlarms();
      setAlarmDelayedServiceStop();
      stopForeground(Service.STOP_FOREGROUND_REMOVE);
   }

   /** Read's the internal Antenna available state from the FM
    *  Device.
    */
   public void readInternalAntennaAvailable()
   {
      mInternalAntennaAvailable  = false;
      if (mReceiver != null)
      {
         mInternalAntennaAvailable = mReceiver.getInternalAntenna();
         Log.d(LOGTAG, "getInternalAntenna: " + mInternalAntennaAvailable);
      }
   }

   /*
    * By making this a static class with a WeakReference to the Service, we
    * ensure that the Service can be GCd even when the system process still
    * has a remote reference to the stub.
    */
   static class ServiceStub extends IFMRadioService.Stub
   {
      WeakReference<FMRadioService> mService;

      ServiceStub(FMRadioService service)
      {
         mService = new WeakReference<FMRadioService>(service);
      }

      public boolean fmOn() throws RemoteException
      {
         return(mService.get().fmOn());
      }

      public boolean fmOff() throws RemoteException
      {
         return(mService.get().fmOff(FM_OFF_FROM_APPLICATION));
      }

      public boolean fmRadioReset() throws RemoteException
      {
         return true;
      }

      public boolean isFmOn()
      {
         return(mService.get().isFmOn());
      }

      public boolean isAnalogModeEnabled()
      {
         return(mService.get().isAnalogModeEnabled());
      }

      public boolean isFmRecordingOn()
      {
         return(mService.get().isFmRecordingOn());
      }

      public boolean isRtPlusSupported()
      {
         return(mService.get().isRtPlusSupported());
      }

      public boolean isSpeakerEnabled()
      {
         return(mService.get().isSpeakerEnabled());
      }

      public boolean fmReconfigure()
      {
         return(mService.get().fmReconfigure());
      }

      public void registerCallbacks(IFMRadioServiceCallbacks cb) throws RemoteException
      {
         mService.get().registerCallbacks(cb);
      }

      public void unregisterCallbacks() throws RemoteException
      {
         mService.get().unregisterCallbacks();
      }

      public boolean mute()
      {
         return(mService.get().mute());
      }

      public boolean unMute()
      {
         return(mService.get().unMute());
      }

      public boolean isMuted()
      {
         return(mService.get().isMuted());
      }

      public boolean startRecording()
      {
         return(mService.get().startRecording());
      }

      public void stopRecording()
      {
         mService.get().stopRecording();
      }

      public boolean tune(int frequency)
      {
         return(mService.get().tune(frequency));
      }

      public boolean seek(boolean up)
      {
         return(mService.get().seek(up));
      }

      public void enableSpeaker(boolean speakerOn)
      {

          mService.get().enableSpeaker(speakerOn);
      }

      public boolean scan(int pty)
      {
         return(mService.get().scan(pty));
      }

      public boolean seekPI(int piCode)
      {
         return(mService.get().seekPI(piCode));
      }
      public boolean searchStrongStationList(int numStations)
      {
         return(mService.get().searchStrongStationList(numStations));
      }

      public boolean cancelSearch()
      {
         return(mService.get().cancelSearch());
      }

      public String getProgramService()
      {
         return(mService.get().getProgramService());
      }
      public String getRadioText()
      {
         return(mService.get().getRadioText());
      }
      public String getExtenRadioText()
      {
         return(mService.get().getExtenRadioText());
      }
      public int getProgramType()
      {
         return(mService.get().getProgramType());
      }
      public int getProgramID()
      {
         return(mService.get().getProgramID());
      }
      public boolean setLowPowerMode(boolean enable)
      {
         return(mService.get().setLowPowerMode(enable));
      }

      public int getPowerMode()
      {
         return(mService.get().getPowerMode());
      }
      public boolean enableAutoAF(boolean bEnable)
      {
         return(mService.get().enableAutoAF(bEnable));
      }
      public boolean enableStereo(boolean bEnable)
      {
         return(mService.get().enableStereo(bEnable));
      }
      public boolean isAntennaAvailable()
      {
         return(mService.get().isAntennaAvailable());
      }
      public boolean isWiredHeadsetAvailable()
      {
         return(mService.get().isWiredHeadsetAvailable());
      }
      public boolean isCallActive()
      {
          return(mService.get().isCallActive());
      }
      public int getRssi()
      {
          return (mService.get().getRssi());
      }
      public int getIoC()
      {
          return (mService.get().getIoC());
      }
      public int getMpxDcc()
      {
          return (mService.get().getMpxDcc());
      }
      public int getIntDet()
      {
          return (mService.get().getIntDet());
      }
      public void setHiLoInj(int inj)
      {
          mService.get().setHiLoInj(inj);
      }
      public void delayedStop(long duration, int nType)
      {
          mService.get().delayedStop(duration, nType);
      }
      public void cancelDelayedStop(int nType)
      {
          mService.get().cancelDelayedStop(nType);
      }
      public void requestFocus()
      {
          mService.get().requestFocus();
      }
      public int getSINR()
      {
          return (mService.get().getSINR());
      }
      public boolean setSinrSamplesCnt(int samplesCnt)
      {
          return (mService.get().setSinrSamplesCnt(samplesCnt));
      }
      public boolean setSinrTh(int sinr)
      {
          return (mService.get().setSinrTh(sinr));
      }
      public boolean setIntfDetLowTh(int intfLowTh)
      {
          return (mService.get().setIntfDetLowTh(intfLowTh));
      }
      public boolean getIntfDetLowTh()
      {
          return (mService.get().getIntfDetLowTh());
      }
      public boolean setIntfDetHighTh(int intfHighTh)
      {
          return (mService.get().setIntfDetHighTh(intfHighTh));
      }
      public boolean getIntfDetHighTh()
      {
          return (mService.get().getIntfDetHighTh());
      }
      public int getSearchAlgoType()
      {
          return (mService.get().getSearchAlgoType());
      }
      public boolean setSearchAlgoType(int searchType)
      {
          return (mService.get().setSearchAlgoType(searchType));
      }
      public int getSinrFirstStage()
      {
          return (mService.get().getSinrFirstStage());
      }
      public boolean setSinrFirstStage(int sinr)
      {
          return (mService.get().setSinrFirstStage(sinr));
      }
      public int getRmssiFirstStage()
      {
          return (mService.get().getRmssiFirstStage());
      }
      public boolean setRmssiFirstStage(int rmssi)
      {
          return (mService.get().setRmssiFirstStage(rmssi));
      }
      public int getCFOMeanTh()
      {
          return (mService.get().getCFOMeanTh());
      }
      public boolean setCFOMeanTh(int th)
      {
          return (mService.get().setCFOMeanTh(th));
      }
      public int getSinrSamplesCnt()
      {
          return (mService.get().getSinrSamplesCnt());
      }
      public int getSinrTh()
      {
          return (mService.get().getSinrTh());
      }
      public int getAfJmpRmssiTh()
      {
          return (mService.get().getAfJmpRmssiTh());
      }
      public boolean setAfJmpRmssiTh(int afJmpRmssiTh)
      {
          return (mService.get().setAfJmpRmssiTh(afJmpRmssiTh));
      }
      public int getGoodChRmssiTh()
      {
          return (mService.get().getGoodChRmssiTh());
      }
      public boolean setGoodChRmssiTh(int gdChRmssiTh)
      {
          return (mService.get().setGoodChRmssiTh(gdChRmssiTh));
      }
      public int getAfJmpRmssiSamplesCnt()
      {
          return (mService.get().getAfJmpRmssiSamplesCnt());
      }

      public String getSocName()
      {
          return (mService.get().getSocName());
      }

      public boolean setAfJmpRmssiSamplesCnt(int afJmpRmssiSmplsCnt)
      {
          return (mService.get().setAfJmpRmssiSamplesCnt(afJmpRmssiSmplsCnt));
      }
      public boolean setRxRepeatCount(int count)
      {
           return (mService.get().setRxRepeatCount(count));
      }
      public boolean getRxRepeatCount()
      {
           return (mService.get().getRxRepeatCount());
      }
      public long getRecordingStartTime()
      {
           return (mService.get().getRecordingStartTime());
      }
      public boolean isSleepTimerActive()
      {
           return (mService.get().isSleepTimerActive());
      }
      public boolean isSSRInProgress()
      {
         return(mService.get().isSSRInProgress());
      }
      public boolean isA2DPConnected()
      {
         return(mService.get().isA2DPConnected());
      }

      public int getExtenCountryCode()
      {
         return(mService.get().getExtenCountryCode());
      }

      public boolean getFmStatsProp()
      {
          return (mService.get().getFmStatsProp());
      }

      public void restoreDefaults()
      {
         mService.get().restoreDefaults();
      }
   }
   private final IBinder mBinder = new ServiceStub(this);

   private boolean waitForEvent() {
       boolean status = false;

       synchronized (mEventWaitLock) {
           Log.d(LOGTAG, "waiting for event");
           try {
               if (mEventReceived == false)
                   mEventWaitLock.wait(RADIO_TIMEOUT);
               if (mEventReceived == true)
                   status = true;
           } catch (IllegalMonitorStateException e) {
               Log.e(LOGTAG, "Exception caught while waiting for event");
               e.printStackTrace();
           } catch (InterruptedException ex) {
               Log.e(LOGTAG, "Exception caught while waiting for event");
               ex.printStackTrace();
           }
       }
       return status;
   }

   private boolean waitForFWEvent() {
       boolean status = false;

       synchronized (mEventWaitLock) {
           Log.d(LOGTAG, "waiting for FW event");
           try {
               if (mEventReceived == false)
                   mEventWaitLock.wait(FW_TIMEOUT);
               if (mEventReceived == true) {
                   status = true;
               }
           } catch (IllegalMonitorStateException e) {
               Log.e(LOGTAG, "Exception caught while waiting for event");
               e.printStackTrace();
           } catch (InterruptedException ex) {
               Log.e(LOGTAG, "Exception caught while waiting for event");
               ex.printStackTrace();
           }
       }
       return status;
   }

   private boolean enableSlimbus(int flag) {
       Log.d(LOGTAG, "enableSlimbus");
       boolean bStatus = false;
       if (mReceiver != null)
       {
           // Send command to enable/disable FM core
           mEventReceived = false;
           mReceiver.EnableSlimbus(flag);
           bStatus = waitForFWEvent();
       }
       return bStatus;
   }

  /*
   * Turn ON FM: Powers up FM hardware, and initializes the FM module
   *                                                                                 .
   * @return true if fm Enable api was invoked successfully, false if the api failed.
   */
   private boolean fmTurnOnSequence() {
       boolean bStatus = false;
       AudioManager audioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
       if ((audioManager != null) & (false == mPlaybackInProgress)) {
           Log.d(LOGTAG, "mAudioManager.setFmRadioOn = true \n" );
           int state =  getCallState();
           if (TelephonyManager.CALL_STATE_IDLE != getCallState())
           {
               fmActionOnCallState(state);
           } else {
               startFM(); // enable FM Audio only when Call is IDLE
           }
           Log.d(LOGTAG, "mAudioManager.setFmRadioOn done \n" );
       }

       // This sets up the FM radio device
       FmConfig config = FmSharedPreferences.getFMConfiguration();
       Log.d(LOGTAG, "fmOn: RadioBand   :"+ config.getRadioBand());
       Log.d(LOGTAG, "fmOn: Emphasis    :"+ config.getEmphasis());
       Log.d(LOGTAG, "fmOn: ChSpacing   :"+ config.getChSpacing());
       Log.d(LOGTAG, "fmOn: RdsStd      :"+ config.getRdsStd());
       Log.d(LOGTAG, "fmOn: LowerLimit  :"+ config.getLowerLimit());
       Log.d(LOGTAG, "fmOn: UpperLimit  :"+ config.getUpperLimit());

       // Enable FM receiver
       mEventReceived = false;
       bStatus = mReceiver.enable(FmSharedPreferences.getFMConfiguration(), this);
       bStatus = waitForEvent();
       mReceiver.setRawRdsGrpMask();

       Log.d(LOGTAG, "mReceiver.enable done, Status :" +  bStatus);

       if (bStatus == true) {
           /* Put the hardware into normal mode */
           bStatus = setLowPowerMode(false);
           Log.d(LOGTAG, "setLowPowerMode done, Status :" +  bStatus);

           if (mReceiver != null) {
               bStatus = mReceiver.registerRdsGroupProcessing(FmReceiver.FM_RX_RDS_GRP_RT_EBL|
                                                           FmReceiver.FM_RX_RDS_GRP_PS_EBL|
                                                           FmReceiver.FM_RX_RDS_GRP_AF_EBL|
                                                           FmReceiver.FM_RX_RDS_GRP_PS_SIMPLE_EBL|
                                                           FmReceiver.FM_RX_RDS_GRP_ECC_EBL|
                                                           FmReceiver.FM_RX_RDS_GRP_PTYN_EBL|
                                                           FmReceiver.FM_RX_RDS_GRP_RT_PLUS_EBL);
                Log.d(LOGTAG, "registerRdsGroupProcessing done, Status :" +  bStatus);
            }
            bStatus = enableAutoAF(FmSharedPreferences.getAutoAFSwitch());
            Log.d(LOGTAG, "enableAutoAF done, Status :" +  bStatus);

            /* There is no internal Antenna*/
            bStatus = mReceiver.setInternalAntenna(false);
            Log.d(LOGTAG, "setInternalAntenna done, Status :" +  bStatus);

            /* Read back to verify the internal Antenna mode*/
            readInternalAntennaAvailable();

            startNotification();
            bStatus = true;
       } else {
            mReceiver = null; // as enable failed no need to disable
                              // failure of enable can be because handle
                              // already open which gets effected if
                              // we disable
            audioManager.abandonAudioFocusRequest(mGainFocusReq);
            stop();
       }

       return bStatus;
   }

  /*
   * Turn ON FM: Powers up FM hardware, and initializes the FM module
   *                                                                                 .
   * @return true if fm Enable api was invoked successfully, false if the api failed.
   */
   private boolean fmOn () {
      boolean bStatus = false;
      mWakeLock.acquire(10*1000);

      if (TelephonyManager.CALL_STATE_IDLE != getCallState()) {
         return bStatus;
      }

      if (mReceiver == null)
      {
         try {
            mReceiver = new FmReceiver(FMRADIO_DEVICE_FD_STRING, fmCallbacks);
            isfmOffFromApplication = false;
         }
         catch (InstantiationException e)
         {
            throw new RuntimeException("FmReceiver service not available!");
         }
      }

      if (mReceiver != null)
      {
         if (isFmOn())
         {
            /* FM Is already on,*/
            bStatus = true;
            Log.d(LOGTAG, "mReceiver.already enabled");
         }
         else
         {
             enableSlimbus(ENABLE_SLIMBUS_DATA_PORT);
             bStatus = fmTurnOnSequence();
                /* reset SSR flag */
           mIsSSRInProgressFromActivity = false;
         }

         if (mReceiver != null)
            mFmStats = mReceiver.getFmStatsProp();
      }
      return(bStatus);
   }

  /*
   * Turn OFF FM Operations: This disables all the current FM operations             .
   */
   private void fmOperationsOff() {
     // disable audio path
      AudioManager audioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
      if(audioManager != null)
      {
         Log.d(LOGTAG, "audioManager.setFmRadioOn = false \n" );
         stopFM();
         unMute();
         // If call is active, we will use audio focus to resume fm after call ends.
         // So don't abandon audiofocus automatically
         if (getCallState() == TelephonyManager.CALL_STATE_IDLE) {
             audioManager.abandonAudioFocusRequest(mGainFocusReq);
             mStoppedOnFocusLoss = true;
         }
         //audioManager.setParameters("FMRadioOn=false");
         Log.d(LOGTAG, "audioManager.setFmRadioOn false done \n" );
      }
      // stop recording
      if (isFmRecordingOn())
      {
          stopRecording();
          try {
               Thread.sleep(300);
          } catch (Exception ex) {
               Log.d( LOGTAG, "RunningThread InterruptedException");
               return;
          }
      }

      if (isMuted() == true)
          unMute();

      if (isAnalogModeEnabled()) {
              misAnalogPathEnabled = false;
      }
   }


  /*
   * Reset (OFF) FM Operations: This resets all the current FM operations             .
   */
   private void fmOperationsReset() {
      AudioManager audioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
      if(audioManager != null)
      {
         Log.d(LOGTAG, "audioManager.setFmRadioOn = false \n" );
         resetFM();
         //audioManager.setParameters("FMRadioOn=false");
         Log.d(LOGTAG, "audioManager.setFmRadioOn false done \n" );
      }

      if (isAnalogModeEnabled()) {
              misAnalogPathEnabled = false;
      }

      if ( mSpeakerPhoneOn) {
          mSpeakerPhoneOn = false;
      }
   }

   private boolean fmOffImpl() {
      boolean bStatus=false;

      fmOperationsOff();
      stop();
      try {
          Thread.sleep(200);
      } catch (Exception ex) {
          Log.d( LOGTAG, "RunningThread InterruptedException");
      }

      // This will disable the FM radio device
      if (mReceiver != null)
      {
         if(mReceiver.getFMState() == mReceiver.FMState_Srch_InProg) {
             Log.d(LOGTAG, "Cancelling the on going search operation prior to disabling FM");
             mEventReceived = false;
             cancelSearch();
             waitForEvent();
         }
         mEventReceived = false;
         bStatus = mReceiver.disable(this);
         if (bStatus &&
                 (mReceiver.getFMState() == mReceiver.subPwrLevel_FMTurning_Off)) {
             bStatus = waitForEvent();
         }
         mReceiver = null;
      }
      return(bStatus);
   }
  /*
   * Turn OFF FM: Disable the FM Host and hardware                                  .
   *                                                                                 .
   * @return true if fm Disable api was invoked successfully, false if the api failed.
   */
   private boolean fmOff() {
       boolean ret = false;
       if (mReceiver != null) {
           ret = fmOffImpl();
       }
       mWakeLock.release();
       return ret;
   }

   private boolean fmOff(int off_from) {
       if (off_from == FM_OFF_FROM_APPLICATION || off_from == FM_OFF_FROM_ANTENNA) {
           Log.d(LOGTAG, "FM application close button pressed or antenna removed");
           mSession.setActive(false);
       }
       if(off_from == FM_OFF_FROM_APPLICATION) {
           Log.d(LOGTAG, "FM off from Application");
           isfmOffFromApplication = true;
       }

       //stop Notification
       stopNotification();

       return fmOff();
   }
  /*
   * Turn OFF FM: Disable the FM Host when hardware resets asynchronously            .
   *                                                                                 .
   * @return true if fm Reset api was invoked successfully, false if the api failed  .
   */
   private boolean fmRadioReset() {
      boolean bStatus=false;

      Log.v(LOGTAG, "fmRadioReset");

      fmOperationsReset();

      // This will reset the FM radio receiver
      if (mReceiver != null)
      {
         mReceiver = null;
      }
      stop();
      return(bStatus);
   }

   public boolean isSSRInProgress() {
      return mIsSSRInProgress;
   }

   public boolean isA2DPConnected() {
       return (mA2dpConnected);
   }
   /* Returns whether FM hardware is ON.
    *
    * @return true if FM was tuned, searching. (at the end of
    * the search FM goes back to tuned).
    *
    */
   public boolean isFmOn() {
      return mFMOn;
   }

   /* Returns true if Analog Path is enabled */
   public boolean isAnalogModeEnabled() {
         return misAnalogPathEnabled;
   }

   public boolean isFmRecordingOn() {
      return mFmRecordingOn;
   }

   public boolean isRtPlusSupported() {
      return mRtPlusSupport;
   }

   public boolean isSpeakerEnabled() {
      return mSpeakerPhoneOn;
   }

   public void enableSpeaker(boolean speakerOn) {
       Log.d(LOGTAG, "speakerOn: " + speakerOn);
       int mAudioDeviceType;
       String outputDevice;
       if (isCallActive())
           return;

       mSpeakerPhoneOn = speakerOn;
       enableSlimbus(DISABLE_SLIMBUS_DATA_PORT);

       if (speakerOn == false) {
           mAudioDevice = AudioDeviceInfo.TYPE_WIRED_HEADPHONES;
           outputDevice = "WiredHeadset";
       } else {
           mAudioDevice = AudioDeviceInfo.TYPE_BUILTIN_SPEAKER;
           outputDevice = "Speaker";
       }
       mAudioDeviceType = mAudioDevice | AudioSystem.DEVICE_OUT_FM;
       AudioManager audioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
       String keyValPairs = new String("fm_routing="+mAudioDeviceType);
       Log.d(LOGTAG, "keyValPairs = "+keyValPairs);
       audioManager.setParameters(keyValPairs);
       enableSlimbus(ENABLE_SLIMBUS_DATA_PORT);
   }
  /*
   *  ReConfigure the FM Setup parameters
   *  - Band
   *  - Channel Spacing (50/100/200 KHz)
   *  - Emphasis (50/75)
   *  - Frequency limits
   *  - RDS/RBDS standard
   *
   * @return true if configure api was invoked successfully, false if the api failed.
   */
   public boolean fmReconfigure() {
      boolean bStatus=false;
      Log.d(LOGTAG, "fmReconfigure");
      if (mReceiver != null)
      {
         // This sets up the FM radio device
         FmConfig config = FmSharedPreferences.getFMConfiguration();
         Log.d(LOGTAG, "RadioBand   :"+ config.getRadioBand());
         Log.d(LOGTAG, "Emphasis    :"+ config.getEmphasis());
         Log.d(LOGTAG, "ChSpacing   :"+ config.getChSpacing());
         Log.d(LOGTAG, "RdsStd      :"+ config.getRdsStd());
         Log.d(LOGTAG, "LowerLimit  :"+ config.getLowerLimit());
         Log.d(LOGTAG, "UpperLimit  :"+ config.getUpperLimit());
         bStatus = mReceiver.configure(config);
      }
      return(bStatus);
   }

   /*
    * Register UI/Activity Callbacks
    */
   public void registerCallbacks(IFMRadioServiceCallbacks cb)
   {
      mCallbacks = cb;
   }

   /*
    *  unRegister UI/Activity Callbacks
    */
   public void unregisterCallbacks()
   {
      mCallbacks=null;
   }

  /*
   *  Mute FM Hardware (SoC)
   * @return true if set mute mode api was invoked successfully, false if the api failed.
   */
   public boolean mute() {
      boolean bCommandSent=true;
      if(isMuted())
          return bCommandSent;
      if(isCallActive())
         return false;
      AudioManager audioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
      Log.d(LOGTAG, "mute:");
      if (audioManager != null)
      {
         mMuted = true;
         audioManager.setParameters("fm_mute=1");
         if (mAudioTrack != null)
             mAudioTrack.setVolume(0.0f);
      }
      return bCommandSent;
   }

   /*
   *  UnMute FM Hardware (SoC)
   * @return true if set mute mode api was invoked successfully, false if the api failed.
   */
   public boolean unMute() {
      boolean bCommandSent=true;
      if(!isMuted())
          return bCommandSent;
      if(isCallActive())
         return false;
      Log.d(LOGTAG, "unMute:");
      AudioManager audioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
      if (audioManager != null)
      {
         mMuted = false;
         audioManager.setParameters("fm_mute=0");
         if (mAudioTrack != null)
             mAudioTrack.setVolume(1.0f);
         if (mResumeAfterCall)
         {
             //We are unmuting FM in a voice call. Need to enable FM audio routing.
             startFM();
         }
      }
      return bCommandSent;
   }

   /* Returns whether FM Hardware(Soc) Audio is Muted.
    *
    * @return true if FM Audio is muted, false if not muted.
    *
    */
   public boolean isMuted() {
      return mMuted;
   }

   /* Tunes to the specified frequency
    *
    * @return true if Tune command was invoked successfully, false if not muted.
    *  Note: Callback FmRxEvRadioTuneStatus will be called when the tune
    *        is complete
    */
   public boolean tune(int frequency) {
      boolean bCommandSent=false;
      double doubleFrequency = frequency/1000.00;

      Log.d(LOGTAG, "tuneRadio:  " + doubleFrequency);
      if (mReceiver != null)
      {
         mReceiver.setStation(frequency);
         bCommandSent = true;
      }
      return bCommandSent;
   }

   /* Seeks (Search for strong station) to the station in the direction specified
    * relative to the tuned station.
    * boolean up: true - Search in the forward direction.
    *             false - Search in the backward direction.
    * @return true if Seek command was invoked successfully, false if not muted.
    *  Note: 1. Callback FmRxEvSearchComplete will be called when the Search
    *        is complete
    *        2. Callback FmRxEvRadioTuneStatus will also be called when tuned to a station
    *        at the end of the Search or if the seach was cancelled.
    */
   public boolean seek(boolean up)
   {
      boolean bCommandSent=false;
      if (mReceiver != null)
      {
         if (up == true)
         {
            Log.d(LOGTAG, "seek:  Up");
            mReceiver.searchStations(FmReceiver.FM_RX_SRCH_MODE_SEEK,
                                             FmReceiver.FM_RX_DWELL_PERIOD_1S,
                                             FmReceiver.FM_RX_SEARCHDIR_UP);
         }
         else
         {
            Log.d(LOGTAG, "seek:  Down");
            mReceiver.searchStations(FmReceiver.FM_RX_SRCH_MODE_SEEK,
                                             FmReceiver.FM_RX_DWELL_PERIOD_1S,
                                             FmReceiver.FM_RX_SEARCHDIR_DOWN);
         }
         bCommandSent = true;
      }
      return bCommandSent;
   }

   /* Scan (Search for station with a "preview" of "n" seconds)
    * FM Stations. It always scans in the forward direction relative to the
    * current tuned station.
    * int pty: 0 or a reserved PTY value- Perform a "strong" station search of all stations.
    *          Valid/Known PTY - perform RDS Scan for that pty.
    *
    * @return true if Scan command was invoked successfully, false if not muted.
    *  Note: 1. Callback FmRxEvRadioTuneStatus will be called when tuned to various stations
    *           during the Scan.
    *        2. Callback FmRxEvSearchComplete will be called when the Search
    *        is complete
    *        3. Callback FmRxEvRadioTuneStatus will also be called when tuned to a station
    *        at the end of the Search or if the seach was cancelled.
    *
    */
   public boolean scan(int pty)
   {
      boolean bCommandSent=false;
      if (mReceiver != null)
      {
         Log.d(LOGTAG, "scan:  PTY: " + pty);
         if(FmSharedPreferences.isRBDSStd())
         {
            /* RBDS : Validate PTY value?? */
            if( ((pty  > 0) && (pty  <= 23)) || ((pty  >= 29) && (pty  <= 31)) )
            {
               bCommandSent = mReceiver.searchStations(FmReceiver.FM_RX_SRCHRDS_MODE_SCAN_PTY,
                                                       FmReceiver.FM_RX_DWELL_PERIOD_2S,
                                                       FmReceiver.FM_RX_SEARCHDIR_UP,
                                                       pty,
                                                       0);
            }
            else
            {
               bCommandSent = mReceiver.searchStations(FmReceiver.FM_RX_SRCH_MODE_SCAN,
                                                FmReceiver.FM_RX_DWELL_PERIOD_2S,
                                                FmReceiver.FM_RX_SEARCHDIR_UP);
            }
         }
         else
         {
            /* RDS : Validate PTY value?? */
            if( (pty  > 0) && (pty  <= 31) )
            {
               bCommandSent = mReceiver.searchStations(FmReceiver.FM_RX_SRCHRDS_MODE_SCAN_PTY,
                                                          FmReceiver.FM_RX_DWELL_PERIOD_2S,
                                                          FmReceiver.FM_RX_SEARCHDIR_UP,
                                                          pty,
                                                          0);
            }
            else
            {
               bCommandSent = mReceiver.searchStations(FmReceiver.FM_RX_SRCH_MODE_SCAN,
                                                FmReceiver.FM_RX_DWELL_PERIOD_2S,
                                                FmReceiver.FM_RX_SEARCHDIR_UP);
            }
         }
      }
      return bCommandSent;
   }

   /* Search for the 'numStations' number of strong FM Stations.
    *
    * It searches in the forward direction relative to the current tuned station.
    * int numStations: maximum number of stations to search.
    *
    * @return true if Search command was invoked successfully, false if not muted.
    *  Note: 1. Callback FmRxEvSearchListComplete will be called when the Search
    *        is complete
    *        2. Callback FmRxEvRadioTuneStatus will also be called when tuned to
    *        the previously tuned station.
    */
   public boolean searchStrongStationList(int numStations)
   {
      boolean bCommandSent=false;
      if (mReceiver != null)
      {
         Log.d(LOGTAG, "searchStrongStationList:  numStations: " + numStations);
         bCommandSent = mReceiver.searchStationList(FmReceiver.FM_RX_SRCHLIST_MODE_STRONG,
                                                    FmReceiver.FM_RX_SEARCHDIR_UP,
                                                    numStations,
                                                    0);
      }
      return bCommandSent;
   }

   /* Search for the FM Station that matches the RDS PI (Program Identifier) code.
    * It always scans in the forward direction relative to the current tuned station.
    * int piCode: PI Code of the station to search.
    *
    * @return true if Search command was invoked successfully, false if not muted.
    *  Note: 1. Callback FmRxEvSearchComplete will be called when the Search
    *        is complete
    *        2. Callback FmRxEvRadioTuneStatus will also be called when tuned to a station
    *        at the end of the Search or if the seach was cancelled.
    */
   public boolean seekPI(int piCode)
   {
      boolean bCommandSent=false;
      if (mReceiver != null)
      {
         Log.d(LOGTAG, "seekPI:  piCode: " + piCode);
         bCommandSent = mReceiver.searchStations(FmReceiver.FM_RX_SRCHRDS_MODE_SEEK_PI,
                                                            FmReceiver.FM_RX_DWELL_PERIOD_1S,
                                                            FmReceiver.FM_RX_SEARCHDIR_UP,
                                                            0,
                                                            piCode
                                                            );
      }
      return bCommandSent;
   }


  /* Cancel any ongoing Search (Seek/Scan/SearchStationList).
   *
   * @return true if Search command was invoked successfully, false if not muted.
   *  Note: 1. Callback FmRxEvSearchComplete will be called when the Search
   *        is complete/cancelled.
   *        2. Callback FmRxEvRadioTuneStatus will also be called when tuned to a station
   *        at the end of the Search or if the seach was cancelled.
   */
   public boolean cancelSearch()
   {
      boolean bCommandSent=false;
      if (mReceiver != null)
      {
         Log.d(LOGTAG, "cancelSearch");
         bCommandSent = mReceiver.cancelSearch();
      }
      return bCommandSent;
   }

   /* Retrieves the RDS Program Service (PS) String.
    *
    * @return String - RDS PS String.
    *  Note: 1. This is a synchronous call that should typically called when
    *           Callback FmRxEvRdsPsInfo is invoked.
    *        2. Since PS contains multiple fields, this Service reads all the fields and "caches"
    *        the values and provides this helper routine for the Activity to get only the information it needs.
    *        3. The "cached" data fields are always "cleared" when the tune status changes.
    */
   public String getProgramService() {
      String str = "";
      if (mFMRxRDSData != null)
      {
         str = mFMRxRDSData.getPrgmServices();
         if(str == null)
         {
            str= "";
         }
      }
      Log.d(LOGTAG, "Program Service: [" + str + "]");
      return str;
   }

   /* Retrieves the RDS Radio Text (RT) String.
    *
    * @return String - RDS RT String.
    *  Note: 1. This is a synchronous call that should typically called when
    *           Callback FmRxEvRdsRtInfo is invoked.
    *        2. Since RT contains multiple fields, this Service reads all the fields and "caches"
    *        the values and provides this helper routine for the Activity to get only the information it needs.
    *        3. The "cached" data fields are always "cleared" when the tune status changes.
    */
   public String getRadioText() {
      String str = "";
      if (mFMRxRDSData != null)
      {
         str = mFMRxRDSData.getRadioText();
         if(str == null)
         {
            str= "";
         }
      }
      Log.d(LOGTAG, "Radio Text: [" + str + "]");
      return str;
   }

   public String getExtenRadioText() {
      String str = "";
      if (mFMRxRDSData != null)
      {
         str = mFMRxRDSData.getERadioText();
         if(str == null)
         {
            str= "";
         }
      }
      Log.d(LOGTAG, "eRadio Text:[" + str +"]");
      return str;
   }
   public int  getExtenCountryCode() {
      int val = 0;
      if (mFMRxRDSData != null)
      {
         val = mFMRxRDSData.getECountryCode();
      }
      Log.d(LOGTAG, "eCountry Code :[" + val +"]");
      return val;
   }

   /* Retrieves the RDS Program Type (PTY) code.
    *
    * @return int - RDS PTY code.
    *  Note: 1. This is a synchronous call that should typically called when
    *           Callback FmRxEvRdsRtInfo and or FmRxEvRdsPsInfo is invoked.
    *        2. Since RT/PS contains multiple fields, this Service reads all the fields and "caches"
    *        the values and provides this helper routine for the Activity to get only the information it needs.
    *        3. The "cached" data fields are always "cleared" when the tune status changes.
    */
   public int getProgramType() {
      int pty = -1;
      if (mFMRxRDSData != null)
      {
         pty = mFMRxRDSData.getPrgmType();
      }
      Log.d(LOGTAG, "PTY: [" + pty + "]");
      return pty;
   }

   /* Retrieves the RDS Program Identifier (PI).
    *
    * @return int - RDS PI code.
    *  Note: 1. This is a synchronous call that should typically called when
    *           Callback FmRxEvRdsRtInfo and or FmRxEvRdsPsInfo is invoked.
    *        2. Since RT/PS contains multiple fields, this Service reads all the fields and "caches"
    *        the values and provides this helper routine for the Activity to get only the information it needs.
    *        3. The "cached" data fields are always "cleared" when the tune status changes.
    */
   public int getProgramID() {
      int pi = -1;
      if (mFMRxRDSData != null)
      {
         pi = mFMRxRDSData.getPrgmId();
      }
      Log.d(LOGTAG, "PI: [" + pi + "]");
      return pi;
   }

   /* Set the FM Power Mode on the FM hardware SoC.
    * Typically used when UI/Activity is in the background, so the Host is interrupted less often.
    *
    * boolean bLowPower: true: Enable Low Power mode on FM hardware.
    *                    false: Disable Low Power mode on FM hardware. (Put into normal power mode)
    * @return true if set power mode api was invoked successfully, false if the api failed.
    */
   public boolean setLowPowerMode(boolean bLowPower)
   {
      boolean bCommandSent=false;
      if (mReceiver != null)
      {
         Log.d(LOGTAG, "setLowPowerMode: " + bLowPower);
         if(bLowPower)
         {
            bCommandSent = mReceiver.setPowerMode(FmReceiver.FM_RX_LOW_POWER_MODE);
         }
         else
         {
            bCommandSent = mReceiver.setPowerMode(FmReceiver.FM_RX_NORMAL_POWER_MODE);
         }
      }
      return bCommandSent;
   }

   /* Get the FM Power Mode on the FM hardware SoC.
    *
    * @return the device power mode.
    */
   public int getPowerMode()
   {
      int powerMode=FmReceiver.FM_RX_NORMAL_POWER_MODE;
      if (mReceiver != null)
      {
         powerMode = mReceiver.getPowerMode();
         Log.d(LOGTAG, "getLowPowerMode: " + powerMode);
      }
      return powerMode;
   }

  /* Set the FM module to auto switch to an Alternate Frequency for the
   * station if one the signal strength of that frequency is stronger than the
   * current tuned frequency.
   *
   * boolean bEnable: true: Auto switch to stronger alternate frequency.
   *                  false: Do not switch to alternate frequency.
   *
   * @return true if set Auto AF mode api was invoked successfully, false if the api failed.
   *  Note: Callback FmRxEvRadioTuneStatus will be called when tune
   *        is complete to a different frequency.
   */
   public boolean enableAutoAF(boolean bEnable)
   {
      boolean bCommandSent=false;
      if (mReceiver != null)
      {
         Log.d(LOGTAG, "enableAutoAF: " + bEnable);
         bCommandSent = mReceiver.enableAFjump(bEnable);
      }
      return bCommandSent;
   }

   /* Set the FM module to Stereo Mode or always force it to Mono Mode.
    * Note: The stereo mode will be available only when the station is broadcasting
    * in Stereo mode.
    *
    * boolean bEnable: true: Enable Stereo Mode.
    *                  false: Always stay in Mono Mode.
    *
    * @return true if set Stereo mode api was invoked successfully, false if the api failed.
    */
   public boolean enableStereo(boolean bEnable)
   {
      boolean bCommandSent=false;
      synchronized(mReceiverLock) {
          if (mReceiver != null)
          {
             Log.d(LOGTAG, "enableStereo: " + bEnable);
             bCommandSent = mReceiver.setStereoMode(bEnable);
          }
      }
      return bCommandSent;
   }

   /** Determines if an internal Antenna is available.
    *  Returns the cached value initialized on FMOn.
    *
    * @return true if internal antenna is available or wired
    *         headset is plugged in, false if internal antenna is
    *         not available and wired headset is not plugged in.
    */
   public boolean isAntennaAvailable()
   {
      boolean bAvailable = false;
      if ((mInternalAntennaAvailable) || (mHeadsetPlugged) )
      {
         bAvailable = true;
      }
      return bAvailable;
   }

   public static long getAvailableSpace() {
       String state = Environment.getExternalStorageState();
       Log.d(LOGTAG, "External storage state=" + state);
       if (Environment.MEDIA_CHECKING.equals(state)) {
           return PREPARING;
       }
       if (!Environment.MEDIA_MOUNTED.equals(state)) {
           return UNAVAILABLE;
       }

       try {
            File sampleDir = Environment.getExternalStorageDirectory();
            StatFs stat = new StatFs(sampleDir.getAbsolutePath());
            return stat.getAvailableBlocksLong() * stat.getBlockSizeLong();
       } catch (Exception e) {
            Log.i(LOGTAG, "Fail to access external storage", e);
       }
       return UNKNOWN_SIZE;
  }

   private boolean updateAndShowStorageHint() {
       mStorageSpace = getAvailableSpace();
       return showStorageHint();
   }

   private boolean showStorageHint() {
       String errorMessage = null;
       if (mStorageSpace == UNAVAILABLE) {
           errorMessage = getString(R.string.no_storage);
       } else if (mStorageSpace == PREPARING) {
           errorMessage = getString(R.string.preparing_sd);
       } else if (mStorageSpace == UNKNOWN_SIZE) {
           errorMessage = getString(R.string.access_sd_fail);
       } else if (mStorageSpace < LOW_STORAGE_THRESHOLD) {
           errorMessage = getString(R.string.spaceIsLow_content);
       }

       if (errorMessage != null) {
           Toast.makeText(this, errorMessage,
                    Toast.LENGTH_LONG).show();
           return false;
       }
       return true;
   }

   /** Determines if a Wired headset is plugged in. Returns the
    *  cached value initialized on broadcast receiver
    *  initialization.
    *
    * @return true if wired headset is plugged in, false if wired
    *         headset is not plugged in.
    */
   public boolean isWiredHeadsetAvailable()
   {
      return (mHeadsetPlugged);
   }
   public boolean isCallActive()
   {
       //Non-zero: Call state is RINGING or OFFHOOK on the available subscriptions
       //zero: Call state is IDLE on all the available subscriptions
       if(0 != getCallState()) return true;
       return false;
   }
   public int getCallState()
   {
       return mCallStatus;
   }

   public void clearStationInfo() {
       if(mFMRxRDSData != null) {
          mRtPlusSupport = false;
          mFMRxRDSData.setRadioText("");
          mFMRxRDSData.setPrgmId(0);
          mFMRxRDSData.setPrgmType(0);
          mFMRxRDSData.setPrgmServices("");
          mFMRxRDSData.setERadioText("");
          mFMRxRDSData.setTagValue("", 1);
          mFMRxRDSData.setTagValue("", 2);
          mFMRxRDSData.setTagCode((byte)0, 1);
          mFMRxRDSData.setTagCode((byte)0, 2);
          Log.d(LOGTAG, "clear tags data");
          FmSharedPreferences.clearTags();
       }
   }

   /* Receiver callbacks back from the FM Stack */
   FmRxEvCallbacksAdaptor fmCallbacks = new FmRxEvCallbacksAdaptor()
   {
      public void FmRxEvEnableReceiver()
      {
         Log.d(LOGTAG, "FmRxEvEnableReceiver");
         if (mReceiver != null) {
             synchronized(mEventWaitLock) {
                 mEventReceived = true;
                 mEventWaitLock.notify();
             }
         }
      }
      public void FmRxEvDisableReceiver()
      {
         Log.d(LOGTAG, "FmRxEvDisableReceiver");
         mFMOn = false;
         FmSharedPreferences.clearTags();
         synchronized (mEventWaitLock) {
             mEventReceived = true;
             mEventWaitLock.notify();
         }
      }
      public void FmRxEvRadioReset()
      {
         boolean bStatus;
         Log.d(LOGTAG, "FmRxEvRadioReset");
         if(isFmOn()) {
             // Received radio reset event while FM is ON
             Log.d(LOGTAG, "FM Radio reset");
             fmRadioReset();
             try
             {
                /* Notify the UI/Activity, only if the service is "bound"
                   by an activity and if Callbacks are registered
                */
                if((mServiceInUse) && (mCallbacks != null) )
                {
                    mIsSSRInProgressFromActivity = true;
                    mCallbacks.onRadioReset();
                } else {
                    Log.d(LOGTAG, "Activity is not in foreground, turning on from service");
                    if (isAntennaAvailable())
                    {
                        mIsSSRInProgress = true;
                        try {
                             Thread.sleep(2000);
                        } catch (Exception ex) {
                            Log.d( LOGTAG, "RunningThread InterruptedException in RadioReset");
                        }
                        bStatus = fmOn();
                        if(bStatus)
                        {
                             bStatus = tune(FmSharedPreferences.getTunedFrequency());
                             if(!bStatus)
                               Log.e(LOGTAG, "Tuning after SSR from service failed");
                        } else {
                           Log.e(LOGTAG, "Turning on after SSR from service failed");
                        }
                        mIsSSRInProgress = false;
                    }
                }
             }
             catch (RemoteException e)
             {
                e.printStackTrace();
             }
         }
      }
      public void FmRxEvConfigReceiver()
      {
         Log.d(LOGTAG, "FmRxEvConfigReceiver");
      }
      public void FmRxEvMuteModeSet()
      {
         Log.d(LOGTAG, "FmRxEvMuteModeSet");
      }
      public void FmRxEvStereoModeSet()
      {
         Log.d(LOGTAG, "FmRxEvStereoModeSet");
      }
      public void FmRxEvRadioStationSet()
      {
         Log.d(LOGTAG, "FmRxEvRadioStationSet");
      }
      public void FmRxEvPowerModeSet()
      {
         Log.d(LOGTAG, "FmRxEvPowerModeSet");
      }
      public void FmRxEvSetSignalThreshold()
      {
         Log.d(LOGTAG, "FmRxEvSetSignalThreshold");
      }

      public void FmRxEvRadioTuneStatus(int frequency)
      {
         Log.d(LOGTAG, "FmRxEvRadioTuneStatus: Tuned Frequency: " +frequency);
         try
         {
            FmSharedPreferences.setTunedFrequency(frequency);
            mPrefs.Save();
            //Log.d(LOGTAG, "Call mCallbacks.onTuneStatusChanged");
            /* Since the Tuned Status changed, clear out the RDSData cached */
            if(mReceiver != null) {
               clearStationInfo();
            }
            if(mCallbacks != null)
            {
               mCallbacks.onTuneStatusChanged();
            }
            /* Update the frequency in the StatusBar's Notification */
            startNotification();
            enableStereo(FmSharedPreferences.getAudioOutputMode());
         }
         catch (RemoteException e)
         {
            e.printStackTrace();
         }
      }

      public void FmRxEvStationParameters()
      {
         Log.d(LOGTAG, "FmRxEvStationParameters");
      }

      public void FmRxEvRdsLockStatus(boolean bRDSSupported)
      {
         Log.d(LOGTAG, "FmRxEvRdsLockStatus: " + bRDSSupported);
         try
         {
            if(mCallbacks != null)
            {
               mCallbacks.onStationRDSSupported(bRDSSupported);
            }
         }
         catch (RemoteException e)
         {
            e.printStackTrace();
         }
      }

      public void FmRxEvStereoStatus(boolean stereo)
      {
         Log.d(LOGTAG, "FmRxEvStereoStatus: " + stereo);
         try
         {
            if(mCallbacks != null)
            {
               mCallbacks.onAudioUpdate(stereo);
            }
         }
         catch (RemoteException e)
         {
            e.printStackTrace();
         }
      }
      public void FmRxEvServiceAvailable(boolean signal)
      {
         Log.d(LOGTAG, "FmRxEvServiceAvailable");
         if(signal) {
             Log.d(LOGTAG, "FmRxEvServiceAvailable: Tuned frequency is above signal threshold level");
         }
         else {
             Log.d(LOGTAG, "FmRxEvServiceAvailable: Tuned frequency is below signal threshold level");
         }
      }
      public void FmRxEvGetSignalThreshold(int val, int status)
      {
         Log.d(LOGTAG, "FmRxEvGetSignalThreshold");

         if (mCallbacks != null) {
             try {
                 mCallbacks.getSigThCb(val, status);
             } catch (RemoteException e) {
                 Log.e(LOGTAG, "FmRxEvGetSignalThreshold: Exception:" + e.toString());
             }
         }
      }

      public void FmRxEvGetChDetThreshold(int val, int status)
      {
          Log.e(LOGTAG, "FmRxEvGetChDetThreshold");
          if (mCallbacks != null) {
              try {
                  mCallbacks.getChDetThCb(val, status);
              } catch (RemoteException e) {
                  Log.e(LOGTAG, "FmRxEvGetChDetThreshold: Exception = " + e.toString());
              }
          }
      }

      public void FmRxEvSetChDetThreshold(int status)
      {
          Log.e(LOGTAG, "FmRxEvSetChDetThreshold");
          if (mCallbacks != null) {
              try {
                  mCallbacks.setChDetThCb(status);
              } catch (RemoteException e) {
                    e.printStackTrace();
              }
          }
      }

      public void FmRxEvDefDataRead(int val, int status) {
          Log.e(LOGTAG, "FmRxEvDefDataRead");
          if (mCallbacks != null) {
              try {
                  mCallbacks.DefDataRdCb(val, status);
              } catch (RemoteException e) {
                  Log.e(LOGTAG, "FmRxEvDefDataRead: Exception = " + e.toString());
              }
          }
      }

      public void FmRxEvDefDataWrite(int status)
      {
          Log.e(LOGTAG, "FmRxEvDefDataWrite");
          if (mCallbacks != null) {
              try {
                  mCallbacks.DefDataWrtCb(status);
              } catch (RemoteException e) {
                  e.printStackTrace();
              }
          }
      }

      public void FmRxEvGetBlend(int val, int status)
      {
          Log.e(LOGTAG, "FmRxEvGetBlend");

          if (mCallbacks != null) {
              try {
                  mCallbacks.getBlendCb(val, status);
              } catch (RemoteException e) {
                  e.printStackTrace();
              }
          }
      }

      public void FmRxEvSetBlend(int status)
      {
          Log.e(LOGTAG, "FmRxEvSetBlend");
          if (mCallbacks != null) {
              try {
                  mCallbacks.setBlendCb(status);
              } catch (RemoteException e) {
                  e.printStackTrace();
              }
          }
      }

      public void FmRxGetStationParam(int val, int status)
      {
          if (mCallbacks != null) {
              try {
                  mCallbacks.getStationParamCb(val, status);
                  synchronized(mEventWaitLock) {
                      mEventReceived = true;
                      mEventWaitLock.notify();
                  }
              } catch (RemoteException e) {
                  e.printStackTrace();
              }
          }
      }

      public void FmRxGetStationDbgParam(int val, int status)
      {
          if (mCallbacks != null) {
              try {
                  mCallbacks.getStationDbgParamCb(val, status);
              } catch (RemoteException e) {
                  e.printStackTrace();
              }
          }
      }

      public void FmRxEvSearchInProgress()
      {
         Log.d(LOGTAG, "FmRxEvSearchInProgress");
      }
      public void FmRxEvSearchRdsInProgress()
      {
         Log.d(LOGTAG, "FmRxEvSearchRdsInProgress");
      }
      public void FmRxEvSearchListInProgress()
      {
         Log.d(LOGTAG, "FmRxEvSearchListInProgress");
      }
      public void FmRxEvSearchComplete(int frequency)
       {
         Log.d(LOGTAG, "FmRxEvSearchComplete: Tuned Frequency: " +frequency);
         try
         {
            FmSharedPreferences.setTunedFrequency(frequency);
            mPrefs.Save();
            //Log.d(LOGTAG, "Call mCallbacks.onSearchComplete");
            /* Since the Tuned Status changed, clear out the RDSData cached */
            if(mReceiver != null) {
               clearStationInfo();
            }
            if(mCallbacks != null)
            {
               mCallbacks.onSearchComplete();
            }
            synchronized (mEventWaitLock) {
               mEventReceived = true;
               mEventWaitLock.notify();
            }
            /* Update the frequency in the StatusBar's Notification */
            startNotification();
         }
         catch (RemoteException e)
         {
            e.printStackTrace();
         }
      }

      public void FmRxEvSearchRdsComplete()
      {
         Log.d(LOGTAG, "FmRxEvSearchRdsComplete");
      }

      public void FmRxEvSearchListComplete()
      {
         Log.d(LOGTAG, "FmRxEvSearchListComplete");
         try
         {
            if(mCallbacks != null)
            {
               mCallbacks.onSearchListComplete();
            }
         } catch (RemoteException e)
         {
            e.printStackTrace();
         }
      }

      public void FmRxEvSearchCancelled()
      {
         Log.d(LOGTAG, "FmRxEvSearchCancelled: Cancelled the on-going search operation.");
      }
      public void FmRxEvRdsGroupData()
      {
         Log.d(LOGTAG, "FmRxEvRdsGroupData");
      }

      public void FmRxEvRdsPsInfo() {
         Log.d(LOGTAG, "FmRxEvRdsPsInfo: ");
         try
         {
            if(mReceiver != null)
            {
               mFMRxRDSData = mReceiver.getPSInfo();
               if(mFMRxRDSData != null)
               {
                  Log.d(LOGTAG, "PI: [" + mFMRxRDSData.getPrgmId() + "]");
                  Log.d(LOGTAG, "PTY: [" + mFMRxRDSData.getPrgmType() + "]");
                  Log.d(LOGTAG, "PS: [" + mFMRxRDSData.getPrgmServices() + "]");
               }
               if(mCallbacks != null)
               {
                  mCallbacks.onProgramServiceChanged();
               }
            }
         } catch (RemoteException e)
         {
            e.printStackTrace();
         }
      }

      public void FmRxEvRdsRtInfo() {
         Log.d(LOGTAG, "FmRxEvRdsRtInfo");
         try
         {
            //Log.d(LOGTAG, "Call mCallbacks.onRadioTextChanged");
            if(mReceiver != null)
            {
               mFMRxRDSData = mReceiver.getRTInfo();
               if(mFMRxRDSData != null)
               {
                  Log.d(LOGTAG, "PI: [" + mFMRxRDSData.getPrgmId() + "]");
                  Log.d(LOGTAG, "PTY: [" + mFMRxRDSData.getPrgmType() + "]");
                  Log.d(LOGTAG, "RT: [" + mFMRxRDSData.getRadioText() + "]");
               }
               if(mCallbacks != null)
               {
                  mCallbacks.onRadioTextChanged();
               }
            }
         } catch (RemoteException e)
         {
            e.printStackTrace();
         }

      }

      public void FmRxEvRdsAfInfo()
      {
         Log.d(LOGTAG, "FmRxEvRdsAfInfo");
         mReceiver.getAFInfo();
      }
      public void FmRxEvRTPlus()
      {
         int tag_nums;
         Log.d(LOGTAG, "FmRxEvRTPlusInfo");
         mRtPlusSupport =  true;
         if (mReceiver != null) {
             mFMRxRDSData = mReceiver.getRTPlusInfo();
             tag_nums = mFMRxRDSData.getTagNums();
             if (tag_nums >= 1) {
                 Log.d(LOGTAG, "tag1 is: " + mFMRxRDSData.getTagCode(1) + "value: "
                        + mFMRxRDSData.getTagValue(1));
                 FmSharedPreferences.addTags(mFMRxRDSData.getTagCode(1), mFMRxRDSData.getTagValue(1));
             }
             if(tag_nums == 2) {
                Log.d(LOGTAG, "tag2 is: " + mFMRxRDSData.getTagCode(2) + "value: "
                      + mFMRxRDSData.getTagValue(2));
                 FmSharedPreferences.addTags(mFMRxRDSData.getTagCode(2), mFMRxRDSData.getTagValue(2));
             }
         }
      }
      public void FmRxEvERTInfo()
      {
         Log.d(LOGTAG, "FmRxEvERTInfo");
         try {
             if (mReceiver != null) {
                mFMRxRDSData = mReceiver.getERTInfo();
                if(mCallbacks != null)
                   mCallbacks.onExtenRadioTextChanged();
             }
         } catch (RemoteException e) {
             e.printStackTrace();
         }
      }
      public void FmRxEvECCInfo()
      {
         Log.d(LOGTAG, "FmRxEvECCInfo");
         try {
             if (mReceiver != null) {
                mFMRxRDSData = mReceiver.getECCInfo();
                if(mCallbacks != null)
                   mCallbacks.onExtenCountryCodeChanged();
             }
         } catch (RemoteException e) {
             e.printStackTrace();
         }
      }
      public void FmRxEvRdsPiMatchAvailable()
      {
         Log.d(LOGTAG, "FmRxEvRdsPiMatchAvailable");
      }
      public void FmRxEvRdsGroupOptionsSet()
      {
         Log.d(LOGTAG, "FmRxEvRdsGroupOptionsSet");
      }
      public void FmRxEvRdsProcRegDone()
      {
         Log.d(LOGTAG, "FmRxEvRdsProcRegDone");
      }
      public void FmRxEvRdsPiMatchRegDone()
      {
         Log.d(LOGTAG, "FmRxEvRdsPiMatchRegDone");
      }
      public void FmRxEvEnableSlimbus(int status)
      {
         Log.e(LOGTAG, "FmRxEvEnableSlimbus status = " + status);
         synchronized(mEventWaitLock) {
             mEventReceived = true;
             mEventWaitLock.notify();
         }

      }
      public void FmRxEvEnableSoftMute(int status)
      {
         Log.e(LOGTAG, "FmRxEvEnableSoftMute status = " + status);
         synchronized(mEventWaitLock) {
             mEventReceived = true;
             mEventWaitLock.notify();
         }
      }
   };


   /*
    *  Read the Tuned Frequency from the FM module.
    */
   private String getTunedFrequencyString() {

      double frequency = FmSharedPreferences.getTunedFrequency() / 1000.0;
      String frequencyString = getString(R.string.stat_notif_frequency, (""+frequency));
      return frequencyString;
   }
   public int getRssi() {
      if (mReceiver != null) {
          mEventReceived = false;
          int rssi = mReceiver.getRssi();
          waitForFWEvent();
          return rssi;
      } else
          return Integer.MAX_VALUE;
   }
   public int getIoC() {
      if (mReceiver != null)
          return mReceiver.getIoverc();
      else
          return Integer.MAX_VALUE;
   }
   public int getIntDet() {
      if (mReceiver != null)
          return mReceiver.getIntDet();
      else
          return Integer.MAX_VALUE;
   }
   public int getMpxDcc() {
      if (mReceiver != null)
          return mReceiver.getMpxDcc();
      else
          return Integer.MAX_VALUE;
   }
   public void setHiLoInj(int inj) {
      if (mReceiver != null)
          mReceiver.setHiLoInj(inj);
   }
   public int getSINR() {
      if (mReceiver != null) {
          mEventReceived = false;
          int sinr = mReceiver.getSINR();;
          waitForFWEvent();
          return sinr;
      } else
          return Integer.MAX_VALUE;
   }
   public boolean setSinrSamplesCnt(int samplesCnt) {
      if(mReceiver != null)
         return mReceiver.setSINRsamples(samplesCnt);
      else
         return false;
   }
   public boolean setSinrTh(int sinr) {
      if(mReceiver != null)
         return mReceiver.setSINRThreshold(sinr);
      else
         return false;
   }
   public boolean setIntfDetLowTh(int intfLowTh) {
      if(mReceiver != null)
         return mReceiver.setOnChannelThreshold(intfLowTh);
      else
         return false;
   }
   public boolean getIntfDetLowTh()
   {
       if (mReceiver != null)
           return mReceiver.getOnChannelThreshold();
       else
           return false;
   }
   public boolean setIntfDetHighTh(int intfHighTh) {
      if(mReceiver != null)
         return mReceiver.setOffChannelThreshold(intfHighTh);
      else
         return false;
   }
   public boolean getIntfDetHighTh()
   {
       if (mReceiver != null)
           return mReceiver.getOffChannelThreshold();
       else
           return false;
   }
   public int getSearchAlgoType() {
       if(mReceiver != null)
          return mReceiver.getSearchAlgoType();
       else
          return -1;
   }
   public boolean setSearchAlgoType(int searchType) {
        if(mReceiver != null)
           return mReceiver.setSearchAlgoType(searchType);
        else
           return false;
   }
   public int getSinrFirstStage() {
        if(mReceiver != null)
           return mReceiver.getSinrFirstStage();
        else
           return Integer.MAX_VALUE;
   }
   public boolean setSinrFirstStage(int sinr) {
        if(mReceiver != null)
           return mReceiver.setSinrFirstStage(sinr);
        else
           return false;
   }
   public int getRmssiFirstStage() {
        if(mReceiver != null)
           return mReceiver.getRmssiFirstStage();
        else
           return Integer.MAX_VALUE;
   }
   public boolean setRmssiFirstStage(int rmssi) {
         if(mReceiver != null)
            return mReceiver.setRmssiFirstStage(rmssi);
         else
            return false;
   }
   public int getCFOMeanTh() {
          if(mReceiver != null)
             return mReceiver.getCFOMeanTh();
          else
             return Integer.MAX_VALUE;
   }
   public boolean setCFOMeanTh(int th) {
          if(mReceiver != null)
             return mReceiver.setCFOMeanTh(th);
          else
             return false;
   }
   public int getSinrSamplesCnt() {
          if(mReceiver != null)
             return mReceiver.getSINRsamples();
          else
             return Integer.MAX_VALUE;
   }
   public int getSinrTh() {
          if(mReceiver != null)
             return mReceiver.getSINRThreshold();
          else
             return Integer.MAX_VALUE;
   }

   boolean setAfJmpRmssiTh(int afJmpRmssiTh) {
          if(mReceiver != null)
             return mReceiver.setAFJumpRmssiTh(afJmpRmssiTh);
          else
             return false;
   }
   boolean setGoodChRmssiTh(int gdChRmssiTh) {
          if(mReceiver != null)
             return mReceiver.setGdChRmssiTh(gdChRmssiTh);
          else
             return false;
   }
   boolean setAfJmpRmssiSamplesCnt(int afJmpRmssiSmplsCnt) {
          if(mReceiver != null)
             return mReceiver.setAFJumpRmssiSamples(afJmpRmssiSmplsCnt);
          else
             return false;
   }
   int getAfJmpRmssiTh() {
          if(mReceiver != null)
             return mReceiver.getAFJumpRmssiTh();
          else
             return Integer.MIN_VALUE;
   }
   int getGoodChRmssiTh() {
          if(mReceiver != null)
             return mReceiver.getGdChRmssiTh();
          else
             return Integer.MAX_VALUE;
   }
   int getAfJmpRmssiSamplesCnt() {
          if(mReceiver != null)
             return mReceiver.getAFJumpRmssiSamples();
          else
             return Integer.MIN_VALUE;
   }

   String getSocName() {
          if(mReceiver != null)
             return mReceiver.getSocName();
          else
             return null;
   }

   boolean getFmStatsProp() {
          return mFmStats;
   }

   private void setAlarmSleepExpired (long duration) {
       Intent i = new Intent(SLEEP_EXPIRED_ACTION);
       AlarmManager am = (AlarmManager)getSystemService(Context.ALARM_SERVICE);
       PendingIntent pi = PendingIntent.getBroadcast(this, 0, i, PendingIntent.FLAG_IMMUTABLE);
       Log.d(LOGTAG, "delayedStop called" + SystemClock.elapsedRealtime() + duration);
       am.set(AlarmManager.ELAPSED_REALTIME_WAKEUP, SystemClock.elapsedRealtime() + duration, pi);
       mSleepActive = true;
   }
   private void cancelAlarmSleepExpired() {
       Intent i = new Intent(SLEEP_EXPIRED_ACTION);
       AlarmManager am = (AlarmManager)getSystemService(Context.ALARM_SERVICE);
       PendingIntent pi = PendingIntent.getBroadcast(this, 0, i, PendingIntent.FLAG_IMMUTABLE);
       am.cancel(pi);
       mSleepActive = false;
   }
   private void setAlarmRecordTimeout(long duration) {
       Intent i = new Intent(RECORD_EXPIRED_ACTION);
       AlarmManager am = (AlarmManager)getSystemService(Context.ALARM_SERVICE);
       PendingIntent pi = PendingIntent.getBroadcast(this, 0, i, PendingIntent.FLAG_IMMUTABLE);
       Log.d(LOGTAG, "delayedStop called" + SystemClock.elapsedRealtime() + duration);
       am.set(AlarmManager.ELAPSED_REALTIME_WAKEUP, SystemClock.elapsedRealtime() + duration, pi);
   }
   private void cancelAlarmRecordTimeout() {
       Intent i = new Intent(RECORD_EXPIRED_ACTION);
       AlarmManager am = (AlarmManager)getSystemService(Context.ALARM_SERVICE);
       PendingIntent pi = PendingIntent.getBroadcast(this, 0, i, PendingIntent.FLAG_IMMUTABLE);
       am.cancel(pi);
   }
   private void setAlarmDelayedServiceStop() {
       Intent i = new Intent(SERVICE_DELAYED_STOP_ACTION);
       AlarmManager am = (AlarmManager)getSystemService(Context.ALARM_SERVICE);
       PendingIntent pi = PendingIntent.getBroadcast(this, 0, i, PendingIntent.FLAG_IMMUTABLE);
       am.set(AlarmManager.ELAPSED_REALTIME_WAKEUP, SystemClock.elapsedRealtime() + IDLE_DELAY, pi);
   }
   private void cancelAlarmDealyedServiceStop() {
       Intent i = new Intent(SERVICE_DELAYED_STOP_ACTION);
       AlarmManager am = (AlarmManager)getSystemService(Context.ALARM_SERVICE);
       PendingIntent pi = PendingIntent.getBroadcast(this, 0, i, PendingIntent.FLAG_IMMUTABLE);
       am.cancel(pi);
   }
   private void cancelAlarms() {
       cancelAlarmSleepExpired();
       cancelAlarmRecordTimeout();
       cancelAlarmDealyedServiceStop();
   }
   public boolean setRxRepeatCount(int count) {
      if(mReceiver != null)
         return mReceiver.setPSRxRepeatCount(count);
      else
         return false;
   }
   public boolean getRxRepeatCount() {
      if(mReceiver != null)
         return mReceiver.getPSRxRepeatCount();
      else
         return false;
   }
   public long getRecordingStartTime() {
      return mSampleStart;
   }

   public boolean isSleepTimerActive() {
      return mSleepActive;
   }
   //handling the sleep and record stop when FM App not in focus
   private void delayedStop(long duration, int nType) {
       int whatId = (nType == STOP_SERVICE) ? STOPSERVICE_ONSLEEP: STOPRECORD_ONTIMEOUT;
       if (nType == STOP_SERVICE)
           setAlarmSleepExpired(duration);
       else
           setAlarmRecordTimeout(duration);
   }
   private void cancelDelayedStop(int nType) {
       int whatId = (nType == STOP_SERVICE) ? STOPSERVICE_ONSLEEP: STOPRECORD_ONTIMEOUT;
       if (nType == STOP_SERVICE)
           cancelAlarmSleepExpired();
       else
           cancelAlarmRecordTimeout();
   }

   private void requestFocusImpl() {
      Log.d(LOGTAG, "++requestFocusImpl mPlaybackInProgress: " +
                    mPlaybackInProgress + " mStoppedOnFocusLoss: " +
                    mStoppedOnFocusLoss + " isFmOn: " + isFmOn());
      if( (false == mPlaybackInProgress) &&
          (true  == mStoppedOnFocusLoss) && isFmOn()) {
           // adding code for audio focus gain.
           AudioManager audioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
           audioManager.requestAudioFocus(mGainFocusReq);
           if(false == mPlaybackInProgress) {
               startFM();
               enableSlimbus(ENABLE_SLIMBUS_DATA_PORT);
           }
           mStoppedOnFocusLoss = false;
       }
   }

   private void requestFocus() {
       Log.d(LOGTAG, "++requestFocus");
       requestFocusImpl();
       Log.d(LOGTAG, "--requestFocus");
   }

   public void onAudioFocusChange(int focusChange) {
           mDelayedStopHandler.obtainMessage(FOCUSCHANGE, focusChange, 0).sendToTarget();
   }


   class A2dpServiceListener implements BluetoothProfile.ServiceListener {
       private List<BluetoothDevice> mA2dpDeviceList = null;
       private BluetoothA2dp mA2dpProfile = null;

       public void onServiceConnected(int profile, BluetoothProfile proxy) {
           mA2dpProfile = (BluetoothA2dp) proxy;
           mA2dpDeviceList = mA2dpProfile.getConnectedDevices();
           if (mA2dpDeviceList == null || mA2dpDeviceList.size() == 0)
               mA2dpConnected = false;
           else
               mA2dpConnected = true;

           mA2dpDisconnected = !mA2dpConnected;
           //mSpeakerPhoneOn = mA2dpConnected;
           Log.d(LOGTAG, "A2DP Status: " + mA2dpConnected);
       }

       public void onServiceDisconnected(int profile) {
           mA2dpProfile = null;
           mA2dpDeviceList = null;
       }
   }

   private void getA2dpStatusAtStart () {
       BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();

       if (!adapter.getProfileProxy(this, new A2dpServiceListener(),
                                       BluetoothProfile.A2DP)) {
           Log.d(LOGTAG, "Failed to get A2DP profile proxy");
       }
   }

   private void restoreDefaults () {
        mStoppedOnFactoryReset = true;
   }


   class FMDeathRecipient implements IBinder.DeathRecipient {
       public FMDeathRecipient(FMRadioService service, IBinder binder) {
       }
       public void binderDied() {
           Log.d(LOGTAG, "** Binder is dead - cleanup audio now ** ");
           //TODO unregister the fm service here.
       }
   }
}
