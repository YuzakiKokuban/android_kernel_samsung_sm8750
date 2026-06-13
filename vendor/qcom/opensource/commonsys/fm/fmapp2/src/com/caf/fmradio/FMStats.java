/*
 * Copyright (c) 2011-2014, The Linux Foundation. All rights reserved.
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
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Handler;
import android.os.Message;
import android.os.RemoteException;
import android.util.Log;
import android.util.TypedValue;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.RadioButton;
import android.widget.EditText;
import android.widget.ProgressBar;
import android.widget.Spinner;
import android.widget.TableLayout;
import android.widget.TableRow;
import android.widget.TextView;
import android.widget.AdapterView.OnItemSelectedListener;
import qcom.fmradio.FmReceiver;
import android.os.SystemProperties;
import android.app.AlertDialog;
import android.app.Dialog;
import android.view.LayoutInflater;
import android.content.DialogInterface;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.IntentFilter;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MenuInflater;
import android.os.SystemClock;
import android.graphics.Point;

import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.util.HashMap;
import android.os.SystemProperties;
import java.io.BufferedReader;
import java.io.FileReader;
import java.util.Formatter;
import java.util.Locale;

public class FMStats extends Activity  {

    EditText txtbox1;
    Button button1;
    Button button2;
    TextView tv1;
    Button SetButton;
    Button RunButton;
    ProgressBar  pbar;
    TableLayout tLayout;

    private FmReceiver mReceiver;

    TextView  bandSweepSettingButton;

    /*Data structure for band*/
    private class Band {
      public int lFreq;
      public int hFreq;
      public int Spacing;
      public int cur_freq;
    }
    /* Data structure for Result*/
    private class Result {

      private String mFreq;
      private String mRSSI;
      private String mIoC;
      private String mIntDet;
      private String mMpxDcc;
      private String mSINR;


      public void setFreq(String aFreq) {
         this.mFreq = aFreq;
      }

      public String getFreq() {
         return mFreq;
      }

      public void setRSSI(String aRSSI) {
         this.mRSSI = aRSSI;
      }

      public String getRSSI() {
         return mRSSI;
      }

      public void setIoC(String aIoC) {
         this.mIoC = aIoC;
      }

      public String getIoC() {
         return mIoC;
      }

      public void setIntDet(String aIntDet) {
         this.mIntDet = aIntDet;
      }

      public String getIntDet() {
         return mIntDet;
      }

      public void setMpxDcc(String aMpxDcc) {
         this.mMpxDcc = aMpxDcc;
      }

      public String getMpxDcc() {
         return mMpxDcc;
      }
      public void setSINR(String aSINR) {
         this.mSINR = aSINR;
      }

      public String getSINR() {
         return mSINR;
      }
    };

    /*constant column header*/
    Result mColumnHeader = new Result();

    boolean mTestRunning = false;
    FmRfItemSelectedListener mSpinFmRfListener =
                                     new FmRfItemSelectedListener();
    RfCfgItemSelectedListener mSpinRfCfgListener =
                                     new RfCfgItemSelectedListener();
    CfgRfItemSelectedListener1 mSpinCfgRfListener1 = null;
    CfgRfItemSelectedListener2 mSpinCfgRfListener2 = null;
    CfgRfItemSelectedListener3 mSpinCfgRfListener3 = null;
    CfgRfItemSelectedListener4 mSpinCfgRfListener4 = null;
    BandSweepMthdsSelectedListener mSweepMthdsListener =
                                     new BandSweepMthdsSelectedListener();

    int  mTestSelected = 0;
    boolean mIsSearching = false;
    private static String LOGTAG = "FMStats";
    private IFMRadioService mService = null;
    private Thread mMultiUpdateThread = null;
    private static final int STATUS_UPDATE = 1;
    private static final int STATUS_DONE = 2;
    private static final int RECORDTIMER_UPDATE = 3;
    private static final int RECORDTIMER_EXPIRED = 4;
    private static final int STOP_ROW_ID = 200;
    private static final int NEW_ROW_ID = 300;
    private int mStopIds = STOP_ROW_ID;
    private int mNewRowIds = NEW_ROW_ID;
    private static final int SCAN_DWELL_PERIOD = 1;

    private static final int CUR_FREQ_TEST = 0;
    private static final int CUR_MULTI_TEST = 1;
    private static final int SEARCH_TEST =2;
    private static final int SWEEP_TEST = 3;
    private Band mBand = null;
    private Band mSync = null;
    int Lo = 1, Auto = 0;

    private FileOutputStream mFileCursor = null;
    private String mCurrentFileName = null;

    Spinner spinOptionFmRf;
    Spinner spinOptionBandSweepMthds;
    ArrayAdapter<CharSequence> adaptCfgRf;
    ArrayAdapter<CharSequence> adaptRfCfg;
    ArrayAdapter<CharSequence> adaptFmRf;
    ArrayAdapter<CharSequence> bandSweepMthds;

    private static boolean mIsTransportSMD = false;
    private static boolean setCmdSent = false;
    private static int lastCmdSent = 0;
    private final int CMD_CHDET_SINR_TH = 1;
    private final int CMD_CHDET_SINR_SAMPLE = 2;
    private final int CMD_CHDET_INTF_TH_LOW = 3;
    private final int CMD_CHDET_INTF_TH_HIGH = 4;
    private final int CMD_DEFRD_AF_RMSSI_TH = 5;
    private final int CMD_DEFRD_AF_RMSSI_SAMPLE = 6;
    private final int CMD_DEFRD_GD_CH_RMSSI_TH = 7;
    private final int CMD_DEFRD_SEARCH_ALGO = 8;
    private final int CMD_DEFRD_SINR_FIRST_STAGE = 9;
    private final int CMD_DEFRD_RMSSI_FIRST_STAGE = 10;
    private final int CMD_DEFRD_CF0TH12 = 11;
    private final int CMD_DEFRD_TUNE_POWER = 12;
    private final int CMD_DEFRD_REPEATCOUNT = 13;
    private final int CMD_BLENDTBL_SINR_HI = 14;
    private final int CMD_BLENDTBL_RMSSI_HI = 15;
    private final int CMD_STNPARAM_SINR = 16;
    private final int CMD_STNPARAM_RSSI = 17;
    private final int CMD_STNDBGPARAM_IOVERC = 18;
    private final int CMD_STNDBGPARAM_INFDETOUT = 19;

    private static final int MPX_DCC = 0;
    private static final int SINR_INTF = 1;
    private static final int MIN_SINR_FIRST_STAGE = -128;
    private static final int MAX_SINR_FIRST_STAGE = 127;
    private static final int MIN_RMSSI_FIRST_STAGE = -128;
    private static final int MAX_RMSSI_FIRST_STAGE = 127;
    private static final int MIN_CF0TH12 = -2147483648;
    private static final int MAX_CF0TH12 = 2147483647;
    private static final int MIN_SINR_TH = -128;
    private static final int MAX_SINR_TH = 127;
    private static final int MIN_SINR_SAMPLES = 0;
    private static final int MAX_SINR_SAMPLES = 255;
    private static final int MIN_AF_JMP_RMSSI_TH = 0;
    private static final int MAX_AF_JMP_RMSSI_TH = 65535;
    private static final int MIN_GD_CH_RMSSI_TH = -128;
    private static final int MAX_GD_CH_RMSSI_TH = 127;
    private static final int MIN_AF_JMP_RMSSI_SAMPLES = 0;
    private static final int MAX_AF_JMP_RMSSI_SAMPLES = 255;
    private static final int MIN_SINR_TH_SILABS = 0;
    private static final int MAX_SINR_TH_SILABS = 127;
    private static final int MIN_RSSI_TH_SILABS = 0;
    private static final int MAX_RSSI_TH_SILABS = 127;
    private static final int MIN_RDS_FIFO_CNT_SILABS = 0;
    private static final int MAX_RDS_FIFO_CNT_SILABS = 25;
    private static final int MIN_BLEND_SINRHI = -128;
    private static final int MAX_BLEND_SINRHI = 127;
    private static final int MIN_BLEND_RMSSIHI = -128;
    private static final int MAX_BLEND_RMSSIHI = 127;

    private static final int DIALOG_BAND_SWEEP_SETTING = 1;

    private int prevDwellTime = 2; //2secs
    private int prevDelayTime = 0;//0secs
    private int prevSweepMthd = 0; //Manual (using band min, max)

    private int curSweepMthd = 0;
    private int textBoxVal = 0;
    private int algo_type = -1;

    private Thread mRecordUpdateHandlerThread = null;
    private Thread mRunTestThread = null;
    private Thread mTuneCompleteThread = null;
    boolean mRecording = false;


    private static StringBuilder sFormatBuilder = new StringBuilder();
    private static Formatter sFormatter = new Formatter(sFormatBuilder, Locale
                                                       .getDefault());
    private static final Object[] sTimeArgs = new Object[5];

    private final String FREQ_LIST_FILE_NAME = "/freq_list_comma_separated.txt";
    private static final String BAND_SWEEP_START_DELAY_TIMEOUT = "com.caf.fmradio.SWEEP_START_DELAY_EXP";
    private static final String BAND_SWEEP_DWELL_DELAY_TIMEOUT = "com.caf.fmradio.SWEEP_DWELL_DELAY_EXP";

    private BroadcastReceiver mBandSweepDelayExprdListener = null;
    private BroadcastReceiver mBandSweepDwellExprdListener = null;

    private WakeLock mWakeLock;
    private Dialog mDialgBandSweep;

    private GetNextFreqInterface mNextFreqInterface;
    private CommaSeparatedFreqFileReader mFreqFileReader;
    private final int SIGNAL_THRESHOLD = 1;
    private final int GET_CHANNEL_DET_THRESHOLD = 2;
    private final int DEFAULT_DATA_READ = 3;
    private final int GET_BLEND_TBL = 4;
    private final int SET_CHANNEL_DET_THRESHOLD = 5;
    private final int DEFAULT_DATA_WRITE = 6;
    private final int SET_BLEND_TBL = 7;
    private final int GET_STATION_PARAM = 8;
    private final int GET_STATION_DBG_PARAM = 9;
    private Object obj = new Object();
    private int nRssi = 0;
    private int nIoC = 0;
    private int nIntDet = 0;
    private int nMpxDcc = 0;
    private int nSINR = 0;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        this.setContentView(R.layout.fmstats);

        spinOptionFmRf = (Spinner)findViewById(R.id.spinner);
        adaptFmRf = ArrayAdapter.createFromResource
                       (this, R.array.stats_options
                         , android.R.layout.simple_spinner_item);
        adaptFmRf.setDropDownViewResource
                   (android.R.layout.simple_spinner_dropdown_item);
        if (spinOptionFmRf != null) {
           spinOptionFmRf.setAdapter(adaptFmRf);
           spinOptionFmRf.setOnItemSelectedListener(mSpinFmRfListener);
        }

        bandSweepMthds = ArrayAdapter.createFromResource
                          (this, R.array.band_sweep_methods,
                            android.R.layout.simple_spinner_item);

        if(mReceiver == null)
            mReceiver = new FmReceiver();

        Log.d(LOGTAG, "oncreate");
        checkTransportLayer();
        if (isCherokeeChip()) {
            mSpinCfgRfListener4 = new CfgRfItemSelectedListener4();
            adaptCfgRf = ArrayAdapter.createFromResource(
                    this, R.array.cfg_rf4,
                    android.R.layout.simple_spinner_item);
        } else if (isRomeChip()) {
            mSpinCfgRfListener3 = new CfgRfItemSelectedListener3();
            adaptCfgRf = ArrayAdapter.createFromResource(
                           this, R.array.cfg_rf3,
                           android.R.layout.simple_spinner_item);
        }else if (isTransportLayerSMD()) {
            mSpinCfgRfListener2 = new CfgRfItemSelectedListener2();
            adaptCfgRf = ArrayAdapter.createFromResource(
                           this, R.array.cfg_rf2,
                           android.R.layout.simple_spinner_item);
        }else {
            /* default RFstats app support,used for richwave FM chip
             * set rssi threshold supported
             */
            mSpinCfgRfListener1 = new CfgRfItemSelectedListener1();
            adaptCfgRf = ArrayAdapter.createFromResource(
                           this, R.array.cfg_rf1,
                           android.R.layout.simple_spinner_item);
        }

        adaptRfCfg = ArrayAdapter.createFromResource(
            this, R.array.rf_cfg, android.R.layout.simple_spinner_item);

        tLayout = (TableLayout) findViewById(R.id.maintable);

        long curTime = System.currentTimeMillis();
        mCurrentFileName = "FMStats_".concat(
                                 Long.toString(curTime).concat(".txt")
                              );
        Log.e(LOGTAG,"Filename is " + mCurrentFileName);
        try {
            mFileCursor = openFileOutput(
                                 mCurrentFileName,
                                 Context.MODE_PRIVATE);
            if(null != mFileCursor) {
               Log.e(LOGTAG, "location of the file is"+getFilesDir());
            }
        }catch (IOException e) {
            e.printStackTrace();
            Log.e(LOGTAG,"Couldn't create the file to writeLog");
            mCurrentFileName = null;
        }

        /*Initialize the column header with
        constant values*/
        if (isRomeChip()) {
            mColumnHeader.setFreq("Freq");
            mColumnHeader.setRSSI("RMSSI");
            mColumnHeader.setSINR("SINR");
        } else if(isHastingsChip()) {
            mColumnHeader.setFreq("Freq");
            mColumnHeader.setRSSI("RSSI");
        } else {
            mColumnHeader.setFreq("Freq");
            mColumnHeader.setRSSI("RMSSI");
            mColumnHeader.setIoC("IoC");
            mColumnHeader.setSINR("SINR");
            mColumnHeader.setMpxDcc("Offset");
            mColumnHeader.setIntDet("IntDet");
        }

        bandSweepSettingButton = (TextView)findViewById(R.id.BandSweepSetting);
        if(bandSweepSettingButton != null) {
           bandSweepSettingButton.setOnClickListener(mClicktBandSweepSettingListener);
        }

        PowerManager pm = (PowerManager)getSystemService(Context.POWER_SERVICE);
        mWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, this.getClass().getName());
        mWakeLock.setReferenceCounted(false);

        registerBandSweepDelayExprdListener();
        registerBandSweepDwellExprdListener();
    }

    private final Handler mCallbackHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            Log.e(LOGTAG, "mCallbackHandler: msg.what = " + msg.what);
            int val, status;
            tv1 = (TextView)findViewById(R.id.label);
            switch (msg.what) {
                case SIGNAL_THRESHOLD:
                case GET_CHANNEL_DET_THRESHOLD:
                case GET_BLEND_TBL:
                    status = msg.arg2;
                    if (status != 0) {
                        tv1.setText("Error:"  + String.valueOf(status));
                    } else {
                        val = msg.arg1;
                        tv1.setText(String.valueOf(val));
                    }
                    break;
                case DEFAULT_DATA_READ:
                    status = msg.arg2;
                    if (status != 0) {
                        tv1.setText("Error:"  + String.valueOf(status));
                    } else {
                        val = msg.arg1;
                        if (lastCmdSent == CMD_DEFRD_SEARCH_ALGO  && val == MPX_DCC) {
                            tv1.setText(R.string.search_algo_mpx);
                        } else if (lastCmdSent == CMD_DEFRD_SEARCH_ALGO && val == SINR_INTF) {
                            tv1.setText(R.string.search_algo_sinrint);
                        } else {
                            tv1.setText(String.valueOf(val));
                        }
                        lastCmdSent = 0;
                    }
                    break;
                case SET_CHANNEL_DET_THRESHOLD:
                case DEFAULT_DATA_WRITE:
                case SET_BLEND_TBL:
                    tv1.setVisibility(View.VISIBLE);
                    status = msg.arg1;
                    if (status != 0)
                        tv1.setText("Error:" + String.valueOf(status));
                    else
                        tv1.setText("Success");
                    break;
                case GET_STATION_PARAM:
                    status = msg.arg2;
                    if (status != 0) {
                        Log.e(LOGTAG, "GET_STATION_DBG_PARAM: status = " + status);
                        nRssi = Integer.MAX_VALUE;
                        nSINR = Integer.MAX_VALUE;
                    } else {
                        if (lastCmdSent == CMD_STNPARAM_RSSI)
                            nRssi = msg.arg1;
                        if (lastCmdSent == CMD_STNPARAM_SINR)
                            nSINR = msg.arg1;
                    }
                    lastCmdSent = 0;
                    break;
                case GET_STATION_DBG_PARAM:
                    status = msg.arg2;
                    if (status != 0) {
                        Log.e(LOGTAG, "GET_STATION_DBG_PARAM: status = " + status);
                        nIoC = Integer.MAX_VALUE;
                        nIntDet = Integer.MAX_VALUE;
                    } else {
                        if (lastCmdSent == CMD_STNDBGPARAM_IOVERC)
                            nIoC = msg.arg1;
                        if  (lastCmdSent == CMD_STNDBGPARAM_INFDETOUT)
                            nIntDet = msg.arg1;
                    }
                    synchronized (obj) {
                        obj.notify();
                    }
                    break;
                default:
                    Log.e(LOGTAG, "mCallbackHandler:Default");
                    break;
            }
            Log.e(LOGTAG, "mCallbackHandler--");
        }
    };

    @Override
    public void onStart() {
       super.onStart();

        if (false == bindToService(this, osc)) {
            Log.e(LOGTAG, "onCreate: Failed to Start Service");
        }else {
            Log.d(LOGTAG, "onCreate: Start Service completed successfully");
        }
       if(isRecording()) {
          Log.d(LOGTAG, "onStart");
          initiateRecordThread();
       }
    }

    @Override
    public void onStop() {
       Log.d(LOGTAG, "onStop");
       super.onStop();
       if(isRecording()) {
          try {
               if(null != mRecordUpdateHandlerThread) {
                  mRecordUpdateHandlerThread.interrupt();
               }
          }catch (NullPointerException e) {
               e.printStackTrace();
          }
       }
    }

    @Override
    public void onDestroy() {
        Log.d(LOGTAG, "onDestroy");
        stopCurTest();
        if(mUIUpdateHandlerHandler != null) {
           mUIUpdateHandlerHandler.removeCallbacksAndMessages(null);
        }
        unRegisterBroadcastReceiver(mBandSweepDelayExprdListener);
        unRegisterBroadcastReceiver(mBandSweepDwellExprdListener);
        if(null != mFileCursor ) {
            try {
                mFileCursor.close();
            } catch (IOException e) {
                 e.printStackTrace();
           }
        }

        unbindFromService(this);
        Log.d(LOGTAG, "onDestroy: unbindFromService completed");
        mReceiver = null;
        mService = null;
        closeDialog(DIALOG_BAND_SWEEP_SETTING);
        mWakeLock.release();
        super.onDestroy();
    }

    private Runnable mRunTest = new Runnable(){
        public void run() {
              runCurrentTest();
        }
    };

     private Runnable mTuneComplete = new Runnable(){
         public void run(){
             if((null != mMultiUpdateThread) &&(null != mSync))
             {
                 synchronized(mSync){
                     mSync.notify();
                 }
             }
            if((mTestSelected == SEARCH_TEST) && (mService != null)) {
                /* On every Tune Complete generate the result for the current
                Frequency*/
                Message updateUI = new Message();
                updateUI.what = STATUS_UPDATE;
                int freq = FmSharedPreferences.getTunedFrequency();
                updateUI.obj = (Object)GetFMStatsForFreq(freq);
                if (updateUI.obj == null)
                    updateUI.what = STATUS_DONE;
                mUIUpdateHandlerHandler.sendMessage(updateUI);
            }
         }
     };

    private View.OnClickListener mOnRunListener = new View.OnClickListener() {
        public void onClick(View v) {
            Log.d(LOGTAG, "mTestRunning=" + mTestRunning);
           if(false == mTestRunning) {
              clearPreviousTestResults();
              mTestRunning = true;
              if(mTestSelected == SWEEP_TEST) {
                 disableBandSweepSetting();
              }
              SetButtonState(false);
              createResult(mColumnHeader);
              if ((mRunTestThread == null) || (mRunTestThread.getState() == Thread.State.TERMINATED)) {
                    mRunTestThread = new Thread(mRunTest,
                            "mRunTestThread");
              } else {
                  Log.e(LOGTAG, "Error: Thread is already running");
                  return;
              }
              if (mRunTestThread != null) {
                  mRunTestThread.start();
              } else {
                  SetButtonState(true);
                  Log.e(LOGTAG, "RunTestThread: new Thread failed");
                  return;
              }
           } else {
              mTestRunning = false;
              SetButtonState(true);
              /*Stop the thread by interrupting it*/
              if (mRunTestThread != null) {
                  mRunTestThread.interrupt();
                  mRunTestThread = null;
              }
              if(SEARCH_TEST == mTestSelected) {
                 try {
                      mService.cancelSearch();
                 }catch (RemoteException e) {
                      e.printStackTrace();
                 }
              }
              if(mTestSelected == SWEEP_TEST) {
                 stopBandSweep();
                 sendStatusDoneMsg();
                 enableBandSweepSetting();
              }
           }
       }
    };

    private void clearPreviousTestResults() {
       TableLayout tl = (TableLayout)findViewById(R.id.maintable);
       if (tl != null) {
           tl.removeAllViewsInLayout();
       }
       mNewRowIds = NEW_ROW_ID;
    }

    private void SetButtonState(boolean state) {
       // Get the TableRow
       Button RunButton = (Button)findViewById(R.id.Runbutton);
       ProgressBar  pbar = (ProgressBar)findViewById(R.id.progressbar);
       /*Update the state of the button based on
        state*/
       if(state) {
          if(RunButton != null) {
             RunButton.setText(R.string.test_run);
          }
          if(pbar != null) {
             pbar.setVisibility(View.INVISIBLE);
          }
          if(mTestSelected == SWEEP_TEST) {
             enableBandSweepSetting();
          }
       }else {
          if(RunButton != null) {
             RunButton.setText("Stop Test");
          }
          if(pbar != null) {
             pbar.setVisibility(View.VISIBLE);
          }
       }
    }

    private void chooseFMRFoption(){
       String[] szTestInformation = getResources().getStringArray(
                        R.array.stats_options);
       final StringBuilder szbTestHeader = new StringBuilder();
       szbTestHeader.append("running test:").append
                              (szTestInformation[mTestSelected]);
       String szTestHeader = new String(szbTestHeader);
       Log.d(LOGTAG, "chooseFMRfoption");
       switch(mTestSelected)
       {
       case 1:
               RunButton = (Button)findViewById(R.id.Runbutton);
               if(RunButton != null) {
                  RunButton.setVisibility(View.INVISIBLE);
               }
               pbar = (ProgressBar) findViewById(R.id.progressbar);
               if(pbar != null) {
                  pbar.setVisibility(View.INVISIBLE);
               }
               adaptCfgRf.setDropDownViewResource
                           (android.R.layout.simple_spinner_dropdown_item);
               spinOptionFmRf.setAdapter(adaptCfgRf);
               if (isCherokeeChip()) {
                   spinOptionFmRf.setOnItemSelectedListener
                       (mSpinCfgRfListener4);
               } else if (isRomeChip())
                  spinOptionFmRf.setOnItemSelectedListener
                                   (mSpinCfgRfListener3);
               else if(isTransportLayerSMD())
                  spinOptionFmRf.setOnItemSelectedListener
                                   (mSpinCfgRfListener2);
               else
                  spinOptionFmRf.setOnItemSelectedListener
                                   (mSpinCfgRfListener1);
               break;
       case 2:
               txtbox1 = (EditText)findViewById(R.id.txtbox1);
               tv1 = (TextView)findViewById(R.id.label);
               if(txtbox1 != null) {
                  txtbox1.setVisibility(View.INVISIBLE);
               }
               if(tv1 != null) {
                  tv1.setVisibility(View.INVISIBLE);
               }
               Button SetButton = (Button)findViewById(R.id.Setbutton);
               if(SetButton != null) {
                  SetButton.setVisibility(View.INVISIBLE);
               }
               adaptRfCfg.setDropDownViewResource
                               (android.R.layout.simple_spinner_dropdown_item);
               spinOptionFmRf.setAdapter(adaptRfCfg);
               spinOptionFmRf.setOnItemSelectedListener(mSpinRfCfgListener);
               break;
        }
    }

    private View.OnClickListener mOnSetRxRePeatCount = new View.OnClickListener() {
        public void onClick(View v) {
            String a;
            a =  txtbox1.getText().toString();
            try {
                 textBoxVal = Integer.parseInt(a);
                 Log.d(LOGTAG, "Value entered for mOnSetRxRePeatCount: " + textBoxVal);
                 if((textBoxVal < 0) ||
                     (textBoxVal > 255))
                     return;
                 if(mService != null) {
                    try {
                        mService.setRxRepeatCount(textBoxVal);
                        setCmdSent = true;
                        lastCmdSent = CMD_DEFRD_REPEATCOUNT;
                    } catch (RemoteException e) {
                         e.printStackTrace();
                    }
                 }
            } catch (NumberFormatException e) {
                 Log.e(LOGTAG, "Value entered is not in correct format : " + a);
                 txtbox1.setText("");
            }
        }
    };

    private View.OnClickListener mOnSetBlendSinrHiListener = new View.OnClickListener() {
        public void onClick(View v) {
            String a;
            a =  txtbox1.getText().toString();
            try {
                 textBoxVal = (byte) Integer.parseInt(a);
                 Log.d(LOGTAG, "Value entered for mOnSetBlendSinrHiListener: " + textBoxVal);
                 if((textBoxVal < MIN_BLEND_SINRHI ) ||
                     (textBoxVal > MAX_BLEND_SINRHI))
                     return;
                 if(mReceiver != null) {
                     mReceiver.setBlendSinr(textBoxVal);
                     setCmdSent = true;
                     lastCmdSent = CMD_BLENDTBL_SINR_HI;
                 }
            } catch (NumberFormatException e) {
                 Log.e(LOGTAG, "Value entered is not in correct format : " + a);
                 txtbox1.setText("");
            }
        }
    };

    private View.OnClickListener mOnSetBlendRmssiHiListener = new View.OnClickListener() {
        public void onClick(View v) {
            String a;
            a =  txtbox1.getText().toString();
            try {
                 textBoxVal = (byte)Integer.parseInt(a);
                 Log.d(LOGTAG, "Value entered for mOnSetBlendRmssiHiListener: " + textBoxVal);
                 if((textBoxVal < MIN_BLEND_RMSSIHI) ||
                     (textBoxVal > MAX_BLEND_RMSSIHI))
                     return;
                 if(mReceiver != null) {
                     mReceiver.setBlendRmssi(textBoxVal);
                     setCmdSent = true;
                     lastCmdSent = CMD_BLENDTBL_RMSSI_HI;
                 }
            } catch (NumberFormatException e) {
                 Log.e(LOGTAG, "Value entered is not in correct format : " + a);
                 txtbox1.setText("");
            }
        }
    };

    private View.OnClickListener mOnSetSinrSmplCntListener =
    new View.OnClickListener() {
       public void onClick(View v) {
          String a;
          a = txtbox1.getText().toString();
          try {
              textBoxVal = Integer.parseInt(a);
              Log.d(LOGTAG, "Value of Sinr Samples count is : " + textBoxVal);
              if(mService != null) {
                 try {
                     mService.setSinrSamplesCnt(textBoxVal);
                     setCmdSent = true;
                     lastCmdSent = CMD_CHDET_SINR_SAMPLE;
                 }catch (RemoteException e) {
                     e.printStackTrace();
                 }
              }
          }catch (NumberFormatException e) {
              txtbox1.setText("");
          }
      }
    };

    private View.OnClickListener mOnSetSinrThListener =
    new View.OnClickListener() {
       public void onClick(View v) {
          String a;
          a = txtbox1.getText().toString();
          try {
              textBoxVal = Integer.parseInt(a);
              Log.d(LOGTAG, "Value of Sinr Th is : " + textBoxVal);
              if(mService != null) {
                 try {
                     mService.setSinrTh(textBoxVal);
                     setCmdSent = true;
                     lastCmdSent = CMD_CHDET_SINR_TH;
                 }catch (RemoteException e) {
                     e.printStackTrace();
                 }
              }
          }catch (NumberFormatException e) {
              Log.e(LOGTAG, "Value entered is not in correct format: " + a);
              txtbox1.setText("");
          }
      }
    };

    private View.OnClickListener mOnSetIntfLowThListener =
     new View.OnClickListener() {
       public void onClick(View v) {
          String a;
          a = txtbox1.getText().toString();
          try {
              textBoxVal = Integer.parseInt(a);
              Log.d(LOGTAG, "Value of Intf Det Low Th is : " + textBoxVal);
              if(mService != null) {
                 try {
                     mService.setIntfDetLowTh(textBoxVal);
                     setCmdSent =  true;
                     lastCmdSent = CMD_CHDET_INTF_TH_LOW;
                 }catch (RemoteException e) {
                     e.printStackTrace();
                 }
              }
          }catch (NumberFormatException e) {
              Log.e(LOGTAG, "Value entered is not in correct format: " + a);
              txtbox1.setText("");
          }
      }
    };
    private View.OnClickListener mOnSetIntfHighThListener =
    new View.OnClickListener() {
       public void onClick(View v) {
          String a;
          a = txtbox1.getText().toString();
          try {
              textBoxVal = Integer.parseInt(a);
              Log.d(LOGTAG, "Value of Intf Det Low Th is : " + textBoxVal);
              if(mService != null) {
                 try {
                     mService.setIntfDetHighTh(textBoxVal);
                     setCmdSent = true;
                     lastCmdSent = CMD_CHDET_INTF_TH_HIGH;
                 }catch (RemoteException e) {
                     e.printStackTrace();
                 }
              }
          }catch (NumberFormatException e) {
              Log.e(LOGTAG, "Value entered is not in correct format : " + a);
              txtbox1.setText("");
          }
       }
    };

    private View.OnClickListener mOnSetSinrFirstStageListener =
    new View.OnClickListener() {
       public void onClick(View v) {
          String a;
          a = txtbox1.getText().toString();
          try {
              textBoxVal = Integer.parseInt(a);
              Log.d(LOGTAG, "Value entered for SINR FIRST STAGE is : " + textBoxVal);
              if((textBoxVal < MIN_SINR_FIRST_STAGE) ||
                     (textBoxVal > MAX_SINR_FIRST_STAGE))
                  return;
              if(mService != null) {
                 try {
                     mService.setSinrFirstStage(textBoxVal);
                     setCmdSent = true;
                     lastCmdSent = CMD_DEFRD_SINR_FIRST_STAGE;
                 }catch (RemoteException e) {
                     e.printStackTrace();
                 }
              }
          }catch (NumberFormatException e) {
              Log.e(LOGTAG, "Value entered is not in correct format : " + a);
              txtbox1.setText("");
          }
       }
    };

    private View.OnClickListener mOnSetRmssiFirstStageListener =
    new View.OnClickListener() {
       public void onClick(View v) {
          String a;
          a = txtbox1.getText().toString();
          try {
              textBoxVal = Integer.parseInt(a);
              Log.d(LOGTAG, "Value entered for RMSSI FIRST STAGE is: " + textBoxVal);
              if((textBoxVal < MIN_RMSSI_FIRST_STAGE) ||
                     (textBoxVal > MAX_RMSSI_FIRST_STAGE))
                  return;
              if(mService != null) {
                 try {
                     mService.setRmssiFirstStage(textBoxVal);
                     setCmdSent = true;
                     lastCmdSent = CMD_DEFRD_RMSSI_FIRST_STAGE;
                 }catch (RemoteException e) {
                     e.printStackTrace();
                 }
              }
          }catch (NumberFormatException e) {
              Log.e(LOGTAG, "Value entered is not in correct format : " + a);
              txtbox1.setText("");
          }
       }
    };

    private View.OnClickListener mOnSetCFOMeanThListener =
    new View.OnClickListener() {
       public void onClick(View v) {
          String a;
          a = txtbox1.getText().toString();
          try {
              textBoxVal = Integer.parseInt(a);
              Log.d(LOGTAG, "Value entered for CF0TH12 is: " + textBoxVal);
              if((textBoxVal < MIN_CF0TH12) ||
                     (textBoxVal > MAX_CF0TH12))
                  return;
              if(mService != null) {
                 try {
                     mService.setCFOMeanTh(textBoxVal);
                     setCmdSent = true;
                     lastCmdSent = CMD_DEFRD_CF0TH12;
                 }catch (RemoteException e) {
                     e.printStackTrace();
                 }
              }
          }catch (NumberFormatException e) {
              Log.e(LOGTAG, "Value entered is not in correct format : " + a);
              txtbox1.setText("");
          }
       }
    };

    private View.OnClickListener mOnSetSearchMPXDCCListener =
    new View.OnClickListener() {
       public void onClick(View v) {
          Log.d(LOGTAG, "Value entered for search is: MPX DCC");
          if(mService != null) {
             try {
                 mService.setSearchAlgoType(MPX_DCC);
                 setCmdSent = true;
                 algo_type = MPX_DCC;
                 lastCmdSent = CMD_DEFRD_SEARCH_ALGO;
             }catch (RemoteException e) {
                 e.printStackTrace();
             }
          }
       }
    };

    private View.OnClickListener mOnSetSearchSinrIntfListener =
    new View.OnClickListener() {
       public void onClick(View v) {
          Log.d(LOGTAG, "Value entered for search is: SINR INTF");
          if(mService != null) {
             try {
                 mService.setSearchAlgoType(SINR_INTF);
                 setCmdSent = true;
                 algo_type = SINR_INTF;
                 lastCmdSent = CMD_DEFRD_SEARCH_ALGO;
             }catch (RemoteException e) {
                  e.printStackTrace();
             }
          }
       }
    };

    private View.OnClickListener mOnSetAfJmpRmssiThListener =
    new View.OnClickListener() {
       public void onClick(View v) {
          String a;
          a = txtbox1.getText().toString();
          try {
              textBoxVal = Integer.parseInt(a);
              Log.d(LOGTAG, "Value entered for AfJmpRmssiTh is: " + textBoxVal);
              if((textBoxVal < MIN_AF_JMP_RMSSI_TH) ||
                     (textBoxVal > MAX_AF_JMP_RMSSI_TH))
                  return;
              if(mService != null) {
                 try {
                     mService.setAfJmpRmssiTh(textBoxVal);
                     setCmdSent = true;
                     lastCmdSent = CMD_DEFRD_AF_RMSSI_TH;
                 }catch (RemoteException e) {
                     e.printStackTrace();
                 }
              }
          }catch (NumberFormatException e) {
              Log.e(LOGTAG, "Value entered is not in correct format : " + a);
              txtbox1.setText("");
          }
       }
    };

    private View.OnClickListener mOnSetGdChRmssiThListener =
    new View.OnClickListener() {
       public void onClick(View v) {
          String a;
          a = txtbox1.getText().toString();
          try {
              textBoxVal = Integer.parseInt(a);
              Log.d(LOGTAG, "Value entered for Good channel Rmssi Th is: " + textBoxVal);
              if((textBoxVal < MIN_GD_CH_RMSSI_TH) ||
                     (textBoxVal > MAX_GD_CH_RMSSI_TH))
                  return;
              if(mService != null) {
                 try {
                     mService.setGoodChRmssiTh(textBoxVal);
                     setCmdSent = true;
                     lastCmdSent = CMD_DEFRD_GD_CH_RMSSI_TH;
                 }catch (RemoteException e) {
                     e.printStackTrace();
                 }
              }
          }catch (NumberFormatException e) {
              Log.e(LOGTAG, "Value entered is not in correct format : " + a);
              txtbox1.setText("");
          }
       }
    };

    private View.OnClickListener mOnSetAfJmpRmssiSmplsCntListener =
    new View.OnClickListener() {
       public void onClick(View v) {
          String a;
          a = txtbox1.getText().toString();
          try {
              textBoxVal = Integer.parseInt(a);
              Log.d(LOGTAG, "Value entered for AfJmpRmssiSamples is: " + textBoxVal);
              if((textBoxVal < MIN_AF_JMP_RMSSI_SAMPLES) ||
                     (textBoxVal > MAX_AF_JMP_RMSSI_SAMPLES))
                  return;
              if(mService != null) {
                 try {
                     mService.setAfJmpRmssiSamplesCnt(textBoxVal);
                     setCmdSent = true;
                     lastCmdSent = CMD_DEFRD_AF_RMSSI_SAMPLE;
                 }catch (RemoteException e) {
                     e.printStackTrace();
                 }
              }
          }catch (NumberFormatException e) {
              Log.e(LOGTAG, "Value entered is not in correct format : " + a);
              txtbox1.setText("");
          }
       }
    };

    private View.OnClickListener mOnSetSinrThListenerSilabs =
    new View.OnClickListener() {
       public void onClick(View v) {
          String a;
          a = txtbox1.getText().toString();
          try {
              int rdel = Integer.parseInt(a);
              Log.d(LOGTAG, "Value of Sinr Th is : " + rdel);
              if(mService != null) {
                 try {
                     if((rdel >= MIN_SINR_TH_SILABS) &&
                             (rdel <= MAX_SINR_TH_SILABS))
                             mService.setSinrTh(rdel);
                     else
                        Log.d(LOGTAG, "SINR out of valid range");
                 }catch (RemoteException e) {
                     e.printStackTrace();
                 }
              }
          }catch (NumberFormatException e) {
              Log.e(LOGTAG, "Value entered is not in correct format: " + a);
              txtbox1.setText("");
          }
      }
    };

    private View.OnClickListener mOnSetRssiThListener =
    new View.OnClickListener() {
       public void onClick(View v) {
          String a;
          a = txtbox1.getText().toString();
          try {
              int rdel = Integer.parseInt(a);
              Log.d(LOGTAG, "Value of Rssi Th is : " + rdel);
              if(mReceiver != null) {
                  if((rdel >= MIN_RSSI_TH_SILABS) &&
                          (rdel <= MAX_RSSI_TH_SILABS))
                          mReceiver.setRssiThreshold(rdel);
                  else
                     Log.d(LOGTAG, "RSSI out of valid range");
              }
          }catch (NumberFormatException e) {
              Log.e(LOGTAG, "Value entered is not in correct format: " + a);
              txtbox1.setText("");
          }
      }
    };

    private View.OnClickListener mOnSetAfJumpRssiThListenerSilabs =
    new View.OnClickListener() {
       public void onClick(View v) {
          String a;
          a = txtbox1.getText().toString();
          try {
              int rdel = Integer.parseInt(a);
              Log.d(LOGTAG, "Value of Af jump Rssi Th is : " + rdel);
              if(mReceiver != null) {
                  if((rdel >= MIN_RSSI_TH_SILABS) &&
                          (rdel <= MAX_RSSI_TH_SILABS))
                          mReceiver.setAfJumpRssiThreshold(rdel);
                  else
                     Log.d(LOGTAG, "Af jump rssi out of valid range");
              }
          }catch (NumberFormatException e) {
              Log.e(LOGTAG, "Value entered is not in correct format: " + a);
              txtbox1.setText("");
          }
      }
    };

    private View.OnClickListener mOnSetRdsFifoCntListenerSilabs =
    new View.OnClickListener() {
       public void onClick(View v) {
          String a;
          a = txtbox1.getText().toString();
          try {
              int rdel = Integer.parseInt(a);
              Log.d(LOGTAG, "Value of RDS fifo count is : " + rdel);
              if(mReceiver != null) {
                  if((rdel >= MIN_RDS_FIFO_CNT_SILABS) &&
                          (rdel <= MAX_RDS_FIFO_CNT_SILABS))
                          mReceiver.setRdsFifoCnt(rdel);
                  else
                     Log.d(LOGTAG, "RDS fifo count out of valid range");
              }
          }catch (NumberFormatException e) {
              Log.e(LOGTAG, "Value entered is not in correct format: " + a);
              txtbox1.setText("");
          }
      }
    };

    public class CfgRfItemSelectedListener1 implements OnItemSelectedListener {
        public void onItemSelected(
                     AdapterView<?> parent, View view, int pos, long id) {
            Log.d("Table","onItemSelected is hit with " + pos);
            txtbox1 = (EditText)findViewById(R.id.txtbox1);
            tv1 = (TextView)findViewById(R.id.label);
            Button SetButton = (Button)findViewById(R.id.Setbutton);
            tLayout.setVisibility(View.INVISIBLE);
            switch(pos)
            {
            case 0:
                   if (txtbox1 != null) {
                       txtbox1.setText(R.string.type_rd);
                       txtbox1.setVisibility(View.VISIBLE);
                   }
                   if (tv1 != null) {
                       tv1.setText(R.string.enter_RssiTh);
                       tv1.setVisibility(View.VISIBLE);
                   }
                   if (SetButton != null) {
                       SetButton.setText(R.string.set_RssiTh);
                       SetButton.setVisibility(View.VISIBLE);
                       SetButton.setOnClickListener(mOnSetRssiThListener);
                   }
                   break;
            case 1:
                   tLayout.removeAllViewsInLayout();
                   mNewRowIds = NEW_ROW_ID;
                   tLayout.setVisibility(View.VISIBLE);
                   if (txtbox1 != null) {
                       txtbox1.setVisibility(View.INVISIBLE);
                   }
                   if (tv1 != null) {
                       tv1.setVisibility(View.INVISIBLE);
                   }
                   if (SetButton != null) {
                       SetButton.setVisibility(View.INVISIBLE);
                   }
                   adaptRfCfg.setDropDownViewResource(
                              android.R.layout.simple_spinner_dropdown_item);
                   spinOptionFmRf.setAdapter(adaptRfCfg);
                   spinOptionFmRf.setOnItemSelectedListener(
                                                 mSpinRfCfgListener);
                   break;
            }
        }

        public void onNothingSelected(AdapterView<?> parent) {
            // Do Nothing
        }
    }

    public class CfgRfItemSelectedListener2 implements OnItemSelectedListener {
        public void onItemSelected(AdapterView<?> parent,
                                    View view, int pos, long id) {
            Log.d("Table","onItemSelected is hit with " + pos);
            int ret = Integer.MAX_VALUE;
            byte retval = Byte.MAX_VALUE;
            txtbox1 = (EditText) findViewById(R.id.txtbox1);
            tv1 = (TextView) findViewById(R.id.label);
            button1 = (Button)findViewById(R.id.SearchMpxDcc);
            button2 = (Button)findViewById(R.id.SearchSinrInt);
            Button SetButton = (Button)findViewById(R.id.Setbutton);
            tLayout.setVisibility(View.INVISIBLE);
            switch(pos)
            {
                case 0:
                    if (txtbox1 != null) {
                       txtbox1.setText(R.string.type_rd);
                       txtbox1.setVisibility(View.VISIBLE);
                    }
                    if (tv1 != null) {
                       tv1.setText(R.string.enter_SinrSmplsCnt);
                       tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                       SetButton.setText(R.string.set_SinrSmplsCnt);
                       SetButton.setVisibility(View.VISIBLE);
                       SetButton.setOnClickListener(mOnSetSinrSmplCntListener);
                    }
                    break;
                case 1:
                    if (txtbox1 != null) {
                       txtbox1.setText(R.string.type_rd);
                       txtbox1.setVisibility(View.VISIBLE);
                    }
                    if (tv1 != null) {
                       tv1.setText(R.string.enter_SinrTh);
                       tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                       SetButton.setText(R.string.set_SinrTh);
                       SetButton.setVisibility(View.VISIBLE);
                       SetButton.setOnClickListener(mOnSetSinrThListener);
                    }
                    break;
                case 2:
                    if (txtbox1 != null) {
                       txtbox1.setText(R.string.type_rd);
                       txtbox1.setVisibility(View.VISIBLE);
                    }
                    if (tv1 != null) {
                       tv1.setText(R.string.enter_IntfLowTh);
                       tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                       SetButton.setText(R.string.set_IntfLowTh);
                       SetButton.setVisibility(View.VISIBLE);
                       SetButton.setOnClickListener(mOnSetIntfLowThListener);
                    }
                    break;
                case 3:
                    if (txtbox1 != null) {
                       txtbox1.setText(R.string.type_rd);
                       txtbox1.setVisibility(View.VISIBLE);
                    }
                    if (tv1 != null) {
                       tv1.setText(R.string.enter_IntfHighTh);
                       tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                       SetButton.setText(R.string.set_IntfHighTh);
                       SetButton.setVisibility(View.VISIBLE);
                       SetButton.setOnClickListener(mOnSetIntfHighThListener);
                    }
                    break;
                case 4:
                    if (txtbox1 != null) {
                       txtbox1.setText(R.string.type_rd);
                       txtbox1.setVisibility(View.VISIBLE);
                    }
                    if (tv1 != null) {
                       tv1.setText(R.string.enter_SinrFirstStage);
                       tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                       SetButton.setText(R.string.set_SinrFirstStage);
                       SetButton.setVisibility(View.VISIBLE);
                       SetButton.setOnClickListener(mOnSetSinrFirstStageListener);
                    }
                    break;
                case 5:
                    if (txtbox1 != null) {
                       txtbox1.setText(R.string.type_rd);
                       txtbox1.setVisibility(View.VISIBLE);
                    }
                    if (tv1 != null) {
                       tv1.setText(R.string.enter_RmssiFirstStage);
                       tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                       SetButton.setText(R.string.set_RmssiFirstStage);
                       SetButton.setVisibility(View.VISIBLE);
                       SetButton.setOnClickListener(mOnSetRmssiFirstStageListener);
                    }
                    break;
                case 6:
                    if (txtbox1 != null) {
                       txtbox1.setText(R.string.type_rd);
                       txtbox1.setVisibility(View.VISIBLE);
                    }
                    if (tv1 != null) {
                       tv1.setText(R.string.enter_CF0Th12);
                       tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                       SetButton.setText(R.string.set_CF0Th12);
                       SetButton.setVisibility(View.VISIBLE);
                       SetButton.setOnClickListener(mOnSetCFOMeanThListener);
                    }
                    break;
                case 7:
                    if (txtbox1 != null) {
                        txtbox1.setVisibility(View.INVISIBLE);
                    }
                    if (tv1 != null) {
                        tv1.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                        SetButton.setVisibility(View.INVISIBLE);
                    }
                    if(button1 != null) {
                       button1.setText(R.string.search_algo_mpx);
                       button1.setVisibility(View.VISIBLE);
                       button1.setOnClickListener(mOnSetSearchMPXDCCListener);
                    }
                    if(button2 != null) {
                       button2.setText(R.string.search_algo_sinrint);
                       button2.setVisibility(View.VISIBLE);
                       button2.setOnClickListener(mOnSetSearchSinrIntfListener);
                    }
                    break;
                case 8:
                    if (txtbox1 != null) {
                        txtbox1.setVisibility(View.INVISIBLE);
                    }
                    if (tv1 != null) {
                        tv1.setText("");
                        tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                        SetButton.setVisibility(View.INVISIBLE);
                    }
                    try {
                        if(mService != null)
                           ret = mService.getSinrSamplesCnt();
                           Log.d(LOGTAG, "Get Sinr Samples Count: " + ret);
                           if((ret >= MIN_SINR_SAMPLES) &&
                                  (ret <= MAX_SINR_SAMPLES))
                              tv1.setText(" " + String.valueOf(ret));
                    }catch (RemoteException e) {
                    }
                    break;
                case 9:
                    if (txtbox1 != null) {
                        txtbox1.setVisibility(View.INVISIBLE);
                    }
                    if (tv1 != null) {
                        tv1.setText("");
                        tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                        SetButton.setVisibility(View.INVISIBLE);
                    }
                    try {
                        if(mService != null)
                           ret = mService.getSinrTh();
                           Log.d(LOGTAG, "Get Sinr Threshold: " + ret);
                           if((ret >= MIN_SINR_TH) &&
                                   (ret <= MAX_SINR_TH))
                              tv1.setText(" " + String.valueOf(ret));
                    }catch (RemoteException e) {

                    }
                    break;
                case 10:
                    if (txtbox1 != null) {
                        txtbox1.setVisibility(View.INVISIBLE);
                    }
                    if (tv1 != null) {
                        tv1.setText("");
                        tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                        SetButton.setVisibility(View.INVISIBLE);
                    }
                    try {
                        if(mService != null) {
                           ret = mService.getSinrFirstStage();
                           Log.d(LOGTAG, "Get Sinr First Stage: " + ret);
                           if (ret >= MIN_SINR_FIRST_STAGE &&
                                   ret <= MAX_SINR_FIRST_STAGE)
                               tv1.setText(" " + String.valueOf(ret));
                        }
                    }catch (RemoteException e) {

                    }
                    break;
                case 11:
                    if (txtbox1 != null) {
                        txtbox1.setVisibility(View.INVISIBLE);
                    }
                    if (tv1 != null) {
                        tv1.setText("");
                        tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                        SetButton.setVisibility(View.INVISIBLE);
                    }
                    try {
                        if(mService != null) {
                           ret = mService.getRmssiFirstStage();
                           Log.d(LOGTAG, "Get Rmssi First Stage: " + ret);
                           if (ret >= MIN_RMSSI_FIRST_STAGE &&
                                   ret <= MAX_RMSSI_FIRST_STAGE)
                               tv1.setText(" " + String.valueOf(ret));
                        }
                    }catch (RemoteException e) {

                    }
                    break;
                case 12:
                    if (txtbox1 != null) {
                        txtbox1.setVisibility(View.INVISIBLE);
                    }
                    if (tv1 != null) {
                        tv1.setText("");
                        tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                        SetButton.setVisibility(View.INVISIBLE);
                    }
                    try {
                        if(mService != null) {
                           ret = mService.getCFOMeanTh();
                           Log.d(LOGTAG, "Get CF0 Threshold: " + ret);
                           if (ret >= MIN_CF0TH12 &&
                                   ret <= MAX_CF0TH12)
                               tv1.setText(" " + String.valueOf(ret));
                        }
                    }catch (RemoteException e) {

                    }
                    break;
                case 13:
                    if (txtbox1 != null) {
                        txtbox1.setVisibility(View.INVISIBLE);
                    }
                    if (tv1 != null) {
                        tv1.setText("");
                        tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                        SetButton.setVisibility(View.INVISIBLE);
                    }
                    try {
                        if(mService != null) {
                           ret = mService.getSearchAlgoType();
                           Log.d(LOGTAG, "Search Type: " + ret);
                           if (ret == MPX_DCC)
                               tv1.setText(R.string.search_algo_mpx);
                           else if(ret == SINR_INTF)
                               tv1.setText(R.string.search_algo_sinrint);
                        }
                    }catch (RemoteException e) {

                    }
                    break;
                case 14:
                    if (txtbox1 != null) {
                       txtbox1.setText(R.string.type_rd);
                       txtbox1.setVisibility(View.VISIBLE);
                    }
                    if (tv1 != null) {
                       tv1.setText(R.string.enter_AfJmpRmssiTh);
                       tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                       SetButton.setText(R.string.set_AfJmpRmssiTh);
                       SetButton.setVisibility(View.VISIBLE);
                       SetButton.setOnClickListener(mOnSetAfJmpRmssiThListener);
                    }
                    break;
                case 15:
                    if (txtbox1 != null) {
                       txtbox1.setText(R.string.type_rd);
                       txtbox1.setVisibility(View.VISIBLE);
                    }
                    if (tv1 != null) {
                       tv1.setText(R.string.enter_GdChRmssiTh);
                       tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                       SetButton.setText(R.string.set_GdChRmssiTh);
                       SetButton.setVisibility(View.VISIBLE);
                       SetButton.setOnClickListener(mOnSetGdChRmssiThListener);
                    }
                    break;
                case 16:
                    if (txtbox1 != null) {
                       txtbox1.setText(R.string.type_rd);
                       txtbox1.setVisibility(View.VISIBLE);
                    }
                    if (tv1 != null) {
                       tv1.setText(R.string.enter_AfJmpRmssiSmplsCnt);
                       tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                       SetButton.setText(R.string.set_AfJmpRmssiSmplsCnt);
                       SetButton.setVisibility(View.VISIBLE);
                       SetButton.setOnClickListener(mOnSetAfJmpRmssiSmplsCntListener);
                    }
                    break;
                case 17:
                    if (txtbox1 != null) {
                        txtbox1.setVisibility(View.INVISIBLE);
                    }
                    if (tv1 != null) {
                        tv1.setText("");
                        tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                        SetButton.setVisibility(View.INVISIBLE);
                    }
                    try {
                        if(mService != null) {
                           ret = mService.getAfJmpRmssiTh();
                           Log.d(LOGTAG, "Get Af Jmp Rmssi Th: " + ret);
                           if ((ret >= MIN_AF_JMP_RMSSI_TH) &&
                                  (ret <= MAX_AF_JMP_RMSSI_TH))
                               tv1.setText(" " + String.valueOf(ret));
                        }
                    }catch (RemoteException e) {

                    }
                    break;
                case 18:
                    if (txtbox1 != null) {
                        txtbox1.setVisibility(View.INVISIBLE);
                    }
                    if (tv1 != null) {
                        tv1.setText("");
                        tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                        SetButton.setVisibility(View.INVISIBLE);
                    }
                    try {
                        if(mService != null) {
                           ret = mService.getGoodChRmssiTh();
                           Log.d(LOGTAG, "Get GoodChRmssi Threshold: " + ret);
                           if ((ret >= MIN_GD_CH_RMSSI_TH) &&
                                  (ret <= MAX_GD_CH_RMSSI_TH))
                               tv1.setText(" " + String.valueOf(ret));
                        }
                    }catch (RemoteException e) {

                    }
                    break;
                case 19:
                    if (txtbox1 != null) {
                        txtbox1.setVisibility(View.INVISIBLE);
                    }
                    if (tv1 != null) {
                        tv1.setText("");
                        tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                        SetButton.setVisibility(View.INVISIBLE);
                    }
                    try {
                        if(mService != null) {
                           ret = mService.getAfJmpRmssiSamplesCnt();
                           Log.d(LOGTAG, "Get AfJmpRmssiSamples count: " + ret);
                           if ((ret >= MIN_AF_JMP_RMSSI_SAMPLES) &&
                                  (ret <= MAX_AF_JMP_RMSSI_SAMPLES))
                               tv1.setText(" " + String.valueOf(ret));
                        }
                    }catch (RemoteException e) {

                    }
                    break;
                case 20:
                    if (txtbox1 != null) {
                        txtbox1.setText(R.string.type_rd);
                        txtbox1.setVisibility(View.VISIBLE);
                    }
                    if (tv1 != null) {
                        tv1.setText(R.string.enter_RxRePeatCount);
                        tv1.setVisibility(View.VISIBLE);
                    }
                    if (SetButton != null) {
                        SetButton.setText(R.string.set_RxRePeatCount);
                        SetButton.setVisibility(View.VISIBLE);
                        SetButton.setOnClickListener(mOnSetRxRePeatCount);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    break;
                case 21:
                    if (txtbox1 != null) {
                       txtbox1.setText(R.string.type_rd);
                       txtbox1.setVisibility(View.VISIBLE);
                    }
                    if (tv1 != null) {
                       tv1.setText(R.string.enter_BlendSinrHi);
                       tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                       SetButton.setText(R.string.set_BlendSinrHi);
                       SetButton.setVisibility(View.VISIBLE);
                       SetButton.setOnClickListener(mOnSetBlendSinrHiListener);
                    }
                    break;
                case 22:
                    if (txtbox1 != null) {
                        txtbox1.setVisibility(View.INVISIBLE);
                    }
                    if (tv1 != null) {
                        tv1.setText("");
                        tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                        SetButton.setVisibility(View.INVISIBLE);
                    }
                    if (mReceiver != null) {
                        retval = mReceiver.getBlendSinr();
                        Log.d(LOGTAG, "Get BlendSinrHi: " + retval);
                        if((retval >= MIN_BLEND_RMSSIHI) &&
                           (retval <= MAX_BLEND_RMSSIHI))
                            tv1.setText(" " + String.valueOf(retval));
                    }
                    break;
                case 23:
                    if (txtbox1 != null) {
                       txtbox1.setText(R.string.type_rd);
                       txtbox1.setVisibility(View.VISIBLE);
                    }
                    if (tv1 != null) {
                       tv1.setText(R.string.enter_BlendRmssiHi);
                       tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                       SetButton.setText(R.string.set_BlendRmssiHi);
                       SetButton.setVisibility(View.VISIBLE);
                       SetButton.setOnClickListener(mOnSetBlendRmssiHiListener);
                    }
                    break;
                case 24:
                    if (txtbox1 != null) {
                        txtbox1.setVisibility(View.INVISIBLE);
                    }
                    if (tv1 != null) {
                        tv1.setText("");
                        tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                        SetButton.setVisibility(View.INVISIBLE);
                    }
                    if (mReceiver != null) {
                        retval = mReceiver.getBlendRmssi();
                        Log.d(LOGTAG, "Get BlendRmssiHi: " + retval);
                        if((retval >= MIN_BLEND_RMSSIHI) &&
                           (retval <= MAX_BLEND_RMSSIHI))
                            tv1.setText(" " + String.valueOf(retval));
                    }
                    break;
                case 25:
                    tLayout.removeAllViewsInLayout();
                    mNewRowIds = NEW_ROW_ID;
                    tLayout.setVisibility(View.VISIBLE);
                    if (txtbox1 != null) {
                       txtbox1.setVisibility(View.INVISIBLE);
                    }
                    if (tv1 != null) {
                       tv1.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                       SetButton.setVisibility(View.INVISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    adaptRfCfg.setDropDownViewResource(
                              android.R.layout.simple_spinner_dropdown_item);
                    spinOptionFmRf.setAdapter(adaptRfCfg);
                    spinOptionFmRf.setOnItemSelectedListener(mSpinRfCfgListener);
                    break;
            }
        }
        public void onNothingSelected(AdapterView<?> parent) {
            // Do Nothing
        }
    }

    public class CfgRfItemSelectedListener3 implements OnItemSelectedListener {
        public void onItemSelected(
                     AdapterView<?> parent, View view, int pos, long id) {
            Log.d("Table","onItemSelected is hit with " + pos);
            int ret = Integer.MAX_VALUE;
            txtbox1 = (EditText)findViewById(R.id.txtbox1);
            tv1 = (TextView)findViewById(R.id.label);
            button1 = (Button)findViewById(R.id.SearchMpxDcc);
            button2 = (Button)findViewById(R.id.SearchSinrInt);
            Button SetButton = (Button)findViewById(R.id.Setbutton);
            tLayout.setVisibility(View.INVISIBLE);
            switch(pos)
            {
            case 0:
                   if (txtbox1 != null) {
                      txtbox1.setText(R.string.type_rd);
                      txtbox1.setVisibility(View.VISIBLE);
                   }
                   if (tv1 != null) {
                      tv1.setText(R.string.enter_SinrTh);
                      tv1.setVisibility(View.VISIBLE);
                   }
                   if(button1 != null) {
                      button1.setVisibility(View.INVISIBLE);
                   }
                   if(button2 != null) {
                      button2.setVisibility(View.INVISIBLE);
                   }
                   if (SetButton != null) {
                      SetButton.setText(R.string.set_SinrTh);
                      SetButton.setVisibility(View.VISIBLE);
                      SetButton.setOnClickListener(mOnSetSinrThListenerSilabs);
                   }
                   break;
            case 1:
                   if (txtbox1 != null) {
                       txtbox1.setVisibility(View.INVISIBLE);
                   }
                   if (tv1 != null) {
                       tv1.setText("");
                       tv1.setVisibility(View.VISIBLE);
                   }
                   if(button1 != null) {
                      button1.setVisibility(View.INVISIBLE);
                   }
                   if(button2 != null) {
                      button2.setVisibility(View.INVISIBLE);
                   }
                   if (SetButton != null) {
                       SetButton.setVisibility(View.INVISIBLE);
                   }
                   try {
                       if(mService != null)
                          ret = mService.getSinrTh();
                          Log.d(LOGTAG, "Get Sinr Threshold: " + ret);
                          if((ret >= MIN_SINR_TH_SILABS) &&
                                  (ret <= MAX_SINR_TH_SILABS))
                             tv1.setText(" " + String.valueOf(ret));
                   }catch (RemoteException e) {

                   }
                   break;
            case 2:
                   if (txtbox1 != null) {
                       txtbox1.setText(R.string.type_rd);
                       txtbox1.setVisibility(View.VISIBLE);
                   }
                   if (tv1 != null) {
                       tv1.setText(R.string.enter_RssiTh);
                       tv1.setVisibility(View.VISIBLE);
                   }
                   if(button1 != null) {
                      button1.setVisibility(View.INVISIBLE);
                   }
                   if(button2 != null) {
                      button2.setVisibility(View.INVISIBLE);
                   }
                   if (SetButton != null) {
                       SetButton.setText(R.string.set_RssiTh);
                       SetButton.setVisibility(View.VISIBLE);
                       SetButton.setOnClickListener(mOnSetRssiThListener);
                   }
                   break;
            case 3:
                   if (txtbox1 != null) {
                       txtbox1.setVisibility(View.INVISIBLE);
                   }
                   if (tv1 != null) {
                       tv1.setText("");
                       tv1.setVisibility(View.VISIBLE);
                   }
                   if(button1 != null) {
                      button1.setVisibility(View.INVISIBLE);
                   }
                   if(button2 != null) {
                      button2.setVisibility(View.INVISIBLE);
                   }
                   if (SetButton != null) {
                       SetButton.setVisibility(View.INVISIBLE);
                   }
                   if (mReceiver != null) {
                      ret = mReceiver.getRssiThreshold();
                      Log.d(LOGTAG, "Get Rssi Th: " + ret);
                      if((ret >= MIN_RSSI_TH_SILABS) &&
                             (ret <= MAX_RSSI_TH_SILABS))
                         tv1.setText(" " + String.valueOf(ret));
                   }
                   break;
            case 4:
                   if (txtbox1 != null) {
                       txtbox1.setText(R.string.type_rd);
                       txtbox1.setVisibility(View.VISIBLE);
                   }
                   if (tv1 != null) {
                       tv1.setText(R.string.enter_AfJumpRssiTh);
                       tv1.setVisibility(View.VISIBLE);
                   }
                   if(button1 != null) {
                      button1.setVisibility(View.INVISIBLE);
                   }
                   if(button2 != null) {
                      button2.setVisibility(View.INVISIBLE);
                   }
                   if (SetButton != null) {
                       SetButton.setText(R.string.set_AfJumpRssiTh);
                       SetButton.setVisibility(View.VISIBLE);
                       SetButton.setOnClickListener(mOnSetAfJumpRssiThListenerSilabs);
                   }
                   break;
            case 5:
                   if (txtbox1 != null) {
                       txtbox1.setVisibility(View.INVISIBLE);
                   }
                   if (tv1 != null) {
                       tv1.setText("");
                       tv1.setVisibility(View.VISIBLE);
                   }
                   if(button1 != null) {
                      button1.setVisibility(View.INVISIBLE);
                   }
                   if(button2 != null) {
                      button2.setVisibility(View.INVISIBLE);
                   }
                   if (SetButton != null) {
                       SetButton.setVisibility(View.INVISIBLE);
                   }
                   if(mReceiver != null) {
                      ret = mReceiver.getAfJumpRssiThreshold();
                      Log.d(LOGTAG, "Get AF jump Rssi threshold: " + ret);
                      if((ret >= MIN_RSSI_TH_SILABS) &&
                             (ret <= MAX_RSSI_TH_SILABS))
                         tv1.setText(" " + String.valueOf(ret));
                   }
                   break;
            case 6:
                   if (txtbox1 != null) {
                       txtbox1.setText(R.string.type_rd);
                       txtbox1.setVisibility(View.VISIBLE);
                   }
                   if (tv1 != null) {
                       tv1.setText(R.string.enter_AfJumpRssiTh);
                       tv1.setVisibility(View.VISIBLE);
                   }
                   if(button1 != null) {
                      button1.setVisibility(View.INVISIBLE);
                   }
                   if(button2 != null) {
                      button2.setVisibility(View.INVISIBLE);
                   }
                   if (SetButton != null) {
                       SetButton.setText(R.string.set_RdsFifoCnt);
                       SetButton.setVisibility(View.VISIBLE);
                       SetButton.setOnClickListener(mOnSetRdsFifoCntListenerSilabs);
                   }
                   break;
            case 7:
                   if (txtbox1 != null) {
                       txtbox1.setVisibility(View.INVISIBLE);
                   }
                   if (tv1 != null) {
                       tv1.setText("");
                       tv1.setVisibility(View.VISIBLE);
                   }
                   if(button1 != null) {
                      button1.setVisibility(View.INVISIBLE);
                   }
                   if(button2 != null) {
                      button2.setVisibility(View.INVISIBLE);
                   }
                   if (SetButton != null) {
                       SetButton.setVisibility(View.INVISIBLE);
                   }
                   if(mReceiver != null) {
                      ret = mReceiver.getRdsFifoCnt();
                      Log.d(LOGTAG, "Get RDS fifo count: " + ret);
                      if((ret >= MIN_RDS_FIFO_CNT_SILABS) &&
                             (ret <= MAX_RDS_FIFO_CNT_SILABS))
                         tv1.setText(" " + String.valueOf(ret));
                   }
                   break;
            case 8:
                   if (txtbox1 != null) {
                       txtbox1.setText(R.string.type_rd);
                       txtbox1.setVisibility(View.VISIBLE);
                   }
                   if (tv1 != null) {
                       tv1.setText(R.string.enter_AfJmpRmssiSmplsCnt);
                       tv1.setVisibility(View.VISIBLE);
                   }
                   if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                   }
                   if(button2 != null) {
                      button2.setVisibility(View.INVISIBLE);
                   }
                   if (SetButton != null) {
                       SetButton.setText(R.string.set_AfJmpRmssiSmplsCnt);
                       SetButton.setVisibility(View.VISIBLE);
                       SetButton.setOnClickListener(mOnSetAfJmpRmssiSmplsCntListener);
                   }
                   break;
            case 9:
                   if (txtbox1 != null) {
                       txtbox1.setVisibility(View.INVISIBLE);
                   }
                   if (tv1 != null) {
                       tv1.setText("");
                       tv1.setVisibility(View.VISIBLE);
                   }
                   if(button1 != null) {
                      button1.setVisibility(View.INVISIBLE);
                   }
                   if(button2 != null) {
                      button2.setVisibility(View.INVISIBLE);
                   }
                   if (SetButton != null) {
                       SetButton.setVisibility(View.INVISIBLE);
                   }
                   try {
                        if (mService != null) {
                            ret = mService.getAfJmpRmssiSamplesCnt();
                            Log.d(LOGTAG, "Get AfJmpRmssiSamples count: " + ret);
                            if ((ret >= MIN_AF_JMP_RMSSI_SAMPLES) &&
                                (ret <= MAX_AF_JMP_RMSSI_SAMPLES))
                                 tv1.setText(" " + String.valueOf(ret));
                         }
                   } catch (RemoteException e) {

                   }
                   break;
            case 10:
                   tLayout.removeAllViewsInLayout();
                   mNewRowIds = NEW_ROW_ID;
                   tLayout.setVisibility(View.VISIBLE);
                   if (txtbox1 != null) {
                       txtbox1.setVisibility(View.INVISIBLE);
                   }
                   if (tv1 != null) {
                       tv1.setVisibility(View.INVISIBLE);
                   }
                   if (SetButton != null) {
                       SetButton.setVisibility(View.INVISIBLE);
                   }
                   adaptRfCfg.setDropDownViewResource(
                              android.R.layout.simple_spinner_dropdown_item);
                   spinOptionFmRf.setAdapter(adaptRfCfg);
                   spinOptionFmRf.setOnItemSelectedListener(
                                                 mSpinRfCfgListener);
                   break;
            }
        }

        public void onNothingSelected(AdapterView<?> parent) {
            // Do Nothing
        }
    }

    public class CfgRfItemSelectedListener4 implements OnItemSelectedListener {
        public void onItemSelected(AdapterView<?> parent,
                                    View view, int pos, long id) {
            Log.d("Table","onItemSelected is hit with " + pos);
            int ret = Integer.MAX_VALUE;
            byte retval = Byte.MAX_VALUE;
            txtbox1 = (EditText) findViewById(R.id.txtbox1);
            tv1 = (TextView) findViewById(R.id.label);
            button1 = (Button)findViewById(R.id.SearchMpxDcc);
            button2 = (Button)findViewById(R.id.SearchSinrInt);
            Button SetButton = (Button)findViewById(R.id.Setbutton);
            tLayout.setVisibility(View.INVISIBLE);
            switch(pos)
            {
                case 1:
                    if (txtbox1 != null) {
                       txtbox1.setText(R.string.type_rd);
                       txtbox1.setVisibility(View.VISIBLE);
                    }
                    if (tv1 != null) {
                       tv1.setText(R.string.enter_SinrSmplsCnt);
                       tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                       SetButton.setText(R.string.set_SinrSmplsCnt);
                       SetButton.setVisibility(View.VISIBLE);
                       SetButton.setOnClickListener(mOnSetSinrSmplCntListener);
                    }
                    break;
                case 2:
                    if (txtbox1 != null) {
                       txtbox1.setText(R.string.type_rd);
                       txtbox1.setVisibility(View.VISIBLE);
                    }
                    if (tv1 != null) {
                       tv1.setText(R.string.enter_SinrTh);
                       tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                       SetButton.setText(R.string.set_SinrTh);
                       SetButton.setVisibility(View.VISIBLE);
                       SetButton.setOnClickListener(mOnSetSinrThListener);
                    }
                    break;
                case 3:
                    if (txtbox1 != null) {
                       txtbox1.setText(R.string.type_rd);
                       txtbox1.setVisibility(View.VISIBLE);
                    }
                    if (tv1 != null) {
                       tv1.setText(R.string.enter_IntfLowTh);
                       tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                       SetButton.setText(R.string.set_IntfLowTh);
                       SetButton.setVisibility(View.VISIBLE);
                       SetButton.setOnClickListener(mOnSetIntfLowThListener);
                    }
                    break;
                case 4:
                    if (txtbox1 != null) {
                       txtbox1.setText(R.string.type_rd);
                       txtbox1.setVisibility(View.VISIBLE);
                    }
                    if (tv1 != null) {
                       tv1.setText(R.string.enter_IntfHighTh);
                       tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                       SetButton.setText(R.string.set_IntfHighTh);
                       SetButton.setVisibility(View.VISIBLE);
                       SetButton.setOnClickListener(mOnSetIntfHighThListener);
                    }
                    break;
                case 5:
                    if (txtbox1 != null) {
                       txtbox1.setText(R.string.type_rd);
                       txtbox1.setVisibility(View.VISIBLE);
                    }
                    if (tv1 != null) {
                       tv1.setText(R.string.enter_SinrFirstStage);
                       tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                       SetButton.setText(R.string.set_SinrFirstStage);
                       SetButton.setVisibility(View.VISIBLE);
                       SetButton.setOnClickListener(mOnSetSinrFirstStageListener);
                    }
                    break;
                case 6:
                    if (txtbox1 != null) {
                       txtbox1.setText(R.string.type_rd);
                       txtbox1.setVisibility(View.VISIBLE);
                    }
                    if (tv1 != null) {
                       tv1.setText(R.string.enter_RmssiFirstStage);
                       tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                       SetButton.setText(R.string.set_RmssiFirstStage);
                       SetButton.setVisibility(View.VISIBLE);
                       SetButton.setOnClickListener(mOnSetRmssiFirstStageListener);
                    }
                    break;
                case 7:
                    if (txtbox1 != null) {
                       txtbox1.setText(R.string.type_rd);
                       txtbox1.setVisibility(View.VISIBLE);
                    }
                    if (tv1 != null) {
                       tv1.setText(R.string.enter_CF0Th12);
                       tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                       SetButton.setText(R.string.set_CF0Th12);
                       SetButton.setVisibility(View.VISIBLE);
                       SetButton.setOnClickListener(mOnSetCFOMeanThListener);
                    }
                    break;
                case 8:
                    if (txtbox1 != null) {
                        txtbox1.setVisibility(View.INVISIBLE);
                    }
                    if (tv1 != null) {
                        tv1.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                        SetButton.setVisibility(View.INVISIBLE);
                    }
                    if(button1 != null) {
                       button1.setText(R.string.search_algo_mpx);
                       button1.setVisibility(View.VISIBLE);
                       button1.setOnClickListener(mOnSetSearchMPXDCCListener);
                    }
                    if(button2 != null) {
                       button2.setText(R.string.search_algo_sinrint);
                       button2.setVisibility(View.VISIBLE);
                       button2.setOnClickListener(mOnSetSearchSinrIntfListener);
                    }
                    break;
                case 9:
                    if (txtbox1 != null) {
                        txtbox1.setVisibility(View.INVISIBLE);
                    }
                    if (tv1 != null) {
                        tv1.setText("");
                        tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                        SetButton.setVisibility(View.INVISIBLE);
                    }
                    try {
                        if(mService != null)
                           ret = mService.getSinrSamplesCnt();
                           Log.d(LOGTAG, "Get Sinr Samples Count: " + ret);
                    }catch (RemoteException e) {
                    }
                    break;
                case 10:
                    if (txtbox1 != null) {
                        txtbox1.setVisibility(View.INVISIBLE);
                    }
                    if (tv1 != null) {
                        tv1.setText("");
                        tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                        SetButton.setVisibility(View.INVISIBLE);
                    }
                    try {
                        if(mService != null)
                           ret = mService.getSinrTh();
                           Log.d(LOGTAG, "Get Sinr Threshold: " + ret);
                    }catch (RemoteException e) {

                    }
                    break;
                case 11:
                    if (txtbox1 != null) {
                        txtbox1.setVisibility(View.INVISIBLE);
                    }
                    if (tv1 != null) {
                        tv1.setText("");
                        tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                        SetButton.setVisibility(View.INVISIBLE);
                    }
                    try {
                        if(mService != null) {
                           ret = mService.getSinrFirstStage();
                           Log.d(LOGTAG, "Get Sinr First Stage: " + ret);
                        }
                    }catch (RemoteException e) {

                    }
                    break;
                case 12:
                    if (txtbox1 != null) {
                        txtbox1.setVisibility(View.INVISIBLE);
                    }
                    if (tv1 != null) {
                        tv1.setText("");
                        tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                        SetButton.setVisibility(View.INVISIBLE);
                    }
                    try {
                        if(mService != null) {
                           ret = mService.getRmssiFirstStage();
                           Log.d(LOGTAG, "Get Rmssi First Stage: " + ret);
                        }
                    }catch (RemoteException e) {

                    }
                    break;
                case 13:
                    if (txtbox1 != null) {
                        txtbox1.setVisibility(View.INVISIBLE);
                    }
                    if (tv1 != null) {
                        tv1.setText("");
                        tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                        SetButton.setVisibility(View.INVISIBLE);
                    }
                    try {
                        if(mService != null) {
                           ret = mService.getCFOMeanTh();
                           Log.d(LOGTAG, "Get CF0 Threshold: " + ret);
                        }
                    }catch (RemoteException e) {

                    }
                    break;
                case 14:
                    if (txtbox1 != null) {
                        txtbox1.setVisibility(View.INVISIBLE);
                    }
                    if (tv1 != null) {
                        tv1.setText("");
                        tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                        SetButton.setVisibility(View.INVISIBLE);
                    }
                    try {
                        if(mService != null) {
                            ret = mService.getSearchAlgoType();
                            lastCmdSent = CMD_DEFRD_SEARCH_ALGO;
                            Log.d(LOGTAG, "Search Type: " + ret);
                        }
                    }catch (RemoteException e) {

                    }
                    break;
                case 15:
                    if (txtbox1 != null) {
                       txtbox1.setText(R.string.type_rd);
                       txtbox1.setVisibility(View.VISIBLE);
                    }
                    if (tv1 != null) {
                       tv1.setText(R.string.enter_AfJmpRmssiTh);
                       tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                       SetButton.setText(R.string.set_AfJmpRmssiTh);
                       SetButton.setVisibility(View.VISIBLE);
                       SetButton.setOnClickListener(mOnSetAfJmpRmssiThListener);
                    }
                    break;
                case 16:
                    if (txtbox1 != null) {
                       txtbox1.setText(R.string.type_rd);
                       txtbox1.setVisibility(View.VISIBLE);
                    }
                    if (tv1 != null) {
                       tv1.setText(R.string.enter_GdChRmssiTh);
                       tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                       SetButton.setText(R.string.set_GdChRmssiTh);
                       SetButton.setVisibility(View.VISIBLE);
                       SetButton.setOnClickListener(mOnSetGdChRmssiThListener);
                    }
                    break;
                case 17:
                    if (txtbox1 != null) {
                       txtbox1.setText(R.string.type_rd);
                       txtbox1.setVisibility(View.VISIBLE);
                    }
                    if (tv1 != null) {
                       tv1.setText(R.string.enter_AfJmpRmssiSmplsCnt);
                       tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                       SetButton.setText(R.string.set_AfJmpRmssiSmplsCnt);
                       SetButton.setVisibility(View.VISIBLE);
                       SetButton.setOnClickListener(mOnSetAfJmpRmssiSmplsCntListener);
                    }
                    break;
                case 18:
                    if (txtbox1 != null) {
                        txtbox1.setVisibility(View.INVISIBLE);
                    }
                    if (tv1 != null) {
                        tv1.setText("");
                        tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                        SetButton.setVisibility(View.INVISIBLE);
                    }
                    try {
                        if(mService != null) {
                           ret = mService.getAfJmpRmssiTh();
                           Log.d(LOGTAG, "Get Af Jmp Rmssi Th: " + ret);
                        }
                    }catch (RemoteException e) {

                    }
                    break;
                case 19:
                    if (txtbox1 != null) {
                        txtbox1.setVisibility(View.INVISIBLE);
                    }
                    if (tv1 != null) {
                        tv1.setText("");
                        tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                        SetButton.setVisibility(View.INVISIBLE);
                    }
                    try {
                        if(mService != null) {
                           ret = mService.getGoodChRmssiTh();
                           Log.d(LOGTAG, "Get GoodChRmssi Threshold: " + ret);
                        }
                    }catch (RemoteException e) {

                    }
                    break;
                case 20:
                    if (txtbox1 != null) {
                        txtbox1.setVisibility(View.INVISIBLE);
                    }
                    if (tv1 != null) {
                        tv1.setText("");
                        tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                        SetButton.setVisibility(View.INVISIBLE);
                    }
                    try {
                        if(mService != null) {
                           ret = mService.getAfJmpRmssiSamplesCnt();
                           Log.d(LOGTAG, "Get AfJmpRmssiSamples count: " + ret);
                        }
                    }catch (RemoteException e) {

                    }
                    break;
                case 21:
                    if (txtbox1 != null) {
                        txtbox1.setText(R.string.type_rd);
                        txtbox1.setVisibility(View.VISIBLE);
                    }
                    if (tv1 != null) {
                        tv1.setText(R.string.enter_RxRePeatCount);
                        tv1.setVisibility(View.VISIBLE);
                    }
                    if (SetButton != null) {
                        SetButton.setText(R.string.set_RxRePeatCount);
                        SetButton.setVisibility(View.VISIBLE);
                        SetButton.setOnClickListener(mOnSetRxRePeatCount);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    break;
                case 22:
                    if (txtbox1 != null) {
                       txtbox1.setText(R.string.type_rd);
                       txtbox1.setVisibility(View.VISIBLE);
                    }
                    if (tv1 != null) {
                       tv1.setText(R.string.enter_BlendSinrHi);
                       tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                       SetButton.setText(R.string.set_BlendSinrHi);
                       SetButton.setVisibility(View.VISIBLE);
                       SetButton.setOnClickListener(mOnSetBlendSinrHiListener);
                    }
                    break;
                case 23:
                    if (txtbox1 != null) {
                        txtbox1.setVisibility(View.INVISIBLE);
                    }
                    if (tv1 != null) {
                        tv1.setText("");
                        tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                        SetButton.setVisibility(View.INVISIBLE);
                    }
                    if (mReceiver != null) {
                        retval = mReceiver.getBlendSinr();
                        Log.d(LOGTAG, "Get BlendSinrHi: " + retval);
                    }
                    break;
                case 24:
                    if (txtbox1 != null) {
                       txtbox1.setText(R.string.type_rd);
                       txtbox1.setVisibility(View.VISIBLE);
                    }
                    if (tv1 != null) {
                       tv1.setText(R.string.enter_BlendRmssiHi);
                       tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                       SetButton.setText(R.string.set_BlendRmssiHi);
                       SetButton.setVisibility(View.VISIBLE);
                       SetButton.setOnClickListener(mOnSetBlendRmssiHiListener);
                    }
                    break;
                case 25:
                    if (txtbox1 != null) {
                        txtbox1.setVisibility(View.INVISIBLE);
                    }
                    if (tv1 != null) {
                        tv1.setText("");
                        tv1.setVisibility(View.VISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                        SetButton.setVisibility(View.INVISIBLE);
                    }
                    if (mReceiver != null) {
                        retval = mReceiver.getBlendRmssi();
                        Log.d(LOGTAG, "Get BlendRmssiHi: " + retval);
                    }
                    break;
                case 26:
                    tLayout.removeAllViewsInLayout();
                    mNewRowIds = NEW_ROW_ID;
                    tLayout.setVisibility(View.VISIBLE);
                    if (txtbox1 != null) {
                       txtbox1.setVisibility(View.INVISIBLE);
                    }
                    if (tv1 != null) {
                       tv1.setVisibility(View.INVISIBLE);
                    }
                    if (SetButton != null) {
                       SetButton.setVisibility(View.INVISIBLE);
                    }
                    if(button1 != null) {
                       button1.setVisibility(View.INVISIBLE);
                    }
                    if(button2 != null) {
                       button2.setVisibility(View.INVISIBLE);
                    }
                    adaptRfCfg.setDropDownViewResource(
                              android.R.layout.simple_spinner_dropdown_item);
                    spinOptionFmRf.setAdapter(adaptRfCfg);
                    spinOptionFmRf.setOnItemSelectedListener(mSpinRfCfgListener);
                    break;
            }
        }
        public void onNothingSelected(AdapterView<?> parent) {
            // Do Nothing
        }
    }

    public class RfCfgItemSelectedListener implements OnItemSelectedListener {
        public void onItemSelected(AdapterView<?> parent,
                                    View view, int pos, long id) {
            Log.d("Table","onItemSelected is hit with "+pos);
            tLayout.setVisibility(View.INVISIBLE);
            if (mTestRunning)
                stopCurTest();
            switch(pos)
            {
                case 0:
                case 1:
                case 2:
                case 3:
                    mTestSelected = pos;
                    tLayout.removeAllViewsInLayout();
                    mNewRowIds = NEW_ROW_ID;
                    tLayout.setVisibility(View.VISIBLE);
                    RunButton = (Button)findViewById(R.id.Runbutton);
                    if (RunButton != null) {
                       RunButton.setText(R.string.test_run);
                       RunButton.setVisibility(View.VISIBLE);
                       RunButton.setOnClickListener(mOnRunListener);
                    }
                    if(mTestSelected == SWEEP_TEST) {
                       enableBandSweepSetting();
                    }else {
                       disableBandSweepSetting();
                    }
                    break;
                case 4:
                    RunButton = (Button)findViewById(R.id.Runbutton);
                    if (RunButton != null) {
                       RunButton.setVisibility(View.INVISIBLE);
                    }
                    pbar = (ProgressBar) findViewById(R.id.progressbar);
                    if (pbar != null) {
                       pbar.setVisibility(View.INVISIBLE);
                    }
                    adaptCfgRf.setDropDownViewResource(
                                     android.R.layout.simple_spinner_dropdown_item);
                    spinOptionFmRf.setAdapter(adaptCfgRf);
                    if (isCherokeeChip())
                       spinOptionFmRf.setOnItemSelectedListener(mSpinCfgRfListener4);
                    if (isRomeChip())
                       spinOptionFmRf.setOnItemSelectedListener(mSpinCfgRfListener3);
                    else if(isTransportLayerSMD())
                       spinOptionFmRf.setOnItemSelectedListener(mSpinCfgRfListener2);
                    else
                       spinOptionFmRf.setOnItemSelectedListener(mSpinCfgRfListener1);
                    disableBandSweepSetting();
                    break;
            }
        }

        public void onNothingSelected(AdapterView<?> parent) {
            // Do Nothing
        }
    }
    public class FmRfItemSelectedListener implements OnItemSelectedListener {
        public void onItemSelected(AdapterView<?> parent, View view, int pos, long id) {
            Log.d("Table","onItemSelected is hit with "+pos);
            mTestSelected = pos;
            tLayout.setVisibility(View.INVISIBLE);
            chooseFMRFoption();
        }
        public void onNothingSelected(AdapterView<?> parent) {
            // Do Nothing
        }
    }
    private void checkTransportLayer() {
       String chip = mReceiver.getSocName();
       if (chip.equals("pronto"))
           mIsTransportSMD = true;
       else
           mIsTransportSMD = false;
    }
    private boolean isTransportLayerSMD() {
        return mIsTransportSMD;
    }

    private boolean isCherokeeChip() {
        Log.d(LOGTAG, "isCherokeeChip");

        String chip = mReceiver.getSocName();

        if (chip.equals("cherokee"))
            return true;
        else
            return false;
    }

    private boolean isRomeChip() {
        String chip = mReceiver.getSocName();

        if(chip.equals("rome"))
           return true;
        return false;
    }

    private boolean isHastingsChip() {
        String chip = mReceiver.getSocName();

        if(chip.equals("hastings"))
           return true;
        return false;
    }

    private void createResult(Result aRes) {
        // Get the TableLayout
        TableLayout tl = (TableLayout) findViewById(R.id.maintable);
        if (tl == null) {
            Log.e(LOGTAG, "Tl is null");
           return;
        }

         /* Create a new row to be added. */
        mNewRowIds++;
        TableRow tr2 = new TableRow(getApplicationContext());
        Point size = new Point();
        getWindowManager().getDefaultDisplay().getSize(size);
        int width = size.x;
        tr2.setLayoutParams(new LayoutParams(
                     LayoutParams.MATCH_PARENT,
                     LayoutParams.WRAP_CONTENT));
        tr2.setId(mNewRowIds);
        /* Create a Button to be the row-content. */
        TextView colFreq = new TextView(getApplicationContext());
        colFreq.setText(aRes.getFreq());
        colFreq.setTextSize(TypedValue.COMPLEX_UNIT_SP, 25);
        colFreq.setWidth(width/4);
                /* Add Button to row. */
        tr2.addView(colFreq);

        TextView colRMSSI = new TextView(getApplicationContext());
        colRMSSI.setText(aRes.getRSSI());
        colRMSSI.setTextSize(TypedValue.COMPLEX_UNIT_SP, 25);
        colRMSSI.setWidth(width/4);
        tr2.addView(colRMSSI);

        if(!isRomeChip() && !isHastingsChip()) {
            TextView colIoC = new TextView(getApplicationContext());
            colIoC.setText(aRes.getIoC());
            colIoC.setTextSize(TypedValue.COMPLEX_UNIT_SP, 25);
            colIoC.setWidth(width/4);
            tr2.addView(colIoC);
        }

        if(isTransportLayerSMD() || isRomeChip() || isCherokeeChip())
        {
             TextView colSINR = new TextView(getApplicationContext());
             colSINR.setText(aRes.getSINR());
             colSINR.setTextSize(TypedValue.COMPLEX_UNIT_SP, 25);
             colSINR.setWidth(width/4);
             tr2.addView(colSINR);
        }
          /* Add row to TableLayout. */
          /* Add row to TableLayout. */
        tl.addView(tr2,new TableLayout.LayoutParams(
             LayoutParams.MATCH_PARENT,
             LayoutParams.WRAP_CONTENT));
        if(null != mFileCursor)
        {
           try {
                 StringBuilder tempStr = new StringBuilder();
                 tempStr.append(String.format("%10s", aRes.getFreq()));
                 tempStr.append(String.format("%10s", aRes.getRSSI()));

                 if(!isRomeChip() && !isHastingsChip()) {
                     tempStr.append(String.format("%10s", aRes.getIoC()));
                     tempStr.append(String.format("%10s", aRes.getIntDet()));
                 }

                 if(isTransportLayerSMD() || isRomeChip() || isCherokeeChip())
                 {
                    tempStr.append(String.format("%10s", aRes.getSINR()));
                 }

                 tempStr.append("\r\n");
                 String testStr = new String(tempStr);
                 mFileCursor.write(testStr.getBytes());
           } catch(IOException ioe) {
                 ioe.printStackTrace();
           }
        }
    }

    private void runCurrentTest() {
        Log.d(LOGTAG, "The test being run is" +mTestSelected);

        //get test summary
        String[] szTestInformation = getResources().getStringArray(
                        R.array.rf_cfg);
        final StringBuilder szbTestHeader = new StringBuilder();
        szbTestHeader.append("running test:").append(szTestInformation[mTestSelected]);
        szbTestHeader.append("\r\n");
        String szTestHeader = new String(szbTestHeader);
        Message updateStop;
        int freq;
        if(null != mFileCursor ) {
           try {
                mFileCursor.write(szTestHeader.getBytes());
           } catch (IOException ioe) {
                ioe.printStackTrace();
           }
        }
        switch(mTestSelected)
        {
        case CUR_FREQ_TEST:
             Log.d(LOGTAG,"Current Freq test is going to run");
             freq = FmSharedPreferences.getTunedFrequency();
             Result res = GetFMStatsForFreq(freq);
             if(res != null) {
                 Log.e(LOGTAG, "CUR_FREQ_TEST: Updating UI");
                 Message updateUI = new Message();
                 updateUI.what = STATUS_UPDATE;
                 updateUI.obj = (Object)res;
                 mUIUpdateHandlerHandler.sendMessage(updateUI);
             }
             updateStop = new Message();
             updateStop.what = STATUS_DONE;
             mUIUpdateHandlerHandler.sendMessage(updateStop);
             mTestRunning = false;
              break;
        case CUR_MULTI_TEST:

              freq = FmSharedPreferences.getTunedFrequency();

              for(int i = 0; i < 20 && !Thread.currentThread().isInterrupted(); i++) {
                  try {
                      Thread.sleep(500);
                      Message updateUI = new Message();
                      updateUI.what = STATUS_UPDATE;
                      updateUI.obj = (Object)GetFMStatsForFreq(freq);
                      if(updateUI.obj == null)
                          break;
                      mUIUpdateHandlerHandler.sendMessage(updateUI);
                  }catch (InterruptedException e) {
                      /*break the loop*/
                      break;
                  }
              }
              mTestRunning = false;
              updateStop = new Message();
              updateStop.what = STATUS_DONE;
              mUIUpdateHandlerHandler.sendMessage(updateStop);
              // returns and UI in different thread.

              break;
        case SEARCH_TEST:
              try {
                  Log.e(LOGTAG, "start scanning\n");
                  Log.d(LOGTAG,"Scanning with 0 scan time");
                  if (mReceiver != null)
                      mIsSearching = mReceiver.searchStations(FmReceiver.FM_RX_SRCH_MODE_SCAN,
                              SCAN_DWELL_PERIOD, FmReceiver.FM_RX_SEARCHDIR_UP);
              } catch (Exception e) {
                  e.printStackTrace();
              }

             if(mIsSearching) {
                 Log.d(LOGTAG, "Created the results and cancel UI\n");
             }else {
                 mTestRunning = false;
             }
             break;
        case SWEEP_TEST:
             int Spacing = FmSharedPreferences.getChSpacing();
             int lowerFreq = FmSharedPreferences.getLowerLimit();
             int higherFreq = FmSharedPreferences.getUpperLimit();
             try {
                 Log.d(LOGTAG, "Going to set low side injection\n");
                 mService.setHiLoInj(Lo);
             }catch (RemoteException e) {
                 e.printStackTrace();
             }

             getFMStatsInBand(lowerFreq, higherFreq, Spacing);
             break;
        }
    }

    /* Thread processing */
    private Runnable getMultipleResults = new Runnable() {
       public void run() {
          /*Collect the data for the current frequency
           20 times*/
          int freq = FmSharedPreferences.getTunedFrequency();

          for(int i = 0; i < 20 && !Thread.currentThread().isInterrupted(); i++) {
              try {
                   Thread.sleep(500);
                   Message updateUI = new Message();
                   updateUI.what = STATUS_UPDATE;
                   updateUI.obj = (Object)GetFMStatsForFreq(freq);
                   if(updateUI.obj == null)
                        break;
                   mUIUpdateHandlerHandler.sendMessage(updateUI);
              }catch (InterruptedException e) {
                   /*break the loop*/
                   break;
              }
            }
            mTestRunning = false;
            Message updateStop = new Message();
            updateStop.what = STATUS_DONE;
            mUIUpdateHandlerHandler.sendMessage(updateStop);
       }
    };

    private void getFMStatsInBand(int lFreq, int hFreq, int Spacing) {
       if(null == mBand) {
          mBand = new Band();
       }
       mBand.lFreq = lFreq;
       mBand.hFreq = hFreq;
       mBand.cur_freq = lFreq;
       if(Spacing == 0) {
          mBand.Spacing = 200; // 200KHz
       }else if(Spacing == 1) {
          mBand.Spacing = 100; // 100KHz
       }else {
          mBand.Spacing = 50;
       }

       setAlarm(prevDelayTime * 1000, BAND_SWEEP_START_DELAY_TIMEOUT);
    }

    /* Thread processing */
    private Runnable getManualSweepResults = new Runnable() {
       public void run() {
           try {
               if(mBand == null) {
                  return;
               }
               mWakeLock.acquire(10 * 1000);
               if(mBand.cur_freq <= mBand.hFreq) {
                  if(!tuneAndUpdateSweepResult(mBand.cur_freq)) {
                     sendStatusDoneMsg();
                     return;
                  }
                  mBand.cur_freq += mBand.Spacing;
                  if(mBand.cur_freq > mBand.hFreq) {
                     sendStatusDoneMsg();
                  }else {
                     setAlarm(prevDwellTime * 1000, BAND_SWEEP_DWELL_DELAY_TIMEOUT);
                  }
               }else {
                  sendStatusDoneMsg();
               }
           }catch(Exception e) {
               e.printStackTrace();
           }
       }
    };

    private void sendStatusDoneMsg() {
       mTestRunning = false;
       Message updateStop = new Message();
       updateStop.what = STATUS_DONE;
       try {
           Log.d(LOGTAG, "Going to set auto hi-lo injection\n");
           mService.setHiLoInj(Auto);
       } catch (RemoteException e) {
           e.printStackTrace();
       }
       if(mUIUpdateHandlerHandler != null) {
          Log.d(LOGTAG, "Sending message to stop test");
          mUIUpdateHandlerHandler.sendMessage(updateStop);
       }
    }

    private Runnable getFileSweepResults = new Runnable() {
       public void run() {
         boolean status = true;
         int freq;

         try {
              mWakeLock.acquire(10 * 1000);
              freq = mNextFreqInterface.getNextFreq();
              for(; (status = (!mNextFreqInterface.errorOccured()) &
                     (!Thread.currentThread().isInterrupted()));
                          freq = mNextFreqInterface.getNextFreq()) {
                  if(validFreq(freq)) {
                     if(!tuneAndUpdateSweepResult(freq)) {
                        status = false;
                        break;
                     }else {
                        setAlarm(prevDwellTime * 1000,
                                  BAND_SWEEP_DWELL_DELAY_TIMEOUT);
                        break;
                     }
                  }
              }
              if(!status) {
                 sendStatusDoneMsg();
                 mNextFreqInterface.Stop();
                 mNextFreqInterface = null;
              }
         }catch (Exception e) {
              e.printStackTrace();
         }
       }
    };

    private boolean validFreq(int freq) {
       if((freq >= mBand.lFreq) && (freq <= mBand.hFreq)
           &&
          (((freq - mBand.lFreq) / mBand.Spacing) >= 0)) {
           return true;
       }else {
           return false;
       }
    }

    private boolean tuneAndUpdateSweepResult(int freq) {
       try {
            if(!mService.tune(freq)) {
               Log.e(LOGTAG, "tune failed");
               return false;
            }
            mSync = new Band();
            synchronized(mSync) {
                mSync.wait(); //wait till notified
            }
            mSync = null;
            Message updateUI = new Message();
            updateUI.what = STATUS_UPDATE;
            updateUI.obj = (Object)GetFMStatsForFreq(freq);
            if(updateUI.obj == null) {
               return false;
            }else {
               mUIUpdateHandlerHandler.sendMessage(updateUI);
               Log.d(LOGTAG,"highFerq is " + mBand.hFreq);
            }
       }catch (RemoteException e) {
            Log.e(LOGTAG, "SweepResults:Tune failed\n");
            return false;
       }catch (InterruptedException e) {
            return false;
       }
       return true;
    }

    private Result GetFMStatsForFreq(int freq)
    {
        Result result = new Result();
        int ret;
        Log.d(LOGTAG,"freq is "+freq);
        result.setFreq(Integer.toString(freq));
        boolean isCherokeeChip = isCherokeeChip();
        if((null != mService)) {
            try {
                lastCmdSent = CMD_STNPARAM_RSSI;
                ret = mService.getRssi();
                if (ret != 0) {
                    Log.e(LOGTAG, "getrssi cmd failed: ret = " + ret);
                    lastCmdSent = 0;
                    return null;
                }
                Log.e(LOGTAG, "Got response of mService.getRssi");
                if (nRssi != Integer.MAX_VALUE) {
                    result.setRSSI(Integer.toString((byte)nRssi));
                } else {
                    return null;
                }
            } catch (RemoteException e) {
                e.printStackTrace();
            } catch(Exception e) {
                e.printStackTrace();
            }

            if(!isRomeChip() && !isHastingsChip()) {
                try {
                    if (isCherokeeChip) {
                        mService.getIoC();
                        lastCmdSent = CMD_STNDBGPARAM_IOVERC;
                        Log.e(LOGTAG, "wait for response of mService.getIoC");
                        synchronized (obj) {
                            try {
                                obj.wait();
                            } catch (InterruptedException e) {
                                Log.e(LOGTAG, "getIOC:THREAD interrupted");
                                e.printStackTrace();
                                return null;
                            }
                        }
                    } else {
                        nIoC = mService.getIoC();
                    }
                    Log.e(LOGTAG, "GOT response of mService.getIoC");
                    if (nIoC != Integer.MAX_VALUE)
                        result.setIoC(Integer.toString(nIoC));
                    else
                        return null;
                } catch (RemoteException e) {
                    e.printStackTrace();
                } catch(Exception e) {
                    e.printStackTrace();
                }
            }

            if(isTransportLayerSMD() || isRomeChip() || isCherokeeChip) {
                try {
                    if (isCherokeeChip) {
                        lastCmdSent = CMD_STNPARAM_SINR;
                        mService.getSINR();
                    } else {
                        nSINR = mService.getSINR();
                    }
                    Log.e(LOGTAG, "Got response of mService.getSINR");
                    if (nSINR != Integer.MAX_VALUE) {
                        result.setSINR(Integer.toString((byte)nSINR));
                    } else {
                        return null;
                    }
                } catch (RemoteException e) {
                    e.printStackTrace();
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }

            if(!isRomeChip() && !isHastingsChip()) {
                try {
                    if (isCherokeeChip) {
                        mService.getIntDet();
                        lastCmdSent = CMD_STNDBGPARAM_INFDETOUT;
                        Log.e(LOGTAG, "wait for response of mService.getIntDet");
                        synchronized (obj) {
                            try {
                                obj.wait();
                            } catch (InterruptedException e) {
                                Log.e(LOGTAG, "getIntDet:THREAD interrupted");
                                e.printStackTrace();
                                return null;
                            }
                        }
                    } else {
                        nIntDet = mService.getIntDet();
                    }
                    Log.e(LOGTAG, "Got response of mService.getIntDet");
                    if (nIntDet != Integer.MAX_VALUE)
                        result.setIntDet(Integer.toString(nIntDet));
                    else
                        return null;
                } catch (RemoteException e) {
                    e.printStackTrace();
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        } else {
            return null;
        }
        return result;
    }


    private Handler mUIUpdateHandlerHandler = new Handler() {
            public void handleMessage(Message msg) {
                Log.d(LOGTAG, "mUIUpdateHandlerHandler: msg.what = " + msg.what);
               switch (msg.what)
               {
               case STATUS_UPDATE:
                    Result myRes = (Result) msg.obj;
                    Log.d(LOGTAG,"Status update is" +myRes.mFreq + "mRSSI=" + myRes.mRSSI + "mSINR=" + myRes.mSINR);
                    createResult(myRes);
                    break;
               case STATUS_DONE:
                    SetButtonState(true);
                    break;
               case RECORDTIMER_EXPIRED:
                    Log.d(LOGTAG, "mUIUpdateHandlerHandler - RECORDTIMER_EXPIRED");
                    if(!isRecording()) {
                       Log.d(LOGTAG, "Stop Recording");
                       stopRecording();
                    }
                   break;
               case RECORDTIMER_UPDATE:
                   Log.d(LOGTAG, "mUIUpdateHandlerHandler - RECORDTIMER_UPDATE");
                   updateExpiredRecordTime();
                   break;
               }
            }
    };

    public IFMRadioService sService = null;
    private HashMap<Context, ServiceBinder> sConnectionMap = new HashMap<Context, ServiceBinder>();

    public boolean bindToService(Context context) {
       Log.e(LOGTAG, "bindToService: Context");
       return bindToService(context, null);
    }

    public boolean bindToService(Context context, ServiceConnection callback) {
       Log.e(LOGTAG, "bindToService: Context with serviceconnection callback");
       context.startService(new Intent(context, FMRadioService.class));
       ServiceBinder sb = new ServiceBinder(callback);
       sConnectionMap.put(context, sb);
       return context.bindService((new Intent()).setClass(context,
                                                          FMRadioService.class), sb, 0);
    }

    public void unbindFromService(Context context) {
       ServiceBinder sb = (ServiceBinder) sConnectionMap.remove(context);
       Log.e(LOGTAG, "unbindFromService: Context");
       if (sb == null)
       {
          Log.e(LOGTAG, "Trying to unbind for unknown Context");
          return;
       }
       context.unbindService(sb);
       if (sConnectionMap.isEmpty())
       {
          // presumably there is nobody interested in the service at this point,
          // so don't hang on to the ServiceConnection
          mService = null;
       }
    }

    private class ServiceBinder implements ServiceConnection
    {
       ServiceConnection mCallback;
       ServiceBinder(ServiceConnection callback) {
          mCallback = callback;
       }

       public void onServiceConnected(ComponentName className, android.os.IBinder service) {
          if (mCallback != null)
          {
             Log.e(LOGTAG, "onServiceConnected: mCallback");
             mCallback.onServiceConnected(className, service);
          }
       }

       public void onServiceDisconnected(ComponentName className) {
          if (mCallback != null)
          {
             mCallback.onServiceDisconnected(className);
          }
          mService = null;
       }
    }


    private ServiceConnection osc = new ServiceConnection() {
          public void onServiceConnected(ComponentName classname, IBinder obj) {
             mService = IFMRadioService.Stub.asInterface(obj);
             Log.e(LOGTAG, "ServiceConnection: onServiceConnected: ");
             if (mService != null)
             {
                try
                {
                   mService.registerCallbacks(mServiceCallbacks);

                } catch (RemoteException e)
                {
                   e.printStackTrace();
                }
                if(isRecording()) {
                   initiateRecordThread();
                }
                return;
             } else
             {
                Log.e(LOGTAG, "IFMRadioService onServiceConnected failed");
             }
             finish();
          }
          public void onServiceDisconnected(ComponentName classname) {
          }
       };


       private IFMRadioServiceCallbacks.Stub  mServiceCallbacks = new IFMRadioServiceCallbacks.Stub()
       {
          public void onEnabled()
          {
             Log.d(LOGTAG, "mServiceCallbacks.onEnabled :");
             invalidateOptionsMenu();
          }

          public void onDisabled()
          {
             Log.d(LOGTAG, "mServiceCallbacks.onDisabled :");
             stopAllOperations();
          }

          public void onRadioReset()
          {
             Log.d(LOGTAG, "mServiceCallbacks.onRadioReset :");
             stopAllOperations();
          }

          public void onTuneStatusChanged()
          {
             Log.d(LOGTAG, "mServiceCallbacks.onTuneStatusChanged :" + mTestRunning);
             if (mTestRunning) {
                 if ((mTuneCompleteThread == null) || (mTuneCompleteThread.getState() == Thread.State.TERMINATED)) {
                     mTuneCompleteThread = new Thread(mTuneComplete,
                                                "mTuneCompleteThread");
                 } else {
                     Log.e(LOGTAG, "mTuneCompleteThread is already running");
                     return;
                 }
                 if (mTuneCompleteThread != null) {
                     mTuneCompleteThread.start();
                 } else {
                     Log.e(LOGTAG, "mTuneCompleteThread: new thread create failed");
                     return;
                 }
             }
          }

          public void onProgramServiceChanged()
          {
             Log.d(LOGTAG, "mServiceCallbacks.onProgramServiceChanged :");
          }

          public void onRadioTextChanged()
          {
             Log.d(LOGTAG, "mServiceCallbacks.onRadioTextChanged :");
          }
          public void onExtenRadioTextChanged()
          {
             Log.d(LOGTAG, "Extended Radio Text changed:");
          }
          public void onExtenCountryCodeChanged()
          {
             Log.d(LOGTAG, "Extended ountry Code  changed:");
          }
          public void onAlternateFrequencyChanged()
          {
             Log.d(LOGTAG, "mServiceCallbacks.onAlternateFrequencyChanged :");
          }

          public void onSignalStrengthChanged()
          {
             Log.d(LOGTAG, "mServiceCallbacks.onSignalStrengthChanged :");
          }

          public void onSearchComplete()
          {
             Log.d(LOGTAG, "mServiceCallbacks.onSearchComplete :");
             /*Stop the update*/
             mTestRunning = false;
             Message updateStop = new Message();
             updateStop.what = STATUS_DONE;
             mUIUpdateHandlerHandler.sendMessage(updateStop);
          }
          public void onSearchListComplete()
          {
             Log.d(LOGTAG, "mServiceCallbacks.onSearchListComplete :");

          }

          public void onMute(boolean bMuted)
          {
             Log.d(LOGTAG, "mServiceCallbacks.onMute :" + bMuted);
          }

          public void onAudioUpdate(boolean bStereo)
          {
             Log.d(LOGTAG, "mServiceCallbacks.onAudioUpdate :" + bStereo);
          }

          public void onStationRDSSupported(boolean bRDSSupported)
          {
             Log.d(LOGTAG, "mServiceCallbacks.onStationRDSSupported :" + bRDSSupported);
          }
          public void onRecordingStopped()
          {
             Log.d(LOGTAG, "mServiceCallbacks.onDisabled :");
             stopRecording();
          }
          public void onFinishActivity()
          {
             Log.d(LOGTAG, "mServiceCallbacks.onFinishActivity:");
          }
          public void onRecordingStarted()
          {
             Log.d(LOGTAG, "mServiceCallbacks.onRecordingStarted:");
             int durationInMins = FmSharedPreferences.getRecordDuration();
             Log.e(LOGTAG, " Fected duration: " + durationInMins);
             initiateRecordDurationTimer(durationInMins);
             invalidateOptionsMenu();
          }
          public void onSeekNextStation() {
             Log.d(LOGTAG, "mServiceCallbacks.onSeekNextStation:");
          }
          public void onA2DPConnectionstateChanged(boolean state){
             Log.d(LOGTAG, "mServiceCallbacks.onA2DPConnectionstateChanged :");
          }
          public void onFmAudioPathStarted() {
             Log.d(LOGTAG, "mServiceCallbacks.onFmAudioPathStarted:");
          }
          public void onFmAudioPathStopped() {
             Log.d(LOGTAG, "mServiceCallbacks.onFmAudioPathStopped:");
          }
          public void getSigThCb(int val, int status) {
              Log.d(LOGTAG, "getSigThCb ");
              mCallbackHandler.obtainMessage(SIGNAL_THRESHOLD, val, status).sendToTarget();
          }

          public void getChDetThCb(int val, int status) {
              Log.d(LOGTAG, "getChDetThCb");
              mCallbackHandler.obtainMessage(GET_CHANNEL_DET_THRESHOLD, val, status).sendToTarget();
          }

          public void setChDetThCb(int status)
          {
              Log.d(LOGTAG, "setChDetTh++");
              mCallbackHandler.obtainMessage(SET_CHANNEL_DET_THRESHOLD, status).sendToTarget();
          }

          public void DefDataRdCb(int val, int status) {
              Log.d(LOGTAG, "DefDataRdCb");
              mCallbackHandler.obtainMessage(DEFAULT_DATA_READ, val, status).sendToTarget();
          }

          public void DefDataWrtCb(int status)
          {
              Log.d(LOGTAG, "DefDataWrtCb");
              mCallbackHandler.obtainMessage(DEFAULT_DATA_WRITE, status).sendToTarget();
          }

          public void getBlendCb(int val, int status) {
              Log.d(LOGTAG, "getBlend");
              mCallbackHandler.obtainMessage(GET_BLEND_TBL, val, status).sendToTarget();
          }

          public void setBlendCb(int status)
          {
              Log.d(LOGTAG, "setBlendCb");
              mCallbackHandler.obtainMessage(SET_BLEND_TBL, status).sendToTarget();
          }
          public void getStationParamCb(int val, int status)
          {
              Log.d(LOGTAG, "getStationParamCb");
              mCallbackHandler.obtainMessage(GET_STATION_PARAM, val, status).sendToTarget();
          }
          public void getStationDbgParamCb(int val, int status)
          {
              Log.d(LOGTAG, "getStationDbgParamCb");
              mCallbackHandler.obtainMessage(GET_STATION_DBG_PARAM, val, status).sendToTarget();
          }
      };

     private void stopCurTest() {
         if (mRunTestThread != null) {
             mRunTestThread.interrupt();
         }
         if (mTestRunning) {
             switch(mTestSelected) {
             case CUR_FREQ_TEST:
                  break;
             case SWEEP_TEST:
                  stopBandSweep();
                  sendStatusDoneMsg();
                  break;
             case CUR_MULTI_TEST:
                  if (mMultiUpdateThread != null)
                      mMultiUpdateThread.interrupt();
                  break;
             case SEARCH_TEST:
                  if (mTuneCompleteThread != null)
                      mTuneCompleteThread.interrupt();
                  if (mService != null) {
                      try {
                           Message updateStop = new Message();
                           updateStop.what = STATUS_DONE;
                           mUIUpdateHandlerHandler.sendMessage(updateStop);
                           mService.cancelSearch();
                      } catch (RemoteException e) {
                           e.printStackTrace();
                      }
                  }
                  break;
             }
             mTestRunning = false;
         }
     }

     private Dialog createBandSweepDialog(int id, AlertDialog.Builder dlgBuilder) {
         LayoutInflater inflater = LayoutInflater.from(this);
         final View listview = inflater.inflate(R.layout.band_sweep_setting, null);
         spinOptionBandSweepMthds = (Spinner)listview.findViewById(R.id.band_sweep_spinner);
         final EditText delayBox = (EditText)listview.findViewById(R.id.txtboxDelayTime);
         final EditText dwellBox = (EditText)listview.findViewById(R.id.txtboxDwellTime);

         if(delayBox != null) {
            delayBox.setText("" + prevDelayTime);
         }
         if(dwellBox != null) {
            dwellBox.setText("" + prevDwellTime);
         }
         if(spinOptionBandSweepMthds != null) {
            spinOptionBandSweepMthds.setAdapter(bandSweepMthds);
            spinOptionBandSweepMthds.setOnItemSelectedListener(mSweepMthdsListener);
            spinOptionBandSweepMthds.setSelection(prevSweepMthd);
         }else {
            Log.e(LOGTAG, "could not find spinner for methods\n");
         }
         dlgBuilder.setView(listview)
                .setPositiveButton(R.string.band_sweep_setting_set, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        prevSweepMthd = curSweepMthd;
                        if(delayBox != null) {
                           String s = delayBox.getText().toString();
                           prevDelayTime = Integer.parseInt(s);
                        }
                        if(dwellBox != null) {
                           String s = dwellBox.getText().toString();
                           prevDwellTime = Integer.parseInt(s);
                        }
                        closeDialog(DIALOG_BAND_SWEEP_SETTING);
                    }
                })
                .setNegativeButton(R.string.band_sweep_setting_cancel, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        closeDialog(DIALOG_BAND_SWEEP_SETTING);
                    }
                });
         return dlgBuilder.create();
     }

    private View.OnClickListener mClicktBandSweepSettingListener = new View.OnClickListener() {
         public void onClick(View v) {
            AlertDialog.Builder dlgBuilder = new AlertDialog.Builder(FMStats.this);
            mDialgBandSweep = createBandSweepDialog(DIALOG_BAND_SWEEP_SETTING, dlgBuilder);
            mDialgBandSweep.show();
         }
    };

    private void enableBandSweepSetting() {
         if(bandSweepSettingButton != null) {
            bandSweepSettingButton.setEnabled(true);
            bandSweepSettingButton.setVisibility(View.VISIBLE);
         }
    }

    private void disableBandSweepSetting() {
         if(bandSweepSettingButton != null) {
            bandSweepSettingButton.setEnabled(false);
            bandSweepSettingButton.setVisibility(View.INVISIBLE);
         }
    }

    public class BandSweepMthdsSelectedListener implements OnItemSelectedListener {
        public void onItemSelected(AdapterView<?> parent, View view, int pos, long id) {
            Log.d("Band Sweep Methods","onItemSelected is hit with " + pos);
            curSweepMthd = pos;
        }

        public void onNothingSelected(AdapterView<?> parent) {
            // Do Nothing
        }
    }

    private void registerBandSweepDelayExprdListener() {
        if(mBandSweepDelayExprdListener == null) {
           mBandSweepDelayExprdListener = new BroadcastReceiver() {
              @Override
               public void onReceive(Context context, Intent intent) {
                   Log.d(LOGTAG, "Received Band sweep delay expired");
                   mWakeLock.acquire(5 * 1000);
                   StartBandSweep();
               }
           };
           IntentFilter intentFilter = new IntentFilter(BAND_SWEEP_START_DELAY_TIMEOUT);
           registerReceiver(mBandSweepDelayExprdListener, intentFilter, Context.RECEIVER_EXPORTED);
        }
    }

    private void registerBandSweepDwellExprdListener() {
        if(mBandSweepDwellExprdListener == null) {
           mBandSweepDwellExprdListener = new BroadcastReceiver() {
              @Override
               public void onReceive(Context context, Intent intent) {
                   Log.d(LOGTAG, "received Band sweep Dwell expired");
                   if(mTestRunning) {
                      mWakeLock.acquire(5 * 1000);
                      ResumeBandSweep();
                   }
               }
           };
           IntentFilter intentFilter = new IntentFilter(BAND_SWEEP_DWELL_DELAY_TIMEOUT);
           registerReceiver(mBandSweepDwellExprdListener, intentFilter, Context.RECEIVER_EXPORTED);
        }
    }

    private void unRegisterBroadcastReceiver(BroadcastReceiver receiver) {
        if(receiver != null) {
           unregisterReceiver(receiver);
           receiver = null;
        }
    }

    private void setAlarm(long duration, String action) {
        Intent i = new Intent(action);
        AlarmManager am = (AlarmManager)getSystemService(Context.ALARM_SERVICE);
        PendingIntent pi = PendingIntent.getBroadcast(this, 0, i, PendingIntent.FLAG_IMMUTABLE);
        am.set(AlarmManager.ELAPSED_REALTIME_WAKEUP, SystemClock.elapsedRealtime() + duration, pi);
    }

    private void cancelAlarm(String action) {
        Intent i = new Intent(action);
        AlarmManager am = (AlarmManager)getSystemService(Context.ALARM_SERVICE);
        PendingIntent pi = PendingIntent.getBroadcast(this, 0, i, PendingIntent.FLAG_IMMUTABLE);
        am.cancel(pi);
    }

    private void StartBandSweep() {
       if(mMultiUpdateThread == null ||
            (mMultiUpdateThread.getState() == Thread.State.TERMINATED)) {
          if(prevSweepMthd == 0) {
             mMultiUpdateThread = new Thread(null, getManualSweepResults,
                                           "MultiResultsThread");
          }else {
             mNextFreqInterface = new CommaSeparatedFreqFileReader(getFilesDir() + FREQ_LIST_FILE_NAME);
             mMultiUpdateThread = new Thread(null, getFileSweepResults,
                                           "MultiResultsThread");
          }
       }

       /* If the thread state is "new" then the thread has not yet started */
       if(mMultiUpdateThread.getState() == Thread.State.NEW) {
          mMultiUpdateThread.start();
       }
    }

    private void ResumeBandSweep() {
       if(mMultiUpdateThread == null ||
            (mMultiUpdateThread.getState() == Thread.State.TERMINATED)) {
          if(prevSweepMthd == 0) {
             mMultiUpdateThread = new Thread(null, getManualSweepResults,
                                           "MultiResultsThread");
          }else {
             mMultiUpdateThread = new Thread(null, getFileSweepResults,
                                           "MultiResultsThread");
          }
       }

       /* If the thread state is "new" then the thread has not yet started */
       if(mMultiUpdateThread.getState() == Thread.State.NEW) {
          mMultiUpdateThread.start();
       }
    }

    private void stopBandSweep() {
       cancelAlarm(BAND_SWEEP_START_DELAY_TIMEOUT);
       cancelAlarm(BAND_SWEEP_DWELL_DELAY_TIMEOUT);
       if(mMultiUpdateThread != null) {
          mMultiUpdateThread.interrupt();
       }
       if(mNextFreqInterface != null) {
          mNextFreqInterface.Stop();
          mNextFreqInterface = null;
       }
    }

    public boolean isRecording() {
       if(mService == null)
          return false;
       try {
            return mService.isFmRecordingOn();
       }catch(RemoteException e) {
            return false;
       }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.menu_rf_stats, menu);
        return true;
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        super.onPrepareOptionsMenu(menu);
        MenuItem item;

        item = menu.findItem(R.id.menu_recording);
        if(item != null && !isRecording()) {
           item.setTitle(R.string.menu_record_start);
           item.setEnabled(isFmOn());
        }else if(item != null) {
           item.setTitle(R.string.menu_record_stop);
           setRecordDurationDisplay(menu, R.id.menu_record_duration);
        }
        return true;
    }

    private void setRecordDurationDisplay(Menu menu, int id) {
       MenuItem item;
       long timeNow;
       long seconds;

       if(menu == null)
          return;
       item = menu.findItem(id);
       if(item != null) {
          timeNow = SystemClock.elapsedRealtime();
          seconds = (timeNow - getRecordingStartTime()) / 1000;
          item.setTitle(makeTimeString(seconds));
       }
    }

    private void startRecording() {
      if(isFmOn()) {
         try {
              mService.startRecording();
         }catch(RemoteException e) {
              e.printStackTrace();
         }
      }
    }

    private void stopRecording() {
       if(null != mRecordUpdateHandlerThread) {
          mRecordUpdateHandlerThread.interrupt();
       }
       if(mService != null) {
          try {
               mService.stopRecording();
           }catch (RemoteException e) {
               e.printStackTrace();
           }
        }
        invalidateOptionsMenu();
    }

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
      if(mRecordUpdateHandlerThread == null) {
         mRecordUpdateHandlerThread = new Thread(null, doRecordProcessing,
                                                "RecordUpdateThread");
      }
      /* Launch the dummy thread to simulate the transfer progress */
      Log.d(LOGTAG, "Thread State: " + mRecordUpdateHandlerThread.getState());
      if(mRecordUpdateHandlerThread.getState() == Thread.State.TERMINATED) {
         mRecordUpdateHandlerThread = new Thread(null, doRecordProcessing,
                                                "RecordUpdateThread");
      }
      /* If the thread state is "new" then the thread has not yet started */
      if(mRecordUpdateHandlerThread.getState() == Thread.State.NEW) {
         mRecordUpdateHandlerThread.start();
      }
   }

   /* Recorder Thread processing */
   private Runnable doRecordProcessing = new Runnable() {
      public void run() {
         while(isRecording() &&
                 (!Thread.currentThread().isInterrupted())) {
               try {
                    Thread.sleep(500);
                    Message statusUpdate = new Message();
                    statusUpdate.what = RECORDTIMER_UPDATE;
                    mUIUpdateHandlerHandler.sendMessage(statusUpdate);
               }catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
               }
               if(!isRecording()) {
                  Message finished = new Message();
                  finished.what = RECORDTIMER_EXPIRED;
                  mUIUpdateHandlerHandler.sendMessage(finished);
               }
         }
      }
   };

   private void updateExpiredRecordTime() {
      int vis = View.VISIBLE;
      if(isRecording()) {
         invalidateOptionsMenu();
      }
   }

   private String makeTimeString(long secs) {
      String durationformat = getString(R.string.durationformat);

      /** Provide multiple arguments so the format can be changed easily by
       *  modifying the xml.
       **/
      sFormatBuilder.setLength(0);

      final Object[] timeArgs = sTimeArgs;
      timeArgs[0] = secs / 3600;
      timeArgs[1] = secs / 60;
      timeArgs[2] = (secs / 60) % 60;
      timeArgs[3] = secs;
      timeArgs[4] = secs % 60;

      return sFormatter.format(durationformat, timeArgs).toString();
   }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch(item.getItemId())
        {
          case R.id.menu_recording:
               if(isRecording()) {
                  stopRecording();
               }else {
                  startRecording();
               }
               break;
        }
        return true;
    }

    private boolean isFmOn() {
       boolean status = false;

       if(mService != null) {
          try {
               status = mService.isFmOn();
          }catch(RemoteException e) {
          }
       }
       return status;
    }

    private void stopAllOperations() {
       stopCurTest();
       stopRecording();
    }

    private void closeDialog(int id) {
        Log.d(LOGTAG, " closeDialog " + id);
        switch (id) {
        case DIALOG_BAND_SWEEP_SETTING:
            if (mDialgBandSweep != null && mDialgBandSweep.isShowing()) {
                mDialgBandSweep.dismiss();
            }
            break;
        }
    }
}
