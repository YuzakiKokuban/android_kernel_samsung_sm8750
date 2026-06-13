/* Copyright (c) 2011-2015, The Linux Foundation. All rights reserved.
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
 *     * Neither the name of The Linux Foundatoin, nor the names of its
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
 */

/*
Changes from Qualcomm Innovation Center are provided under the following license:

Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted (subject to the limitations in the
disclaimer below) provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <log_util.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <timepps.h>
#include <linux/types.h>
#include <stdbool.h>
#include "loc_cfg.h"
#include <inttypes.h>
#include <time.h>
#include <math.h>

#define BILLION_NSEC  (1E9)

typedef struct timespec pps_sync_time;

//DRsync kernel timestamp
static struct timespec drsyncKernelTs = {0,0};
//DRsync userspace timestamp
static struct timespec drsyncUserTs = {0,0};

static struct timespec prevDrsyncKernelTs = {0,0};

//flag to stop fetching timestamp
static int isActive = 0;
static pthread_t threadId;
static pps_handle handle;

static pthread_mutex_t ts_lock;
static int skipPPSPulseCnt = 0;
static int gnssOutageInSec = 1;
static loc_param_s_type gps_conf_param_table[] =
{
    {"IGNORE_PPS_PULSE_COUNT", &skipPPSPulseCnt, NULL, 'n'},
    {"GNSS_OUTAGE_DURATION", &gnssOutageInSec, NULL, 'n'}
};

typedef enum {
    KERNEL_REPORTED_CLOCK_BOOTTIME = 0,
    KERNEL_REPORTED_CLOCK_REALTIME,
    KERNEL_REPORTED_CLOCK_UNKNOWN = 0xfe,
    KERNEL_REPORTED_CLOCK_MAX = 0xff
} kernel_reported_time_e;

  /*  checks the PPS source and opens it */
int check_device(char *path, pps_handle *handle)
{
     int ret;

     /* Try to find the source by using the supplied "path" name */
     ret = open(path, O_RDONLY);
     if (ret < 0)
     {
         LOC_LOGV("%s:%d unable to open device %s", __func__, __LINE__, path);
         return ret;
     }

     /* Open the PPS source */
     ret = pps_create(ret, handle);
     if (ret < 0)
     {
         LOC_LOGV( "%s:%d cannot create a PPS source from device %s", __func__, __LINE__, path);
         return -1;
     }
     return 0;
}


/* calculate the time difference between two given times in argument -
** returns the result in timeDifference parametter*/
static inline void calculate_time_difference(pps_sync_time time1,
                            pps_sync_time time2, pps_sync_time* timeDifference)
{
    if (time2.tv_nsec < time1.tv_nsec) {
        timeDifference->tv_sec = time2.tv_sec - 1 - time1.tv_sec;
        timeDifference->tv_nsec = BILLION_NSEC + time2.tv_nsec - time1.tv_nsec;
    } else {
        timeDifference->tv_sec = time2.tv_sec - time1.tv_sec;
        timeDifference->tv_nsec = time2.tv_nsec - time1.tv_nsec;
    }
}

/*add two timespec values and return the result in third resultTime parameter*/
static inline void pps_sync_time_add(pps_sync_time time1,
                            pps_sync_time time2, pps_sync_time* resultTime)
{
    if (time2.tv_nsec + time1.tv_nsec >= BILLION_NSEC) {
        resultTime->tv_sec = time2.tv_sec + time1.tv_sec + 1;
        resultTime->tv_nsec = time2.tv_nsec + time1.tv_nsec - BILLION_NSEC;
    } else {
        resultTime->tv_sec = time2.tv_sec + time1.tv_sec;
        resultTime->tv_nsec = time2.tv_nsec + time1.tv_nsec;
    }
}

#define MAX_REALTIME_OFFSET_DELTA_DIFFERENCE  10
#define MAX_PPS_KERNEL_TO_USERSPACE_LATENCY   100    /*in milli-second*/

