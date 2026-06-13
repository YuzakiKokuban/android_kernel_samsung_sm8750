/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <algorithm>
#include <cstddef>
#include <memory>
#define LOG_TAG "AHAL_Effect_VolumeListenerQti"
#include <cutils/properties.h>
#include <unordered_set>

#include "GlobalVolumeListenerSession.h"

namespace aidl::qti::effects {

GlobalConfigs::GlobalConfigs() {
    initGainMappings();
    mHeadsetCalEnabled = property_get_bool("vendor.audio.volume.headset.gain.depcal", false);
}

void GlobalConfigs::initGainMappings() {
    size_t payloadSize = 0;
    pal_param_gain_lvl_map_t gainLevelMap;
    gainLevelMap.mapping_tbl = mGainMappingTable;
    gainLevelMap.table_size = MAX_VOLUME_CAL_STEPS;
    gainLevelMap.filled_size = 0;

    printVolumeTable();

    int ret =
            pal_get_param(PAL_PARAM_ID_GAIN_LVL_MAP, (void **)&gainLevelMap, &payloadSize, nullptr);

    if (ret != 0) {
        LOG(ERROR) << "fail to get PAL_PARAM_ID_GAIN_LVL_MAP " << ret;
        gainLevelMap.filled_size = 0;
    }

    int maxTableEntries = gainLevelMap.filled_size;

    if (maxTableEntries > 0 && maxTableEntries <= MAX_VOLUME_CAL_STEPS) {
        if (maxTableEntries < mTotalVolumeCalSteps) {
            for (int i = maxTableEntries; i < mTotalVolumeCalSteps; i++) {
                mGainMappingTable[i].amp = 0;
                mGainMappingTable[i].db = 0;
                mGainMappingTable[i].level = 0;
            }
        }
        mTotalVolumeCalSteps = maxTableEntries;
        LOG(DEBUG) << "Using custom volume table";
    } else {
        LOG(DEBUG) << "Using default volume table";
    }
    printVolumeTable();
}

void GlobalConfigs::printVolumeTable() {
    for (int i = 0; i < mTotalVolumeCalSteps; i++) {
        LOG(VERBOSE) << "Index: " << i << " (amp, db, level)" << mGainMappingTable[i].amp << " "
                     << mGainMappingTable[i].db << " " << mGainMappingTable[i].level;
    }
}

GlobalVolumeListenerSession::GlobalVolumeListenerSession() {
    LOG(VERBOSE) << __func__ << "Global Session created";
    mTotalVolumeSteps = mConfig.getVolumeCalSteps();
    mGainTable = mConfig.getGainTable();
}

std::shared_ptr<VolumeListenerContext> GlobalVolumeListenerSession::createSession(
        const VolumeListenerType &type, const Parameter::Common &common, bool processData) {
    int sessionId = common.session;
    LOG(VERBOSE) << __func__ << type << " with sessionId " << sessionId;
    std::lock_guard lg(mSessionMutex);

    auto context = std::make_shared<VolumeListenerContext>(common, type, processData);
    RETURN_VALUE_IF(!context, nullptr, "failedToCreateContext");

    mSessionsMap[sessionId] = context;
    return context;
}

void GlobalVolumeListenerSession::releaseSession(int sessionId) {
    LOG(VERBOSE) << __func__ << "Enter: sessionId " << sessionId << "total sessions "
               << mSessionsMap.size();
    std::lock_guard lg(mSessionMutex);

    if (mSessionsMap.find(sessionId) != mSessionsMap.end()) {
        mSessionsMap.erase(sessionId);
    }
    // recalculate the checkAndSetGainDepCal once any sesesion closes.
    checkAndSetGainDepCal_l();
    LOG(VERBOSE) << __func__ << "Exit: sessionId " << sessionId << "total sessions "
                 << mSessionsMap.size();
}

RetCode GlobalVolumeListenerSession::setOutputDevice(int sessionId,
                                                     const AudioDeviceDescriptionVector &devices) {
    LOG(VERBOSE) << __func__ << " sessionId " << sessionId;
    std::lock_guard lg(mSessionMutex);
    if (mSessionsMap.find(sessionId) != mSessionsMap.end()) {
        auto &context = mSessionsMap[sessionId];
        context->setOutputDevice(devices);
        checkAndSetGainDepCal_l();
    }
    return RetCode::SUCCESS;
}

RetCode GlobalVolumeListenerSession::setVolumeStereo(int sessionId,
                                                     const Parameter::VolumeStereo &volumeStereo) {
    LOG(VERBOSE) << __func__ << " sessionId " << sessionId;
    std::lock_guard lg(mSessionMutex);
    if (mSessionsMap.find(sessionId) != mSessionsMap.end()) {
        auto &context = mSessionsMap[sessionId];
        context->setVolumeStereo(volumeStereo);
        checkAndSetGainDepCal_l();
    }
    return RetCode::SUCCESS;
}

RetCode GlobalVolumeListenerSession::enable(int sessionId) {
    LOG(VERBOSE) << __func__ << " sessionId " << sessionId;
    std::lock_guard lg(mSessionMutex);
    RetCode status = RetCode::SUCCESS;
    if (mSessionsMap.find(sessionId) != mSessionsMap.end()) {
        auto &context = mSessionsMap[sessionId];
        status = context->enable();
        checkAndSetGainDepCal_l();
    }
    return status;
}

RetCode GlobalVolumeListenerSession::disable(int sessionId) {
    LOG(VERBOSE) << __func__ << " sessionId " << sessionId;
    std::lock_guard lg(mSessionMutex);
    RetCode status = RetCode::SUCCESS;
    if (mSessionsMap.find(sessionId) != mSessionsMap.end()) {
        auto &context = mSessionsMap[sessionId];
        status = context->disable();
        checkAndSetGainDepCal_l();
    }
    return status;
}

void GlobalVolumeListenerSession::checkAndSetGainDepCal_l() {
    // iterate through list and make decision to set new gain dep cal level for speaker device
    // 1. find all usecase active on speaker
    // 2. find energy sum for each usecase
    // 3. find the highest of all the active usecase
    // 4. if new value is different than the current value then load new calibration

    float newVolume = -1.0f;
    auto sumEnergy = getSumEnergy_l();

    if (sumEnergy != 0.0f) {
        newVolume = fmin(sqrt(sumEnergy), 1.0);
    }

    uint32_t gain = (uint32_t)(round(newVolume * (1 << LIN_VOLUME_QFACTOR_28)));
    LOG(DEBUG) << __func__ << " use volume " << ::android::internal::ToString(newVolume) << " gain "
               << gain;
    sendLinearGain(gain);
    applyUpdatedCalibration(newVolume);
}

void GlobalVolumeListenerSession::applyUpdatedCalibration(float newVolume) {
    if (newVolume != mCurrentVolume) {
        // send Gain dep cal level
        int gainDepCalLevel = -1;
        if (newVolume >= 1 && mTotalVolumeSteps > 0) { // max amplitude, use highest DRC level
            gainDepCalLevel = mGainTable[mTotalVolumeSteps - 1].level;
        } else if (newVolume == -1) {
            gainDepCalLevel = DEFAULT_CAL_STEP;
        } else if (newVolume == 0) {
            gainDepCalLevel = mGainTable[0].level;
        } else {
            for (int index = 0; index < mTotalVolumeSteps - 1; index++) {
                if (newVolume < mGainTable[index + 1].amp && newVolume >= mGainTable[index].amp) {
                    gainDepCalLevel = mGainTable[index].level;
                    LOG(VERBOSE) << __func__ << " found " << newVolume << " at " << index
                                 << " gain " << gainDepCalLevel;
                    break;
                }
            }
        }

        // check here if previous gain dep cal level was not same
        if (gainDepCalLevel != mCurrentGainDepCalLevel) {
            // decision made .. send new level now
            if (!sendGainDepCalibration(gainDepCalLevel)) {
                LOG(ERROR) << __func__ << " Failed to set gain dep cal level";
            } else {
                // Success in setting the gain dep cal level, store new level and Volume
                mCurrentGainDepCalLevel = gainDepCalLevel;
                mCurrentVolume = newVolume;
            }
        }
    } else {
        LOG(VERBOSE) << "volume unchanged";
    }
}

float GlobalVolumeListenerSession::getSumEnergy_l() {
    // iterate through list and make decision to set new gain dep cal level for speaker device
    // 1. find all usecase active on speaker
    // 2. find energy sum for each usecase
    // 3. find the highest of all the active usecase
    float sumEnergy = 0.0f, tempVolume;
    for (auto &sessionEntry : mSessionsMap) {
        auto &session = sessionEntry.second;
        if (session->isEffectActiveAndApplicable()) {
            tempVolume = session->getMaxOfLeftRightChannels();
            sumEnergy += tempVolume * tempVolume;
            LOG(VERBOSE) << __func__ << " size " << mSessionsMap.size() << " sum energy "
                         << sumEnergy;
        }
    }
    LOG(VERBOSE) << __func__ << " size " << mSessionsMap.size() << " sum energy " << sumEnergy;
    return sumEnergy;
}

bool GlobalVolumeListenerSession::sendGainDepCalibration(int level) {
    int32_t ret = 0;
    pal_param_gain_lvl_cal_t gainLevelCal;
    gainLevelCal.level = level;
    LOG(VERBOSE) << __func__ << " level " << level;
    ret = pal_set_param(PAL_PARAM_ID_GAIN_LVL_CAL, (void *)&gainLevelCal,
                        sizeof(pal_param_gain_lvl_cal_t));
    if (ret != 0) {
        LOG(ERROR) << "fail to set PAL_PARAM_ID_GAIN_LVL_CAL " << ret;
    }

    return (ret == 0);
}

bool GlobalVolumeListenerSession::sendLinearGain(int32_t gain) {
    int32_t ret = 0;
    pal_param_mspp_linear_gain_t linearGain;
    linearGain.gain = gain;
    LOG(VERBOSE) << __func__ << " gain " << gain;
    ret = pal_set_param(PAL_PARAM_ID_MSPP_LINEAR_GAIN, (void *)&linearGain,
                        sizeof(pal_param_mspp_linear_gain_t));
    if (ret != 0) {
        LOG(ERROR) << "fail to set PAL_PARAM_ID_MSPP_LINEAR_GAIN " << ret;
    }

    return (ret == 0);
}

} // namespace aidl::qti::effects
