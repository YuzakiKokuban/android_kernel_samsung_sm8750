/*
 * Copyright (c) 2009-2015, The Linux Foundation. All rights reserved.
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

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.bluetooth.BluetoothA2dp;
import android.bluetooth.BluetoothDevice;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnKeyListener;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;
import android.content.res.Configuration;
import android.media.AudioSystem;
import android.media.AudioManager;
import android.media.MediaRecorder;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.RemoteException;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.Window;
import android.view.KeyEvent;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;
import android.text.TextUtils;

import java.util.*;
import java.io.File;
import java.io.IOException;
import java.lang.ref.WeakReference;
import java.util.Formatter;
import java.util.HashMap;
import java.util.List;
import java.util.ListIterator;
import java.util.Locale;
import java.util.ArrayList;

import com.caf.utils.FrequencyPicker;
import com.caf.utils.FrequencyPickerDialog;
import android.content.ServiceConnection;
import android.media.MediaRecorder;

import qcom.fmradio.FmConfig;
import android.os.ServiceManager;

import com.caf.fmradio.HorizontalNumberPicker.OnScrollFinishListener;
import com.caf.fmradio.HorizontalNumberPicker.OnValueChangeListener;
import com.caf.fmradio.HorizontalNumberPicker.Scale;

import android.content.SharedPreferences;
import android.graphics.Color;
import android.graphics.Point;
import android.Manifest;
import android.content.pm.PackageManager;

public class FMRadio extends Activity
{
   public static final String LOGTAG = "FMRadio";

   public static final boolean RECORDING_ENABLE = true;
   MediaRecorder mRecorder = null;

   /* menu Identifiers */
   private static final int MENU_SCAN_START = Menu.FIRST + 2;
   private static final int MENU_SCAN_STOP = Menu.FIRST + 3;
   private static final int MENU_RECORD_START = Menu.FIRST + 4;
   private static final int MENU_RECORD_STOP = Menu.FIRST + 5;
   private static final int MENU_SLEEP = Menu.FIRST + 6;
   private static final int MENU_SLEEP_CANCEL = Menu.FIRST + 7;
   private static final int MENU_SETTINGS = Menu.FIRST + 8;
   private static final int MENU_SPEAKER = Menu.FIRST + 9;
   private static final int MENU_TAGS = Menu.FIRST + 10;
   private static final int MENU_STAT_TEST = Menu.FIRST + 11;
   private static final int MENU_STATION_LIST = Menu.FIRST + 12;
   /* Dialog Identifiers */
   private static final int DIALOG_SEARCH = 1;
   private static final int DIALOG_SLEEP = 2;
   private static final int DIALOG_PRESET_LIST_RENAME = 5;
   private static final int DIALOG_PRESET_LIST_DELETE = 6;
   private static final int DIALOG_PRESET_LIST_AUTO_SET = 7;
   private static final int DIALOG_PICK_FREQUENCY = 8;
   private static final int DIALOG_PROGRESS_PROGRESS = 9;
   private static final int DIALOG_PRESET_OPTIONS = 10;
   private static final int DIALOG_PRESET_RENAME = 11;
   private static final int DIALOG_CMD_TIMEOUT = 12;
   private static final int DIALOG_CMD_FAILED = 13;
   private static final int DIALOG_CMD_FAILED_CALL_ON = 15;
   private static final int DIALOG_TAGS = 16;

   /* Activity Return ResultIdentifiers */
   private static final int ACTIVITY_RESULT_SETTINGS = 1;

   /* Activity Return ResultIdentifiers */
   private static final int MAX_PRESETS_PER_PAGE = 7;


   /* Station's Audio is Stereo */
   private static final int FMRADIO_UI_STATION_AUDIO_STEREO = 1;
   /* Station's Audio is Mono */
   private static final int FMRADIO_UI_STATION_AUDIO_MONO = 2;

   /* The duration during which the "Sleep: xx:xx" string will be toggling
    */
   private static final int SLEEP_TOGGLE_SECONDS = 60;

   /* The number of Preset Stations to create.
    * The hardware supports a maximum of 12.
    */
   private static final int NUM_AUTO_PRESETS_SEARCH= 12;
   /*
    * Command time out: For asynchonous operations, if no response
    * is received with int this duration, a timeout msg will be displayed.
    */
   private static final int CMD_TIMEOUT_DELAY_MS = 5000;
   private static final int MSG_CMD_TIMEOUT = 101;

   private static final int CMD_NONE = 0;
   private static final int CMD_TUNE = 1;
   private static final int CMD_FMON = 2;
   private static final int CMD_FMOFF = 3;
   private static final int CMD_FMCONFIGURE = 4;
   private static final int CMD_MUTE = 5;
   private static final int CMD_SEEK = 6;
   private static final int CMD_SCAN = 7;
   private static final int CMD_SEEKPI = 8;
   private static final int CMD_SEARCHLIST = 9;
   private static final int CMD_CANCELSEARCH = 10;
   private static final int CMD_SET_POWER_MODE = 11;
   private static final int CMD_SET_AUDIO_MODE = 12;
   private static final int CMD_SET_AUTOAF = 13;
   private static final int CMD_GET_INTERNALANTENNA_MODE = 14;

   private static final int PRESETS_OPTIONS_TUNE = 0;
   private static final int PRESETS_OPTIONS_REPLACE = 1;
   private static final int PRESETS_OPTIONS_RENAME = 2;
   private static final int PRESETS_OPTIONS_DELETE = 3;
   private static final int PRESETS_OPTIONS_SEARCHPI = 4;

   public static final String SCAN_STATION_PREFS_NAME = "scan_station_list";
   public static final String NUM_OF_STATIONS= "number_of_stations";
   public static final String STATION_NAME = "name_of_station";
   public static final String STATION_FREQUENCY = "frequency_of_station";

   private IFMRadioService mService = null;
   private FmSharedPreferences mPrefs;

   /* Button Resources */
   private ImageView mOnOffButton;
   private ImageView mMuteButton;
   private ImageView mSpeakerButton;
   /* Button to navigate Preset pages */
   private ImageButton mPresetPageButton;
   /* 6 Preset Buttons */
   private Button[] mPresetButtons = {null, null, null, null, null, null, null};
   private Button mPresetListButton;
   // private ImageButton mSearchButton;
   private ImageView mForwardButton;
   private ImageView mBackButton;

   /* Top row in the station info layout */
   private ImageView mRSSI;
   private TextView mProgramServiceTV;
   private TextView mStereoTV;

   /* Middle row in the station info layout */
   private TextView mTuneStationFrequencyTV;
   private TextView mStationCallSignTV;
   private TextView mProgramTypeTV;

   /* Bottom row in the station info layout */
   private TextView mRadioTextTV;
   private TextView mERadioTextTV;
   private TextView mEContryCodeTV;

   /* Sleep and Recording Messages */
   private TextView mSleepMsgTV;
   private TextView mRecordingMsgTV;

   private double mOutputFreq;
   private int mPresetPageNumber = 0;
   private int mStereo = -1;

   private static boolean mFMStats = false;


   /* Current Status Indicators */
   private static boolean mRecording = false;
   private static boolean mRtPlusSupported = false;
   private static boolean mIsScaning = false;
   private static boolean mIsSeeking = false;
   private static boolean mIsSearching = false;
   private static int mScanPty = 0;
   private static int mScanPtyIndex = 0;
   private Animation mAnimation = null;
   private ScrollerText mRadioTextScroller = null;
   private ScrollerText mERadioTextScroller = null;

   private PresetStation mTunedStation = new PresetStation("", 102100);
   private PresetStation mPresetButtonStation = null;

   /* Radio Vars */
   private Handler mHandler = new Handler();
   /* Search Progress Dialog */
   private AlertDialog mProgressDialog = null;
   private Dialog mDlgResetOptions;
   private Dialog mDlgSearch;
   private Dialog mDlgSleep;
   private Dialog mDlgPresetRename;
   private Dialog mDlgCmdTout;
   private Dialog mDlgCmdFailed;
   private Dialog mDlgCmdFailedCallOn;
   private Dialog mDlgPickFrequency;

   /* Asynchronous command active */
   private static int mCommandActive = 0;

   /* Command that failed (Sycnhronous or Asynchronous) */
   private static int mCommandFailed = 0;

   private  HorizontalNumberPicker mPicker;
   private int mFrequency;

   /** Index of arrays.xml key word "search_category_rbds_entries or search_category_rds_entries resources*/
   private int mItemsIndex = -1;
   private static int mDisplayWidth;
   private static final int TEXTSIZE_PARAMETER_FOR_NUMBER_PICKER = 20;
   private static final int FREQUENCY_STEP_SMALL = 50;
   private static final int FREQUENCY_STEP_MEDIUM = 100;
   private static final int FREQUENCY_STEP_LARGE = 200;
   public static boolean mUpdatePickerValue = false;

   private LoadedDataAndState SavedDataAndState = null;
   private static String mBTsoc;

   private BroadcastReceiver mFmSettingReceiver = null;
   private static String[] RECORD_PERMISSIONS = {
            Manifest.permission.RECORD_AUDIO,
            Manifest.permission.CAPTURE_AUDIO_OUTPUT,
            Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE
    };
   private static final int ALL_PERMISSIONS = 101;

    @Override
