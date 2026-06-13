/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
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
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#ifndef ACDENGINE_H
#define ACDENGINE_H

#include <map>

#include "ContextDetectionEngine.h"
#include "SoundTriggerUtils.h"
#include "StreamACD.h"
#include "detection_cmn_api.h"

class Session;
class Stream;

/* This is used to maintain list of streams with threshold info per context */
struct stream_context_info {
    uint32_t threshold;
    uint32_t step_size;
    uint32_t last_event_type;
    uint32_t last_confidence_score;
};

class ACDEngine : public ContextDetectionEngine
{
public:
    ACDEngine(Stream *s, std::shared_ptr<ACDStreamConfig> sm_cfg);
    ~ACDEngine();

    static std::shared_ptr<ContextDetectionEngine> GetInstance(Stream *s,
                          std::shared_ptr<ACDStreamConfig> sm_cfg);
    int32_t StartEngine(Stream *s) override;
    int32_t StopEngine(Stream *s) override;
    int32_t SetupEngine(Stream *s, void *config);
    int32_t TeardownEngine(Stream *s, void *config);
    int32_t ReconfigureEngine(Stream *s, void *old_config, void *new_config);

private:
    static void EventProcessingThread(ACDEngine *engine);
    static void HandleSessionCallBack(uint64_t hdl, uint32_t event_id, void *data,
                                      uint32_t event_size);

    int32_t LoadSoundModel() override;
    int32_t UnloadSoundModel() override;
    int32_t RegDeregSoundModel(uint32_t param_id, uint8_t *payload, size_t payload_size);
    int32_t PopulateSoundModel(std::string model_file_name, uint32_t model_uuid);
    int32_t PopulateEventPayload();
    void ParseEventAndNotifyClient();
    void HandleSessionEvent(uint32_t event_id __unused, void *data, uint32_t size);
    bool AreOtherStreamsAttached(Stream *s);
    void UpdateModelCount(struct pal_param_context_list *context_cfg, bool enable);
    void AddEventInfoForStream(Stream *s, struct acd_recognition_cfg *recog_cfg);
    void UpdateEventInfoForStream(Stream *s, struct acd_recognition_cfg *recog_cfg);
    void RemoveEventInfoForStream(Stream *s);
    bool IsModelBinAvailable(uint32_t model_id);
    void ResetModelLoadUnloadFlags();
    int32_t HandleMultiStreamLoadUnload(Stream *s);
    int32_t ProcessStartEngine(Stream *s);
    int32_t ProcessStopEngine(Stream *s);
    bool IsEngineActive();

    static std::shared_ptr<ACDEngine> eng_;
    std::queue<void *> eventQ;
    /* contextinfo_stream_map_ maps context_id with map of stream*
     * and associated threshold values.
     * e.g.
     *    context_id1 -> (s1->threshold_1, step_size_1)
     *                -> (s2->threshold_2, step_size_2)
     *    ...
     *    context_idn -> (s1->threshold_n1, step_size_n1)
     *                -> (s2->threshold_n2, step_size_n2)
     */
    std::map<uint32_t, std::map<Stream *, struct stream_context_info *>*> contextinfo_stream_map_;
    /* cumulative_contextinfo_map_ maps context_id with final threshold values
     * e.g.
     *    context_id1 -> (threshold_1, step_size_1)
     *    ...
     *    context_idn -> (threshold_n, step_size_n)
     */
    std::map<uint32_t, struct stream_context_info *> cumulative_contextinfo_map_;

    std::unordered_map<uint32_t, uint32_t>    model_count_;
    std::unordered_map<uint32_t, std::string> model_load_needed_;
    std::unordered_map<uint32_t, std::string> model_unload_needed_;
    bool     is_confidence_value_updated_;
};
#endif  // ACDENGINE_H
