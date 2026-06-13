/*
 * Copyright (c) 2009-2014, The Linux Foundation. All rights reserved.
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

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.content.Intent;
import android.os.Bundle;
import android.preference.PreferenceActivity;
import android.preference.PreferenceScreen;
import android.preference.ListPreference;
import android.preference.CheckBoxPreference;
import android.preference.PreferenceCategory;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.EditTextPreference;
import android.text.InputType;
import android.widget.Toast;
import java.util.Locale;

import android.util.Log;

public class Settings extends PreferenceActivity implements
                OnSharedPreferenceChangeListener, OnPreferenceClickListener {
        public static final String RX_MODE = "rx_mode";
        public static final String REGIONAL_BAND_KEY = "regional_band";
        public static final String AUDIO_OUTPUT_KEY = "audio_output_mode";
        public static final String RECORD_DURATION_KEY = "record_duration";
        public static final String AUTO_AF = "af_checkbox_preference";
        public static final String RESTORE_FACTORY_DEFAULT = "revert_to_fac";
        public static final int RESTORE_FACTORY_DEFAULT_INT = 1;
        public static final String USER_DEFINED_BAND_MIN_KEY = "user_defined_band_min";
        public static final String USER_DEFINED_BAND_MAX_KEY = "user_defined_band_max";
        public static final String CHAN_SPACING_KEY = "chanl_spacing";
        public static final String RESTORE_FACTORY_DEFAULT_ACTION = "com.caf.fmradio.settings.revert_to_defaults";
        public static final String ACTION_FM_SETTING = "com.caf.fmradio.settings.changed";
        public static final int FM_BAND_CHANGED = 1;
        public static final int FM_CHAN_SPACING_CHANGED = 2;
        public static final int FM_AF_OPTION_CHANGED = 3;
        public static final int FM_AUDIO_MODE_CHANGED = 4;
        private static final String LOGTAG = FMRadio.LOGTAG;
        private static final String USR_BAND_MSG = "Enter Freq from range 76.0 - 108.0";

        private ListPreference mBandPreference;
        private ListPreference mAudioPreference;
        private ListPreference mRecordDurPreference;
        private CheckBoxPreference mAfPref;
        private EditTextPreference mUserBandMinPref;
        private EditTextPreference mUserBandMaxPref;
        private ListPreference mChannelSpacingPref;
        private Preference mRestoreDefaultPreference;

        private FmSharedPreferences mPrefs = null;
        private boolean mRxMode = false;
        private Dialog mDlgRestoreFactory;

        private int min_freq;
        private int max_freq;
        private int chan_spacing;
        private String[] summaryBandItems;
        private String[] chSpacingItems;
        private String[] summaryAudioModeItems;
        private String[] summaryRecordItems;

        @Override
        protected void onCreate(Bundle savedInstanceState) {
          super.onCreate(savedInstanceState);
          Intent intent = getIntent();
          if (intent != null) {
              mRxMode = intent.getBooleanExtra(RX_MODE, false);
          }
          mPrefs = new FmSharedPreferences(this);
          if (mPrefs != null) {
              setPreferenceScreen(createPreferenceHierarchy());
          }
        }

        private PreferenceScreen createPreferenceHierarchy() {
          int index = 0;
          if (mPrefs == null) {
              return null;
          }
          // Root
          PreferenceScreen root = getPreferenceManager().createPreferenceScreen(
                                this);

           summaryBandItems = getResources().getStringArray(
                                R.array.regional_band_summary);
           chSpacingItems = getResources().getStringArray(
                                R.array.channel_spacing_entries);
           mBandPreference = new ListPreference(this);
           mBandPreference.setEntries(R.array.regional_band_entries);
           mBandPreference.setEntryValues(R.array.regional_band_values);
           mBandPreference.setDialogTitle(R.string.sel_band_menu);
           mBandPreference.setKey(REGIONAL_BAND_KEY);
           mBandPreference.setTitle(R.string.regional_band);
           index = FmSharedPreferences.getCountry();
           Log.d(LOGTAG, "createPreferenceHierarchy: Country: " + index);
           // Get the preference and list the value.
           if ((index < 0) || (index >= summaryBandItems.length)) {
                index = 0;
           }
           mBandPreference.setValueIndex(index);
           root.addPreference(mBandPreference);

           mChannelSpacingPref = new ListPreference(this);
           mChannelSpacingPref.setEntries(R.array.channel_spacing_entries);
           mChannelSpacingPref.setEntryValues(R.array.channel_spacing_val);
           mChannelSpacingPref.setDialogTitle(R.string.sel_chanl_spacing);
           mChannelSpacingPref.setTitle(R.string.chanl_spacing);
           mChannelSpacingPref.setKey(CHAN_SPACING_KEY);

           mUserBandMinPref = new EditTextPreference(this);
           mUserBandMinPref.setKey(USER_DEFINED_BAND_MIN_KEY);
           mUserBandMinPref.setTitle(R.string.usr_def_band_min);
           mUserBandMinPref.getEditText().setInputType(InputType.TYPE_CLASS_NUMBER |
                                                        InputType.TYPE_NUMBER_FLAG_DECIMAL);
           mUserBandMinPref.setDialogTitle(R.string.usr_def_band_min);

           mUserBandMaxPref = new EditTextPreference(this);
           mUserBandMaxPref.setKey(USER_DEFINED_BAND_MAX_KEY);
           mUserBandMaxPref.setTitle(R.string.usr_def_band_max);
           mUserBandMaxPref.setDialogTitle(R.string.usr_def_band_max);
           mUserBandMaxPref.getEditText().setInputType(InputType.TYPE_CLASS_NUMBER |
                                                        InputType.TYPE_NUMBER_FLAG_DECIMAL);

           setBandSummary(index);

           root.addPreference(mChannelSpacingPref);
           root.addPreference(mUserBandMinPref);
           root.addPreference(mUserBandMaxPref);

           boolean audiomode = FmSharedPreferences.getAudioOutputMode();
           if (mRxMode) {
               // Audio Output (Stereo or Mono)
               if (getResources().getBoolean(R.bool.def_only_stereo_enabled)) {
                   summaryAudioModeItems = getResources()
                           .getStringArray(R.array.ster_entries);
                   mAudioPreference = new ListPreference(this);
                   mAudioPreference.setEntries(R.array.ster_entries);
                   mAudioPreference.setEntryValues(R.array.ster_values);
                   index = 0;
               } else {
                   summaryAudioModeItems = getResources()
                           .getStringArray(R.array.ster_mon_entries);
                   mAudioPreference = new ListPreference(this);
                   mAudioPreference.setEntries(R.array.ster_mon_entries);
                   mAudioPreference.setEntryValues(R.array.ster_mon_values);
                   if (audiomode) {
                      index = 0;
                   } else {
                      index = 1;
                   }
               }
               mAudioPreference.setDialogTitle(R.string.sel_audio_output);
               mAudioPreference.setKey(AUDIO_OUTPUT_KEY);
               mAudioPreference.setTitle(R.string.aud_output_mode);

               Log.d(LOGTAG, "createPreferenceHierarchy: audiomode: " + audiomode);
               mAudioPreference.setSummary(summaryAudioModeItems[index]);
               mAudioPreference.setValueIndex(index);
               root.addPreference(mAudioPreference);

               // AF Auto Enable (Checkbox)
               mAfPref = new CheckBoxPreference(this);
               mAfPref.setKey(AUTO_AF);
               mAfPref.setTitle(R.string.auto_select_af);
               mAfPref.setSummaryOn(R.string.auto_select_af_enabled);
               mAfPref.setSummaryOff(R.string.auto_select_af_disabled);
               boolean bAFAutoSwitch = FmSharedPreferences.getAutoAFSwitch();
               Log.d(LOGTAG, "createPreferenceHierarchy: bAFAutoSwitch: "
                              + bAFAutoSwitch);
               mAfPref.setChecked(bAFAutoSwitch);
               root.addPreference(mAfPref);

             if(FMRadio.RECORDING_ENABLE) {
                  summaryRecordItems = getResources().getStringArray(
                   R.array.record_durations_entries);
                int nRecordDuration = 0;
                mRecordDurPreference = new ListPreference(this);
                mRecordDurPreference.setEntries(R.array.record_durations_entries);
                mRecordDurPreference.setEntryValues(R.array.record_duration_values);
                mRecordDurPreference.setDialogTitle(R.string.sel_rec_dur);
                mRecordDurPreference.setKey(RECORD_DURATION_KEY);
                mRecordDurPreference.setTitle(R.string.record_dur);
                nRecordDuration = FmSharedPreferences.getRecordDuration();
                Log.d(LOGTAG, "createPreferenceHierarchy: recordDuration: "
                        + nRecordDuration);
                switch(nRecordDuration) {
                case FmSharedPreferences.RECORD_DUR_INDEX_0_VAL:
                     index = 0;
                     break;
                case FmSharedPreferences.RECORD_DUR_INDEX_1_VAL:
                     index = 1;
                     break;
                case FmSharedPreferences.RECORD_DUR_INDEX_2_VAL:
                     index = 2;
                     break;
                case FmSharedPreferences.RECORD_DUR_INDEX_3_VAL:
                     index = 3;
                     break;
                }
                // Get the preference and list the value.
                if ((index < 0) || (index >= summaryRecordItems.length)) {
                   index = 0;
                }
                Log.d(LOGTAG, "createPreferenceHierarchy: recordDurationSummary: "
                    + summaryRecordItems[index]);
                mRecordDurPreference.setSummary(summaryRecordItems[index]);
                mRecordDurPreference.setValueIndex(index);
                root.addPreference(mRecordDurPreference);
             }
          }

          mRestoreDefaultPreference = new Preference(this);
          mRestoreDefaultPreference.setTitle(
                                       R.string.settings_revert_defaults_title);
          mRestoreDefaultPreference.setKey(RESTORE_FACTORY_DEFAULT);
          mRestoreDefaultPreference
                                .setSummary(R.string.settings_revert_defaults_summary);
          mRestoreDefaultPreference.setOnPreferenceClickListener(this);
          root.addPreference(mRestoreDefaultPreference);
          return root;
        }

        public void clearStationList() {
          SharedPreferences sp = getSharedPreferences(FMRadio.SCAN_STATION_PREFS_NAME, 0);
          SharedPreferences.Editor editor = sp.edit();
          editor.clear();
          editor.commit();
        }

        public void onSharedPreferenceChanged(SharedPreferences sharedPreferences,
                        String key) {
          int mTunedFreq = 0;
          boolean bStatus = false;
          int noOfChannels = 0;
          int channelSpacing = 0;
          int preIndex;
          int band_width;

          if (key.equals(REGIONAL_BAND_KEY)) {
              int curListIndex = FmSharedPreferences.getCurrentListIndex();
              PresetList curList = FmSharedPreferences.getStationList(curListIndex);
              String valueStr = sharedPreferences.getString(key, "");
              int index = 0;
              if(valueStr != null) {
                  index = mBandPreference.findIndexOfValue(valueStr);
              }
              if((index < 0) || (index >= summaryBandItems.length)) {
                  index = 0;
                  mBandPreference.setValueIndex(0);
              }else if((index + 1) == summaryBandItems.length) {
                   mChannelSpacingPref.setEnabled(true);
              }else {
                   mChannelSpacingPref.setEnabled(false);
              }
              Log.d(LOGTAG, "onSharedPreferenceChanged: Country Change: "
                                                        + index);
              FmSharedPreferences.setCountry(index);
              setBandSummary(index);
              sendSettingsChangedIntent(FM_BAND_CHANGED);
              if (curList != null) {
                  curList.clear();
              }
              clearStationList();
           }else if(key.equals(CHAN_SPACING_KEY)) {
               int curListIndex = FmSharedPreferences.getCurrentListIndex();
               PresetList curList = FmSharedPreferences.getStationList(curListIndex);
               String valStr = mChannelSpacingPref.getValue();
               int index = 0;
               preIndex = FmSharedPreferences.getChSpacing();
               if(valStr != null) {
                  index  = mChannelSpacingPref.findIndexOfValue(valStr);
               }
               if (index == 0)
                   channelSpacing = 50;
               else if (index == 1)
                   channelSpacing = 100;
               else if (index == 2)
                   channelSpacing = 200;
               noOfChannels = 0;
               max_freq = FmSharedPreferences.getUpperLimit();
               min_freq = FmSharedPreferences.getLowerLimit();
               noOfChannels =  (int) (max_freq - min_freq)/channelSpacing;
               if (noOfChannels < 1) {
                   index = 2 - preIndex;
                   mChannelSpacingPref.setValueIndex(index);
                   Toast.makeText(this, getString(R.string.user_defind_band_msg),
                                                            Toast.LENGTH_SHORT).show();
               }
               if ((index < 0) || (index >= chSpacingItems.length)) {
                   index = 0;
                   mChannelSpacingPref.setValueIndex(0);
               }
               mChannelSpacingPref.setSummary(chSpacingItems[index]);
               FmSharedPreferences.setChSpacing(2 - index);
               sendSettingsChangedIntent(FM_CHAN_SPACING_CHANGED);
               if(curList != null) {
                  curList.clear();
               }
               clearStationList();
           }else if(key.equals(USER_DEFINED_BAND_MIN_KEY)) {
               String valStr = mUserBandMinPref.getText();
               double freq = 0;
               try {
                    freq = Double.parseDouble(valStr) * 1000;
               }catch(NumberFormatException e) {
                    e.printStackTrace();
                    return;
               }
               noOfChannels = 0;
               max_freq = FmSharedPreferences.getUpperLimit();
               min_freq = FmSharedPreferences.getLowerLimit();
               band_width = (int) (max_freq - freq);
               noOfChannels =  band_width/FmSharedPreferences.getFrequencyStepSize();
               if((freq > 0) && (freq < max_freq) && (freq >= 76000)
                  && (noOfChannels > 0) && (band_width >= 100)) {
                  FmSharedPreferences.setLowerLimit((int)freq);
                  sendSettingsChangedIntent(FM_BAND_CHANGED);
                  setBandSummary(summaryBandItems.length - 1);
                  clearStationList();
               }else {
                  if ((Locale.getDefault().toString().equals("zh_HK")))
                      Toast.makeText(this,"請輸入有效的頻道範圍76.0-108.0",
                                                            Toast.LENGTH_SHORT).show();
                  else
                      Toast.makeText(this, getString(R.string.user_defind_band_msg),
                                                            Toast.LENGTH_SHORT).show();
               }
           }else if(key.equals(USER_DEFINED_BAND_MAX_KEY)) {
               String valStr = mUserBandMaxPref.getText();
               double freq = 0;
               try {
                    freq = Double.parseDouble(valStr) * 1000;
               }catch(NumberFormatException e) {
                    e.printStackTrace();
                    return;
               }
               noOfChannels = 0;
               min_freq = FmSharedPreferences.getLowerLimit();
               max_freq = FmSharedPreferences.getUpperLimit();
               band_width = (int) (freq - min_freq);
               noOfChannels = band_width/FmSharedPreferences.getFrequencyStepSize();
               if((freq > 0) && (freq > min_freq) && (freq <= 108000)
                  && (noOfChannels > 0) && (band_width >= 100)) {
                  FmSharedPreferences.setUpperLimit((int)freq);
                  sendSettingsChangedIntent(FM_BAND_CHANGED);
                  setBandSummary(summaryBandItems.length - 1);
                  clearStationList();
               }else {
                  if ((Locale.getDefault().toString().equals("zh_HK")))
                      Toast.makeText(this,"請輸入有效的頻道範圍76.0-108.0",
                                                            Toast.LENGTH_SHORT).show();
                  else
                      Toast.makeText(this, getString(R.string.user_defind_band_msg),
                                                            Toast.LENGTH_SHORT).show();
               }
          }else {
              if(mRxMode) {
                 if (key.equals(AUTO_AF)) {
                     boolean bAFAutoSwitch = mAfPref.isChecked();
                     Log.d(LOGTAG, "onSharedPreferenceChanged: Auto AF Enable: "
                                               + bAFAutoSwitch);
                     FmSharedPreferences.setAutoAFSwitch(bAFAutoSwitch);
                     mPrefs.Save();
                     sendSettingsChangedIntent(FM_AF_OPTION_CHANGED);
                 }else if(key.equals(RECORD_DURATION_KEY)) {
                     if(FMRadio.RECORDING_ENABLE) {
                        String valueStr = mRecordDurPreference.getValue();
                        int index = 0;
                        if (valueStr != null) {
                            index = mRecordDurPreference.findIndexOfValue(valueStr);
                        }
                          if ((index < 0) || (index >= summaryRecordItems.length)) {
                             index = 0;
                             mRecordDurPreference.setValueIndex(index);
                        }
                        Log.d(LOGTAG, "onSharedPreferenceChanged: recorddur: "
                                     + summaryRecordItems[index]);
                          mRecordDurPreference.setSummary(summaryRecordItems[index]);
                          FmSharedPreferences.setRecordDuration(index);
                       }
                   } else if (key.equals(AUDIO_OUTPUT_KEY)) {
                       String valueStr = mAudioPreference.getValue();
                       int index = 0;
                       if (valueStr != null) {
                           index = mAudioPreference.findIndexOfValue(valueStr);
                       }
                       if (index != 1) {
                          if (index != 0) {
                              index = 0;
                              /* It shud be 0(Stereo) or 1(Mono) */
                              mAudioPreference.setValueIndex(index);
                          }
                       }
                       Log.d(LOGTAG, "onSharedPreferenceChanged: audiomode: "
                                      + summaryAudioModeItems[index]);
                       mAudioPreference.setSummary(summaryAudioModeItems[index]);
                     if (index == 0) {
                         // Stereo
                         FmSharedPreferences.setAudioOutputMode(true);
                     }else {
                         // Mono
                         FmSharedPreferences.setAudioOutputMode(false);
                     }
                     mPrefs.Save();
                     sendSettingsChangedIntent(FM_AUDIO_MODE_CHANGED);
                 }
              }
          }
          if (mPrefs != null) {
              if(bStatus) {
                 mPrefs.Save();
              }else {
                 mTunedFreq = FmSharedPreferences.getTunedFrequency();
                 if (mTunedFreq > FmSharedPreferences.getUpperLimit() ||
                          mTunedFreq < FmSharedPreferences.getLowerLimit()) {
                     FmSharedPreferences.setTunedFrequency(
                          FmSharedPreferences.getLowerLimit());
                 }
                 mPrefs.Save();
              }
          }
        }

        public boolean onPreferenceClick(Preference preference) {
          boolean handled = false;
          if (preference == mRestoreDefaultPreference) {
             mDlgRestoreFactory = createDlgRestoreFactory();
             mDlgRestoreFactory.show();
          }
          return handled;
        }

        private Dialog createDlgRestoreFactory() {
               return new AlertDialog.Builder(this).setIcon(
                      R.drawable.alert_dialog_icon).setTitle(
                      R.string.settings_revert_confirm_title).setMessage(
                      R.string.settings_revert_confirm_msg).setPositiveButton(
                      R.string.alert_dialog_ok,
                      new DialogInterface.OnClickListener() {
                           public void onClick(DialogInterface dialog,
                              int whichButton) {
                              Intent data = new Intent(
                                         RESTORE_FACTORY_DEFAULT_ACTION);
                              setResult(RESULT_OK, data);
                              restoreSettingsDefault();
                              finish();
                           }
                      }).setNegativeButton(R.string.alert_dialog_cancel,
                              new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog,
                                                       int whichButton) {
                                }
                              }).create();
        }

        private void restoreSettingsDefault() {
          if (mPrefs != null) {
             if (Locale.getDefault().equals(Locale.CHINA)) {
                 mBandPreference
                        .setValueIndex(FmSharedPreferences.REGIONAL_BAND_CHINA);
             }else {
                 mBandPreference
                        .setValueIndex(FmSharedPreferences.REGIONAL_BAND_NORTH_AMERICA);
             }
             if (mRxMode) {
                mAudioPreference.setValueIndex(0);
                if (FMRadio.RECORDING_ENABLE) {
                    mRecordDurPreference.setValueIndex(0);
                }
                mAfPref.setChecked(true);
                FmSharedPreferences.SetDefaults();
             }else {
                if (Locale.getDefault().equals(Locale.CHINA)) {
                    FmSharedPreferences
                    .setCountry(FmSharedPreferences.REGIONAL_BAND_CHINA);
                }else{
                    FmSharedPreferences
                    .setCountry(FmSharedPreferences.REGIONAL_BAND_NORTH_AMERICA);
                }
             }
             mPrefs.Save();
          }
        }

        @Override
        protected void onResume() {
          super.onResume();
          PreferenceScreen preferenceScreen = getPreferenceScreen();
          SharedPreferences sharedPreferences = null;
          if (preferenceScreen != null) {
              sharedPreferences = preferenceScreen.getSharedPreferences();
          }
          if (sharedPreferences != null) {
              sharedPreferences.registerOnSharedPreferenceChangeListener(this);
          }
        }

        @Override
        protected void onPause() {
          super.onPause();
          PreferenceScreen preferenceScreen = getPreferenceScreen();
          SharedPreferences sharedPreferences = null;
          if (preferenceScreen != null) {
              sharedPreferences = preferenceScreen.getSharedPreferences();
          }
          if (sharedPreferences != null) {
              sharedPreferences.unregisterOnSharedPreferenceChangeListener(this);
          }
        }
        private void setBandSummary(int index) {
           if((index + 1) == summaryBandItems.length) {
              min_freq = FmSharedPreferences.getLowerLimit();
              max_freq = FmSharedPreferences.getUpperLimit();
              chan_spacing = FmSharedPreferences.getChSpacing();
              if(chan_spacing < 0) {
                 chan_spacing = 0;
              }
              mBandPreference.setSummary(summaryBandItems[index] + "( "
                       + (min_freq / 1000.0) +"Mhz To " + (max_freq / 1000.0) +
                       "Mhz)");
              mChannelSpacingPref.setValueIndex(2 - chan_spacing);
              mChannelSpacingPref.setSummary(chSpacingItems[2 - chan_spacing]);
              mChannelSpacingPref.setEnabled(true);
              mUserBandMinPref.setEnabled(true);
              mUserBandMaxPref.setEnabled(true);
              mUserBandMinPref.setSummary((min_freq / 1000.0) + "Mhz");
              mUserBandMaxPref.setSummary((max_freq / 1000.0) + "Mhz");
           }else {
              mBandPreference.setSummary(summaryBandItems[index]);
              mChannelSpacingPref.setEnabled(false);
              mUserBandMinPref.setEnabled(false);
              mUserBandMaxPref.setEnabled(false);
           }
        }
        private void displayToast(String msg) {
           Toast.makeText(getApplicationContext(), msg, Toast.LENGTH_LONG).show();
        }

        private void sendSettingsChangedIntent(int action) {
           Intent intent = new Intent(ACTION_FM_SETTING);
           intent.putExtra("state", action);
           Log.d(LOGTAG, "Sending  FM SETTING Change intent for = " + action);
           getApplicationContext().sendBroadcast(intent);
        }
}
