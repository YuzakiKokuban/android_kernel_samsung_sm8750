/*
*Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
*SPDX-License-Identifier: BSD-3-Clause-Clear
*/
///////////////////////////////////////////////////////////////////////////////
// THIS FILE IS IMMUTABLE. DO NOT EDIT IN ANY CASE.                          //
///////////////////////////////////////////////////////////////////////////////

// This file is a snapshot of an AIDL file. Do not edit it manually. There are
// two cases:
// 1). this is a frozen version file - do not edit this in any case.
// 2). this is a 'current' file. If you make a backwards compatible change to
//     the interface (from the latest frozen version), the build system will
//     prompt you to update this file with `m <name>-update-api`.
//
// You must not make a backward incompatible change to any AIDL file built
// with the aidl_interface module type with versions property set. The module
// type is used to build AIDL files in a way that they can be used across
// independently updatable components of the system. If a device is shipped
// with such a backward incompatible change, it has a high risk of breaking
// later when a module using the interface is updated, e.g., Mainline modules.

package vendor.qti.hardware.debugutils;
@VintfStability
interface IDebugUtils {
  void collectBinderDebugInfoByPid(in long pid, in boolean isBlocking, in String debugTag);
  void collectBinderDebugInfoByProcessname(in String processName, in boolean isBlocking, in String debugTag);
  void collectCPUInfo(in boolean isBlocking, in String debugTag);
  void collectDependentProcessStackTrace(in long pid, in boolean isBlocking, in String debugTag);
  void collectFullBinderDebugInfo(in boolean isBlocking, in String debugTag);
  void collectHprof(in boolean isBlocking, in long pid, in String debugTag);
  void collectMemoryInfo(in boolean isBlocking, in String debugTag);
  void collectPeriodicTraces(in long pid, in long duration, in String debugTag);
  void collectProcessCPUInfo(in boolean isBlocking, in long pid, in String debugTag);
  void collectProcessMemoryInfo(in boolean isBlocking, in long pid, in String debugTag);
  void collectRamdump(in String debugTag);
  void collectStackTraceByPid(in long pid, in boolean isJava, in boolean isBlocking, in String debugTag);
  void collectStackTraceByProcessName(in String processName, in boolean isJava, in boolean isBlocking, in String debugTag);
  void collectUserspaceLogs(in String logCmd, in String debugTag, in long duration);
  void executeDumpsysCommands(in String command, in boolean isBlocking, in String debugTag);
  void setBreakPoint(in long pid, in boolean isProcess, in String debugTag);
  void setWatchPoint(in long pid, in long add, in String debugTag);
  void startPerfettoTracing(in long duration, in String debugTag);
  void startSimplePerfTracing(in long pid, in long duration, in String debugTag);
  void stopPerfettoTracing(in String debugTag);
  void stopSimplePerfTracing(in String debugTag);
}