public void onRequestPermissionsResult(int requestCode, String[] permissions,  int[] grantResults) {
    super.onRequestPermissionsResult(requestCode, permissions, grantResults);
    // for PERMISSION_CODE:
    switch (requestCode){
        case 101:
            if(grantResults.length > 0){
                if(grantResults[0] == PackageManager.PERMISSION_GRANTED){
                    Log.d(LOGTAG,"  RECORD_AUDIO permission granted");
                }
                else{ Log.d(LOGTAG,"  no RECORD_AUDIO permissions"); }
                if(grantResults[1] == PackageManager.PERMISSION_GRANTED){
                    Log.d(LOGTAG,"   CAPTURE_AUDIO_OUTPUT permission granted");
                }
                else{  Log.d(LOGTAG,"   no CAPTURE_AUDIO_OUTPUT permissions"); }
                if(grantResults[2] == PackageManager.PERMISSION_GRANTED){
                    Log.d(LOGTAG,"   WRITE_EXTERNAL_STORAGE permission granted");
                }
                else{ Log.d(LOGTAG,"   no WRITE_EXTERNAL_STORAGE permissions"); }
                if(grantResults[3] == PackageManager.PERMISSION_GRANTED){
                    Log.d(LOGTAG,"  READ_EXTERNAL_STORAGE permission granted");
                }
                else{Log.d(LOGTAG,"  no READ_EXTERNAL_STORAGE permissions"); }
            }
    }
}
   /** Called when the activity is first created. */
   @Override
   public void onCreate(Bundle savedInstanceState) {
      super.onCreate(savedInstanceState);
      setVolumeControlStream(AudioManager.STREAM_MUSIC);
      Log.d(LOGTAG, "onCreate");
      mPrefs = new FmSharedPreferences(this);
      mCommandActive = CMD_NONE;
      mCommandFailed = CMD_NONE;

      Log.d(LOGTAG, "onCreate permssion req");
      requestPermissions(RECORD_PERMISSIONS, ALL_PERMISSIONS);
      Point p = new Point();
      getWindowManager().getDefaultDisplay().getSize(p);
      mDisplayWidth = p.x;
      Log.d(LOGTAG, " Height : " + p.y + " - mDisplayWidth  : " + mDisplayWidth);
      DisplayMetrics outMetrics = new DisplayMetrics();
      getWindowManager().getDefaultDisplay().getMetrics(outMetrics );

      setContentView(R.layout.fmradio);
      SavedDataAndState = (LoadedDataAndState)getLastNonConfigurationInstance();

      mPicker = (HorizontalNumberPicker)findViewById(R.id.fm_picker);
      if (mPicker != null) {
          mPicker.setTextSize(mDisplayWidth / TEXTSIZE_PARAMETER_FOR_NUMBER_PICKER);
          mPicker.setDensity(outMetrics.densityDpi);
          mPicker.setOnValueChangedListener(new OnValueChangeListener(){
            @Override
            public void onValueChange(HorizontalNumberPicker picker,
                    int oldVal, int newVal) {
                // TODO Auto-generated method stub
                valueToFrequency(newVal);
                mHandler.post(mRadioChangeFrequency);
            }
        });
      }

      mAnimation = AnimationUtils.loadAnimation(this,
                                                R.anim.preset_select);

      mMuteButton = (ImageView)findViewById(R.id.btn_silent);
      if (mMuteButton != null) {
          mMuteButton.setOnClickListener(mMuteModeClickListener);
      }

      mSpeakerButton = (ImageView)findViewById(R.id.btn_speaker_earphone);
      if (mSpeakerButton != null) {
          mSpeakerButton.setOnClickListener(mSpeakerClickListener);
      }

      mOnOffButton = (ImageView)findViewById(R.id.btn_onoff);
      if (mOnOffButton != null) {
          mOnOffButton.setOnClickListener(mTurnOnOffClickListener);
      }

      mForwardButton = (ImageView)findViewById(R.id.btn_forward);
      if (mForwardButton != null) {
          mForwardButton.setOnClickListener(mForwardClickListener);
          mForwardButton.setOnLongClickListener(mForwardLongClickListener);
      }

      mBackButton = (ImageView)findViewById(R.id.btn_back);
      if (mBackButton != null) {
          mBackButton.setOnClickListener(mBackClickListener);
          mBackButton.setOnLongClickListener(mBackLongClickListener);
      }

      /* 6 Preset Buttons */
      mPresetButtons[0] = (Button)findViewById(R.id.presets_button_1);
      mPresetButtons[1] = (Button)findViewById(R.id.presets_button_2);
      mPresetButtons[2] = (Button)findViewById(R.id.presets_button_3);
      mPresetButtons[3] = (Button)findViewById(R.id.presets_button_4);
      mPresetButtons[4] = (Button)findViewById(R.id.presets_button_5);
      mPresetButtons[5] = (Button)findViewById(R.id.presets_button_6);
      mPresetButtons[6] = (Button)findViewById(R.id.presets_button_7);

      for (int nButton = 0; nButton < MAX_PRESETS_PER_PAGE; nButton++) {
         if (mPresetButtons[nButton] != null) {
             mPresetButtons[nButton]
               .setOnClickListener(mPresetButtonClickListener);
             mPresetButtons[nButton]
               .setOnLongClickListener(mPresetButtonOnLongClickListener);
         }
      }

      mTuneStationFrequencyTV = (TextView)findViewById(R.id.prog_frequency_tv);
      if (mTuneStationFrequencyTV != null) {
         mTuneStationFrequencyTV.setOnLongClickListener(mFrequencyViewClickListener);
      }
      mProgramServiceTV = (TextView)findViewById(R.id.prog_service_tv);
      mStereoTV = (TextView)findViewById(R.id.stereo_text_tv);

      mStationCallSignTV = (TextView)findViewById(R.id.call_sign_tv);
      mProgramTypeTV = (TextView)findViewById(R.id.pty_tv);

      mRadioTextTV = (TextView)findViewById(R.id.radio_text_tv);
      mERadioTextTV = (TextView)findViewById(R.id.eradio_text_tv);
      mSleepMsgTV = (TextView)findViewById(R.id.sleep_msg_tv);
      mRecordingMsgTV = (TextView)findViewById(R.id.record_msg_tv);
      if (mRecordingMsgTV != null) {
          mRecordingMsgTV.setOnClickListener(mRecordButtonListener);
      }
      /* Disable displaying RSSI */
      mRSSI = (ImageView)findViewById(R.id.signal_level);
      if (mRSSI != null) {
          mRSSI.setVisibility(View.INVISIBLE);
      }

      if ((mRadioTextScroller == null) && (mRadioTextTV != null)) {
          mRadioTextScroller = new ScrollerText(mRadioTextTV);
      }

      if ((mERadioTextScroller == null) && (mERadioTextTV != null)) {
          mERadioTextScroller = new ScrollerText(mERadioTextTV);
      }
   }

   protected void setDisplayvalue(){
       int max = mPrefs.getUpperLimit();
       int min = mPrefs.getLowerLimit();
       int step = mPrefs.getFrequencyStepSize();
       switch(step) {
       case FREQUENCY_STEP_SMALL:
           mPicker.setScale(Scale.SCALE_SMALL);
           break;
       case FREQUENCY_STEP_MEDIUM:
           mPicker.setScale(Scale.SCALE_MEDIUM);
           break;
       case FREQUENCY_STEP_LARGE:
           mPicker.setScale(Scale.SCALE_LARGE);
       }

       int channels = (int)((max - min) / step);
       String [] displayValues = new String[channels + 1];
       for(int i = 0; i < displayValues.length; i++) {
           displayValues[i] = String.valueOf((min + i * step) / 1000.0f);
       }
       mPicker.setDisplayedValues(displayValues, true);
       try {
           mPicker.setWrapSelectorWheel(true);
       } catch (IllegalStateException e) {
           e.printStackTrace();
       }
       mPicker.invalidate();
   }
   protected int valueToFrequency(int value) {
       mFrequency = mPrefs.getLowerLimit() + value *
                             mPrefs.getFrequencyStepSize();
       return mFrequency;
   }

   @Override
   public void onConfigurationChanged(Configuration newConfig) {
       Log.d(LOGTAG, "onConfigurationChanged");
       super.onConfigurationChanged(newConfig);
   }

   @Override
   public void onRestart() {
      Log.d(LOGTAG, "FMRadio: onRestart");
      try {
         if (null != mService && isAntennaAvailable()) {
              mService.requestFocus();
         }
      } catch (Exception e) {
         e.printStackTrace();
      }
      super.onRestart();
   }

   @Override
   public void onStop() {
      Log.d(LOGTAG, "FMRadio: onStop");
      if(isRecording()) {
          try {
              if (null != mRecordUpdateHandlerThread) {
                  mRecordUpdateHandlerThread.interrupt();
              }
          }catch (NullPointerException e) {
              e.printStackTrace();
          }
      }
      super.onStop();
   }

   @Override
   public void onStart() {
      super.onStart();
      Log.d(LOGTAG, "FMRadio: onStart");
      if ((mService == null ) && (false == bindToService(this, osc))) {
          Log.d(LOGTAG, "onStart: Failed to Start Service");
      } else {
          Log.d(LOGTAG, "onStart: Start Service completed successfully");
      }
      registerFMSettingListner();
      mPrefs.Load();
      if (mPicker != null) {
          setDisplayvalue();
      }
      PresetStation station = new PresetStation("",
                                   FmSharedPreferences.getTunedFrequency());
      if (station != null) {
          mTunedStation.Copy(station);
      }

   }

   @Override
   protected void onPause() {
      Log.d(LOGTAG, "FMRadio: onPause");
      super.onPause();
      if (isSleepTimerActive()) {
          Log.d(LOGTAG, "FMRadio: Sleep Timer active");
          mSleepUpdateHandlerThread.interrupt();
          try {
              mSleepUpdateHandlerThread.join();
          } catch (Exception e) {
              e.printStackTrace();
          }
      }
      mRadioTextScroller.stopScroll();
      mERadioTextScroller.stopScroll();
      FmSharedPreferences.setTunedFrequency(mTunedStation.getFrequency());
      mPrefs.Save();
   }

   @Override
   public void onResume() {
      Log.d(LOGTAG, "FMRadio: onResume");

      super.onResume();

      if (mService == null) {
          Log.d(LOGTAG,"bind callback has not received yet - wait for 100ms");
          mHandler.postDelayed(UpdateFm, 100);
          return;
      }
     mHandler.post(UpdateFm);
   }
   Runnable UpdateFm = new Runnable() {
       public void run() {
           // TODO: We should return on exception or continue?
           if (!isAntennaAvailable()) {
               return;
           }
           try {
               if (mService != null)
                   mService.registerCallbacks(mServiceCallbacks);
           } catch (RemoteException e) {
               e.printStackTrace();
           }

      if (isSleepTimerActive()) {
          Log.d(LOGTAG, "isSleepTimerActive is true");
          try {
               if (mService != null)
                  mService.cancelDelayedStop(FMRadioService.STOP_SERVICE);
               if(null != mSleepUpdateHandlerThread) {
                  mSleepUpdateHandlerThread.interrupt();
                  try {
                      mSleepUpdateHandlerThread.join();
                  } catch (Exception e) {
                      e.printStackTrace();
                  }
               }
          } catch (Exception e) {
               e.printStackTrace();
          }
          initiateSleepThread();
      }

      if (isRecording()) {
          Log.d(LOGTAG,"isRecordTimerActive is true");
          try {
              if (mService != null)
                  mService.cancelDelayedStop(FMRadioService.STOP_RECORD);
          } catch (Exception e) {
              e.printStackTrace();
          }
          if(isRecording()) {
              initiateRecordThread();
          }
      }

      // we might lose audio focus between pause and restart,
      // hence request it again
      try {
           if (mService != null)
               mService.requestFocus();
      } catch (Exception e) {
           e.printStackTrace();
      }

      mStereo = FmSharedPreferences.getLastAudioMode();
      mHandler.post(mUpdateProgramService);
      mHandler.post(mUpdateRadioText);
      mHandler.post(mOnStereo);
      mUpdatePickerValue = true;
      updateStationInfoToUI();
      enableRadioOnOffUI();
       }
   };
   private static class LoadedDataAndState {
      public LoadedDataAndState(){};
      public boolean onOrOff;
   }
   @Override
   public Object onRetainNonConfigurationInstance() {
      LoadedDataAndState data = new LoadedDataAndState();
      if (mService != null) {
         try {
              data.onOrOff = mService.isFmOn();
         }catch(RemoteException e) {
              data.onOrOff = false;
              e.printStackTrace();
         }
      }else {
         data.onOrOff = false;
      }
      return data;
   }

   @Override
   public void onDestroy() {
      super.onDestroy();
      Log.d(LOGTAG, "FMRadio: onDestroy");
      mHandler.removeCallbacksAndMessages(null);
      cleanupTimeoutHandler();
      closeDialog(DIALOG_PROGRESS_PROGRESS);
      if(mSearchProgressHandler != null) {
         mSearchProgressHandler.removeCallbacksAndMessages(null);
      }
      closeDialog(DIALOG_PRESET_OPTIONS);
      unRegisterReceiver(mFmSettingReceiver);
      if (mService != null) {
          try {
               if(!mService.isFmOn()) {
                  endSleepTimer();
               }
          }catch (RemoteException e) {
               e.printStackTrace();
          }
      }
      unbindFromService(this);
      mService = null;
      Log.d(LOGTAG, "onDestroy: unbindFromService completed");
   }

   @Override
   public boolean onCreateOptionsMenu(Menu menu) {
      super.onCreateOptionsMenu(menu);
      MenuItem item;
      boolean radioOn = isFmOn();
      boolean recording = isRecording();
      boolean mSpeakerPhoneOn = isSpeakerEnabled();
      boolean sleepActive = isSleepTimerActive();
      boolean searchActive = isScanActive() || isSeekActive();
      Log.d(LOGTAG, "onCreateOptionsmenu");

      item = menu.add(0, MENU_SCAN_START, 0, R.string.menu_scan_start).
                            setIcon(R.drawable.ic_btn_search);
      if (item != null) {
          item.setVisible((!searchActive) && radioOn);
      }
      item = menu.add(0, MENU_SCAN_STOP, 0, R.string.menu_scan_stop).
                            setIcon(R.drawable.ic_btn_search);
      if (item != null) {
          item.setVisible(searchActive && radioOn);
      }

      if (RECORDING_ENABLE) {
          item = menu.add(0, MENU_RECORD_START, 0, R.string.menu_record_start)
                              .setIcon(R.drawable.ic_menu_record);
          if (item != null) {
             item.setVisible(true);
             item.setEnabled((!recording) && radioOn);
          }
          item = menu.add(0, MENU_RECORD_STOP, 0, R.string.menu_record_stop)
                              .setIcon(R.drawable.ic_menu_record);
          if (item != null) {
             item.setVisible(true);
             item.setEnabled(recording && radioOn);
          }
      }
      /* Settings can be active */
      item = menu.add(0, MENU_SETTINGS, 0, R.string.menu_settings).
                              setIcon(android.R.drawable.ic_menu_preferences);

      item = menu.add(0, MENU_SLEEP, 0, R.string.menu_sleep).
                              setTitle(R.string.menu_sleep);
      if (item != null) {
          item.setVisible((!sleepActive) && radioOn);
      }
      item = menu.add(0, MENU_SLEEP_CANCEL, 0, R.string.menu_sleep_cancel).
                              setTitle(R.string.menu_sleep_cancel);
      if (item != null) {
          item.setVisible(sleepActive && radioOn);
      }

      if (mService != null) {
          try {
              mFMStats = mService.getFmStatsProp();
              Log.d(LOGTAG, "mFMStats: " + mFMStats);
          } catch (RemoteException e) {
              e.printStackTrace();
              return false;
          }
      }

      if(mFMStats) {
          item = menu.add(0, MENU_STAT_TEST, 0,R.string.menu_stats).
                             setIcon(android.R.drawable.ic_menu_info_details);
      }
      menu.add(0, MENU_STATION_LIST, 0, R.string.menu_all_channels);
      item = menu.add(0, MENU_TAGS, 0, R.string.menu_display_tags);
      return true;
   }

   @Override
   public boolean onPrepareOptionsMenu(Menu menu) {
      super.onPrepareOptionsMenu(menu);
      Log.d(LOGTAG, "onPrepare optionsmenu");
      MenuItem item;
      boolean radioOn = isFmOn();
      boolean recording = isRecording();
      boolean RtPlusSupported = isRtPlusSupported();
      boolean mSpeakerPhoneOn = isSpeakerEnabled();
      boolean searchActive = isScanActive() || isSeekActive();

      item = menu.findItem(MENU_SCAN_START);
      if (item != null) {
          item.setVisible((!searchActive) && radioOn);
      }
      item = menu.findItem(MENU_SCAN_STOP);
      if (item != null) {
         item.setVisible(searchActive && radioOn);
      }
      if (RECORDING_ENABLE) {
         item = menu.findItem(MENU_RECORD_START);
         if (item != null) {
            item.setVisible(true);
            item.setEnabled((!recording) && radioOn && (!isAnalogModeEnabled()));
         }
         item = menu.findItem(MENU_RECORD_STOP);
         if (item != null) {
             item.setVisible(true);
             item.setEnabled(recording && radioOn && (!isAnalogModeEnabled()));
         }
      }

      boolean sleepActive = isSleepTimerActive();
      item = menu.findItem(MENU_SLEEP);
      if (item != null) {
          item.setVisible((!sleepActive) && radioOn);
      }
      item = menu.findItem(MENU_SLEEP_CANCEL);
      if (item != null) {
          item.setVisible(sleepActive && radioOn);
      }
      item = menu.findItem(MENU_TAGS);
      if (item != null)
          item.setVisible(RtPlusSupported);
      return true;
   }

   @Override
   public boolean onOptionsItemSelected(MenuItem item) {
       Log.d(LOGTAG, "onOptionSelected");
      switch (item.getItemId()) {
      case MENU_SETTINGS:
         Intent launchPreferencesIntent = new Intent().setClass(this,
                                                                Settings.class);
                  launchPreferencesIntent.putExtra(Settings.RX_MODE,true);
         startActivityForResult(launchPreferencesIntent,
                                ACTIVITY_RESULT_SETTINGS);
         return true;
      case MENU_STAT_TEST:
          Intent launchFMStatIntent = new Intent().setClass(this,
                                                            FMStats.class);
          startActivity(launchFMStatIntent);
          return true;
      case MENU_SCAN_START:
          Log.d(LOGTAG, "mBTsoc: " + mBTsoc + " mService: " + mService);

          if (mBTsoc == null) {
              if (mService != null) {
                  try {
                      mBTsoc = mService.getSocName();
                      Log.d(LOGTAG, "mBTsoc: " + mBTsoc);
                  }catch (RemoteException e) {
                      e.printStackTrace();
                      return false;
                  }
              } else {
                return false;
              }
          }

         if (mBTsoc.equals("rome")) {
             clearStationList();
             initiateSearch(0); // 0 - All stations
         } else {
             displayDialog(DIALOG_SEARCH);
         }

         return true;
      case MENU_SCAN_STOP:
         cancelSearch();
         return true;
      case MENU_RECORD_START:
         startRecording();
         return true;
      case MENU_RECORD_STOP:
         stopRecording();
         return true;
      case MENU_SLEEP:
          displayDialog(DIALOG_SLEEP);
         return true;
      case MENU_SLEEP_CANCEL:
         DebugToasts("Sleep Cancelled", Toast.LENGTH_SHORT);
         endSleepTimer();
         return true;
      case MENU_STATION_LIST:
          Intent stationListIntent = new Intent().setClass(this,
                  StationListActivity.class);
          startActivity(stationListIntent);
          return true;
      case MENU_TAGS:
          Intent launchFMTagsIntent = new Intent().setClass(this,
                                                            FmTags.class);
          startActivity(launchFMTagsIntent);
          return true;
      default:
          break;
      }
      return super.onOptionsItemSelected(item);
   }

   private void enableSpeaker() {
    //This method with toggle Speaker phone based on existing state .
       boolean bSpeakerPhoneOn = isSpeakerEnabled();
       if(mService != null) {
           try {
               if (bSpeakerPhoneOn) {  // as Speaker is already on turn it off.
                   mService.enableSpeaker(false);
                   Log.d(LOGTAG, "Speaker phone is  turned off");
                   mSpeakerButton.setImageResource(R.drawable.btn_earphone);
               }else { // as Speaker is off turn it on.
                   mService.enableSpeaker(true);
                   Log.d(LOGTAG, "Speaker phone is turned on");
                   mSpeakerButton.setImageResource(R.drawable.btn_speaker);
               }
               invalidateOptionsMenu();
           }catch (RemoteException e) {
               e.printStackTrace();
           }
       }
   }

   private static final int RECORDTIMER_EXPIRED = 0x1003;
   private static final int RECORDTIMER_UPDATE = 0x1004;

   private void updateExpiredRecordTime() {
      int vis = View.VISIBLE;
      if(isRecording())
      {
         long timeNow = ((SystemClock.elapsedRealtime()));
         long seconds = (timeNow - getRecordingStartTime()) / 1000;
         String Msg = makeTimeString(seconds);
         mRecordingMsgTV.setText(Msg);
         mRecordingMsgTV.setVisibility(vis);
      }
   }

   /* Recorder Thread processing */
   private Runnable doRecordProcessing = new Runnable() {
      public void run() {
         while (isRecording() &&
                 (!Thread.currentThread().isInterrupted()))
         {
            try
            {
               Thread.sleep(500);
               Message statusUpdate = new Message();
               statusUpdate.what = RECORDTIMER_UPDATE;
               mUIUpdateHandlerHandler.sendMessage(statusUpdate);
            } catch (InterruptedException e)
            {
               break;
            }
            if(!isRecording()) {
               Message finished = new Message();
               finished.what = RECORDTIMER_EXPIRED;
               mUIUpdateHandlerHandler.sendMessage(finished);
            }
         }
      }
   };

   private Thread mRecordUpdateHandlerThread = null;

   private long getRecordingStartTime() {

      if(mService == null)
         return 0;

      try {
           return mService.getRecordingStartTime();
      }catch(RemoteException e) {
           return 0;
      }
   }

   private void initiateRecordDurationTimer(long mins ) {
      Log.d(LOGTAG, "Stop Recording in mins : " + mins);
      initiateRecordThread();
    }
    private void initiateRecordThread() {
      if (mRecordUpdateHandlerThread == null) {
          mRecordUpdateHandlerThread = new Thread(null, doRecordProcessing,
                                                "RecordUpdateThread");
      }
      /* Launch the dummy thread to simulate the transfer progress */
      Log.d(LOGTAG, "Thread State: " + mRecordUpdateHandlerThread.getState());
      if (mRecordUpdateHandlerThread.getState() == Thread.State.TERMINATED) {
          mRecordUpdateHandlerThread = new Thread(null, doRecordProcessing,
                                                "RecordUpdateThread");
      }
      /* If the thread state is "new" then the thread has not yet started */
      if (mRecordUpdateHandlerThread.getState() == Thread.State.NEW) {
          mRecordUpdateHandlerThread.start();
      }
   }

   @Override
   protected void onActivityResult(int requestCode, int resultCode, Intent data) {
      super.onActivityResult(requestCode, resultCode, data);
      Log.d(LOGTAG, "onActivityResult : requestCode -> " + requestCode);
      Log.d(LOGTAG, "onActivityResult : resultCode -> " + resultCode);
      if (requestCode == ACTIVITY_RESULT_SETTINGS) {
         if (resultCode == RESULT_OK) {
             if (data != null) {
                String action = data.getAction();
                if (action != null) {
                  if ((action.equals(Settings.RESTORE_FACTORY_DEFAULT_ACTION))
                                               && (true == isFmOn())) {
                      disableRadio();
                      RestoreDefaults();
                      FmSharedPreferences.setTunedFrequency(FmSharedPreferences.DEFAULT_NO_FREQUENCY);
                      enableRadio();
                      FmSharedPreferences.addStation("", FmSharedPreferences.DEFAULT_NO_FREQUENCY, 0);
                  }
               }
            }
         } //if ACTIVITY_RESULT_SETTINGS
      }//if (resultCode == RESULT_OK)
   }

   /**
    * @return true if a wired headset is connected.
    */
   boolean isWiredHeadsetAvailable() {
      boolean bAvailable = false;
      if(mService != null) {
         try {
            bAvailable = mService.isWiredHeadsetAvailable();
         }catch (RemoteException e) {
            e.printStackTrace();
         }
      }
      Log.e(LOGTAG, "isWiredHeadsetAvailable: " + bAvailable);
      return bAvailable;
   }

    /**
     * @return true if a internal antenna is available.
     *
     */
    boolean isAntennaAvailable() {
       boolean bAvailable = false;
       if(mService != null) {
          try {
             bAvailable = mService.isAntennaAvailable();
          }catch (RemoteException e) {
             e.printStackTrace();
          }
       }
       return bAvailable;
    }

    boolean isCallActive(){
        boolean bCallActive = false;
        if(mService != null) {
            try {
                bCallActive = mService.isCallActive();
            }catch (RemoteException e) {
                e.printStackTrace();
            }
        }
        return bCallActive;
    }

   private Dialog createSearchDlg(int id, AlertDialog.Builder dlgBuilder) {
      String[] items;
      dlgBuilder.setIcon(R.drawable.ic_btn_search);
      dlgBuilder.setTitle(getString(R.string.search_dialog_title));
      /* Pick RBDS or RDS */
      if(FmSharedPreferences.isRBDSStd()) {
         items = getResources().getStringArray(R.array.search_category_rbds_entries);
      }else { // if(FmSharedPreferences.isRDSStd())
         items = getResources().getStringArray(R.array.search_category_rds_entries);
      }
      dlgBuilder.setItems(items, new DialogInterface.OnClickListener() {
          public void onClick(DialogInterface dialog, int item) {
             String[] items;
             String[] values;

             mItemsIndex = item ;
             /* Pick RBDS or RDS */
             if(FmSharedPreferences.isRBDSStd()) {
                items = getResources().
                              getStringArray(R.array.search_category_rbds_entries);
                values = getResources().
                              getStringArray(R.array.search_category_rbds_values);
             }else { // if(FmSharedPreferences.isRDSStd())
                items = getResources().
                              getStringArray(R.array.search_category_rds_entries);
                values = getResources().
                              getStringArray(R.array.search_category_rds_values);
             }
             if ((items != null) && (values != null) && (item >= 0)) {
                 if ((item >= 0) && (item <= items.length)
                       && (item <= items.length)) {
                    DebugToasts("Search Stations for : " + items[item]
                                + " (" + values[item] + ")",
                                Toast.LENGTH_SHORT);
                    int pty = Integer.parseInt(values[item]);
                    clearStationList();
                    mScanPtyIndex = item;
                    initiateSearch(pty);
                 }
             }
             closeDialog(DIALOG_SEARCH);
          }
      });
      return dlgBuilder.create();
   }

   private Dialog createPresetOptionsDlg(int id, AlertDialog.Builder dlgBuilder) {
      if(mPresetButtonStation != null) {
         dlgBuilder.setTitle(mPresetButtonStation.getName());
         ArrayList<String> arrayList = new ArrayList<String>();
         //PRESETS_OPTIONS_TUNE=0
         arrayList.add(getResources().getString(R.string.preset_tune));
         //PRESETS_OPTIONS_REPLACE=1
         arrayList.add(getResources().getString(R.string.preset_replace));
         //PRESETS_OPTIONS_RENAME=2
         arrayList.add(getResources().getString(R.string.preset_rename));
         //PRESETS_OPTIONS_DELETE=3
         arrayList.add(getResources().getString(R.string.preset_delete));
         String piString = mPresetButtonStation.getPIString();
         if (!TextUtils.isEmpty(piString)) {
            //PRESETS_OPTIONS_SEARCHPI=4
            arrayList.add(getResources().getString(R.string.preset_search, piString));
         }

         dlgBuilder.setCancelable(true);
         dlgBuilder.setOnCancelListener(new DialogInterface.OnCancelListener() {
            public void onCancel(DialogInterface dialog) {
               mPresetButtonStation = null;
               closeDialog(DIALOG_PRESET_OPTIONS);
            }
         });
         String[] items = new String [arrayList.size ()];
         arrayList.toArray(items);
         dlgBuilder.setItems(items, new DialogInterface.OnClickListener() {
             public void onClick(DialogInterface dialog, int item) {
                if(mPresetButtonStation != null) {
                   switch(item) {
                   case PRESETS_OPTIONS_TUNE: {
                         // Tune to the station
                         tuneRadio(mPresetButtonStation.getFrequency());
                         mPresetButtonStation = null;
                         break;
                      }
                   case PRESETS_OPTIONS_REPLACE: {
                         // Replace preset Station with currently tuned station
                         if(!stationExists(mTunedStation)) {
                            Log.d(LOGTAG, "station - " + mPresetButtonStation.getName() + " ("
                               + mPresetButtonStation.getFrequency() + ")");
                            mPresetButtonStation.Copy(mTunedStation);
                            mPresetButtonStation = null;
                            setupPresetLayout();
                            mPrefs.Save();
                         }
                         break;
                      }
                   case PRESETS_OPTIONS_RENAME: {
                         // Rename
                         displayDialog(DIALOG_PRESET_RENAME);
                         break;
                      }
                   case PRESETS_OPTIONS_DELETE: {
                         // Delete
                         int curListIndex = FmSharedPreferences.getCurrentListIndex();
                         FmSharedPreferences.removeStation(curListIndex, mPresetButtonStation);
                         if (mPresetButtonStation.getFrequency() == mTunedStation.getFrequency()) {
                             // Restore current tuned station's name as its frequency
                             mTunedStation.setName("");
                         }
                         mPresetButtonStation = null;
                         setupPresetLayout();
                         mPrefs.Save();
                         break;
                      }
                   case PRESETS_OPTIONS_SEARCHPI: {
                         // SearchPI
                         String piString = mPresetButtonStation.getPIString();
                         int pi = mPresetButtonStation.getPI();
                         if ((!TextUtils.isEmpty(piString)) &&  (pi > 0)) {
                            initiatePISearch(pi);
                         }
                         mPresetButtonStation = null;
                         break;
                      }
                   default: {
                         // Should not happen
                         mPresetButtonStation = null;
                         break;
                      }
                   }//switch item
                }//if(mPresetButtonStation != null)
                closeDialog(DIALOG_PRESET_OPTIONS);
             }//onClick
         });
         return dlgBuilder.create();
      }
      return null;
   }

   private Dialog createSleepDlg(int id, AlertDialog.Builder dlgBuilder) {
      dlgBuilder.setTitle(R.string.dialog_sleep_title);
      String[] items = getResources().getStringArray(R.array.sleep_duration_values);

      dlgBuilder.setItems(items, new DialogInterface.OnClickListener() {
          public void onClick(DialogInterface dialog, int item) {
              String[] items = getResources().getStringArray(R.array.sleep_duration_values);
              Toast.makeText(getApplicationContext(), items[item], Toast.LENGTH_SHORT).show();
              if ((item >= 0) && (item <= items.length)) {
                 long seconds = (long) (900 * (item + 1));
                 initiateSleepTimer(seconds);
              }
              closeDialog(DIALOG_SLEEP);
          }
      });
      return dlgBuilder.create();
   }

   private AlertDialog createProgressDialog() {
      String msgStr = "";
      String titleStr = "";
      String []items;
      double frequency = mTunedStation.getFrequency() / 1000.0;
      boolean bSearchActive = false;

      if (isSeekActive()) {
          msgStr = getString(R.string.msg_seeking);
          bSearchActive = true;
      }else if (isScanActive()) {
          if(FmSharedPreferences.isRBDSStd()) {
                items = getResources().
                         getStringArray(R.array.search_category_rbds_entries);
          }else { // if(FmSharedPreferences.isRDSStd())
                items = getResources().
                         getStringArray(R.array.search_category_rds_entries);
          }
          String ptyStr = "";
          if (items.length > mScanPtyIndex)
              ptyStr = items[mScanPtyIndex];
          if (!TextUtils.isEmpty(ptyStr)) {
             msgStr = getString(R.string.msg_scanning_pty, ptyStr);
          }else {
             Log.d(LOGTAG, "pty is null\n");
             msgStr = getString(R.string.msg_scanning);
          }
          titleStr = getString(R.string.msg_search_title, ("" + frequency));
          bSearchActive=true;
      }else if (isSearchActive()) {
         msgStr = getString(R.string.msg_searching);
         titleStr = getString(R.string.msg_searching_title);
         bSearchActive = true;
      }
      Log.v(LOGTAG," bSearchActive :"+bSearchActive);
      if (bSearchActive) {
          AlertDialog.Builder builder = new AlertDialog.Builder(this);
          builder.setTitle(titleStr);
          builder.setIcon(R.drawable.ic_launcher_fmradio);
          View view = getLayoutInflater().inflate(R.layout.layout_dialog_progress,null);
          builder.setView(view);
          TextView tvMessage = (TextView)view.findViewById(R.id.id_tv_message);
          tvMessage.setText(msgStr);
          mProgressDialog = builder.create();
              mProgressDialog.setCanceledOnTouchOutside(false);
              mProgressDialog.setButton(DialogInterface.BUTTON_POSITIVE,
                                   getText(R.string.button_text_stop),
               new DialogInterface.OnClickListener() {
                  public void onClick(DialogInterface dialog, int whichButton) {
                      cancelSearch();
                  }
              });
              mProgressDialog.setOnCancelListener(new DialogInterface.OnCancelListener() {
                public void onCancel(DialogInterface dialog) {
                   cancelSearch();
                }
              });
              mProgressDialog.setOnKeyListener(new OnKeyListener() {
                public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
                    Log.d(LOGTAG, "OnKeyListener event received in ProgressDialog" + keyCode);
                    switch (keyCode) {
                        case KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE:
                        case 126: //KeyEvent.KEYCODE_MEDIA_PLAY:
                        case 127: //KeyEvent.KEYCODE_MEDIA_PAUSE:
                        case KeyEvent.KEYCODE_MEDIA_FAST_FORWARD:
                        case KeyEvent.KEYCODE_MEDIA_NEXT:
                        case KeyEvent.KEYCODE_MEDIA_PREVIOUS:
                        case KeyEvent.KEYCODE_MEDIA_REWIND:
                        case KeyEvent.KEYCODE_MEDIA_STOP:
                            return true;
                    }
                    return false;
                }
            });
          Message msg = new Message();
          msg.what = TIMEOUT_PROGRESS_DLG;
          mSearchProgressHandler.sendMessageDelayed(msg, SHOWBUSY_TIMEOUT);
      }
      return mProgressDialog;
   }

   private void updateSelectPresetListDlg(ListView lv) {
      if (lv != null) {
         List<PresetList> presetLists = FmSharedPreferences.getPresetLists();
         ListIterator<PresetList> presetIter;
         presetIter = presetLists.listIterator();
         int numLists = presetLists.size();
         int curIndex = FmSharedPreferences.getCurrentListIndex();
         ArrayAdapter<String> typeAdapter = new ArrayAdapter<String>
                                                (this, android.R.layout.
                                                simple_list_item_single_choice);
         for (int stationIter = 0; stationIter < numLists; stationIter++) {
            PresetList temp = presetIter.next();
            if (temp != null) {
               typeAdapter.add(getString(R.string.presetlist_select_name,
                                           temp.getName()));
            }
         }
         typeAdapter.add(getString(R.string.presetlist_add_new));
         lv.setAdapter(typeAdapter);
         lv.setChoiceMode(ListView.CHOICE_MODE_SINGLE);
         lv.clearChoices();
         if (curIndex >= numLists) {
             curIndex = 0;
         }
         if (lv.getCount() >= curIndex) {
            lv.setItemChecked(curIndex, true);
            lv.setSelection(curIndex);
         }else {
            lv.setItemChecked(0, true);
            lv.setSelection(0);
         }
      }
   }

   private Dialog createPresetRenameDlg(int id, AlertDialog.Builder dlgBuilder) {
      if(mPresetButtonStation == null) {
         return null;
      }
      LayoutInflater factory = LayoutInflater.from(this);
      final View textEntryView = factory.inflate(
                                                R.layout.alert_dialog_text_entry, null);
      dlgBuilder.setTitle(R.string.dialog_presetlist_rename_title);
      dlgBuilder.setView(textEntryView);
      dlgBuilder.setPositiveButton(R.string.alert_dialog_ok,
                                   new DialogInterface.OnClickListener() {
                                      public void onClick(DialogInterface dialog, int whichButton) {
                                         //int curList = FmSharedPreferences.getCurrentListIndex();

                                         EditText mTV = (EditText) textEntryView
                                         .findViewById(R.id.list_edit);
                                         CharSequence newName = null;
                                         if (mTV != null) {
                                            newName = mTV.getEditableText();
                                         }
                                         String nName = String.valueOf(newName);
                                         mPresetButtonStation.setName(nName);
                                         mPresetButtonStation=null;
                                         setupPresetLayout();
                                         mPrefs.Save();
                                         closeDialog(DIALOG_PRESET_RENAME);
                                      }
                                   });
      dlgBuilder.setNegativeButton(R.string.alert_dialog_cancel,
                                   new DialogInterface.OnClickListener() {
                                      public void onClick(DialogInterface dialog, int whichButton) {
                                         closeDialog(DIALOG_PRESET_RENAME);
                                      }
                                   });
      return(dlgBuilder.create());
   }

   private Dialog createCmdTimeoutDlg(int id, AlertDialog.Builder dlgBuilder) {
      if(mCommandActive > 0) {
         dlgBuilder.setIcon(R.drawable.alert_dialog_icon)
                   .setTitle(R.string.fm_command_timeout_title);
         dlgBuilder.setMessage(R.string.fm_tune_timeout_msg);
         dlgBuilder.setPositiveButton(R.string.alert_dialog_ok,
                                      new DialogInterface.OnClickListener() {
                                         public void onClick(DialogInterface dialog,
                                                             int whichButton) {
                                            cleanupTimeoutHandler();
                                            closeDialog(DIALOG_CMD_TIMEOUT);
                                         }
                                      });
         return(dlgBuilder.create());
      }else {
         return(null);
      }
   }

   private Dialog createCmdFailedDlg(int id, AlertDialog.Builder dlgBuilder) {
      dlgBuilder.setIcon(R.drawable.alert_dialog_icon)
                .setTitle(R.string.fm_command_failed_title);
      dlgBuilder.setMessage(R.string.fm_cmd_failed_msg);

      dlgBuilder.setPositiveButton(R.string.alert_dialog_ok,
                                   new DialogInterface.OnClickListener() {
                                      public void onClick(DialogInterface dialog,
                                                          int whichButton) {
                                          closeDialog(DIALOG_CMD_FAILED);
                                         mCommandFailed = CMD_NONE;
                                      }
                                   });

      return(dlgBuilder.create());
   }

   private Dialog createCmdFailedDlgCallOn(int id) {
       AlertDialog.Builder dlgBuilder = new AlertDialog.Builder(this);
       dlgBuilder.setIcon(R.drawable.alert_dialog_icon)
                 .setTitle(R.string.fm_command_failed_title);
       dlgBuilder.setMessage(R.string.fm_cmd_failed_call_on);

       dlgBuilder.setPositiveButton(R.string.alert_dialog_ok,
                                    new DialogInterface.OnClickListener() {
                                       public void onClick(DialogInterface dialog,
                                                           int whichButton) {
                                           closeDialog(DIALOG_CMD_FAILED_CALL_ON);
                                          mCommandFailed = CMD_NONE;
                                          finish();
                                       }
                                    });

       return(dlgBuilder.create());
    }
   private void RestoreDefaults() {
      try {
          mService.restoreDefaults();
      } catch (RemoteException e) {
          e.printStackTrace();
      }
      FmSharedPreferences.SetDefaults();
      mPrefs.Save();
   }

   private View.OnLongClickListener mFrequencyViewClickListener =
      new View.OnLongClickListener() {
        public boolean onLongClick(View v) {
          FmConfig fmConfig = FmSharedPreferences.getFMConfiguration();
          mDlgPickFrequency = new FrequencyPickerDialog(FMRadio.this, fmConfig,
                        mTunedStation.getFrequency(), mFrequencyChangeListener);
          mDlgPickFrequency.show();
          return true;
        }
   };

   private View.OnClickListener mForwardClickListener =
      new View.OnClickListener() {
        public void onClick(View v) {
          int frequency = FmSharedPreferences.getNextTuneFrequency();
          Log.d(LOGTAG, "Tune Up: to " + frequency);
          tuneRadio(frequency);
      }
   };

   private View.OnClickListener mBackClickListener =
      new View.OnClickListener() {
        public void onClick(View v) {
          int frequency = FmSharedPreferences.getPrevTuneFrequency();
          Log.d(LOGTAG, "Tune Down: to " + frequency);
          tuneRadio(frequency);
      }
   };

   private View.OnLongClickListener mForwardLongClickListener =
      new View.OnLongClickListener() {
        public boolean onLongClick(View view) {
          SeekNextStation();
          return true;
        }
   };

   private View.OnLongClickListener mBackLongClickListener =
      new View.OnLongClickListener() {
        public boolean onLongClick(View view) {
          SeekPreviousStation();
          return true;
        }
   };

   private View.OnClickListener mPresetsPageClickListener =
      new View.OnClickListener() {
        public void onClick(View v) {
          mPresetPageNumber++;
          setupPresetLayout();
      }
   };

   private View.OnClickListener mPresetButtonClickListener =
      new View.OnClickListener() {
         public void onClick(View view) {
           PresetStation station = (PresetStation)view.getTag();
           if (station != null) {
              Log.d(LOGTAG, "station - " + station.getName() + " ("
                    + station.getFrequency() + ")");
              tuneRadio(station.getFrequency());
              view.startAnimation(mAnimation);
           }
      }
   };

   private View.OnLongClickListener mPresetButtonOnLongClickListener =
      new View.OnLongClickListener() {
        public boolean onLongClick(View view) {
           PresetStation station = (PresetStation)view.getTag();
           mPresetButtonStation = station;
           if (station != null) {
               Log.v(LOGTAG," onLongClick DIALOG_PRESET_OPTIONS :");
               closeDialog(DIALOG_PRESET_OPTIONS);
               displayDialog(DIALOG_PRESET_OPTIONS);
           }else {
               addToPresets();
               view.startAnimation(mAnimation);
           }
         return true;
      }
   };

   FrequencyPickerDialog.OnFrequencySetListener mFrequencyChangeListener
      = new FrequencyPickerDialog.OnFrequencySetListener() {
        public void onFrequencySet(FrequencyPicker view, int frequency) {
           Log.d(LOGTAG, "mFrequencyChangeListener: onFrequencyChanged to: " +
                   frequency);
           tuneRadio(frequency);
        }
   };

    private View.OnClickListener mSpeakerClickListener =
        new View.OnClickListener() {
          @Override
          public void onClick(View v) {
             // TODO Auto-generated method stub
             mSpeakerButton.setClickable(false);
             mSpeakerButton.setOnClickListener(null);
             mHandler.removeCallbacks(mEnableRadioTask);
             mHandler.postDelayed(mEnableSpeakerTask, 0);
          }
    };

   private Runnable mEnableSpeakerTask = new Runnable() {
     public void run() {
       enableSpeaker();
       mSpeakerButton.setClickable(true);
       mSpeakerButton.setOnClickListener(mSpeakerClickListener);
     }
   };

   private View.OnClickListener mMuteModeClickListener =
      new View.OnClickListener() {
        public void onClick(View v) {
           boolean bStatus = false;
           if (mService != null) {
              try {
               if (true == isMuted()) {
                   bStatus = mService.unMute();
               }else {
                   bStatus = mService.mute();
               }
               if (bStatus) {
                   setMuteModeButtonImage(true);
                   v.startAnimation(mAnimation);
               }else {
                   mCommandFailed = CMD_MUTE;
                   if(isCallActive()) {
                       displayDialog(DIALOG_CMD_FAILED_CALL_ON);
                   }else {
                       displayDialog(DIALOG_CMD_FAILED);
                   }
               }
              }catch (RemoteException e) {
                e.printStackTrace();
              }
           }
      }
   };

   private View.OnClickListener mRecordButtonListener =
      new View.OnClickListener() {
        public void onClick(View v) {
            if (isRecording()) {
               stopRecording();
            }else if(!isAnalogModeEnabled()) {
               startRecording();
            }
            invalidateOptionsMenu();
        }
   };
   private Handler mEnableRadioHandler = new Handler();
   private Handler mDisableRadioHandler = new Handler();
   private Runnable mEnableRadioTask = new Runnable() {
    public void run() {
       enableRadio();
       mOnOffButton.setEnabled(true);
       mOnOffButton.setClickable(true);
       mOnOffButton.setOnClickListener(mTurnOnOffClickListener);
    }
  };

  private Runnable mDisableRadioTask = new Runnable() {
    public void run() {
       disableRadio();
       mOnOffButton.setEnabled(true);
       mOnOffButton.setClickable(true);
       mOnOffButton.setOnClickListener(mTurnOnOffClickListener);
    }
  };
  private View.OnClickListener mTurnOnOffClickListener =
      new View.OnClickListener() {
        public void onClick(View v) {
           mOnOffButton.setEnabled(false);
           mOnOffButton.setClickable(false);
           mOnOffButton.setOnClickListener(null);
           if (isFmOn()) {
              mEnableRadioHandler.removeCallbacks(mEnableRadioTask);
              mDisableRadioHandler.removeCallbacks(mDisableRadioTask);
              mDisableRadioHandler.postDelayed(mDisableRadioTask, 0);
           }else {
              mDisableRadioHandler.removeCallbacks(mDisableRadioTask);
              mEnableRadioHandler.removeCallbacks(mEnableRadioTask);
              mEnableRadioHandler.postDelayed(mEnableRadioTask, 0);
           }
           cleanupTimeoutHandler();
      }
   };

   private void setTurnOnOffButtonImage() {
      if (isFmOn() == true) {
         mOnOffButton.setEnabled(true);
      }else {
         mOnOffButton.setEnabled(false);
      }
   }

   private void setMuteModeButtonImage(boolean notify) {
      String fmMutedString;
      if (isMuted() == true) {
         mMuteButton.setImageResource(R.drawable.ic_silent_mode);
         fmMutedString = "FM Radio Muted";
      }else {
         /* Find a icon for Stations */
         mMuteButton.setImageResource(R.drawable.ic_silent_mode_off);
         fmMutedString = "FM Radio Playing";
      }
      if (notify) {
         //Toast.makeText(this, fmMutedString, Toast.LENGTH_SHORT).show();
         Log.d(LOGTAG, fmMutedString);
      }
   }

   private void enableRadio() {
      mIsScaning = false;
      mIsSeeking = false;
      mIsSearching = false;
      boolean bStatus = false;
      if (mService != null) {
          try {
            if(mService.isSSRInProgress()) {
               Log.e(LOGTAG, "SSR In Progress, looping");
               while(mService.isSSRInProgress()) {
                  try
                  {
                     Thread.sleep(500);
                  } catch (InterruptedException e)
                  {
                     break;
                  }
               }
               Log.e(LOGTAG, "SSR done, continuing");
            }

            if((false == mService.isFmOn()) && isAntennaAvailable()) {
                bStatus = mService.fmOn();
                if(bStatus) {
                   tuneRadio(FmSharedPreferences.getTunedFrequency());
                   enableRadioOnOffUI();
                }else {
                   Log.e(LOGTAG, "mService.fmOn failed");
                   mCommandFailed = CMD_FMON;
                   if(isCallActive()) {
                      enableRadioOnOffUI();
                      mDlgCmdFailedCallOn = createCmdFailedDlgCallOn(DIALOG_CMD_FAILED_CALL_ON);
                      mDlgCmdFailedCallOn.show();
                   }else {
                      displayDialog(DIALOG_CMD_FAILED);
                   }
                }
            }else {
                enableRadioOnOffUI();
            }
         }catch (RemoteException e) {
            e.printStackTrace();
         }
      }
   }

   private void disableRadio() {
      boolean bStatus = false;

      cancelSearch();
      endSleepTimer();

      // Stop if there is an ongoing Record
      if(mRecording) {
         stopRecording();
      }

      if(mService != null) {
         try {
            bStatus = mService.fmOff();
            if (bStatus == false) {
                mCommandFailed = CMD_FMOFF;
                Log.e(LOGTAG, " mService.fmOff failed");
            }

            enableRadioOnOffUI();
         }catch (RemoteException e) {
            e.printStackTrace();
         }
      }
   }

   private void resetRadio() {
      boolean bSpeakerPhoneOn = isSpeakerEnabled();
      resetSearch();
      endSleepTimer();
      if (mRecording) {
         //Stop if there is an ongoing Record
         stopRecording();
      }
      if (mService != null) {
         try {
            mService.fmRadioReset();
            enableRadioOnOffUI(false);

            Log.e(LOGTAG, "Done with reset, restarting FM");
            /* Start Turn ON sequence again */
            mOnOffButton.setEnabled(false);
            mOnOffButton.setClickable(false);
            mOnOffButton.setOnClickListener(null);

            mDisableRadioHandler.removeCallbacks(mDisableRadioTask);
            mEnableRadioHandler.removeCallbacks(mEnableRadioTask);
            mEnableRadioHandler.postDelayed(mEnableRadioTask, 2000);

            cleanupTimeoutHandler();
            Log.e(LOGTAG, "Done with restart");
         }catch (RemoteException e) {
            e.printStackTrace();
         }
      }
   }
   public void clearStationList() {
      SharedPreferences sp = getSharedPreferences(SCAN_STATION_PREFS_NAME, 0);
      SharedPreferences.Editor editor = sp.edit();
      editor.clear();
      editor.commit();
   }
   public boolean fmConfigure() {
      boolean bStatus = true;
      if(mService != null) {
         try {
            bStatus = mService.fmReconfigure();
            if (bStatus == false) {
               mCommandFailed = CMD_FMCONFIGURE;
               Log.e(LOGTAG, "mService.fmReconfigure failed");
            }else {
            }
         }catch (RemoteException e) {
            e.printStackTrace();
         }
      }
      return bStatus;
   }
   public void fmAutoAFSwitch() {
      boolean bStatus = false;
      if (mService != null) {
         try {
            bStatus = mService.enableAutoAF(FmSharedPreferences.getAutoAFSwitch());
            if (bStatus == false) {
               mCommandFailed = CMD_SET_AUTOAF;
               Log.e(LOGTAG, " mService.enableAutoAF failed");
            }
         }catch (RemoteException e) {
            e.printStackTrace();
         }
      }
   }

   public void fmAudioOutputMode() {
      boolean bStatus = false;
      if (mService != null) {
         try {
            bStatus = mService.enableStereo(FmSharedPreferences.getAudioOutputMode());
            if (bStatus == false) {
               mCommandFailed = CMD_SET_AUDIO_MODE;
               Log.e(LOGTAG, "mService.enableStereo failed");
            }
         }catch (RemoteException e) {
            e.printStackTrace();
         }
      }
   }

   private void startRecording() {
      if(mService != null) {
         try {
             Log.d(LOGTAG, "startRecording permssion req");
             requestPermissions(RECORD_PERMISSIONS, ALL_PERMISSIONS);
             mRecording = mService.startRecording();
         }catch (RemoteException e) {
             e.printStackTrace();
         }
         //Initiate record timer thread here
         if(mRecording == true) {
             mRecordingMsgTV.setCompoundDrawablesWithIntrinsicBounds
                                   (R.drawable.recorder_stop, 0, 0, 0);
             int durationInMins = FmSharedPreferences.getRecordDuration();
             Log.e(LOGTAG, "Fected duration: " + durationInMins);
             initiateRecordDurationTimer( durationInMins );
         }
      }
   }

   private void setRecordingStopImage() {
       if(null != mRecordingMsgTV) {
          mRecordingMsgTV.setCompoundDrawablesWithIntrinsicBounds
                           (R.drawable.recorder_stop, 0, 0, 0);
       }
   }

   private void setRecordingStartImage() {
       if(null != mRecordingMsgTV) {
          mRecordingMsgTV.setCompoundDrawablesWithIntrinsicBounds
                           (R.drawable.recorder_start, 0, 0, 0);
       }
   }

   private void startRecordingTimer() {
      mRecording = true;
      int durationInMins = FmSharedPreferences.getRecordDuration();
      Log.e(LOGTAG, " Fected duration:" + durationInMins );
      initiateRecordDurationTimer( durationInMins );
      setRecordingStopImage();
      invalidateOptionsMenu();
   }

   private void stopRecording() {
       if (mService != null) {
           try {
              mService.stopRecording();
           }catch (RemoteException e) {
              e.printStackTrace();
           }
        }
   }

   private boolean isRecording() {
      mRecording = false;
      if (mService != null) {
         try {
             mRecording = mService.isFmRecordingOn();
         }catch (RemoteException e) {
             e.printStackTrace();
         }
      }
      return(mRecording);
   }

   private boolean isRtPlusSupported() {
      mRtPlusSupported = false;
      if (mService != null) {
          try {
             mRtPlusSupported = mService.isRtPlusSupported();
          } catch (RemoteException e) {
             e.printStackTrace();
         }
      }
      Log.d(LOGTAG, "mRtPlusSupported: " + mRtPlusSupported);
      return(mRtPlusSupported);
   }

   private boolean isA2DPConnected() {
      boolean A2DPConnected = false;
      if (mService != null) {
          try {
             A2DPConnected = mService.isA2DPConnected();
          } catch (RemoteException e) {
             e.printStackTrace();
         }
      }
      Log.d(LOGTAG, "A2DPConnected: " + A2DPConnected);
      return(A2DPConnected);
   }

   private boolean isSpeakerEnabled() {
      boolean speakerEnabled = false;
      if (mService != null) {
         try {
             speakerEnabled = mService.isSpeakerEnabled();
         }catch (RemoteException e) {
             e.printStackTrace();
         }
      }
      return(speakerEnabled);
   }

   private boolean stationExists(PresetStation station ){
       boolean exists = FmSharedPreferences.sameStationExists(station);
       if(exists){
           Toast t = Toast.makeText(this, getString(R.string.station_exists), Toast.LENGTH_SHORT);
           t.show();
       }
       return exists;
   }

   private void addToPresets() {
      int currentList = FmSharedPreferences.getCurrentListIndex();
      PresetStation selectedStation = getCurrentTunedStation();
      if(!stationExists(selectedStation)) {
         FmSharedPreferences.addStation(selectedStation.getName(), selectedStation
                        .getFrequency(), currentList);
         setupPresetLayout();
      }
   }

   private void enableRadioOnOffUI() {
      boolean bEnable = isFmOn();
      /* Disable if no antenna/headset is available */
      if (!isAntennaAvailable()) {
          bEnable = false;
      }
      enableRadioOnOffUI(bEnable);
   }

   private void enableRadioOnOffUI(boolean bEnable) {
      if (mMuteButton != null) {
          mMuteButton.setEnabled(bEnable);
          setMuteModeButtonImage(false);
      }
      if (bEnable) {
         if (mRadioTextScroller != null) {
             mRadioTextScroller.startScroll();
         }
         if (mERadioTextScroller != null) {
             mERadioTextScroller.startScroll();
         }
         if (mTuneStationFrequencyTV != null) {
            mTuneStationFrequencyTV.setOnLongClickListener(mFrequencyViewClickListener);
         }
         invalidateOptionsMenu();
         if ((mRecordingMsgTV != null) && !isRecording()) {
             mRecordingMsgTV.setText("");
         }
         if(isRecording()) {
            setRecordingStopImage();
         }else {
            setRecordingStartImage();
         }
         for (int nButton = 0; nButton < MAX_PRESETS_PER_PAGE; nButton++) {
             if (mPresetButtons[nButton] != null) {
                  mPresetButtons[nButton].setTextColor(Color.WHITE);
             }
        }
      }else {
         if (mRadioTextScroller != null) {
             mRadioTextScroller.stopScroll();
         }
         if (mERadioTextScroller != null) {
             mERadioTextScroller.stopScroll();
         }
         for (int nButton = 0; nButton < MAX_PRESETS_PER_PAGE; nButton++) {
             if (mPresetButtons[nButton] != null) {
                  mPresetButtons[nButton].setTextColor(Color.BLACK);
             }
         }
      }
      if (mForwardButton != null) {
          mForwardButton.setVisibility(((bEnable == true) ? View.VISIBLE
                                        : View.INVISIBLE));
      }
      if (mBackButton != null) {
         mBackButton.setVisibility(((bEnable == true) ? View.VISIBLE
                                        : View.INVISIBLE));
      }
      if (mTuneStationFrequencyTV != null) {
         mTuneStationFrequencyTV.setVisibility(((bEnable == true) ? View.VISIBLE
                                             : View.INVISIBLE));
      }
      if (mPicker != null) {
          mPicker.setVisibility(
                  bEnable ? View.VISIBLE : View.INVISIBLE );
      }
      if (mStationCallSignTV != null) {
          mStationCallSignTV.setVisibility(((bEnable == true) ? View.VISIBLE
                                        : View.INVISIBLE));
      }
      if (mProgramTypeTV != null) {
          mProgramTypeTV.setVisibility(((bEnable == true) ? View.VISIBLE
                                    : View.INVISIBLE));
      }
      if (mSleepMsgTV != null) {
         mSleepMsgTV.setVisibility(((bEnable && isSleepTimerActive()) ? View.VISIBLE
                                 : View.INVISIBLE));
      }
      if (mRecordingMsgTV != null) {
         mRecordingMsgTV.setVisibility(((bEnable == true) ? View.VISIBLE
                                     : View.INVISIBLE));
      }
      if (mRadioTextTV != null) {
         mRadioTextTV.setVisibility(((bEnable == true) ? View.VISIBLE
                                  : View.INVISIBLE));
      }
      if(mERadioTextTV != null) {
         mERadioTextTV.setVisibility(((bEnable == true) ? View.VISIBLE
                                  : View.INVISIBLE));
      }
      if (mProgramServiceTV != null) {
         mProgramServiceTV.setVisibility(((bEnable == true) ? View.VISIBLE
                                  : View.INVISIBLE));
      }

      if (!isAntennaAvailable()) {
         if (mRadioTextTV != null) {
            mRadioTextTV.setVisibility(View.VISIBLE);
            mRadioTextTV.setText(getString(R.string.msg_noantenna));
            mRadioTextScroller.mOriginalString = getString(R.string.msg_noantenna);
         }
         if (mOnOffButton != null) {
            mOnOffButton.setEnabled(false);
         }
      }else if (isCallActive()) {
         if (mRadioTextTV != null) {
            mRadioTextTV.setText("");
            mRadioTextScroller.mOriginalString = "";
         }
         if (mERadioTextTV != null) {
             mERadioTextTV.setText("");
             mERadioTextScroller.mOriginalString = "";
         }
         if (mOnOffButton != null) {
            mOnOffButton.setEnabled(false);
         }
      }else {
         if (mRadioTextTV != null) {
             mRadioTextTV.setText("");
             mRadioTextScroller.mOriginalString = "";
         }
         if (mERadioTextTV != null) {
             mERadioTextTV.setText("");
             mERadioTextScroller.mOriginalString = "";
         }
         if (mOnOffButton != null) {
             mOnOffButton.setEnabled(true);
         }
      }

      if (mStereoTV != null) {
          mStereoTV.setVisibility(((bEnable == true) ? View.VISIBLE
                                   : View.INVISIBLE));
      }
      for (int nButton = 0; nButton < MAX_PRESETS_PER_PAGE; nButton++) {
         if (mPresetButtons[nButton] != null) {
            mPresetButtons[nButton].setEnabled(bEnable);
         }
      }
      if (mPresetListButton != null) {
         mPresetListButton.setEnabled(bEnable);
      }
      if (mPresetPageButton != null) {
         mPresetPageButton.setEnabled(bEnable &&
                                   (FmSharedPreferences.getListStationCount() >= MAX_PRESETS_PER_PAGE));
      }
      if(mSpeakerButton != null) {
        mSpeakerButton.setEnabled(bEnable);
        if (bEnable) {
            if(isSpeakerEnabled()) {
               mSpeakerButton.setImageResource(R.drawable.btn_speaker);
            }else {
               mSpeakerButton.setImageResource(R.drawable.btn_earphone);
            }
        }else{
            mSpeakerButton.setImageResource(R.drawable.btn_earphone);
        }
      }
   }

   private void resetSearchProgress() {
      Message msg = new Message();
      msg.what = END_PROGRESS_DLG;
      mSearchProgressHandler.sendMessage(msg);
   }

   private void updateSearchProgress() {
      boolean searchActive = isScanActive() || isSeekActive() || isSearchActive();
      if (searchActive) {
         synchronized (this) {
            if(mProgressDialog == null) {
                displayDialog(DIALOG_PROGRESS_PROGRESS);
            }else {
               Message msg = new Message();
               msg.what = UPDATE_PROGRESS_DLG;
               mSearchProgressHandler.sendMessage(msg);
            }
         }
      }else {
         Message msg = new Message();
         msg.what = END_PROGRESS_DLG;
         mSearchProgressHandler.sendMessage(msg);
      }
   }

   private void setupPresetLayout() {
      int numStations = FmSharedPreferences.getListStationCount();
      int addedStations = 0;

      /*
       * Validate mPresetPageNumber (Preset Page Number)
       */
      if (mPresetPageNumber > ((numStations) / MAX_PRESETS_PER_PAGE)) {
         mPresetPageNumber = 0;
      }

      /*
       * For every station, save the station as a tag and update the display
       * on the preset Button.
       */
      for (int buttonIndex = 0; (buttonIndex < MAX_PRESETS_PER_PAGE); buttonIndex++) {
         if (mPresetButtons[buttonIndex] != null) {
            mPresetButtons[buttonIndex].setHeight(-1);
            int stationIdex = (mPresetPageNumber * MAX_PRESETS_PER_PAGE)
                                + buttonIndex;
            PresetStation station = FmSharedPreferences.getStationInList(stationIdex);
            String display = "";
            if (station != null) {
               display = station.getName();
                if (display.length() > 6)
                    display = display.substring(0,6)+"...";
               mPresetButtons[buttonIndex].setEllipsize(TextUtils.TruncateAt.END);
               mPresetButtons[buttonIndex].setText(display);
               mPresetButtons[buttonIndex].setTag(station);
               addedStations++;
            }else {
               mPresetButtons[buttonIndex].setText(R.string.add_station);
               mPresetButtons[buttonIndex].setTag(station);
            }
         }
      }
   }

   private void updateStationInfoToUI() {
      double frequency = mTunedStation.getFrequency() / 1000.0;
      mTuneStationFrequencyTV.setText("" + frequency + "MHz");
      if ((mPicker != null) && mUpdatePickerValue) {
          mPicker.setValue(((mTunedStation.getFrequency() - mPrefs.getLowerLimit())
                              / mPrefs.getFrequencyStepSize()));
      }
      mStationCallSignTV.setText(mTunedStation.getPIString());
      mProgramTypeTV.setText(mTunedStation.getPtyString());
      mRadioTextTV.setText("");
      mERadioTextTV.setText("");
      mRadioTextScroller.mOriginalString = "";
      mRadioTextScroller.mStringlength = 0;
      mRadioTextScroller.mIteration = 0;
      mERadioTextScroller.mOriginalString = "";
      mERadioTextScroller.mStringlength = 0;
      mERadioTextScroller.mIteration = 0;
      mProgramServiceTV.setText("");
      mStereoTV.setText("");
      setupPresetLayout();
   }


   private boolean isFmOn() {
      boolean bOn = false;
      if(mService != null) {
         try {
            bOn = mService.isFmOn();
         }catch (RemoteException e) {
            e.printStackTrace();
         }
      }
      return(bOn);
   }

   private boolean isAnalogModeEnabled() {
      boolean aEnabled = false;
      if (mService != null) {
          try {
              aEnabled = mService.isAnalogModeEnabled();
          }catch (RemoteException e) {
              e.printStackTrace();
          }
      }
      return (aEnabled);
   }

   private boolean isMuted() {
      boolean bMuted = false;
      if (mService != null) {
         try {
            bMuted = mService.isMuted();
         }catch (RemoteException e) {
            e.printStackTrace();
         }
      }
      return(bMuted);
   }

   private boolean isScanActive() {
      return(mIsScaning);
   }

   private boolean isSeekActive() {
      return(mIsSeeking);
   }
   private boolean isSearchActive() {
      return(mIsSearching);
   }

   public PresetStation getCurrentTunedStation() {
      return mTunedStation;
   }

   private void SeekPreviousStation() {
      Log.d(LOGTAG, "SeekPreviousStation");
      if (mService != null) {
         try {
            if(!isSeekActive()) {
               mIsSeeking = mService.seek(false);
               if (mIsSeeking == false) {
                  mCommandFailed = CMD_SEEK;
                  Log.e(LOGTAG, "mService.seek failed");
                  displayDialog(DIALOG_CMD_FAILED);
               }
            }
         }catch (RemoteException e) {
            e.printStackTrace();
         }
      }
      updateSearchProgress();
   }

   private void SeekNextStation() {
      Log.d(LOGTAG, "SeekNextStation");
      if(mService != null) {
         try {
            if(!isSeekActive()) {
               mIsSeeking = mService.seek(true);
               if (mIsSeeking == false) {
                  mCommandFailed = CMD_SEEK;
                  Log.e(LOGTAG, "mService.seek failed");
                  displayDialog(DIALOG_CMD_FAILED);
               }
            }
         }catch (RemoteException e) {
            e.printStackTrace();
         }
      }
      updateSearchProgress();
   }

   private void A2DPConnectionState(boolean state) {
      Log.d(LOGTAG, "A2DPConnectionState with: " + state);
      if (state) {
          Log.d(LOGTAG, "A2DP connected, set button to speaker");
          mSpeakerButton.setImageResource(R.drawable.btn_speaker);
      } else {
          Log.d(LOGTAG, "A2DP dis-connected, set button to earphone");
          mSpeakerButton.setImageResource(R.drawable.btn_earphone);
      }
   }
   /** Scan related */
   private void initiateSearch(int pty) {
      synchronized (this) {
         mIsScaning = true;
         if(mService != null) {
            try {
               mIsScaning = mService.scan(pty);
               if (mIsScaning == false) {
                  mCommandFailed = CMD_SCAN;
                  Log.e(LOGTAG, "mService.scan failed");
                  displayDialog(DIALOG_CMD_FAILED);
               }else {
                  mScanPty = pty;
               }
            }catch (RemoteException e) {
               e.printStackTrace();
            }
            updateSearchProgress();
         }
      }
   }

   /** SEEK Station with the matching PI */
   private void initiatePISearch(int pi) {
      Log.d(LOGTAG, "initiatePISearch");
      if(mService != null) {
         try {
            if(!isSeekActive()) {
               mIsSeeking = mService.seekPI(pi);
               if (mIsSeeking == false) {
                  mCommandFailed = CMD_SEEKPI;
                  Log.e(LOGTAG, "mService.seekPI failed");
                  displayDialog(DIALOG_CMD_FAILED);
               }
            }
         }catch (RemoteException e) {
            e.printStackTrace();
         }
      }
      updateSearchProgress();
   }

   private void resetSearch() {
      mIsScaning = false;
      mIsSeeking = false;
      mIsSearching = false;
      resetSearchProgress();
   }

   private void cancelSearch() {
      synchronized (this) {
         if (mService != null) {
            try {
               if ((mIsScaning == true)
                   || (mIsSeeking == true)
                   || (mIsSearching == true)) {
                   mService.cancelSearch();
                   mIsScaning = false;
                   mIsSeeking = false;
                   mIsSearching=false;
               }
            }catch (RemoteException e) {
               e.printStackTrace();
            }
         }
      }
      updateSearchProgress();
      invalidateOptionsMenu();
   }

   /** get Strongest Stations */
   private void initiateSearchList() {
      synchronized (this) {
         mIsSearching = false;
         if (mService != null) {
            try {
               mIsSearching = mService.searchStrongStationList(NUM_AUTO_PRESETS_SEARCH);
               if (mIsSearching == false) {
                  mCommandFailed = CMD_SEARCHLIST;
                  Log.e(LOGTAG, "mService.searchStrongStationList failed");
                  displayDialog(DIALOG_CMD_FAILED);
               }
            }catch (RemoteException e) {
               e.printStackTrace();
            }
            updateSearchProgress();
         }
      }
   }

   private static final int UPDATE_PROGRESS_DLG = 1;
   private static final int END_PROGRESS_DLG = 2;
   private static final int TIMEOUT_PROGRESS_DLG = 3;
   private static final int SHOWBUSY_TIMEOUT = 300000;
   private Handler mSearchProgressHandler = new Handler() {
       public void handleMessage(Message msg) {
           if (msg.what == UPDATE_PROGRESS_DLG) {
              if(mProgressDialog != null) {
                 double frequency = mTunedStation.getFrequency() / 1000.0;
                 String titleStr = getString(R.string.msg_search_title, ("" + frequency));
                 mProgressDialog.setTitle(titleStr);
              }
           }else if (msg.what == END_PROGRESS_DLG) {
              mSearchProgressHandler.removeMessages(END_PROGRESS_DLG);
              mSearchProgressHandler.removeMessages(UPDATE_PROGRESS_DLG);
              mSearchProgressHandler.removeMessages(TIMEOUT_PROGRESS_DLG);
              closeDialog(DIALOG_PROGRESS_PROGRESS);
              mProgressDialog = null;
           }else if (msg.what == TIMEOUT_PROGRESS_DLG) {
              cancelSearch();
           }
       }
   };

   /** Sleep Handling: After the timer expires, the app needs to shut down */
   private static final int SLEEPTIMER_EXPIRED = 0x1001;
   private static final int SLEEPTIMER_UPDATE = 0x1002;
   private Thread mSleepUpdateHandlerThread = null;
   /*
    * Phone time when the App has to be shut down, calculated based on what the
    * user configured
    */
   private static long mSleepAtPhoneTime = 0;

   private void initiateSleepTimer(long seconds) {
      mSleepAtPhoneTime = (SystemClock.elapsedRealtime()) + (seconds * 1000);
      Log.d(LOGTAG, "Sleep in seconds: " + seconds);
      initiateSleepThread();
   }
   private void initiateSleepThread() {
      if (mSleepUpdateHandlerThread == null) {
         mSleepUpdateHandlerThread = new Thread(null, doSleepProcessing,
                                                "SleepUpdateThread");
      }
      /* Launch he dummy thread to simulate the transfer progress */
      Log.d(LOGTAG, "Thread State: " + mSleepUpdateHandlerThread.getState());
      if (mSleepUpdateHandlerThread.getState() == Thread.State.TERMINATED) {
         mSleepUpdateHandlerThread = new Thread(null, doSleepProcessing,
                                                "SleepUpdateThread");
      }
      /* If the thread state is "new" then the thread has not yet started */
      if(mSleepUpdateHandlerThread.getState() == Thread.State.NEW && isFmOn()) {
          try {
              if((mService != null) &&
                 !mService.isSleepTimerActive()) {
                 long timeNow = ((SystemClock.elapsedRealtime()));
                 if(timeNow < mSleepAtPhoneTime) {
                    mService.delayedStop((mSleepAtPhoneTime - timeNow),
                                            FMRadioService.STOP_SERVICE);
                 }
              }
              mSleepUpdateHandlerThread.start();
          }catch(Exception e) {
              e.printStackTrace();
          }
      }
   }

   private void endSleepTimer() {
      mSleepAtPhoneTime = 0;
      try {
           if(mService != null) {
              mService.cancelDelayedStop(FMRadioService.STOP_SERVICE);
           }
      }catch(RemoteException e) {
           e.printStackTrace();
      }

      if(null != mSleepUpdateHandlerThread) {
         mSleepUpdateHandlerThread.interrupt();
         try {
             mSleepUpdateHandlerThread.join();
         } catch (Exception e) {
             e.printStackTrace();
         }
      }
      if(null != mSleepMsgTV) {
         mSleepMsgTV.setVisibility(View.INVISIBLE);
      }
   }

   private boolean hasSleepTimerExpired() {
      boolean expired = true;
      if (isSleepTimerActive()) {
         long timeNow = ((SystemClock.elapsedRealtime()));
         if (timeNow < mSleepAtPhoneTime) {
             expired = false;
         }
      }
      return expired;
   }

   private boolean isSleepTimerActive() {
      boolean active = false;
      try {
          if((mService != null)
             && (mService.isSleepTimerActive())
             && (mSleepAtPhoneTime > 0)) {
             active = true;
             Log.d(LOGTAG, "Sleeptimer is active");
          }else {
          }
      }catch(RemoteException e) {
          e.printStackTrace();
      }
      return active;
   }

   private void updateExpiredSleepTime() {
      int vis = View.INVISIBLE;
      if (isSleepTimerActive()) {
         long timeNow = ((SystemClock.elapsedRealtime()));
         if (mSleepAtPhoneTime >= timeNow) {
            long seconds = (mSleepAtPhoneTime - timeNow) / 1000;
            String sleepMsg = makeTimeString(seconds);
            mSleepMsgTV.setText(sleepMsg);
            if (seconds < SLEEP_TOGGLE_SECONDS) {
               int nowVis = mSleepMsgTV.getVisibility();
               vis = (nowVis == View.INVISIBLE) ? View.VISIBLE
                     : View.INVISIBLE;
            }else {
               vis = View.VISIBLE;
            }
         }else {
            /* Clean up timer */
            mSleepAtPhoneTime = 0;
         }
      }
      mSleepMsgTV.setVisibility(vis);
   }

   private Handler mUIUpdateHandlerHandler = new Handler() {
      public void handleMessage(Message msg) {
         switch (msg.what) {
         case SLEEPTIMER_EXPIRED: {
               mSleepAtPhoneTime = 0;
               DebugToasts("Turning Off FM Radio", Toast.LENGTH_SHORT);
               disableRadio();
               return;
            }
         case SLEEPTIMER_UPDATE: {
               updateExpiredSleepTime();
               break;
            }
         case RECORDTIMER_EXPIRED: {
               Log.d(LOGTAG, "mUIUpdateHandlerHandler - RECORDTIMER_EXPIRED");
               //Clear the Recorder text
               mRecordingMsgTV.setText("");
               if (mRecording != false) {
                  DebugToasts("Stop Recording", Toast.LENGTH_SHORT);
                  stopRecording();
               }
              return;
            }
         case RECORDTIMER_UPDATE: {
               Log.d(LOGTAG, "mUIUpdateHandlerHandler - RECORDTIMER_UPDATE");
               updateExpiredRecordTime();
               break;
            }
         default:
               break;
         }
         super.handleMessage(msg);
      }
   };

   /* Thread processing */
   private Runnable doSleepProcessing = new Runnable() {
      public void run() {
         boolean sleepTimerExpired = hasSleepTimerExpired();
         while ((sleepTimerExpired == false) &&
                    (!Thread.currentThread().isInterrupted())) {
            try {
                Thread.sleep(500);
                Message statusUpdate = new Message();
                statusUpdate.what = SLEEPTIMER_UPDATE;
                Log.d(LOGTAG, "SLEEP TIMER UPDATE");
                mUIUpdateHandlerHandler.sendMessage(statusUpdate);
                sleepTimerExpired = hasSleepTimerExpired();
            }catch (Exception ex) {
                Log.d( LOGTAG, "RunningThread InterruptedException");
                break;
            }
         }
         if(true == sleepTimerExpired) {
             Message finished = new Message();
             finished.what = SLEEPTIMER_EXPIRED;
             mUIUpdateHandlerHandler.sendMessage(finished);
         }
      }
   };

   private static StringBuilder sFormatBuilder = new StringBuilder();
   private static Formatter sFormatter = new Formatter(sFormatBuilder, Locale
                                                       .getDefault());
   private static final Object[] sTimeArgs = new Object[5];

   private String makeTimeString(long secs) {
      String durationformat = getString(R.string.durationformat);

      /*
       * Provide multiple arguments so the format can be changed easily by
       * modifying the xml.
       */
      sFormatBuilder.setLength(0);

      final Object[] timeArgs = sTimeArgs;
      timeArgs[0] = secs / 3600;
      timeArgs[1] = secs / 60;
      timeArgs[2] = (secs / 60) % 60;
      timeArgs[3] = secs;
      timeArgs[4] = secs % 60;

      return sFormatter.format(durationformat, timeArgs).toString();
   }

   private void tuneRadio(int frequency){
      /* Issue the tune command only if tuneCommand is already not active */
      if((mService != null) && (mCommandActive != CMD_TUNE) && isFmOn()) {
         boolean bStatus = false;
         try {
             bStatus = mService.tune(frequency);
             if (bStatus) {
                 postTimeoutHandler(CMD_TUNE);
             }else {
               if (isFmOn()) {
                  mCommandFailed = CMD_TUNE;
                  Log.e(LOGTAG, "mService.tune failed");
                  displayDialog(DIALOG_CMD_FAILED);
               }
             }
             mTunedStation.setName("");
             mTunedStation.setPI(0);
             mTunedStation.setPty(0);
             updateStationInfoToUI();
         }catch (RemoteException e) {
            e.printStackTrace();
         }
      }else {
         Log.e(LOGTAG, "Delayed Tune handler stopped");
      }
   }

   /* Start a Command timeout
   */
   private synchronized void postTimeoutHandler(int cmd){
      mCommandActive = cmd;
      mCommandTimeoutHandler.sendEmptyMessageDelayed(MSG_CMD_TIMEOUT, CMD_TIMEOUT_DELAY_MS);
   }

   /* Stop the Command timeout
   */
   private synchronized void cleanupTimeoutHandler(){
      mCommandActive = CMD_NONE;
      mCommandTimeoutHandler.removeMessages(MSG_CMD_TIMEOUT);
   }
   /* Command timeout Handler
      Routine to handle the Command timeouts for FM operations
      that return asynchronous event callbacks
   */
   private Handler mCommandTimeoutHandler = new Handler() {
      @Override
      public void handleMessage(Message msg) {
         switch (msg.what) {
         case MSG_CMD_TIMEOUT: {
               if (mCommandActive > 0) {
                  Log.d(LOGTAG, "mCommandTimeoutHandler: Cmd failed: " + mCommandActive);
                  mCommandTimeoutHandler.removeMessages(MSG_CMD_TIMEOUT);
                  displayDialog(DIALOG_CMD_TIMEOUT);
                  return;
               }
               break;
            }//case MSG_CMD_TIMEOUT
         }//switch
      }//handleMessage
   };

   private void resetFMStationInfoUI() {
      mTunedStation.setFrequency(FmSharedPreferences.getTunedFrequency());
      mTunedStation.setName("");
      mTunedStation.setPI(0);
      mTunedStation.setRDSSupported(false);
      mTunedStation.setPty(0);
      mRadioTextTV.setText("");
      mERadioTextTV.setText("");
      mRadioTextScroller.mOriginalString = "";
      mProgramServiceTV.setText("");
      mRadioTextScroller.stopScroll();
      mERadioTextScroller.mOriginalString = "";
      mERadioTextScroller.stopScroll();
      mUpdatePickerValue = true;
      updateStationInfoToUI();
   }

   Runnable mRadioEnabled = new Runnable() {
      public void run() {
         /* Update UI to FM On State */
         enableRadioOnOffUI(true);
         /* Tune to the last tuned frequency */
         mUpdatePickerValue = true;
         tuneRadio(FmSharedPreferences.getTunedFrequency());
      }
   };

   Runnable mRadioDisabled = new Runnable() {
      public void run() {
         /* Update UI to FM Off State */
         cleanupTimeoutHandler();
         endSleepTimer();
         stopRecording();
         cancelSearch();
         enableRadioOnOffUI(false);
      }
   };

    Runnable mRadioReset = new Runnable() {
      public void run() {
         /* Update UI to FM Reset (Off) State */
         resetRadio();
      }
   };

   Runnable mUpdateStationInfo = new Runnable() {
      public void run() {
         cleanupTimeoutHandler();
         PresetStation station = new PresetStation("", FmSharedPreferences.getTunedFrequency());
         if (station != null) {
             mTunedStation.Copy(station);
         }
         updateSearchProgress();
         resetFMStationInfoUI();
      }
   };

   Runnable mSearchComplete = new Runnable() {
      public void run() {
         Log.d(LOGTAG, "mSearchComplete: ");
         mScanPty=0;
         mScanPtyIndex = 0;
         mIsScaning = false;
         mIsSeeking = false;
         mIsSearching = false;
         updateSearchProgress();
         resetFMStationInfoUI();
         invalidateOptionsMenu();
      }
   };

   Runnable mOnMute = new Runnable() {
      public void run() {
         setMuteModeButtonImage(true);
      }
   };

   Runnable mOnStereo = new Runnable() {
      public void run() {
         if (FMRADIO_UI_STATION_AUDIO_STEREO == mStereo) {
             if ((Locale.getDefault().toString().equals("zh_HK")))
                 mStereoTV.setText("立體聲");
             else
                 mStereoTV.setText(R.string.audio_type_stereo);
         } else if (FMRADIO_UI_STATION_AUDIO_MONO == mStereo) {
             if ((Locale.getDefault().toString().equals("zh_HK")))
                 mStereoTV.setText("單聲道");
             else
                 mStereoTV.setText(R.string.audio_type_mono);
         }else {
             mStereoTV.setText("");
         }
         FmSharedPreferences.setLastAudioMode(mStereo);
      }
   };

   Runnable mUpdateRadioText = new Runnable() {
      public void run() {
         String str = "";
         if ((mService != null) && isFmOn()) {
            try {
               /* Get Radio Text and update the display */
               str = mService.getRadioText();

               /* Update only if all the characters are printable */
               if (TextUtils.isPrintableAsciiOnly(str)) {
                  Log.d(LOGTAG, "mUpdateRadioText: Updatable string: [" + str + "]");
                  mRadioTextTV.setText(str);
                  mRadioTextScroller.mOriginalString = str;
               }else if(TextUtils.isEmpty(str)) { /* Rest the string to empty*/
                  mRadioTextTV.setText("");
                  mRadioTextScroller.mOriginalString = "";
               }else {
                  Log.v(LOGTAG, "mUpdateRadioText: Leaving old string " + mRadioTextTV.getText());
               }

               /* Get PTY and PI and update the display */
               int tempInt = mService.getProgramType();
               /* Save PTY */
               mTunedStation.setPty(tempInt);
               mProgramTypeTV.setText(PresetStation.parsePTY(tempInt));
               tempInt = mService.getProgramID();
               mStationCallSignTV.setText(PresetStation.parsePI(tempInt));
               if (tempInt != 0) {
                   mTunedStation.setPI(tempInt);
               }
               /* For non-Empty, non-Printable string, just leave the
                  existing old string
               */
               mRadioTextScroller.startScroll();
            }catch (RemoteException e) {
               e.printStackTrace();
            }
         }
      }
   };

   Runnable mRadioChangeFrequency = new Runnable(){
       public void run() {
           mUpdatePickerValue = false;
           tuneRadio(mFrequency);
       }
   };

   Runnable mUpdateExtenRadioText = new Runnable() {
      public void run() {
         String str = "";
         if ((mService != null) && isFmOn()) {
            try {
               /* Get Extended Radio Text and update the display */
               str = mService.getExtenRadioText();
               if (TextUtils.isEmpty(str)) {
                   mERadioTextTV.setText("");
                   mERadioTextScroller.mOriginalString = "";
               }else {
                   mERadioTextTV.setText(str);
                   mERadioTextScroller.mOriginalString = str;
               }
               mERadioTextScroller.startScroll();
            }catch (RemoteException e) {
               e.printStackTrace();
            }
         }
      }
   };

   Runnable mUpdateExtenCountryCode = new Runnable() {
      public void run() {
         String str = "";
         int value;
         if ((mService != null) && isFmOn()) {
            try {
               /* Get Extended Radio Text and update the display */
               value = mService.getExtenCountryCode();
               str = Integer.toString(value);
               Log.d(LOGTAG, "mUpdateExtenCountryCode: Updatable string: [" + str + "]");
               mERadioTextTV.setText(str);
               mERadioTextScroller.mOriginalString = str;
               mERadioTextScroller.startScroll();
            }catch (RemoteException e) {
               e.printStackTrace();
            }
         }
      }
   };

   /* Create runnable for posting */
   Runnable mUpdateProgramService = new Runnable() {
      public void run() {
         String str = "";
         if (mService != null) {
            try {
               /* Get the Station PS and update the display */
               str = mService.getProgramService();
               /* Update only if all the characters are printable */
               //if(isStringPrintable(str))
               if (TextUtils.isPrintableAsciiOnly(str)) {
                  Log.d(LOGTAG, "mUpdateProgramService: Updatable string: [" + str + "]");
                  mProgramServiceTV.setText(str);
               }else if (TextUtils.isEmpty(str)) { /* Rest the string to empty*/
                  mProgramServiceTV.setText("");
               }else {
                  /* For non-Empty, non-Printable string, just leave the
                     existing old string
                  */
               }
               /* Get PTY and PI and update the display */
               int tempInt = mService.getProgramType();
               /* Save PTY */
               mTunedStation.setPty(tempInt);

               mProgramTypeTV.setText(PresetStation.parsePTY(tempInt));
               tempInt =mService.getProgramID();
               /* Save the program ID */
               if (tempInt != 0) {
                   mTunedStation.setPI(tempInt);
               }
               mStationCallSignTV.setText(PresetStation.parsePI(tempInt));
            }catch (RemoteException e) {
               e.printStackTrace();
            }
         }
      }
   };

   private void DebugToasts(String str, int duration) {
      //Toast.makeText(this, str, duration).show();
      Log.d(LOGTAG, "Debug:" + str);
   }

   /**
    * This Handler will scroll the text view.
    * On startScroll, the scrolling starts after SCROLLER_START_DELAY_MS
    * The Text View is scrolled left one character after every
    * SCROLLER_UPDATE_DELAY_MS
    * When the entire text is scrolled, the scrolling will restart
    * after SCROLLER_RESTART_DELAY_MS
    */
   private final class ScrollerText extends Handler {
      private static final byte SCROLLER_STOPPED = 0x51;
      private static final byte SCROLLER_STARTING = 0x52;
      private static final byte SCROLLER_RUNNING = 0x53;

      private static final int SCROLLER_MSG_START   = 0xF1;
      private static final int SCROLLER_MSG_TICK    = 0xF2;
      private static final int SCROLLER_MSG_RESTART = 0xF3;

      private static final int SCROLLER_START_DELAY_MS = 1000;
      private static final int SCROLLER_RESTART_DELAY_MS = 3000;
      private static final int SCROLLER_UPDATE_DELAY_MS = 200;

      private final WeakReference<TextView> mView;

      private byte mStatus = SCROLLER_STOPPED;
      String mOriginalString;
      int mStringlength = 0;
      int mIteration = 0;

      ScrollerText(TextView v) {
         mView = new WeakReference<TextView>(v);
      }

      /**
       * Scrolling Message Handler
       */
      @Override
      public void handleMessage(Message msg) {
         switch (msg.what) {
         case SCROLLER_MSG_START:
            mStatus = SCROLLER_RUNNING;
            updateText();
            break;
         case SCROLLER_MSG_TICK:
            updateText();
            break;
         case SCROLLER_MSG_RESTART:
            if (mStatus == SCROLLER_RUNNING) {
               startScroll();
            }
            break;
         }
      }

      /**
       * Moves the text left by one character and posts a
       * delayed message for next update after SCROLLER_UPDATE_DELAY_MS.
       * If the entire string is scrolled, then it displays the entire string
       * and waits for SCROLLER_RESTART_DELAY_MS for scrolling restart
       */
      void updateText() {
         if (mStatus != SCROLLER_RUNNING) {
            return;
         }
         removeMessages(SCROLLER_MSG_TICK);
         TextView textView = mView.get();
         if (textView != null)  {
            mStringlength = mOriginalString.length();
            String szStr2 = "";
            if (mStringlength > 0) {
               mIteration++;
               if (mIteration >= mStringlength) {
                  mIteration = 0;
                  sendEmptyMessageDelayed(SCROLLER_MSG_RESTART, SCROLLER_RESTART_DELAY_MS);
               }else {
                  sendEmptyMessageDelayed(SCROLLER_MSG_TICK, SCROLLER_UPDATE_DELAY_MS);
               }
               if ((mOriginalString !=null) && (mOriginalString.length() >= mIteration))
                  szStr2 = mOriginalString.substring(mIteration);
            }
            textView.setText(szStr2);
         }
      }

      /**
       * Stops the scrolling
       * The textView will be set to the original string.
       */
      void stopScroll() {
         mStatus = SCROLLER_STOPPED;
         removeMessages(SCROLLER_MSG_TICK);
         removeMessages(SCROLLER_MSG_RESTART);
         removeMessages(SCROLLER_MSG_START);
         resetScroll();
      }

      /**
       * Resets the scroll to display the original string.
       */
      private void resetScroll() {
         mIteration = 0;
         TextView textView = mView.get();
         if (textView != null) {
            textView.setText(mOriginalString);
         }
      }

      /** Starts the Scrolling of the TextView after a
       * delay of SCROLLER_START_DELAY_MS
       * Starts only if Length > 0
       */
      void startScroll() {
         TextView textView = mView.get();
         if (textView != null) {
            mOriginalString = textView.getText().toString();
            mStringlength = mOriginalString.length();
            if (mStringlength > 0) {
               mStatus = SCROLLER_STARTING;
               sendEmptyMessageDelayed(SCROLLER_MSG_START, SCROLLER_START_DELAY_MS);
            }
         }
      }
   }


   public  IFMRadioService sService = null;
   private  HashMap<Context, ServiceBinder> sConnectionMap = new HashMap<Context, ServiceBinder>();

   public  boolean bindToService(Context context) {
      Log.e(LOGTAG, "bindToService: Context");
      return bindToService(context, null);
   }

   public  boolean bindToService(Context context, ServiceConnection callback) {
      Log.e(LOGTAG, "bindToService: Context with serviceconnection callback");
      context.startService(new Intent(context, FMRadioService.class));
      ServiceBinder sb = new ServiceBinder(callback);
      sConnectionMap.put(context, sb);
      return context.bindService((new Intent()).setClass(context,
                                                         FMRadioService.class), sb, 0);
   }

   public  void unbindFromService(Context context) {
      ServiceBinder sb = (ServiceBinder) sConnectionMap.remove(context);
      boolean isFmOn = isFmOn();
      Log.e(LOGTAG, "unbindFromService: Context");
      if (sb == null) {
         Log.e(LOGTAG, "Trying to unbind for unknown Context");
         return;
      }
      context.unbindService(sb);

      Log.e(LOGTAG, "stop FM radio service");
      context.stopService(new Intent(context, FMRadioService.class));

      if (sConnectionMap.isEmpty()) {
         // presumably there is nobody interested in the service at this point,
         // so don't hang on to the ServiceConnection
         sService = null;
      }
   }

   private  class ServiceBinder implements ServiceConnection {
      ServiceConnection mCallback;
      ServiceBinder(ServiceConnection callback) {
         mCallback = callback;
      }

      public void onServiceConnected(ComponentName className, android.os.IBinder service) {
         sService = IFMRadioService.Stub.asInterface(service);
         if (mCallback != null) {
            Log.e(LOGTAG, "onServiceConnected: mCallback");
            mCallback.onServiceConnected(className, service);
         }
      }

      public void onServiceDisconnected(ComponentName className) {
         if (mCallback != null) {
            mCallback.onServiceDisconnected(className);
         }
         sService = null;
         mService = null;
      }
   }

   private ServiceConnection osc = new ServiceConnection() {
      public void onServiceConnected(ComponentName classname, IBinder obj) {
         mService = IFMRadioService.Stub.asInterface(obj);
         Log.e(LOGTAG, "ServiceConnection: onServiceConnected");
         if (mService != null) {
            try {
               mService.registerCallbacks(mServiceCallbacks);
               if (SavedDataAndState == null) {
                  enableRadio();
               }else if (isFmOn()) {
                  enableRadioOnOffUI(true);
               }else {
                  enableRadioOnOffUI(false);
               }
            }catch (RemoteException e) {
               e.printStackTrace();
            }
            if (isRecording()) {
                initiateRecordThread();
            }
            if(isSleepTimerActive()) {
               initiateSleepThread();
            }
            return;
         }else {
            Log.e(LOGTAG, "IFMRadioService onServiceConnected failed");
         }
         if (getIntent().getData() == null) {
            Intent intent = new Intent(Intent.ACTION_MAIN);
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            intent.setClass(FMRadio.this, FMRadio.class);
            startActivity(intent);
         }
         finish();
      }
      public void onServiceDisconnected(ComponentName classname) {
         Log.d(LOGTAG, "Service got disconnected");
         unbindFromService(FMRadio.this);
         mService = null;
         sService = null;
      }
   };

   private IFMRadioServiceCallbacks.Stub  mServiceCallbacks =
    new IFMRadioServiceCallbacks.Stub() {
      public void onEnabled() {
         Log.d(LOGTAG, "mServiceCallbacks.onEnabled :");
         mHandler.post(mRadioEnabled);
      }
      public void onDisabled() {
         Log.d(LOGTAG, "mServiceCallbacks.onDisabled :");
         mHandler.post(mRadioDisabled);
      }
      public void onRadioReset() {
         Log.d(LOGTAG, "mServiceCallbacks.onRadioReset :");
         mHandler.post(mRadioReset);
      }
      public void onTuneStatusChanged()  {
         Log.d(LOGTAG, "mServiceCallbacks.onTuneStatusChanged: ");
         if (mIsScaning) {
             Log.d(LOGTAG, "isScanning....................");
             SharedPreferences sp = getSharedPreferences(SCAN_STATION_PREFS_NAME, 0);
             SharedPreferences.Editor editor = sp.edit();
             int station_number = sp.getInt(NUM_OF_STATIONS, 0);
             station_number++;
             editor.putInt(NUM_OF_STATIONS, station_number);
             editor.putString(STATION_NAME + station_number, station_number + "");
             editor.putInt(STATION_FREQUENCY + station_number,
                                   FmSharedPreferences.getTunedFrequency());
             editor.commit();
         }
         cleanupTimeoutHandler();
         mHandler.post(mUpdateStationInfo);
         mHandler.post(mOnStereo);
      }

      public void onProgramServiceChanged() {
         Log.d(LOGTAG, "mServiceCallbacks.onProgramServiceChanged :");
         mHandler.post(mUpdateProgramService);
      }
      public void onRadioTextChanged()  {
         Log.d(LOGTAG, "mServiceCallbacks.onRadioTextChanged :");
         mHandler.post(mUpdateRadioText);
      }
      public void onExtenRadioTextChanged() {
         mHandler.post(mUpdateExtenRadioText);
      }
      public void onExtenCountryCodeChanged() {
         mHandler.post(mUpdateExtenCountryCode);
      }
      public void onAlternateFrequencyChanged() {
         Log.d(LOGTAG, "mServiceCallbacks.onAlternateFrequencyChanged :");
      }
      public void onSignalStrengthChanged() {
         Log.d(LOGTAG, "mServiceCallbacks.onSignalStrengthChanged :");
      }
      public void onSearchComplete() {
         Log.d(LOGTAG, "mServiceCallbacks.onSearchComplete :");
         mScanPty = 0;
         mScanPtyIndex = 0;
         mIsScaning = false;
         mIsSeeking = false;
         mIsSearching = false;
         mHandler.post(mSearchComplete);
      }
      public void onSearchListComplete() {
         Log.d(LOGTAG, "mServiceCallbacks.onSearchListComplete :");
      }
      public void onMute(boolean bMuted) {
         Log.d(LOGTAG, "mServiceCallbacks.onMute :" + bMuted);
         mHandler.post(mOnMute);
      }
      public void onAudioUpdate(boolean bStereo) {
         if((bStereo) && (FmSharedPreferences.getAudioOutputMode())) {
            mStereo = FMRADIO_UI_STATION_AUDIO_STEREO;
         }else {
            mStereo = FMRADIO_UI_STATION_AUDIO_MONO;
         }
         Log.d(LOGTAG, "mServiceCallbacks.onAudioUpdate :" + mStereo);
         mHandler.post(mOnStereo);
      }
      public void onStationRDSSupported(boolean bRDSSupported) {
         Log.d(LOGTAG, "mServiceCallbacks.onStationRDSSupported :" + bRDSSupported);
         /*
         * Depending on the signal strength etc, RDS Lock Sync/Supported may toggle,
         * Since if a station Supports RDS, it will not change its support intermittently
         * just save the status and ignore any "unsupported" state.
         */
         if (bRDSSupported) {
             mTunedStation.setRDSSupported(true);
         }
      }
      public void onRecordingStopped() {
         Log.d(LOGTAG, "mServiceCallbacks.onRecordingStopped:");
         mRecording = false;
         if(null != mRecordUpdateHandlerThread) {
            mRecordUpdateHandlerThread.interrupt();
         }
         if(null != mRecordingMsgTV) {
            mRecordingMsgTV.setText("");
            setRecordingStartImage();
         }
         invalidateOptionsMenu();
      }
      public void onRecordingStarted()
      {
         Log.d(LOGTAG, "mServiceCallbacks.onRecordingStarted:");
         startRecordingTimer();
      }
      public void onSeekNextStation() {
          Log.d(LOGTAG, "mServiceCallbacks.onSeekNextStation:");
          SeekNextStation();
      }
      public void onA2DPConnectionstateChanged(boolean state){
          Log.d(LOGTAG, "mServiceCallbacks.onA2DPConnectionstateChanged :");
          A2DPConnectionState(state);
      }
      public void onFmAudioPathStarted() {
          Log.d(LOGTAG, "mServiceCallbacks.onFmAudioPathStarted:");
          mSpeakerButton.setClickable(true);
          mMuteButton.setClickable(true);
      }
      public void onFmAudioPathStopped() {
          Log.d(LOGTAG, "mServiceCallbacks.onFmAudioPathStopped:");
          mSpeakerButton.setClickable(false);
          mMuteButton.setClickable(false);
      }
      /* Not used in FmRadio */
      public void getSigThCb(int val, int status) {
          Log.d(LOGTAG,"get Sig Thres callback");
      }
      public void getChDetThCb(int val, int status) {
          Log.d(LOGTAG, "get Channel Det Threshold Callback");
      }
      public void setChDetThCb(int status)
      {
          Log.d(LOGTAG, "set channel Det Threshold Callback");
      }
      public void DefDataRdCb(int val, int status) {
          Log.d(LOGTAG, "Def data Read callback");
      }
      public void DefDataWrtCb(int status)
      {
          Log.d(LOGTAG, "DefDataWrt");
      }
      public void getBlendCb(int val, int status)
      {
          Log.d(LOGTAG, "get blend callback");
      }
      public void setBlendCb(int status)
      {
          Log.d(LOGTAG, "set blend callback");
      }
      public void getStationParamCb(int val, int status)
      {
          Log.d(LOGTAG, "getStationParam");
      }
      public void getStationDbgParamCb(int val, int status)
      {
          Log.d(LOGTAG, "getStationDbgParam");
      }
   };

    private void registerFMSettingListner() {
        if (mFmSettingReceiver == null) {
            mFmSettingReceiver = new BroadcastReceiver() {
                @Override
                public void onReceive(Context context, Intent intent) {
                     Log.d(LOGTAG, "Received intent " + intent);
                     String action = intent.getAction();
                     Log.d(LOGTAG, " action = " + action);
                     if (action.equals(Settings.ACTION_FM_SETTING)) {
                         int state = intent.getIntExtra("state", 0);
                         Log.d(LOGTAG, "ACTION_FM_SETTING Intent received" + state);
                         switch(state) {
                         case Settings.FM_BAND_CHANGED:
                              fmConfigure();
                              break;
                         case Settings.FM_CHAN_SPACING_CHANGED:
                              fmConfigure();
                              break;
                         case Settings.FM_AF_OPTION_CHANGED:
                              fmAutoAFSwitch();
                              break;
                         case Settings.FM_AUDIO_MODE_CHANGED:
                              fmAudioOutputMode();
                              break;
                         }
                     }
                 }
            };
            IntentFilter iFilter = new IntentFilter();
            iFilter.addAction(Settings.ACTION_FM_SETTING);
            registerReceiver(mFmSettingReceiver, iFilter, Context.RECEIVER_EXPORTED);
        }
    }

    private void unRegisterReceiver(BroadcastReceiver myReceiver) {
        if(myReceiver != null) {
           unregisterReceiver(myReceiver);
           myReceiver = null;
        }
    }

    private void displayDialog(int id) {
        Log.d(LOGTAG, " disaplyDialog " + id);
        AlertDialog.Builder dlgBuilder = null;
        if (id != DIALOG_PROGRESS_PROGRESS) {
            dlgBuilder = new AlertDialog.Builder(this);
            dlgBuilder.setOnKeyListener(new OnKeyListener() {
                public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
                    Log.d(LOGTAG, "OnKeyListener event received" + keyCode);
                    switch (keyCode) {
                    case KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE:
                    case 126: // KeyEvent.KEYCODE_MEDIA_PLAY:
                    case 127: // KeyEvent.KEYCODE_MEDIA_PAUSE:
                    case KeyEvent.KEYCODE_MEDIA_FAST_FORWARD:
                    case KeyEvent.KEYCODE_MEDIA_NEXT:
                    case KeyEvent.KEYCODE_MEDIA_PREVIOUS:
                    case KeyEvent.KEYCODE_MEDIA_REWIND:
                    case KeyEvent.KEYCODE_MEDIA_STOP:
                        return true;
                    }
                    return false;
                }
            });
        }
        switch (id) {
        case DIALOG_PROGRESS_PROGRESS:
            mProgressDialog = createProgressDialog();
            mProgressDialog.show();
            break;
        case DIALOG_PRESET_OPTIONS:
            mDlgResetOptions = createPresetOptionsDlg(DIALOG_PRESET_OPTIONS, dlgBuilder);
            mDlgResetOptions.show();
            break;
        case DIALOG_SEARCH:
            mDlgSearch = createSearchDlg(DIALOG_SEARCH, dlgBuilder);
            mDlgSearch.show();
            break;
        case DIALOG_SLEEP:
            mDlgSleep = createSleepDlg(DIALOG_SLEEP, dlgBuilder);
            mDlgSleep.show();
            break;
        case DIALOG_PRESET_RENAME:
            mDlgPresetRename = createPresetRenameDlg(DIALOG_PRESET_RENAME, dlgBuilder);
            mDlgPresetRename.show();
            break;
        case DIALOG_CMD_TIMEOUT:
            mDlgCmdTout = createCmdTimeoutDlg(DIALOG_CMD_TIMEOUT, dlgBuilder);
            mDlgCmdTout.show();
            break;
        case DIALOG_CMD_FAILED:
            mDlgCmdFailed = createCmdFailedDlg(DIALOG_CMD_FAILED, dlgBuilder);
            mDlgCmdFailed.show();
            break;
        case DIALOG_CMD_FAILED_CALL_ON:
            mDlgCmdFailedCallOn = createCmdFailedDlgCallOn(DIALOG_CMD_FAILED_CALL_ON);
            mDlgCmdFailedCallOn.show();
            break;
        }
    }

    private void closeDialog(int id) {
        Log.d(LOGTAG, " closeDialog " + id);
        switch (id) {
        case DIALOG_PROGRESS_PROGRESS:
            if (mProgressDialog != null && mProgressDialog.isShowing()) {
                mProgressDialog.dismiss();
            }
            break;
        case DIALOG_PRESET_OPTIONS:
            if (mDlgResetOptions != null && mDlgResetOptions.isShowing()) {
                mDlgResetOptions.dismiss();
            }
            break;
        case DIALOG_SEARCH:
            if (mDlgSearch != null && mDlgSearch.isShowing()) {
                mDlgSearch.dismiss();
            }
            mDlgSearch = null;
            break;
        case DIALOG_SLEEP:
            if (mDlgSleep != null && mDlgSleep.isShowing()) {
                mDlgSleep.dismiss();
            }
            mDlgSleep = null;
        case DIALOG_PRESET_RENAME:
            if (mDlgPresetRename != null && mDlgPresetRename.isShowing()) {
                mDlgPresetRename.dismiss();
            }
            mDlgPresetRename = null;
            break;
        case DIALOG_CMD_TIMEOUT:
            if (mDlgCmdTout != null && mDlgCmdTout.isShowing()) {
                mDlgCmdTout.dismiss();
            }
            mDlgCmdTout = null;
            break;
        case DIALOG_CMD_FAILED:
            if (mDlgCmdFailed != null && mDlgCmdFailed.isShowing()) {
                mDlgCmdFailed.dismiss();
            }
            mDlgCmdFailed = null;
            break;
        case DIALOG_CMD_FAILED_CALL_ON:
            if (mDlgCmdFailedCallOn != null && mDlgCmdFailedCallOn.isShowing()) {
                mDlgCmdFailedCallOn.dismiss();
            }
            mDlgCmdFailedCallOn = null;
            break;

        default:
            break;
        }
    }
}