static inline double convertTimeToMilliSec(pps_sync_time timeToConvert)
{
   return ((double)(timeToConvert.tv_sec * 1000) + (double)(timeToConvert.tv_nsec * 0.000001));
}

/*returnValue: 1 = offsetTime OK, 0 = offsetTime NOT OK*/
static inline bool isTimeOffsetOk(pps_sync_time offsetTime)
{
    double offsetInMillis = convertTimeToMilliSec(offsetTime);
    return (fabs(offsetInMillis) <= MAX_REALTIME_OFFSET_DELTA_DIFFERENCE)? true: false;
}

/*check whether kernel PPS timestamp is a CLOCK_BOOTTIME or CLOCK_REALTIME*/
static kernel_reported_time_e get_reported_time_type(pps_sync_time userBootTs,
                                                    pps_sync_time userRealTs,
                                                    pps_sync_time kernelTs)
{
    double userRealMs, userBootMs, kernelTsMs;
    kernel_reported_time_e reportedTime = KERNEL_REPORTED_CLOCK_UNKNOWN;

    userRealMs = convertTimeToMilliSec(userRealTs);
    userBootMs = convertTimeToMilliSec(userBootTs);
    kernelTsMs = convertTimeToMilliSec(kernelTs);

    if(fabs(userBootMs - kernelTsMs) <= MAX_PPS_KERNEL_TO_USERSPACE_LATENCY) {
        reportedTime = KERNEL_REPORTED_CLOCK_BOOTTIME;
    } else if (fabs(userRealMs - kernelTsMs) <= MAX_PPS_KERNEL_TO_USERSPACE_LATENCY) {
        reportedTime = KERNEL_REPORTED_CLOCK_REALTIME;
    } else {
        reportedTime = KERNEL_REPORTED_CLOCK_UNKNOWN;
    }

    LOC_LOGV("[%s] Detected PPS timestamp type (0:Boot, 1:Real, 0xfe:Unknown):0x%x",
              __func__, reportedTime);

    return reportedTime;
}

