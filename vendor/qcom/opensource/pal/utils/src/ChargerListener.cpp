/*
* Copyright (C) 2015, 2021, The Linux Foundation. All rights reserved.
*
*      Not a contribution.
*
* Copyright (C) 2011, 2017  The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* Changes from Qualcomm Innovation Center are provided under the following
* license:
* Copyright (c) 2022-2023, Qualcomm Innovation Center, Inc. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted (subject to the limitations in the
* disclaimer below) provided that the following conditions are met:
*
* Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
*
* Redistributions in binary form must reproduce the above
* copyright notice, this list of conditions and the following
* disclaimer in the documentation and/or other materials provided
* with the distribution.
*
* Neither the name of Qualcomm Innovation Center, Inc. nor the names of
* its contributors may be used to endorse or promote products derived
* from this software without specific prior written permission.
*
* NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
* GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
* HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
* GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
* IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
* IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <unistd.h>
#include <mutex>
#include <thread>
#include <string>
#include <sys/stat.h>
#include <log/log.h>
#include <cutils/uevent.h>
#include "ChargerListener.h"

#define N_OF_EVNT_REG     2
#define MAX_BUFFER_LEN    16
#define UEVENT_MSG_LEN    2048
#define UEVENT_SOCKET_RCVBUF_SIZE  64 * 1024
#define SET_BOOST_CONCURRENT_BIT   "1"
#define RESET_BOOST_CONCURRENT_BIT "0"
#define CHARGER_STATE_BUFF_MAX_LEN  2
#define CHARGING_STATUS_BUFF_MAX_LEN 16
#define CHARGER_PRESENCE_PATH "/sys/class/power_supply/usb/present"
#define CHARGING_STATUS_PATH "/sys/class/power_supply/battery/status"
#define BOOST_CONCURRENT_PATH "/sys/class/qcom-smb/boost_concurrent_mode"

int ChargerListenerImpl::readSysfsPath(const char *path, int flag, int length,
                                       char *state)
{
    int fd = -1, ret = -EINVAL;
    ssize_t bytes;

    if (!path || !state)
        goto exit;

    fd = ::open(path, flag);
    if (fd == -1) {
        ALOGE("%s %d, Failed to open fd in read mode: %s", __func__, __LINE__,
              strerror(errno));
        goto exit;
    }
    bytes = read(fd, state, length);
    if (bytes <= 0) {
        ALOGD("%s %d, Failed to get current state", __func__, __LINE__);
    } else {
        state[(bytes/(sizeof(char))) - 1] = '\0';
        ret = 0;
    }

exit:
    if (ret && state)
        state[0] = '\0';
    if (fd != -1)
        close(fd);

    return ret;
}

int ChargerListenerImpl::getInitialStatus()
{
    char state[MAX_BUFFER_LEN];
    int status = 0;
    int charger_state;
    bool concurrent_state;

    if (!info) {
        return -EINVAL;
    } else {
        status = readSysfsPath(CHARGER_PRESENCE_PATH, O_RDONLY,
                               CHARGER_STATE_BUFF_MAX_LEN, state);
        if (0 != status) {
            ALOGE("%s %d, Failed to get Initial state for charger_presence %d",
                  __func__, __LINE__, status);
            return status;
        } else {
            if (!strncmp(state, "0", strlen(state)))
                charger_state = OFFLINE;
            else
              charger_state = ONLINE;

            int status_bit = getConcurrentState();
            concurrent_state = (status_bit == 1) ? true : false;
            info->c_state = (charger_state_t)charger_state;
            mcb(CHARGER_EVENT, charger_state, concurrent_state);
        }
        // No callback registered for battery_status for now
        status = readSysfsPath(CHARGING_STATUS_PATH, O_RDONLY,
                               CHARGING_STATUS_BUFF_MAX_LEN, state);

        if (0 != status)
            ALOGE("%s %d, Failed to get Initial state for charging_status %d",
                  __func__, __LINE__, status);
            // Not handling Error status for charging status.
            status = 0;
    }
    return status;
}

void ChargerListenerImpl::getStateUpdate(int type)
{
    char state[MAX_BUFFER_LEN];
    bool  concurrent_state;
    int charger_state, status = 0;

    switch (type) {
        case CHARGER_EVENT:
            status = readSysfsPath(CHARGER_PRESENCE_PATH, O_RDONLY,
                                   CHARGER_STATE_BUFF_MAX_LEN, state);
            if (0 == status) {
                if (!strncmp(state, "0", strlen(state)))
                    charger_state = OFFLINE;
                else
                  charger_state = ONLINE;

                if (info->c_state != charger_state) {
                    ALOGD("%s %d, charger state change: %d from last state %d",
                          __func__, __LINE__, charger_state, info->c_state);
                    info->c_state = (charger_state_t)charger_state;
                    int status_bit = getConcurrentState();
                    concurrent_state = (status_bit == 1) ? true : false;
                    mcb(CHARGER_EVENT, charger_state, concurrent_state);
                    //TODO dispatcher_thread = std::thread (mcb, CHARGER_EVENT, charger_state, concurrent_state);
                    //TODO dispatcher_thread.detach();
                }
            }
            break;
        case BATTERY_EVENT:
            readSysfsPath(CHARGING_STATUS_PATH, O_RDONLY,
                          CHARGING_STATUS_BUFF_MAX_LEN, state);
            //Add cb to Process for battery event change.
            break;
        default :
            ALOGI("%s %d, Unknown event", __func__, __LINE__);
            break;
    }
}

void  ChargerListenerImpl::readEvent(void *context,  struct charger_info *info)
{
    /* Max size allowed for UEVENT_MSG_LEN is 2048 */
    char msg_info [UEVENT_MSG_LEN];
    int size_rcv;
    int uevent_type;

    size_rcv = uevent_kernel_multicast_recv(info->uevent_fd,
                                            msg_info, sizeof(msg_info));
    /* underflow and overflow condition*/
    if (size_rcv <= 0 ||
        size_rcv >= sizeof(msg_info)) {
        ALOGE("%s %d, rcv size is not under size limit", __func__, __LINE__);
        return;
    }

    msg_info[size_rcv] = '\0';

    /* update specified uevent properties */
    if (strstr(msg_info, "battery"))
        uevent_type = BATTERY_EVENT;
    else if (strstr(msg_info, "usb"))
        uevent_type = CHARGER_EVENT;
    else
        uevent_type = UNKNOWN_EVENT;

    /* Customize condition for other event*/
    if (uevent_type == CHARGER_EVENT)
        reinterpret_cast<ChargerListenerImpl*>(context)->getStateUpdate(uevent_type);
}

