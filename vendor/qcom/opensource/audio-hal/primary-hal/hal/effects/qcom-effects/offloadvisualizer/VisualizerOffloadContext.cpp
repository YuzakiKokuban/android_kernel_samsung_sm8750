/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#define LOG_TAG "AHAL_Effect_VisualizerQti"
#include "VisualizerOffloadContext.h"
#include <Utils.h>
#include <android/binder_status.h>
#include <audio_utils/primitives.h>
#include <math.h>
#include <system/audio.h>
#include <time.h>
#include <algorithm>

using ::aidl::android::hardware::audio::common::getChannelCount;

namespace aidl::qti::effects {
void GlobalVisualizerSession::startEffect(int ioHandle) {
    std::lock_guard lg(mMutex);
    for (auto context : mCreatedEffectsList) {
        if (context->getIoHandle() == ioHandle) {
            if (auto ret = context->startThreadLoop(); ret)
                LOG(ERROR) << __func__ << " failed to start capture thread loop" << ret;
            break;
        }
    }
    mActiveOutputsList.push_back(ioHandle);
}

void GlobalVisualizerSession::stopEffect(int ioHandle) {
    std::lock_guard lg(mMutex);
    for (auto context : mCreatedEffectsList) {
        if (context->getIoHandle() == ioHandle) {
            if (auto ret = context->stopThreadLoop(); ret)
                LOG(ERROR) << __func__ << " failed to stop capture thread loop";
            break;
        }
    }

    auto iter = std::find(mActiveOutputsList.begin(), mActiveOutputsList.end(), ioHandle);
    if (iter != mActiveOutputsList.end()) mActiveOutputsList.erase(iter);
}

std::shared_ptr<VisualizerOffloadContext> GlobalVisualizerSession::createSession(
        const Parameter::Common& common, bool processData) {
    std::lock_guard lg(mMutex);
    auto context = std::make_shared<VisualizerOffloadContext>(common, processData);
    RETURN_VALUE_IF(!context, nullptr, "failedToCreateContext");
    for (auto output : mActiveOutputsList) {
        if (common.ioHandle == output) {
            if (auto ret = context->startThreadLoop(); ret)
                LOG(ERROR) << __func__ << " failed to start capture thread loop";
            break;
        }
    }
    mCreatedEffectsList.push_back(context);
    return context;
}

void GlobalVisualizerSession::releaseSession(std::shared_ptr<VisualizerOffloadContext> context) {
    std::lock_guard lg(mMutex);
    if (context) {
        context->disable();
        context->resetBuffer();
        for (auto output : mActiveOutputsList) {
            if (context->getIoHandle() == output) {
                if (auto ret = context->stopThreadLoop(); ret)
                    LOG(ERROR) << __func__ << " failed to stop capture thread loop";
                break;
            }
        }
    }
    auto iter = std::find(mCreatedEffectsList.begin(), mCreatedEffectsList.end(), context);
    if (iter != mCreatedEffectsList.end())
        mCreatedEffectsList.erase(iter);
    else
        LOG(ERROR) << __func__ << " context is not present";
}
StreamProxy::StreamProxy() {
    init();
}
StreamProxy::~StreamProxy() {
    cleanUp();
}
void StreamProxy::init() {
    memset(&data, 0, sizeof(data));

    chInfo.channels = AUDIO_CAPTURE_CHANNEL_COUNT;
    chInfo.ch_map[0] = PAL_CHMAP_CHANNEL_FL;
    chInfo.ch_map[1] = PAL_CHMAP_CHANNEL_FR;
    streamAttr.type = PAL_STREAM_PROXY;
    streamAttr.direction = PAL_AUDIO_INPUT;
    streamAttr.in_media_config.sample_rate = AUDIO_CAPTURE_SMP_RATE;
    streamAttr.in_media_config.bit_width = AUDIO_CAPTURE_BIT_WIDTH;
    streamAttr.in_media_config.ch_info = chInfo;
    streamAttr.in_media_config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S16_LE;
    devices.id = PAL_DEVICE_IN_PROXY;
    devices.config.sample_rate = AUDIO_CAPTURE_SMP_RATE;
    devices.config.bit_width = AUDIO_CAPTURE_BIT_WIDTH;
    devices.config.ch_info = chInfo;
    devices.config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S16_LE;

    if (openStream()) {
        if (!startStream()) cleanUp();
    }
}

bool StreamProxy::openStream() {
    if (auto ret = pal_stream_open(&streamAttr, noOfDevices, &devices, 0, NULL, NULL, 0,
                                   &inStreamHandle);
        ret) {
        LOG(ERROR) << __func__ << "pal_stream_open failed" << ret;
        return false;
    }
    mStreamOpened = true;
    return true;
}

bool StreamProxy::startStream() {
    inBufferCfg.buf_size = inBufferSize;
    inBufferCfg.buf_count = inBuffCount;

    if (auto ret = pal_stream_set_buffer_size(inStreamHandle, &inBufferCfg, NULL); ret) {
        LOG(ERROR) << __func__ << "pal_stream_set_buffer_size failed with err" << ret;
        return false;
    }

    inBufferSize = inBufferCfg.buf_size;
    if (auto ret = pal_stream_start(inStreamHandle); ret) {
        LOG(ERROR) << __func__ << "pal_stream_start failed with err" << ret;
        return false;
    }
    mStreamStarted = true;
    return true;
}

void StreamProxy::cleanUp() {
    if (mStreamStarted) {
        if (auto ret = pal_stream_stop(inStreamHandle); ret)
            LOG(ERROR) << __func__ << "pal_stream_stop failed with err" << ret;
    }
    mStreamStarted = false;
    if (mStreamOpened) {
        if (auto ret = pal_stream_close(inStreamHandle); ret)
            LOG(ERROR) << __func__ << "pal_stream_close failed with err" << ret;
    }
    mStreamOpened = false;
}

int16_t* StreamProxy::read() {
    int readStatus = 0;
    memset(&inBuffer, 0, sizeof(struct pal_buffer));
    inBuffer.buffer = (uint8_t*)&data[0];
    inBuffer.size = inBufferSize;
    readStatus = pal_stream_read(inStreamHandle, &inBuffer);
    if (readStatus > 0) {
        LOG(VERBOSE) << __func__ << " pal_stream_read success no_of_bytes_read =" << readStatus;
        return data;
    }
    LOG(ERROR) << __func__ << "pal_stream_read failed with read status " << readStatus;
    return nullptr;
}

bool StreamProxy::isStreamStarted() {
    return mStreamStarted;
}

int VisualizerOffloadContext::startThreadLoop() {
    std::lock_guard lg(mMutex);
    mCaptureThreadHandler = std::thread(&VisualizerOffloadContext::captureThreadLoop, this);
    if (!mCaptureThreadHandler.joinable()) {
        LOG(ERROR) << __func__ << "fail to create captureThreadLoop";
        return -EINVAL;
    }
    mExitThread = false;
    mCaptureThreadCondition.notify_one();
    return 0;
}

int VisualizerOffloadContext::stopThreadLoop() {
    if (mCaptureThreadHandler.joinable()) {
        {
            std::lock_guard lg(mMutex);
            mExitThread = true;
        }
        mCaptureThreadCondition.notify_one();
        mCaptureThreadHandler.join();
        LOG(DEBUG) << __func__ << " capture thread joined";
    }
    return 0;
}

void VisualizerOffloadContext::captureThreadLoop() {
    int status = 0;
    bool captureEnabled = false;
    StreamProxy streamProxy;

    if (!streamProxy.isStreamStarted()) return;
    LOG(INFO) << __func__ << "Capture Thread Enter ";

    while (true) {
        {
            std::unique_lock<std::mutex> lck(mMutex);
            LOG(VERBOSE) << __func__ << " waiting for active state";
            mCaptureThreadCondition.wait(lck,
                                         [this] { return mState == State::ACTIVE || mExitThread; });
            LOG(VERBOSE) << __func__ << " done waiting for active state";

            if (mExitThread) break;
        }
        process(streamProxy.read());
    }

    LOG(DEBUG) << __func__ << " Capture Thread Exit ";
}

VisualizerOffloadContext::VisualizerOffloadContext(
        const aidl::android::hardware::audio::effect::Parameter::Common& common, bool processData)
    : EffectContext(common, processData) {
    std::lock_guard lg(mMutex);
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle();
    if (common.input != common.output) {
        LOG(ERROR) << __func__ << " mismatch input: " << common.input.toString()
                   << " and output: " << common.output.toString();
    }
    mState = State::INITIALIZED;
    auto channelCount = getChannelCount(common.input.base.channelMask);
    mChannelCount = channelCount;
}

VisualizerOffloadContext::~VisualizerOffloadContext() {
    std::lock_guard lg(mMutex);
    LOG(DEBUG) << __func__;
    mState = State::UNINITIALIZED;
}

RetCode VisualizerOffloadContext::enable() {
    std::lock_guard lg(mMutex);
    if (mState != State::INITIALIZED) {
        return RetCode::ERROR_EFFECT_LIB_ERROR;
    }
    mState = State::ACTIVE;
    mCaptureThreadCondition.notify_one();
    return RetCode::SUCCESS;
}

RetCode VisualizerOffloadContext::disable() {
    std::lock_guard lg(mMutex);
    if (mState != State::ACTIVE) {
        return RetCode::ERROR_EFFECT_LIB_ERROR;
    }
    mState = State::INITIALIZED;
    mCaptureThreadCondition.notify_one();
    return RetCode::SUCCESS;
}

void VisualizerOffloadContext::reset() {
    std::lock_guard lg(mMutex);
    std::fill_n(mCaptureBuf.begin(), kMaxCaptureBufSize, 0x80);
}

RetCode VisualizerOffloadContext::setCaptureSamples(int samples) {
    std::lock_guard lg(mMutex);
    mCaptureSamples = samples;
    return RetCode::SUCCESS;
}

int VisualizerOffloadContext::getCaptureSamples() {
    std::lock_guard lg(mMutex);
    return mCaptureSamples;
}

RetCode VisualizerOffloadContext::setMeasurementMode(
        aidl::android::hardware::audio::effect::Visualizer::MeasurementMode mode) {
    std::lock_guard lg(mMutex);
    mMeasurementMode = mode;
    return RetCode::SUCCESS;
}

Visualizer::MeasurementMode VisualizerOffloadContext::getMeasurementMode() {
    std::lock_guard lg(mMutex);
    return mMeasurementMode;
}

RetCode VisualizerOffloadContext::setScalingMode(Visualizer::ScalingMode mode) {
    std::lock_guard lg(mMutex);
    mScalingMode = mode;
    return RetCode::SUCCESS;
}

Visualizer::ScalingMode VisualizerOffloadContext::getScalingMode() {
    std::lock_guard lg(mMutex);
    return mScalingMode;
}

RetCode VisualizerOffloadContext::setDownstreamLatency(int latency) {
    std::lock_guard lg(mMutex);
    mDownstreamLatency = latency;
    return RetCode::SUCCESS;
}

int VisualizerOffloadContext::getDownstreamLatency() {
    std::lock_guard lg(mMutex);
    return mDownstreamLatency;
}

uint32_t VisualizerOffloadContext::getDeltaTimeMsFromUpdatedTime_l() {
    uint32_t deltaMs = 0;
    if (mBufferUpdateTime.tv_sec != 0) {
        struct timespec ts;
        if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
            time_t secs = ts.tv_sec - mBufferUpdateTime.tv_sec;
            long nsec = ts.tv_nsec - mBufferUpdateTime.tv_nsec;
            if (nsec < 0) {
                --secs;
                nsec += 1000000000;
            }
            deltaMs = secs * 1000 + nsec / 1000000;
        }
    }
    return deltaMs;
}

