/*
* Copyright (c) 2021 The Linux Foundation. All rights reserved.
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
*/

#ifndef _CHARGER_LISTENER_H_
#define _CHARGER_LISTENER_H_

#include <sys/epoll.h>
#ifdef ANDROID
#include <bits/epoll_event.h>
#else
#include <fcntl.h>
#include <cstring>
#include <functional>
#endif

#define MAX_EVENTS 3

typedef enum {
    OFFLINE = 0,
    ONLINE
} charger_state_t;

typedef enum {
    UNKNOWN,
    CHARGING,
    DISCHARGING,
    NOT_CHARGING,
    FULL
} batt_state_t;

enum {
    PIPE_EVENT,
    U_EVENT
};

typedef enum {
    UNKNOWN_EVENT = -1,
    CHARGER_EVENT,
    BATTERY_EVENT
} uevent_type_t;

struct charger_info {
    struct epoll_event  events[MAX_EVENTS];
    int                 event_count;
    int                 epoll_fd;
    int                 uevent_fd;
    charger_state_t     c_state;
    batt_state_t        b_state;
};

class ChargerListenerImpl {
    typedef void (*func_ptr)(void *context, struct charger_info *info);
    typedef std::function<void(int, int, bool)> cb_fn_t;
    cb_fn_t mcb;
    int intPipe[2];
    int pipe_status;
    std::thread poll_thread, dispatcher_thread;
    std::mutex mlock;
    struct charger_info *info;
    int readSysfsPath(const char *path, int flag, int length, char *state);
    int getInitialStatus();
    void getStateUpdate(int type);
    static void readEvent(void * context, struct charger_info *info);
    int addEvent(func_ptr event_func, int event_type ,int fd);
    int initEvent();
    void chargerMonitor();
    void CLImplInit();
    int getConcurrentState();
public:
    ChargerListenerImpl (cb_fn_t cb);
    ~ChargerListenerImpl ();
    int setConcurrentState(bool is_boost_enable);
};

#ifdef __cplusplus
extern "C" {
#endif

/**
  * \param[in] uevent_type - Charger or Battery Event.
  * \param[in] status - status of charger or battery based on uevent.
  * \param[in] concurrent_state - Handle ASR case only for charger event. No
  *                               Need to set Concurrent bit by pal when bit
  *                               is 1.
  */
typedef void (*charger_status_change_fn_t)(int, int, bool);
typedef void (*cl_init_t)(charger_status_change_fn_t);
typedef void (*cl_deinit_t)();
typedef int (*cl_set_boost_state_t)(bool);

/**
  * \brief - Initialise, register uevent and pipe in epoll.
  *          Spawn new thread to Monitor change in event.
  * \param[in] charger_status_change_fn_t - CB will be executed on initial
  *                                         status and during uevent change
  *                                         to configure pal.
  */
void chargerPropertiesListenerInit(charger_status_change_fn_t fn);

/**
  * \brief - As RM deinit, main thread will write Q on pipe and epoll
  *          thread will be unblocked.
  */
void chargerPropertiesListenerDeinit();

/**
  * \brief - Notify PMIC driver by writing 0/1 on concurrent sysfs
  *          node to start charging.
  *
  * \param[in] status - concurrent state to set.
  * \return - 0 on success, error code otherwise.
  */
int chargerPropertiesListenerSetBoostState(bool status);

#ifdef __cplusplus
}
#endif

#endif /* _CHARGER_LISTENER_H_ */
