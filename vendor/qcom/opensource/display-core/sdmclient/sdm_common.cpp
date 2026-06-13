/*
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "sdm_common.h"

#include <cstring>
#include <poll.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/queue.h>
#include <linux/netlink.h>

namespace sdm {

nsecs_t nanoseconds_to_seconds(nsecs_t secs) {
  return secs / 1000000000;
}

nsecs_t nanoseconds_to_milliseconds(nsecs_t secs) {
  return secs / 1000000;
}

size_t strlcpy(char *dst, const char *src, size_t size) {
  size_t srclen = strlen(src);

  if (size) {
    size_t minlen = std::min(srclen, size - 1);

    memcpy(dst, src, minlen);
    dst[minlen] = '\0';
  }
  return srclen;
}

// uevent ==========================
LIST_HEAD(uevent_handler_head, uevent_handler) uevent_handler_list;
pthread_mutex_t uevent_handler_list_lock = PTHREAD_MUTEX_INITIALIZER;

struct uevent_handler {
  void (*handler)(void *data, const char *msg, int msg_len);
  void *handler_data;
  LIST_ENTRY(uevent_handler) list;
};

static int fd = -1;

/* Returns 0 on failure, 1 on success */
int uevent_init() {
  struct sockaddr_nl addr;
  int sz = 64 * 1024;
  int s;

  memset(&addr, 0, sizeof(addr));
  addr.nl_family = AF_NETLINK;
  addr.nl_pid = getpid();
  addr.nl_groups = 0xffffffff;

  s = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
  if (s < 0)
    return 0;

  setsockopt(s, SOL_SOCKET, SO_RCVBUFFORCE, &sz, sizeof(sz));

  if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    close(s);
    return 0;
  }

  fd = s;
  return (fd > 0);
}

int uevent_get_fd() {
  return fd;
}

int uevent_next_event(char *buffer, int buffer_length) {
  while (1) {
    struct pollfd fds;
    int nr;

    fds.fd = fd;
    fds.events = POLLIN;
    fds.revents = 0;
    nr = poll(&fds, 1, -1);

    if (nr > 0 && (fds.revents & POLLIN)) {
      int count = recv(fd, buffer, buffer_length, 0);
      if (count > 0) {
        struct uevent_handler *h;
        pthread_mutex_lock(&uevent_handler_list_lock);
        LIST_FOREACH(h, &uevent_handler_list, list)
        h->handler(h->handler_data, buffer, buffer_length);
        pthread_mutex_unlock(&uevent_handler_list_lock);

        return count;
      }
    }
  }

  // won't get here
  return 0;
}

}  // namespace sdm