Visualizer::Measurement VisualizerOffloadContext::getMeasure() {
    uint16_t peakU16 = 0;
    float sumRmsSquared = 0.0f;
    uint8_t nbValidMeasurements = 0;
    {
        std::lock_guard lg(mMutex);
        // reset measurements if last measurement was too long ago (which implies stored
        // measurements aren't relevant anymore and shouldn't bias the new one)
        const uint32_t delayMs = getDeltaTimeMsFromUpdatedTime_l();
        if (delayMs > kDiscardMeasurementsTimeMs) {
            LOG(INFO) << __func__ << " Discarding " << delayMs << " ms old measurements";
            for (uint32_t i = 0; i < mMeasurementWindowSizeInBuffers; i++) {
                mPastMeasurements[i].mIsValid = false;
                mPastMeasurements[i].mPeakU16 = 0;
                mPastMeasurements[i].mRmsSquared = 0;
            }
            mMeasurementBufferIdx = 0;
        } else {
            // only use actual measurements, otherwise the first RMS measure happening before
            // MEASUREMENT_WINDOW_MAX_SIZE_IN_BUFFERS have been played will always be artificially
            // low
            for (uint32_t i = 0; i < mMeasurementWindowSizeInBuffers; i++) {
                if (mPastMeasurements[i].mIsValid) {
                    if (mPastMeasurements[i].mPeakU16 > peakU16) {
                        peakU16 = mPastMeasurements[i].mPeakU16;
                    }
                    sumRmsSquared += mPastMeasurements[i].mRmsSquared;
                    nbValidMeasurements++;
                }
            }
        }
    }
    float rms = nbValidMeasurements == 0 ? 0.0f : sqrtf(sumRmsSquared / nbValidMeasurements);
    Visualizer::Measurement measure;
    // convert from I16 sample values to mB and write results
    measure.rms = (rms < 0.000016f) ? -9600 : (int32_t)(2000 * log10(rms / 32767.0f));
    measure.peak = (peakU16 == 0) ? -9600 : (int32_t)(2000 * log10(peakU16 / 32767.0f));
    LOG(DEBUG) << __func__ << " peak " << peakU16 << " (" << measure.peak << "mB), rms " << rms
               << " (" << measure.rms << "mB)";
    return measure;
}

