/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __CPU_INSTRUCTIONS_H__
#define __CPU_INSTRUCTIONS_H__

#include <unistd.h>
#include <debug_handler.h>
#include <linux/perf_event.h>
#include <mutex>

namespace sdm {

class CPUInstructions {
 public:
  /*! @brief Method to create and get handle to CPU instruction instance.

    @details This method will open the perf event with apropriate tracking instructions.
    by default it will tracking cpu instruction for all the cpu cores and all subthreads.
    To start tracking Client need to call StartTracking after creating instance.

    @param[in] track_all_thread : Optional parameter
                                  true: track all subthreads.
                                  false: track only the current thread.
    @param[in] cpu_cores : Optional parameter
                           -1 : track instructions all the cores.
                           <core no> : track instructions for the specified core.
    @return CPUInstructions * : returns the cpu instruction object pointer.
  */
  static CPUInstructions *CreateInstance(bool track_all_thread = true,  int32_t cpu_cores = -1);
  /*! @brief Method to destroy the CPU instruction instance.

    @details This method will destroy the CPU instructions instance and close the perf event fd.
    It provide provision to read the cpu_instructions before closing the fd.

    @param[in] CPUInstructions * :  CPU instruction instance.
    @param[in] cpu_instructions : Optional parameter
                                  CPU instruction gets populated only if client passes.
                                  valid pointer.
    @return void
  */
  static void DestroyInstance(CPUInstructions *intf, uint64_t *cpu_instructions = nullptr);
  /*! @brief Method to start tracking cpu instructions.

    @details This method will start tracking cpu instructions.
    @return int : 0 : No error
  */
  int32_t StartTracking();
  /*! @brief Method to stop tracking cpu instructions.

    @details This method will stop tracking cpu instructions but it will not close
    the perf event fd so if client calls StartTracking it will start measure from the existing
    cpu instructions counter value. This function provide provision to tap cpu instructions value.
    @param[in] cpu_instructions : Optional parameter
                                  CPU instructions are only read if client will pass valid pointer.
                                  if valid pointer is not passed function will not read the
                                  instructions before closing.
    @return int : 0 : No error
  */
  int32_t StopTracking(uint64_t *cpu_instructions = nullptr);

 private:
  CPUInstructions() {}
  ~CPUInstructions() {}
  int32_t Init(bool track_all_thread, int32_t cpu_cores);
  int32_t DeInit(uint64_t *cpu_instructions);
  int64_t PerfEventOpen(struct perf_event_attr *hw_event, pid_t pid, int32_t cpu,
                        int32_t group_fd, unsigned long flags);
  std::mutex cpu_instr_mutex_;
  int32_t event_fd_ = -1;
};

}  // namespace sdm

#endif  // __CPU_INSTRUCTIONS_H__
