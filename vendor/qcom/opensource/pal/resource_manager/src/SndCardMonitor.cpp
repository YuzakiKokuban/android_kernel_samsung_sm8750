/*
 * Copyright (c) 2016-2021, The Linux Foundation. All rights reserved.
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
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "PAL: SndMonitor"
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <list>
#include <sys/eventfd.h>
#include "ResourceManager.h"
#include "PalCommon.h"
#include "SndCardMonitor.h"

#define SNDCARD_PATH "/sys/kernel/snd_card/card_state"
#define MAX_SLEEP_RETRY 100

static int fd = -1, efd = -1;
static int exit_thread = 0; //by default exit thread is made false

void SndCardMonitor::monitorThreadLoop()
{
    struct pollfd *poll_fds;
    int rv = 0;
    char buf[12];
    int card_status = 0;
    int tries = MAX_SLEEP_RETRY;

    card_status_t status = CARD_STATUS_NONE;
    std::shared_ptr<ResourceManager> rm = ResourceManager::getInstance();
    while(--tries) {
        if (exit_thread == 1)
            break;
        if ((fd = open(SNDCARD_PATH, O_RDWR)) < 0) {
            PAL_ERR(LOG_TAG, "Open failed snd sysfs node");
        }
        else {
            PAL_VERBOSE(LOG_TAG, "snd sysfs node open successful");
            break;
        }
        usleep(500000);
    }
    efd = eventfd(0, EFD_CLOEXEC);
    if (fd == -1 || efd == -1)
        goto Done;
    poll_fds = (struct pollfd*) calloc(2, sizeof(struct pollfd));
    if(NULL == poll_fds) {
        PAL_ERR(LOG_TAG, "Memory allocation failed");
        goto Done;
    }

    while(1) {
        memset(buf , 0 ,sizeof(buf));
        read(fd, buf, 10);
        lseek(fd,0L,SEEK_SET);

        poll_fds[0].fd = fd;
        poll_fds[0].events = POLLERR | POLLPRI;
        poll_fds[0].revents = 0;
        poll_fds[1].fd = efd;
        poll_fds[1].events = POLLERR | POLLIN;
        poll_fds[1].revents = 0;

        PAL_VERBOSE(LOG_TAG, "waiting sys_notify event\n");
        if (( rv = poll(poll_fds, 2, -1)) < 0 ) {
             PAL_ERR(LOG_TAG, "snd sysfs node poll error\n");
        } else if ((poll_fds[0].revents & POLLPRI)) {
            lseek(poll_fds[0].fd,0L,SEEK_SET);
            read(poll_fds[0].fd, buf, 1);
            sscanf(buf , "%d", &card_status);
            PAL_INFO(LOG_TAG, "card status %d\n",card_status);
            if (card_status == 0)
                status = CARD_STATUS_OFFLINE;
            else if (card_status == 1)
                status = CARD_STATUS_ONLINE;
            else if (card_status == 2)
                status = CARD_STATUS_STANDBY;
            else if (card_status == 3)
                break;

            rm->ssrHandler(status);
        } else if((poll_fds[1].revents & POLLIN)) {
            uint64_t eval;
            read(poll_fds[1].fd, &eval, 8);
            if (eval == 1) {
                free(poll_fds);
                poll_fds = NULL;
                close(efd);
                close(fd);
                break;
            }
       }
    }
Done:
    return;
}

SndCardMonitor::SndCardMonitor(int sndNum)
{
    sndNum = 0; //not used at present.
    mThread = std::thread(&SndCardMonitor::monitorThreadLoop, this);
    PAL_VERBOSE(LOG_TAG, "Snd card monitor init done.");
    return;
}


SndCardMonitor::~SndCardMonitor()
{
   uint64_t eval = 1;
   exit_thread = 1;
   if(efd != -1)
      write(efd, &eval, 8);
   mThread.join();
}