std::vector<uint8_t> VisualizerOffloadContext::capture() {
    std::vector<uint8_t> result;
    std::lock_guard lg(mMutex);
    if (mState != State::ACTIVE) {
        result.resize(mCaptureSamples);
        memset(result.data(), 0x80, mCaptureSamples);
        return result;
    }
    int32_t latencyMs = mDownstreamLatency;
    const int32_t deltaMs = getDeltaTimeMsFromUpdatedTime_l();
    // if audio framework has stopped playing audio although the effect is still active we must
    // clear the capture buffer to return silence
    if ((mLastCaptureIdx == mCaptureIdx) && (mBufferUpdateTime.tv_sec != 0) &&
        (deltaMs > kMaxStallTimeMs)) {
        LOG(DEBUG) << __func__ << " capture going to idle";
        mBufferUpdateTime.tv_sec = 0;
        return result;
    }
    __builtin_sub_overflow((int32_t)latencyMs, deltaMs, &latencyMs);
    if (latencyMs < 0) latencyMs = 0;
    uint32_t deltaSamples = mCommon.input.base.sampleRate * latencyMs / 1000;
    int64_t capturePoint = mCaptureIdx;
    capturePoint -= mCaptureSamples;
    capturePoint -= deltaSamples;
    int64_t captureSize = mCaptureSamples;
    if (capturePoint < 0) {
        int64_t size = -capturePoint;
        if (size > captureSize) {
            size = captureSize;
        }
        result.insert(result.end(), &mCaptureBuf[kMaxCaptureBufSize + capturePoint],
                      &mCaptureBuf[kMaxCaptureBufSize + capturePoint + size]);
        captureSize -= size;
        capturePoint = 0;
    }
    result.insert(result.end(), &mCaptureBuf[capturePoint],
                  &mCaptureBuf[capturePoint + captureSize]);
    mLastCaptureIdx = mCaptureIdx;
    return result;
}