int ChargerListenerImpl::addEvent(func_ptr event_fun, int event_type, int fd)
{
    int status = 0;
    struct epoll_event reg_event;

    ALOGD("%s %d, Enter :", __func__, __LINE__);

    switch(event_type) {
        case PIPE_EVENT:
            reg_event.data.fd = fd;
            reg_event.events = EPOLLIN | EPOLLWAKEUP;

            if (epoll_ctl(info->epoll_fd, EPOLL_CTL_ADD, fd, &reg_event) == -1) {
                ALOGE("%s %d, Failed to add non uevent :%s\n", __func__,
                      __LINE__, strerror(errno));
                status = -errno;
            } else {
                info->event_count++;
            }
            break;
        case U_EVENT:
            reg_event.data.ptr = (void *)event_fun;
            reg_event.events = EPOLLIN | EPOLLWAKEUP;

            if (epoll_ctl(info->epoll_fd, EPOLL_CTL_ADD, fd, &reg_event) == -1) {
                ALOGE("%s %d, Failed to add uevent :%s\n", __func__, __LINE__,
                      strerror(errno));
                status = -errno;
            } else {
                info->event_count++;
            }
            break;
        default :
            ALOGI("%s %d, Unknown event", __func__, __LINE__);
            break;
    }
    ALOGD("%s %d, Exit status %d", __func__, __LINE__, status);
    return status;
}

int ChargerListenerImpl::initEvent()
{
    int status = 0;

    ALOGD("%s %d, Enter ", __func__, __LINE__);

    pipe_status = pipe(intPipe);

    if (pipe_status < 0) {
        ALOGE("%s %d, Failed to open_pipe: %s", __func__, __LINE__,
              strerror(errno));
        status = -errno;
        goto exit;
    }
    /* Adding pipe Event in Interested List to unblock epoll thread*/
    fcntl(intPipe[0], F_SETFL, O_NONBLOCK);
    status = addEvent(ChargerListenerImpl::readEvent, PIPE_EVENT, intPipe[0]);
    if (status < 0) {
        ALOGE("%s %d, Failed to add pipe event: %d", __func__, __LINE__, errno);
       goto close_pipe;
    }

    info->uevent_fd = uevent_open_socket(UEVENT_SOCKET_RCVBUF_SIZE, true);

    if (info->uevent_fd < 0) {
        ALOGE("%s %d, Failed to open_uevent_socket: %s", __func__, __LINE__,
              strerror(errno));
        status = -errno;
        goto close_pipe;
    }
    /* Adding U-Event Event in Interested List */
    fcntl(info->uevent_fd, F_SETFL, O_NONBLOCK);
    status = addEvent(ChargerListenerImpl::readEvent, U_EVENT, info->uevent_fd);
    if (status < 0) {
        ALOGE("%s %d, Failed to add U-Event: %d", __func__, __LINE__, errno);
        goto close_uevent;
    }
    goto exit;

close_uevent:
    if (info->uevent_fd) {
        close(info->uevent_fd);
        info->uevent_fd = 0;
    }
close_pipe:
    if (intPipe[0]) {
        close(intPipe[0]);
        intPipe[0] = 0;
    }

exit:
    ALOGD("%s %d, Exit status %d", __func__, __LINE__, status);
    return status;
}