/*compute_real_to_boot_time - converts the kernel real time stamp to boot time reference*/
static int compute_real_to_boot_time(pps_info ppsFetchTime)
{
    int retVal = 0;
    /*Offset between REAL_TIME to BOOT_TIME*/
    static pps_sync_time time_offset = {0, 0};
    pps_sync_time  drSyncUserRealTs, drSyncUserBootTs;
    pps_sync_time  deltaRealToBootTime = {0, 0};
    pps_sync_time  deltaOffsetTime = {0, 0};
    kernel_reported_time_e  ppsTimeType;

    retVal = clock_gettime(CLOCK_BOOTTIME,&drSyncUserBootTs);
    if (retVal != 0) {
        LOC_LOGE("[%s]Error Reading CLOCK_BOOTTIME", __func__);
        retVal = -1;
        goto exit0;
    }
    retVal = clock_gettime(CLOCK_REALTIME,&drSyncUserRealTs);
    if (retVal != 0){
        LOC_LOGE("[%s]Error Reading CLOCK_REALTIME", __func__);
        retVal = -1;
        goto exit0;
    }

    ppsTimeType = get_reported_time_type(drSyncUserBootTs, drSyncUserRealTs, ppsFetchTime);

    if (KERNEL_REPORTED_CLOCK_BOOTTIME == ppsTimeType) {

        /*Store PPS kernel time*/
        drsyncKernelTs.tv_sec = ppsFetchTime.tv_sec;
        drsyncKernelTs.tv_nsec = ppsFetchTime.tv_nsec;
        /*Store User PPS fetch time*/
        drsyncUserTs.tv_sec = drSyncUserBootTs.tv_sec;
        drsyncUserTs.tv_nsec = drSyncUserBootTs.tv_nsec;

        LOC_LOGV("[compute_real_to_boot] PPS TimeType CLOCK_BOOTTIME -- report as is");
        retVal = 0;
    } else if (KERNEL_REPORTED_CLOCK_REALTIME == ppsTimeType) {

        /* Calculate time difference between REALTIME to BOOT_TIME*/
        calculate_time_difference(drSyncUserRealTs, drSyncUserBootTs, &deltaRealToBootTime);

        /* Calculate the time difference between stored offset and computed one now*/
        calculate_time_difference(time_offset, deltaRealToBootTime, &deltaOffsetTime);

        if ((0 == time_offset.tv_sec && 0 == time_offset.tv_nsec) ||
            (isTimeOffsetOk(deltaOffsetTime))) {
            /* if Time Offset does not change beyond threshold then
            ** convert to boot time*/
            drsyncUserTs.tv_sec = drSyncUserBootTs.tv_sec;
            drsyncUserTs.tv_nsec = drSyncUserBootTs.tv_nsec;
            pps_sync_time_add(ppsFetchTime, deltaRealToBootTime, &drsyncKernelTs);
            retVal = 0;
        } else {
            /*The offset is too high, jump detected in realTime tick, either
            ** wall clock changed by user of NTP, skip PPS, re-sync @ next tick
            */
            LOC_LOGE("[%s] Jump detected in CLOCK_REALTIME - Offset %ld:%ld", __func__,
                    deltaOffsetTime.tv_sec, deltaOffsetTime.tv_nsec);
            retVal = -1;
        }
        time_offset.tv_sec = deltaRealToBootTime.tv_sec;
        time_offset.tv_nsec = deltaRealToBootTime.tv_nsec;

        LOC_LOGV("[compute_real_to_boot] KernelRealTs %ld:%ld ComputedTs %ld:%ld RealTs %ld:%ld BootTs %ld:%ld Offset %ld:%ld retVal %d ",
                ppsFetchTime.tv_sec, ppsFetchTime.tv_nsec,
                drsyncKernelTs.tv_sec, drsyncKernelTs.tv_nsec, drSyncUserRealTs.tv_sec,
                drSyncUserRealTs.tv_nsec, drsyncUserTs.tv_sec, drsyncUserTs.tv_nsec,
                time_offset.tv_sec, time_offset.tv_nsec, retVal);
    } else {
        LOC_LOGV("[compute_real_to_boot] PPS TimeType Unknown -- KernelTs %ld:%ld userRealTs %ld:%ld userBootTs %ld:%ld",
                ppsFetchTime.tv_sec, ppsFetchTime.tv_nsec,
                drSyncUserRealTs.tv_sec, drSyncUserRealTs.tv_nsec,
                drsyncUserTs.tv_sec, drsyncUserTs.tv_nsec);
        retVal = -1;
    }

exit0:
    return retVal;

}


/* fetches the timestamp from the PPS source */
int read_pps(pps_handle *handle)
{
    struct timespec timeout;
    pps_info infobuf;
    int ret;
    static  bool isFirstPulseReceived = false;
    static unsigned int skipPulseCnt = 0;
    // 3sec timeout
    timeout.tv_sec = 3;
    timeout.tv_nsec = 0;

    ret = pps_fetch(*handle, PPS_TSFMT_TSPEC, &infobuf, &timeout);

    if (ret < 0 && ret !=-EINTR)
    {
        LOC_LOGV("%s:%d pps_fetch() error %d", __func__, __LINE__, ret);
        return -1;
    }
    if (!isFirstPulseReceived && (skipPPSPulseCnt > 0)) {
        skipPulseCnt = skipPPSPulseCnt;
        isFirstPulseReceived = true;
        LOC_LOGV("%s:%d first pps pulse received %ld (sec) %ld (nsec)",
                  __func__, __LINE__, infobuf.tv_sec, infobuf.tv_nsec)
    }else if (isFirstPulseReceived && (skipPPSPulseCnt > 0) &&
              ((infobuf.tv_sec - prevDrsyncKernelTs.tv_sec) > gnssOutageInSec)) {
        LOC_LOGV("%s:%d long RF outage detected, ignore next %d PPS Pulses",
                  __func__, __LINE__, skipPPSPulseCnt)
        skipPulseCnt = skipPPSPulseCnt;
    }
    prevDrsyncKernelTs.tv_sec = infobuf.tv_sec;
    prevDrsyncKernelTs.tv_nsec = infobuf.tv_nsec;
    /* check if this dr sync pulse need to be ignored or not*/
    if (skipPulseCnt > 0) {
        LOC_LOGV("%s:%d skip pps count %d, ignoring this pps pulse %ld (sec)  %ld (nsec)",
                  __func__,__LINE__, skipPulseCnt, infobuf.tv_sec, infobuf.tv_nsec);
        skipPulseCnt--;
        return 0;
    }
    /* update dr syncpulse time*/

    pthread_mutex_lock(&ts_lock);

    ret = compute_real_to_boot_time(infobuf);

    pthread_mutex_unlock(&ts_lock);

    return 0;
}

