/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <asm/unistd.h>
#include <sys/syscall.h>
#include <errno.h>
#include <sched.h>

#include <utils/cpu_instructions.h>

#define __CLASS__ "CPUInstructions"

using namespace display;

namespace sdm {

CPUInstructions * CPUInstructions::CreateInstance(bool track_all_thread,  int32_t cpu_cores) {
  CPUInstructions *intf = new CPUInstructions();
  if (!intf) {
    DLOGE("Failed to create cpu instruction");
    return nullptr;
  }

  int error = intf->Init(track_all_thread, cpu_cores);
  if (error != 0) {
    delete intf;
    intf = nullptr;
  }

  return intf;
}

void CPUInstructions::DestroyInstance(CPUInstructions *intf, uint64_t *cpu_instructions) {
  if (!intf) {
    DLOGW("Invalid cpuinstruction pointer");
    return;
  }

  intf->DeInit(cpu_instructions);
  delete intf;
}

int32_t CPUInstructions::Init(bool track_all_thread, int32_t cpu_cores) {
  std::unique_lock<std::mutex> lock(cpu_instr_mutex_);

  struct perf_event_attr pe;
  memset(&pe, 0, sizeof(pe));
  pe.type = PERF_TYPE_HARDWARE;
  pe.size = sizeof(pe);
  pe.config = PERF_COUNT_HW_INSTRUCTIONS;
  pe.disabled = 1;
  pe.exclude_kernel = 1;
  pe.exclude_hv = 1;
  if (track_all_thread) {
    pe.inherit = 1;
  }

  event_fd_ = PerfEventOpen(&pe, 0, cpu_cores, -1, 0);
  if (event_fd_ < 0) {
    DLOGW("Error opening perf event %llx error %s", pe.config, strerror(errno));
    return -1;
  }

  int32_t result = ioctl(event_fd_, PERF_EVENT_IOC_RESET, 0);
  if (result < 0) {
    DLOGE("PERF_EVENT_IOC_RESET ioctl failed error : %s", strerror(errno));
    return -1;
  }

  return 0;
}

int32_t CPUInstructions::DeInit(uint64_t *cpu_instructions) {
  std::unique_lock<std::mutex> lock(cpu_instr_mutex_);
  if (event_fd_ < 0) {
    DLOGW("Perf event is not opened");
    return -1;
  }

  if (cpu_instructions) {
    read(event_fd_, cpu_instructions, sizeof(uint64_t));
  }

  close(event_fd_);
  event_fd_ = -1;

  return 0;
}

int32_t CPUInstructions::StartTracking() {
  std::lock_guard<std::mutex> lock(cpu_instr_mutex_);
  if (event_fd_ < 0) {
    DLOGW("perf event open is not done");
    return -1;
  }

  int32_t result = ioctl(event_fd_, PERF_EVENT_IOC_ENABLE, 0);
  if (result < 0) {
    DLOGE("PERF_EVENT_IOC_ENABLE failed error %s", strerror(errno));
    return -1;
  }

  return 0;
}

int32_t CPUInstructions::StopTracking(uint64_t *cpu_instructions) {
  std::lock_guard<std::mutex> lock(cpu_instr_mutex_);
  if (event_fd_ < 0) {
    DLOGW("perf event open is not done");
    return -1;
  }

  int32_t result = ioctl(event_fd_, PERF_EVENT_IOC_DISABLE, 0);
  if (result < 0) {
    DLOGE("PERF_EVENT_IOC_DISABLE failed error %s", strerror(errno));
    return -1;
  }

  if (cpu_instructions) {
    read(event_fd_, cpu_instructions, sizeof(uint64_t));
  }

  return 0;
}

int64_t CPUInstructions::PerfEventOpen(struct perf_event_attr *hw_event, pid_t pid,
                                       int32_t cpu, int32_t group_fd, unsigned long flags) {
    int32_t ret = syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
    return ret;
}

}  // namespace sdm