void ChargerListenerImpl::chargerMonitor()
{
    func_ptr event_cb = NULL;
    /**
     *Initialise buf with "P" to continue poll unless set as "Q" to unblock
     *charger monitor thread when rm deinit happen.
    **/
    char buf[2] = "P";

    ALOGD("%s %d, Enter chargerMonitor", __func__, __LINE__);

    while (1) {
        int no_events = epoll_wait(info->epoll_fd, info->events,
                                   info->event_count, -1);
        if (no_events == -1) {
            if (errno == EINTR)
                continue;
            ALOGE("%s %d, epoll_wait failed: %d", __func__,
                  __LINE__, errno);
            break;
        }

        for (int i = 0; i < no_events; ++i) {
            /* Unblock Polling Thread */
            if (info->events[i].data.fd && info->events[i].data.fd == intPipe[0]) {
                read(info->events[i].data.fd, buf, 1);
                if (!strncmp(buf, "Q", strlen(buf)))
                    return;
            } else {
                event_cb = (func_ptr)info->events[i].data.ptr;
                event_cb(this, info);
            }
        }
    }
    ALOGD("%s %d, Exit : ChargerMonitor is unblocked from poll", __func__, __LINE__);
}

void ChargerListenerImpl::CLImplInit()
{
    /* Create poll fd */
    info->epoll_fd = epoll_create(MAX_EVENTS);
    if (info->epoll_fd == -1) {
        ALOGE("%s %d, Failed to epoll_create: %s\n", __func__, __LINE__,
              strerror(errno));
        return;
    }
    /* Init for uevent */
    if (initEvent() < 0) {
        ALOGE("%s %d, Failed to init uevent: %s\n", __func__, __LINE__,
              strerror(errno));
        goto close_epoll_fd;
    }
    /* Initial state is setup for all event */
    if (getInitialStatus() < 0) {
        ALOGE("%s %d, Failed to determine init status: %s", __func__, __LINE__,
              strerror(errno));
        goto close_epoll_fd;
    }

    ALOGI("%s %d, Charger Listener Impl init is successful", __func__, __LINE__);

    poll_thread = std::thread (&ChargerListenerImpl::chargerMonitor, this);
    goto exit;

close_epoll_fd:
    if (info->epoll_fd) {
        close(info->epoll_fd);
        info->epoll_fd = 0;
    }
exit:
    return;
}

int ChargerListenerImpl::getConcurrentState()
{
    int status_bit = -EINVAL;
    char state[22];

    mlock.lock();
    if (0 != readSysfsPath(BOOST_CONCURRENT_PATH, O_RDONLY, 2, state)) {
        ALOGE("%s %d, read Concurrency bit failed %s", __func__, __LINE__,
              strerror(errno));
    } else {
        sscanf(state, "%d", &status_bit);
    }
    mlock.unlock();
    return status_bit;
}

int ChargerListenerImpl::setConcurrentState(bool is_boost_enable)
{
    int intfd;
    int status = 0;
    int status_bit;

    mlock.lock();
    intfd = ::open(BOOST_CONCURRENT_PATH, O_WRONLY);
    if (intfd == -1) {
        ALOGE("%s %d, Failed to open sysnode: %s", __func__,
              __LINE__, strerror(errno));
        status = -errno;
    } else {
        if (is_boost_enable)
            status_bit = write(intfd, SET_BOOST_CONCURRENT_BIT, 1);
        else
          status_bit = write(intfd, RESET_BOOST_CONCURRENT_BIT, 1);

        if (status_bit == -1) {
            ALOGE("%s %d, failed to Write concurrency bit: %s ", __func__,
                  __LINE__, strerror(errno));
            status = -errno;
        } else {
            ALOGD("%s %d, Success on setting charger + boost concurr. bit %d", __func__,
                  __LINE__, is_boost_enable);
        }

        close(intfd);
    }
    mlock.unlock();
    return status;
}

ChargerListenerImpl::ChargerListenerImpl(cb_fn_t cb) :
        mcb(cb)
{
    info  = (struct charger_info *)calloc(sizeof(struct charger_info), 1);
    if (!info) {
        ALOGE("%s %d, Calloc failed for charger_info %s", __func__, __LINE__,
              strerror(errno));
        return;
    }

    CLImplInit();

}

ChargerListenerImpl *chargerListener = NULL;
ChargerListenerImpl::~ChargerListenerImpl()
{   /*TODO
    if (dispatcher_thread.joinable())
        dispatcher_thread.join();
    */
    if (0 == pipe_status) {
        write(intPipe[1], "Q", 1);
        if (poll_thread.joinable()) {
            poll_thread.join();
        }
        if (intPipe[0])
            close(intPipe[0]);
        if (intPipe[1])
            close(intPipe[1]);
    }
    //Deallocating charger info
    if (info) {
        if (info->epoll_fd)
            close(info->epoll_fd);
        if (info->uevent_fd)
            close(info->uevent_fd);
        free(info);
        info = NULL;
    }
}

extern "C" {
void chargerPropertiesListenerInit(charger_status_change_fn_t fn)
{
    chargerListener = new ChargerListenerImpl(fn);
}

void chargerPropertiesListenerDeinit()
{
    delete chargerListener;
}

int chargerPropertiesListenerSetBoostState(bool is_boost_enable)
{
   return(chargerListener->setConcurrentState(is_boost_enable));
}
} // extern C