#ifdef __cplusplus
extern "C" {
#endif
/* infinitely calls read_pps() */
void *thread_handle(void *input)
{
    int ret;
    if(input != NULL)
    {
        LOC_LOGV("%s:%d Thread Input is present", __func__, __LINE__);
    }
    while(isActive)
    {
        ret = read_pps(&handle);

        if (ret == -1 && errno != ETIMEDOUT )
        {
            LOC_LOGV("%s:%d Could not fetch PPS source", __func__, __LINE__);
        }
    }
    return NULL;
}

/*  opens the device and fetches from PPS source */
int initPPS(char *devname)
{
    int ret,pid;
    isActive = 1;

    ret = check_device(devname, &handle);
    if (ret < 0)
    {
        LOC_LOGV("%s:%d Could not find PPS source", __func__, __LINE__);
        return 0;
    }
    UTIL_READ_CONF(LOC_PATH_GPS_CONF, gps_conf_param_table);
    pthread_mutex_init(&ts_lock, NULL);

    pid = pthread_create(&threadId, NULL, &thread_handle, NULL);
    if(pid != 0)
    {
        LOC_LOGV("%s:%d Could not create thread in InitPPS", __func__, __LINE__);
        isActive = 0;
        return 0;
    }
    return 1;
}

/* stops fetching and closes the device */
void deInitPPS()
{
    pthread_mutex_lock(&ts_lock);
    if (0 == isActive) {
        LOC_LOGV("%s:%d deInitPPS called before initPPS", __func__, __LINE__);
        pthread_mutex_unlock(&ts_lock);
        return;
    }
    isActive = 0;
    pthread_mutex_unlock(&ts_lock);

    pthread_join(threadId, NULL);

    pthread_mutex_destroy(&ts_lock);
    pps_destroy(handle);
}

/* retrieves DRsync kernel timestamp,DRsync userspace timestamp
   and updates current timestamp */
/* Returns:
 *     1. @Param out DRsync kernel timestamp
 *     2. @Param out DRsync userspace timestamp
 *     3. @Param out current timestamp
 */
int getPPS(struct timespec *fineKernelTs ,struct timespec *currentTs,
           struct timespec *fineUserTs)
{
    int ret = 0;

    pthread_mutex_lock(&ts_lock);
    if (0 != isActive) {
        fineKernelTs->tv_sec = drsyncKernelTs.tv_sec;
        fineKernelTs->tv_nsec = drsyncKernelTs.tv_nsec;

        fineUserTs->tv_sec = drsyncUserTs.tv_sec;
        fineUserTs->tv_nsec = drsyncUserTs.tv_nsec;

        ret = clock_gettime(CLOCK_BOOTTIME,currentTs);
    } else {
       LOC_LOGV("%s:%d No active thread to read PPS - call initPPS first", __func__, __LINE__);
    }
    pthread_mutex_unlock(&ts_lock);
    if(ret != 0)
    {
       LOC_LOGV("%s:%d clock_gettime() error",__func__,__LINE__);
       return 0;
    }
    return 1;
}

#ifdef __cplusplus
}
#endif