int VisualizerOffloadContext::process(int16_t* inBuffer) {
    if (!inBuffer || mState != State::ACTIVE) return -EINVAL;

    // perform measurements if needed
    if (mMeasurementMode == Visualizer::MeasurementMode::PEAK_RMS) {
        // find the peak and RMS squared for the new buffer
        float rmsSqAcc = 0;
        int16_t maxSample = 0;
        for (size_t inIdx = 0; inIdx < AUDIO_CAPTURE_PERIOD_SIZE * mChannelCount; ++inIdx) {
            if (inBuffer[inIdx] > maxSample) {
                maxSample = inBuffer[inIdx];
            } else if (-inBuffer[inIdx] > maxSample) {
                maxSample = -inBuffer[inIdx];
            }
            rmsSqAcc += inBuffer[inIdx] * inBuffer[inIdx];
        }
        mPastMeasurements[mMeasurementBufferIdx] = {
                .mPeakU16 = (uint16_t)maxSample,
                .mRmsSquared = rmsSqAcc / (AUDIO_CAPTURE_PERIOD_SIZE * mChannelCount),
                .mIsValid = true};
        if (++mMeasurementBufferIdx >= mMeasurementWindowSizeInBuffers) {
            mMeasurementBufferIdx = 0;
        }
    }
    /* all code below assumes stereo 16 bit PCM output and input */
    int32_t shift;
    if (mScalingMode == Visualizer::ScalingMode::NORMALIZED) {
        /* derive capture scaling factor from peak value in current buffer
         * this gives more interesting captures for display. */
        shift = 32;
        int len = AUDIO_CAPTURE_PERIOD_SIZE * 2;
        for (int idx = 0; idx < len; idx++) {
            int32_t smp = inBuffer[idx];
            if (smp < 0) smp = -smp - 1; /* take care to keep the max negative in range */
            int32_t clz = __builtin_clz(smp);
            if (shift > clz) shift = clz;
        }
        /* A maximum amplitude signal will have 17 leading zeros, which we want to
         * translate to a shift of 8 (for converting 16 bit to 8 bit) */
        shift = 25 - shift;
        /* Never scale by less than 8 to avoid returning unaltered PCM signal. */
        if (shift < 3) {
            shift = 3;
        }
        /* add one to combine the division by 2 needed after summing
         * left and right channels below */
        shift++;
    } else {
        assert(mScalingMode == Visualizer::ScalingMode::AS_PLAYED);
        // Note: if channels are uncorrelated, 1/sqrt(N) could be used at the risk of clipping.
        shift = 9;
    }
    uint32_t captIdx;
    uint32_t inIdx;
    for (inIdx = 0, captIdx = mCaptureIdx; inIdx < AUDIO_CAPTURE_PERIOD_SIZE; inIdx++, captIdx++) {
        // wrap
        if (captIdx >= kMaxCaptureBufSize) {
            captIdx = 0;
        }
        int32_t smp = inBuffer[2 * inIdx] + inBuffer[2 * inIdx + 1];
        smp = smp >> shift;
        mCaptureBuf[captIdx] = ((uint8_t)smp) ^ 0x80;
    }
    // the following two should really be atomic, though it probably doesn't
    // matter much for visualization purposes
    mCaptureIdx = captIdx;
    // update last buffer update time stamp
    if (clock_gettime(CLOCK_MONOTONIC, &mBufferUpdateTime) < 0) {
        mBufferUpdateTime.tv_sec = 0;
    }
    if (mState != State::ACTIVE) {
        LOG(DEBUG) << __func__ << "DONE inactive";
        return -ENODATA;
    }
    return 0;
}
} // namespace aidl::qti::effects
