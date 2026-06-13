/*
*Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
*SPDX-License-Identifier: BSD-3-Clause-Clear
*/

package vendor.qti.hardware.debugutils;


@VintfStability
interface IDebugUtils {
    /**
     * Read binder_debugfs specific to a process hosted with @param pid
     */
    void collectBinderDebugInfoByPid(in long pid, in boolean isBlocking, in String debugTag);

    /**
     * Read binder_debugfs specific to a process hosted with @param ProcessName
     */
    void collectBinderDebugInfoByProcessname(in String processName, in boolean isBlocking,
        in String debugTag);

    /**
     * collect CPU specific info
     */
    void collectCPUInfo(in boolean isBlocking, in String debugTag);

    /**
     * Collect the stack trace of processes which in IPC with process
     * hosted by @param pid
     */
    void collectDependentProcessStackTrace(in long pid, in boolean isBlocking,
        in String debugTag);

    /**
     * Read binder debugfs and move it to another place
     */
    void collectFullBinderDebugInfo(in boolean isBlocking, in String debugTag);

    /**
     * collect hprof
     */
    void collectHprof(in boolean isBlocking, in long pid, in String debugTag);

    /**
     * collect memory specific info
     */
    void collectMemoryInfo(in boolean isBlocking, in String debugTag);

    /**
     * collect periodic ANR traces
     */
    void collectPeriodicTraces(in long pid, in long duration, in String debugTag);

    /**
     * collect process specific CPU info
     */
    void collectProcessCPUInfo(in boolean isBlocking, in long pid, in String debugTag);

    /**
     * collect process specific Memory info
     */
    void collectProcessMemoryInfo(in boolean isBlocking, in long pid, in String debugTag);

    /**
     * crash the device to collect the ramdump
     */
    void collectRamdump(in String debugTag);

    /**
     * collect stacktrace of a process specified by @param pid
     */
    void collectStackTraceByPid(in long pid, in boolean isJava, in boolean isBlocking,
        in String debugTag);

    /**
     * collect stacktrace of a process specified by @param ProcessName
     */
    void collectStackTraceByProcessName(in String processName, in boolean isJava,
        in boolean isBlocking, in String debugTag);

    /**
     * Collect logcat buffer speicified by @param bufferName for duration specified by @param duration
     */
    void collectUserspaceLogs(in String logCmd, in String debugTag, in long duration);

    /**
     * execute am command specified by @param Command
     */
    void executeDumpsysCommands(in String command, in boolean isBlocking, in String debugTag);

    /**
     * set breakpoint in a process hosted by @param pid
     */
    void setBreakPoint(in long pid, in boolean isProcess, in String debugTag);

    /**
     * set watchpoint in a thread hosted by @param pid on address stored in @param add
     */
    void setWatchPoint(in long pid, in long add, in String debugTag);

    /**
     * start Perfetto tracing
     */
    void startPerfettoTracing(in long duration, in String debugTag);

    /**
     * start SimplePerf tracing hosted by @param pid
     */
    void startSimplePerfTracing(in long pid, in long duration, in String debugTag);

    /**
    * stop Perfetto tracing
    */
    void stopPerfettoTracing(in String debugTag);

    /**
     * stop SimplePerf tracing for process
     */
    void stopSimplePerfTracing(in String debugTag);
}
