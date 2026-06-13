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

 * Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "PAL: PalRingBuffer"

#ifdef LINUX_ENABLED
#include <algorithm>
#endif
#include "PalRingBuffer.h"
#include "PalCommon.h"
#include "StreamSoundTrigger.h"

int32_t PalRingBuffer::removeReader(PalRingBufferReader *reader)
{
    auto iter = std::find(readers_.begin(), readers_.end(), reader);
    if (iter != readers_.end())
        readers_.erase(iter);

    return 0;
}

size_t PalRingBuffer::read(std::shared_ptr<PalRingBufferReader>reader __unused,
                           void* readBuffer __unused, size_t readSize __unused)
{
    return 0;
}

size_t PalRingBuffer::getFreeSize()
{

    size_t freeSize = bufferEnd_;
    std::vector<PalRingBufferReader*>::iterator it;

    for (it = readers_.begin(); it != readers_.end(); it++) {
        if ((*(it))->state_ == READER_ENABLED)
            freeSize = std::min(freeSize, bufferEnd_ - (*(it))->unreadSize_);
    }
    return freeSize;
}

void PalRingBuffer::updateUnReadSize(size_t writtenSize)
{
    int32_t i = 0;
    std::vector<PalRingBufferReader*>::iterator it;

    for (it = readers_.begin(); it != readers_.end(); it++, i++) {
        (*(it))->unreadSize_ += writtenSize;
        PAL_VERBOSE(LOG_TAG, "Reader (%d), unreadSize(%zu)", i, (*(it))->unreadSize_);

        if ((*(it))->requestedSize_ > 0 &&
            (*(it))->unreadSize_ >= (*(it))->requestedSize_) {
             (*(it))->cv_.notify_one();
        }
    }
}

void PalRingBuffer::updateKwdConfig(Stream *s, uint32_t startIdx, uint32_t endIdx,
                                    uint32_t preRoll)
{
    uint32_t sz = 0;
    struct kwdConfig kc;
    std::vector<PalRingBufferReader *> readers = dynamic_cast<StreamSoundTrigger *>(s)->GetReaders();

    std::lock_guard<std::mutex> lck(mutex_);
    /*
     * If the buffer is shared across concurrent detections, the first keyword
     * offset is almost equal or close (depends on the max pre-roll in shared scenario)
     * to the begining of the buffer. For the subsequent keyword, it can be
     * far from the begining of the buffer relative to start of the keyword within
     * the buffer. Since the unreadSize_ of subsequent detections is linearly
     * increased from the first detection itself, adjust its starting from its
     * pre-roll position in the buffer.
     */
    sz = startIdx >= preRoll ? startIdx - preRoll : 0;
    for (auto reader : readers) {
        if (reader->unreadSize_ > sz) {
            reader->unreadSize_ -= sz;
            PAL_DBG(LOG_TAG, "adjusted unread size %zu", reader->unreadSize_);
        }
        reader->unreadSize_ %= bufferEnd_;
        reader->readOffset_ = sz % bufferEnd_;
    }
    kc.startIdx = startIdx - sz;
    kc.endIdx = endIdx - sz;
    kc.ftrtSize = endIdx;
    kwCfg_[s] = kc;
}

void PalRingBuffer::getIndices(Stream *s,
    uint32_t *startIdx, uint32_t *endIdx, uint32_t *ftrtSize)
{
    std::lock_guard<std::mutex> lck(mutex_);
    *startIdx = kwCfg_[s].startIdx;
    *endIdx = kwCfg_[s].endIdx;
    *ftrtSize = kwCfg_[s].ftrtSize;
}

