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
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "PalUsecaseTest.h"

int enable_usecase(int usecaseType)
{
    int status =0;
    status = OpenAndStartUsecase(usecaseType);
    if(status) {
       fprintf(stdout, "openAndstartusecase failed\n");
    }
    return status;
}

int disable_usecase()
{
    int status =0;
    status = StopAndCloseUsecase();
    if(status) {
       fprintf(stdout, "openAndstartusecase failed\n");
    }
    return status;
}

static void sigint_handler(int sig)
{
   disable_usecase();
   exit(0);
}

int main(int argc, char *argv[])
{
    char ch[3];
    int status = 0;
    unsigned int usecase_id;
    int sleep_time = 0;

    if (argc < 2 || !strcmp(argv[1], "-help")) {
        fprintf(stdout, "Usage for timer : PalTest UsecaseId -T <time>\n"
                "Usage for Nontimer: PalTest UsecaseId\n");
        return 0;
    }
    usecase_id = atoi(argv[1]);
    if (!usecase_id) {
       fprintf(stdout, "Please enter valid usecaseId\n");
       return 0;
    }

    //Handling signals
    signal(SIGINT, sigint_handler);
    signal(SIGHUP, sigint_handler);
    signal(SIGTERM, sigint_handler);

    if (argc > 2 && !strcmp(argv[2], "-T")) {
       if (argc < 4) {
           fprintf(stdout, "Not enough arguments\nUsage : 'PalTest usecaseId -T <time>'\n");
           return 0;
       }

       sleep_time = atoi(argv[3]);
       if (!sleep_time) {
          fprintf(stdout, "Please enter valid sleep time\n");
          return 0;
       }

       status = enable_usecase(usecase_id);
       if (status)
          return status;

       sleep(sleep_time);

       status =  disable_usecase();
       if (status)
          return status;
    }
    else if (argc == 2) {
       fprintf(stdout, "Enter S to start the usecase or C to close the usecase \n");
       while (1) {
           fgets(ch, 3, stdin);
           switch (ch[0]) {
            case 'S' :
                status = enable_usecase(usecase_id);
                if (status) {
                    return status;
                }
                break;
            case 'C'  :
                status = disable_usecase();
                return status;
                break;
            default:
                fprintf(stdout, "Enter S to start the usecase or C to close the usecase\n");
                break;
           }
           fprintf(stdout, "Enter C to close the usecase\n");
       }
    }
    else
        fprintf(stdout, "Usage for timer : PalTest UsecaseId -T <time>\n"
                "Usage for Nontimer: PalTest UsecaseId\n");
    return status;
}
