/*
 * Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include <stdlib.h>
#include <memory>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <string>
#include <iostream>
#include <string.h>
#include "Stream.h"

#ifndef PALRINGBUFFER_H_
#define PALRINGBUFFER_H_

#define DEFAULT_PAL_RING_BUFFER_SIZE 4096 * 10

typedef enum {
    READER_DISABLED = 0,
    READER_ENABLED = 1,
    READER_PREPARED = 2,
} pal_ring_buffer_reader_state;

/*
 * startIdx/endIdx: index compared to beginning of the detection
 * ftrtSize: linear increased during buffering, used to confirm
 * if keyword data is read from adsp
 */
struct kwdConfig {
    uint32_t startIdx;
    uint32_t endIdx;
    uint32_t ftrtSize;
};

class PalRingBuffer;

class PalRingBufferReader {
 public:
     PalRingBufferReader(PalRingBuffer *buffer)
         : ringBuffer_(buffer),
           unreadSize_(0),
           readOffset_(0),
           requestedSize_(0),
           state_(READER_DISABLED) {}

    ~PalRingBufferReader() {};

    size_t advanceReadOffset(size_t advanceSize);
    int32_t read(void* readBuffer, size_t readSize);
    void updateState(pal_ring_buffer_reader_state state);
    void getIndices(Stream *s,
        uint32_t *startIdx, uint32_t *endIdx, uint32_t *ftrtSize);
    int32_t getKwData(Stream *s, uint8_t *data, uint32_t size);
    size_t getUnreadSize();
    size_t getBufferSize();
    void reset();
    bool isEnabled() { return state_ == READER_ENABLED; }
    bool isPrepared() { return state_ == READER_PREPARED; }
    bool waitForBuffers(uint32_t buffer_size);

    friend class PalRingBuffer;

 protected:
    PalRingBuffer *ringBuffer_;
    size_t unreadSize_;
    size_t readOffset_;
    pal_ring_buffer_reader_state state_;
    std::mutex mutex_;
    std::condition_variable cv_;
    uint32_t requestedSize_;
};

class PalRingBuffer {
 public:
    explicit PalRingBuffer(size_t bufferSize)
        : buffer_((char*)(new char[bufferSize])),
          writeOffset_(0),
          bufferEnd_(bufferSize) {}

    ~PalRingBuffer() {
        if (buffer_)
            delete buffer_;

        for (int i = 0; i < readers_.size(); i++)
            delete readers_[i];
    }

    PalRingBufferReader* newReader();

    int32_t removeReader(PalRingBufferReader *reader);
    size_t read(std::shared_ptr<PalRingBufferReader>reader, void* readBuffer,
                size_t readSize);
    size_t write(void* writeBuffer, size_t writeSize);
    size_t getFreeSize();
    void updateKwdConfig(Stream *s, uint32_t startIdx, uint32_t endIdx,
                         uint32_t preRoll);
    void getIndices(Stream *s,
        uint32_t *startIdx, uint32_t *endIdx, uint32_t *ftrtSize);
    void reset();
    size_t getBufferSize() { return bufferEnd_; };
    void resizeRingBuffer(size_t bufferSize);

 protected:
    std::mutex mutex_;
    char* buffer_;
    std::unordered_map<Stream*, struct kwdConfig> kwCfg_;
    size_t writeOffset_;
    size_t bufferEnd_;
    std::vector<PalRingBufferReader*> readers_;
    void updateUnReadSize(size_t writtenSize);
    friend class PalRingBufferReader;
};
#endif