size_t PalRingBuffer::write(void* writeBuffer, size_t writeSize)
{
    /* update the unread size for each reader*/
    size_t freeSize = getFreeSize();
    size_t writtenSize = 0;
    size_t i = 0;
    size_t sizeToCopy = 0;

    std::lock_guard<std::mutex> lck(mutex_);
	
#ifndef SEC_AUDIO_ADD_FOR_DEBUG
    PAL_DBG(LOG_TAG, "Enter. freeSize(%zu), writeOffset(%zu)", freeSize, writeOffset_);
#endif

    if (writeSize <= freeSize)
        sizeToCopy = writeSize;
    else
        sizeToCopy = freeSize;

    if (sizeToCopy) {
        //buffer wrapped around
        if (writeOffset_ + sizeToCopy > bufferEnd_) {
            i = bufferEnd_ - writeOffset_;

            ar_mem_cpy(buffer_ + writeOffset_, i, writeBuffer, i);
            writtenSize += i;
            sizeToCopy -= writtenSize;
            ar_mem_cpy(buffer_, sizeToCopy, (char*)writeBuffer + writtenSize,
                             sizeToCopy);
            writtenSize += sizeToCopy;
            writeOffset_ = sizeToCopy;
        } else {
            ar_mem_cpy(buffer_ + writeOffset_, sizeToCopy, writeBuffer,
                             sizeToCopy);
            writeOffset_ += sizeToCopy;
            writtenSize = sizeToCopy;
        }
    }
    updateUnReadSize(writtenSize);
    writeOffset_ = writeOffset_ % bufferEnd_;
#ifndef SEC_AUDIO_ADD_FOR_DEBUG
    PAL_DBG(LOG_TAG, "Exit. writeOffset(%zu)", writeOffset_);
#endif
    return writtenSize;
}

void PalRingBuffer::reset()
{
    std::vector<PalRingBufferReader*>::iterator it;

    mutex_.lock();
    kwCfg_.clear();
    writeOffset_ = 0;
    mutex_.unlock();

    /* Reset all the associated readers */
    for (it = readers_.begin(); it != readers_.end(); it++)
        (*(it))->reset();
}

void PalRingBuffer::resizeRingBuffer(size_t bufferSize)
{
    if (buffer_) {
        delete[] buffer_;
        buffer_ = nullptr;
    }
    buffer_ = (char *)new char[bufferSize];
    bufferEnd_ = bufferSize;
}

bool PalRingBufferReader::waitForBuffers(uint32_t buffer_size)
{
    std::unique_lock<std::mutex> lck(mutex_);
    if (state_ == READER_ENABLED) {
        if (unreadSize_ >= buffer_size)
            goto exit;
        requestedSize_ = buffer_size;
        cv_.wait_for(lck, std::chrono::milliseconds(3000));
    }

exit:
    requestedSize_ = 0;
    return unreadSize_ >= buffer_size;
}

int32_t PalRingBufferReader::getKwData(Stream *s, uint8_t *data, uint32_t size)
{
    size_t offset = 0;

    std::lock_guard<std::mutex> lck(ringBuffer_->mutex_);

    // assume that we always have enough data after 2nd stage done
    if (readOffset_ + size > ringBuffer_->bufferEnd_) {
        offset = ringBuffer_->bufferEnd_ - readOffset_;
        ar_mem_cpy(data, offset,
            ringBuffer_->buffer_ + readOffset_, offset);
        ar_mem_cpy(data + offset, size - offset,
            ringBuffer_->buffer_, size - offset);
    } else {
        ar_mem_cpy(data, size,
            ringBuffer_->buffer_ + readOffset_, size);
    }

    return size;
}

int32_t PalRingBufferReader::read(void* readBuffer, size_t bufferSize)
{
    int32_t readSize = 0;

    if (state_ == READER_DISABLED) {
        return -EINVAL;
    } else if (state_ == READER_PREPARED) {
        state_ = READER_ENABLED;
    }

    // Return 0 when no data can be read for current reader
    if (unreadSize_ == 0)
        return 0;

    std::lock_guard<std::mutex> lck(ringBuffer_->mutex_);

    // when writeOffset leads readOffset
    if (ringBuffer_->writeOffset_ > readOffset_) {
        unreadSize_ = ringBuffer_->writeOffset_ - readOffset_;

        if (bufferSize >= unreadSize_) {
            ar_mem_cpy(readBuffer, unreadSize_, ringBuffer_->buffer_ +
                             readOffset_, unreadSize_);
            readOffset_ += unreadSize_;
            readSize = unreadSize_;
            unreadSize_ = 0;
        } else {
            ar_mem_cpy(readBuffer, unreadSize_, ringBuffer_->buffer_ +
                             readOffset_, bufferSize);
            readOffset_ += bufferSize;
            readSize = bufferSize;
            unreadSize_ = ringBuffer_->writeOffset_ - readOffset_;
        }
    } else {
        //When readOffset leads WriteOffset
        int32_t freeClientSize = bufferSize;
        int32_t i = ringBuffer_->bufferEnd_ - readOffset_;

        if (bufferSize >= i) {
            ar_mem_cpy(readBuffer, i, (char*)(ringBuffer_->buffer_ +
                             readOffset_), i);
            readSize = i;
            freeClientSize -= readSize;
            unreadSize_ = ringBuffer_->writeOffset_;
            readOffset_ = 0;
            //copy remaining unread buffer
            if (freeClientSize > unreadSize_) {
                ar_mem_cpy((char *)readBuffer + readSize, unreadSize_,
                                 ringBuffer_->buffer_, unreadSize_);
                readSize += unreadSize_;
                readOffset_ = unreadSize_;
                unreadSize_ = 0;
            } else {
                //copy whatever we can
                ar_mem_cpy((char *)readBuffer + readSize, freeClientSize,
                                 ringBuffer_->buffer_, freeClientSize);
                readSize += freeClientSize;
                readOffset_ = freeClientSize;
                unreadSize_ = ringBuffer_->writeOffset_ - readOffset_;
            }

        } else {
            ar_mem_cpy(readBuffer, bufferSize, ringBuffer_->buffer_ +
                             readOffset_, bufferSize);
            readSize = bufferSize;
            readOffset_ += bufferSize;
            unreadSize_ = ringBuffer_->bufferEnd_ - readOffset_ +
                          ringBuffer_->writeOffset_;
        }
    }
    return readSize;
}

size_t PalRingBufferReader::advanceReadOffset(size_t advanceSize)
{
    std::lock_guard<std::mutex> lock(ringBuffer_->mutex_);

    readOffset_ = (readOffset_ + advanceSize) % ringBuffer_->bufferEnd_;

    /*
     * If the buffer is shared across concurrent detections, the second keyword
     * can start anywhere in the buffer and possibly wrap around to the begining.
     * For this case, advanceSize representing the start of keyword position in the
     * buffer can be bigger than unReadSize_ which is aleady adjusted.
     */
    if (unreadSize_ > advanceSize) {
        unreadSize_ -= advanceSize;
    } else {
        PAL_DBG(LOG_TAG, "Warning: trying to advance read offset over write offset");
    }
    PAL_INFO(LOG_TAG, "offset %zu, advanced %zu, unread %zu", readOffset_, advanceSize, unreadSize_);
    return advanceSize;
}

void PalRingBufferReader::updateState(pal_ring_buffer_reader_state state)
{
    PAL_DBG(LOG_TAG, "update reader state to %d", state);
    std::lock_guard<std::mutex> lock(ringBuffer_->mutex_);
    state_ = state;
}

void PalRingBufferReader::getIndices(Stream *s,
    uint32_t *startIdx, uint32_t *endIdx, uint32_t *ftrtSize)
{
    ringBuffer_->getIndices(s, startIdx, endIdx, ftrtSize);
}

size_t PalRingBufferReader::getUnreadSize()
{
    PAL_VERBOSE(LOG_TAG, "unread size %zu", unreadSize_);
    return unreadSize_;
}

size_t PalRingBufferReader::getBufferSize()
{
    return ringBuffer_->getBufferSize();
}

void PalRingBufferReader::reset()
{
    std::lock_guard<std::mutex> lock(ringBuffer_->mutex_);
    readOffset_ = 0;
    unreadSize_ = 0;
    state_ = READER_DISABLED;
    requestedSize_ = 0;
    cv_.notify_all();
}

PalRingBufferReader* PalRingBuffer::newReader()
{
    PalRingBufferReader* reader =
                  new PalRingBufferReader(this);
    readers_.push_back(reader);
    return reader;
}
