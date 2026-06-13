/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

 Time related routines

 GENERAL DESCRIPTION
 This component provides utilities used for timestamp and time
 difference processing

 Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 SPDX-License-Identifier: BSD-3-Clause-Clear
 =============================================================================*/
#include <time.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/time.h>
#include <base_util/time_routines.h>
#include <base_util/log.h>

#define BREAK_IF_ZERO(ERR,X) if(0==(X)) {result = (ERR); break;}
#define BREAK_IF_NON_ZERO(ERR,X) if(0!=(X)) {result = (ERR); break;}
#define BREAK_IF_NON_ZERO_RC(ERR,RC,X) if(0!=(RC=(X))) {result = (ERR); break;}

#ifdef CLOCK_BOOTTIME
#define ENABLED_BOOTTIME_SUPPORT 1
#else
#define ENABLED_BOOTTIME_SUPPORT 0
#endif

#define TIME_RTC_MS_AS_OF_12_1_2014 (1417569370000)

const int NANO_PER_SEC = 1000000000;

namespace qc_loc_fw
{
const char * const TimeDiff::TAG = "TimerDiff";
const char * const Timestamp::TAG = "Timestamp";

TimeDiff::TimeDiff(const bool validity)
{
  reset(validity);
}

void TimeDiff::reset(const bool validity)
{
  m_is_valid = validity;
  memset(&m_timediff, 0, sizeof(m_timediff));
}

bool TimeDiff::is_valid() const
{
  return m_is_valid;
}

const timespec * TimeDiff::getTimeDiffPtr() const
{
  if(!m_is_valid)
  {
    log_warning(TAG, "getTimeDiffPtr time diff is invalid");
  }
  return &m_timediff;
}

float TimeDiff::get_total_sec() const
{
  float conv = -1;
  int result = 1;
  do
  {
    if(!m_is_valid)
    {
      result = 2;
      break;
    }

    conv = m_timediff.tv_nsec / 1e9;
    conv += m_timediff.tv_sec;

    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error(TAG, "get_total_sec failed %d", result);
  }
  return conv;
}

float TimeDiff::get_total_msec() const
{
  float conv = -1;
  int result = 1;
  do
  {
    if(!m_is_valid)
    {
      result = 2;
      break;
    }

    conv = m_timediff.tv_nsec / 1e6f;
    conv += (m_timediff.tv_sec) * 1000.0f;

    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error(TAG, "get_total_msec failed %d", result);
  }
  return conv;
}

int TimeDiff::add_sec(const int sec)
{
  int result = 1;
  do
  {
    if(!m_is_valid)
    {
      result = 2;
      break;
    }

    m_timediff.tv_sec += sec;

    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error(TAG, "add_sec failed %d", result);
  }
  return result;
}

int TimeDiff::add_msec(const int msec)
{
  int result = 1;
  do
  {
    if(!m_is_valid)
    {
      result = 2;
      break;
    }

    int sec = msec / 1000;
    long long int msec_modified = msec - sec * 1000;

    m_timediff.tv_nsec += msec_modified * 1000000;

    while (m_timediff.tv_nsec >= NANO_PER_SEC)
    {
      ++sec;
      m_timediff.tv_nsec -= NANO_PER_SEC;
    }

    m_timediff.tv_sec += sec;

    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error(TAG, "add_msec failed %d", result);
  }
  return result;
}

int TimeDiff::add_nsec(const int nsec)
{
  int result = 1;
  do
  {
    if(!m_is_valid)
    {
      result = 2;
      break;
    }

    m_timediff.tv_nsec += nsec;
    while (m_timediff.tv_nsec >= NANO_PER_SEC)
    {
      ++m_timediff.tv_sec;
      m_timediff.tv_nsec -= NANO_PER_SEC;
    }

    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error(TAG, "add_nsec failed %d", result);
  }
  return result;
}

Timestamp::Timestamp(bool set_to_default_clock)
{
  invalidate();
  if(set_to_default_clock)
  {
    (void) reset_to_default_clock();
  }
}

Timestamp::Timestamp(const int clock_id, const timespec & src)
{
  m_is_valid = true;
  m_clock_id = clock_id;
  memcpy(&m_timestamp, &src, sizeof(m_timestamp));
}

Timestamp::Timestamp(const Timestamp & rhs)
{
  *this = rhs;
}

Timestamp::Timestamp(const int clock_id)
{
  reset_to_clock_id(clock_id);
}

int Timestamp::get_clock_id() const
{
  return m_clock_id;
}

bool Timestamp::is_valid() const
{
  return m_is_valid;
}

bool Timestamp::is_valid_and_default() const
{
  bool result = false;
  if(is_valid())
  {
#if ENABLED_BOOTTIME_SUPPORT
    const int default_clock_id = CLOCK_BOOTTIME;
#else
    const int default_clock_id = CLOCK_MONOTONIC;
#endif //#if ENABLED_BOOTTIME_SUPPORT
    if(default_clock_id == m_clock_id)
    {
      result = true;
    }
  }
  return result;
}

void Timestamp::invalidate()
{
  m_is_valid = false;
  m_clock_id = -1;
  memset(&m_timestamp, 0, sizeof(m_timestamp));
}

timespec * Timestamp::getTimestampPtr()
{
  const timespec * const ptr = static_cast<const Timestamp *>(this)->getTimestampPtr();
  return const_cast<timespec *>(ptr);
}

const timespec * Timestamp::getTimestampPtr() const
{
  if(!m_is_valid)
  {
    log_warning(TAG, "getTimestampPtr timestamp is invalid");
  }
  return &m_timestamp;
}

int Timestamp::reset_to_default_clock()
{
#if ENABLED_BOOTTIME_SUPPORT
  return reset_to_boottime();
#else
  return reset_to_monotonic();
#endif
}

int Timestamp::reset_to_monotonic()
{
  return reset_to_clock_id(CLOCK_MONOTONIC);
}

int Timestamp::reset_to_boottime()
{
#if ENABLED_BOOTTIME_SUPPORT
  return reset_to_clock_id(CLOCK_BOOTTIME);
#else
  invalidate();
  log_error(TAG, "reset_to_boottime not supported");
  return 2;
#endif
}

int Timestamp::reset_to_realtime()
{
  return reset_to_clock_id(CLOCK_REALTIME);
}

int Timestamp::reset_to_clock_id(const int clock_id)
{
  invalidate();
  if(0 == clock_gettime(clock_id, &m_timestamp))
  {
    m_clock_id = clock_id;
    m_is_valid = true;
    return 0;
  }
  log_error(TAG, "reset_to_clock_id failed: id(%d) errno(%d)(%s)", clock_id, errno, strerror(errno));
  return 1;
}

Timestamp Timestamp::operator -(const TimeDiff & rhs) const
{
  Timestamp past(false);
  int result = 1;
  do
  {
    if(!m_is_valid)
    {
      result = 2;
      exit(-1);
    }

    if(!rhs.is_valid())
    {
      result = 3;
      exit(-1);
    }

    const timespec * rhs_ptr = rhs.getTimeDiffPtr();

    past.m_timestamp.tv_sec = m_timestamp.tv_sec - rhs_ptr->tv_sec;
    past.m_timestamp.tv_nsec = m_timestamp.tv_nsec - rhs_ptr->tv_nsec;

    while (past.m_timestamp.tv_nsec < 0)
    {
      --past.m_timestamp.tv_sec;
      past.m_timestamp.tv_nsec += NANO_PER_SEC;
    }
    while (past.m_timestamp.tv_nsec >= NANO_PER_SEC)
    {
      ++past.m_timestamp.tv_sec;
      past.m_timestamp.tv_nsec -= NANO_PER_SEC;
    }

    if(past.m_timestamp.tv_sec < 0)
    {
      log_error(TAG, "diff is returning -ve value possible time rollover %d", past.m_timestamp.tv_sec);
    }

    past.m_is_valid = true;
    past.m_clock_id = m_clock_id;

    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error(TAG, "operator - diff failed %d", result);
  }
  return past;
}

Timestamp Timestamp::operator +(const TimeDiff & rhs) const
{
  Timestamp sum(false);
  int result = 1;
  do
  {
    if(!m_is_valid)
    {
      result = 2;
      exit(-1);
    }

    if(!rhs.is_valid())
    {
      result = 3;
      exit(-1);
    }

    const timespec * rhs_ptr = rhs.getTimeDiffPtr();

    sum.m_timestamp.tv_sec = m_timestamp.tv_sec + rhs_ptr->tv_sec;
    sum.m_timestamp.tv_nsec = m_timestamp.tv_nsec + rhs_ptr->tv_nsec;

    while (sum.m_timestamp.tv_nsec >= NANO_PER_SEC)
    {
      ++sum.m_timestamp.tv_sec;
      sum.m_timestamp.tv_nsec -= NANO_PER_SEC;
    }
    sum.m_is_valid = true;
    sum.m_clock_id = m_clock_id;

    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error(TAG, "operator + failed %d", result);
  }
  return sum;
}

TimeDiff Timestamp::operator -(const Timestamp & rhs) const
{
  // reset to invalid
  TimeDiff delta(false);
  int result = 1;
  do
  {
    if(!m_is_valid)
    {
      result = 2;
      exit(-1);
    }

    if(!rhs.is_valid())
    {
      result = 3;
      exit(-1);
    }

    if(m_clock_id != rhs.m_clock_id)
    {
      // must be the same clock !
      result = 4;
      log_error(TAG, "operator - ts: my clock id: %d, his clock id: %d", m_clock_id, rhs.m_clock_id);
      exit(-1);
    }

    struct timespec time_delta = {};
    time_delta.tv_sec = m_timestamp.tv_sec - rhs.m_timestamp.tv_sec;
    time_delta.tv_nsec = m_timestamp.tv_nsec - rhs.m_timestamp.tv_nsec;

    while (time_delta.tv_nsec < 0)
    {
      --time_delta.tv_sec;
      time_delta.tv_nsec += NANO_PER_SEC;
    }

    while (time_delta.tv_nsec >= NANO_PER_SEC)
    {
      ++time_delta.tv_sec;
      time_delta.tv_nsec -= NANO_PER_SEC;
    }

    if(time_delta.tv_sec < 0)
    {
      log_error(TAG, "operator - ts returning -ve value probable time rollover: sec:%d/%d/%d, nsec:%d/%d/%d", (int) m_timestamp.tv_sec,
          (int) rhs.m_timestamp.tv_sec, (int) time_delta.tv_sec, (int) m_timestamp.tv_nsec,
          (int) rhs.m_timestamp.tv_nsec, (int) time_delta.tv_nsec);
    }

    // set the diff to valid now
    delta.reset(true);
    BREAK_IF_NON_ZERO(10, delta.add_sec(time_delta.tv_sec));
    BREAK_IF_NON_ZERO(11, delta.add_nsec(time_delta.tv_nsec));

    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error(TAG, "operator - ts failed %d", result);
  }
  return delta;
}

bool Timestamp::operator >=(const Timestamp & rhs) const
{
  bool is_greater_then = false;
  int result = 1;
  do
  {
    if(!m_is_valid)
    {
      result = 2;
      exit(-1);
    }

    if(!rhs.is_valid())
    {
      result = 3;
      exit(-1);
    }

    if(m_clock_id != rhs.m_clock_id)
    {
      // must be the same clock !
      result = 4;
      log_error(TAG, "operator >=: my clock id: %d, his clock id: %d", m_clock_id, rhs.m_clock_id);
      exit(-1);
    }

    struct timespec time_delta = {};
    time_delta.tv_sec = m_timestamp.tv_sec - rhs.m_timestamp.tv_sec;
    time_delta.tv_nsec = m_timestamp.tv_nsec - rhs.m_timestamp.tv_nsec;

    while (time_delta.tv_nsec < 0)
    {
      --time_delta.tv_sec;
      time_delta.tv_nsec += NANO_PER_SEC;
    }

    while (time_delta.tv_nsec >= NANO_PER_SEC)
    {
      ++time_delta.tv_sec;
      time_delta.tv_nsec -= NANO_PER_SEC;
    }

    if(time_delta.tv_sec < 0)
    {
      is_greater_then = false;
    }
    else
    {
      is_greater_then = true;
    }

    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error(TAG, "operator >= failed %d", result);
  }
  return is_greater_then;
}

bool Timestamp::operator <(const Timestamp & rhs) const
{
  return !operator >=(rhs);
}

Timestamp & Timestamp::operator =(const Timestamp & rhs)
{
  m_is_valid = rhs.m_is_valid;
  m_clock_id = rhs.m_clock_id;
  m_timestamp = rhs.m_timestamp;
  return *this;
}

int Timestamp::insert_into_postcard(OutPostcard * const dest_card, const char * const name_str)
{
  OutPostcard * nest_card = 0;
  int result = 1;
  do
  {
    BREAK_IF_ZERO(2, name_str);

    nest_card = OutPostcard::createInstance();
    BREAK_IF_ZERO(3, nest_card);

    if(!m_is_valid)
    {
      result = 4;
      break;
    }

    BREAK_IF_NON_ZERO(10, nest_card->init());
    // note that CLOCK_ID is system-specific. For example, you cannot find clock id concept in Java
    // on Android, there is SystemClock.elapsedRealtimeNanos which seemingly returns CLOCK_BOOTTIME,
    // but that is not really documented, so clock id becomes undefined in Java world anywhere
    // so, timestamp doesn't really work if your message has any chance of being interpreted from
    // the Java world (of Android)
    BREAK_IF_NON_ZERO(11, nest_card->addInt32("CLOCK_ID", m_clock_id));
    BREAK_IF_NON_ZERO(12, nest_card->addInt32("TS_SEC", (OutPostcard::INT32 )m_timestamp.tv_sec));
    BREAK_IF_NON_ZERO(13, nest_card->addInt32("TS_NSEC", (OutPostcard::INT32 )m_timestamp.tv_nsec));
    BREAK_IF_NON_ZERO(14, nest_card->finalize());
    BREAK_IF_NON_ZERO(20, dest_card->addCard(name_str, nest_card));

    result = 0;
  } while (0);

  delete nest_card;
  nest_card = 0;

  if(0 != result)
  {
    log_error(TAG, "insert_into_postcard failed %d", result);
  }
  return result;
}

int Timestamp::retrieve_from_postcard(InPostcard * const src_card, const char * const name_str)
{
  InPostcard * nest_card = 0;
  int result = 1;
  do
  {
    invalidate();

    BREAK_IF_ZERO(2, name_str);
    BREAK_IF_ZERO(3, src_card);

    BREAK_IF_NON_ZERO(10, src_card->getCard(name_str, &nest_card));

    BREAK_IF_NON_ZERO(10, nest_card->getInt32("CLOCK_ID", m_clock_id));
    int ts_sec = 0;
    BREAK_IF_NON_ZERO(11, nest_card->getInt32("TS_SEC", ts_sec));
    m_timestamp.tv_sec = ts_sec;
    int ts_nsec = 0;
    BREAK_IF_NON_ZERO(12, nest_card->getInt32("TS_NSEC", ts_nsec));
    m_timestamp.tv_nsec = ts_nsec;

    m_is_valid = true;

    result = 0;
  } while (0);

  delete nest_card;
  nest_card = 0;

  if(0 != result)
  {
    log_error(TAG, "retrieve_from_postcard failed %d", result);
  }
  return result;
}

long long get_time_rtc_ms()
{
  struct timeval  present_time;
  long long       current_time_msec;

  // present time: seconds, and microseconds
  gettimeofday(&present_time, NULL);

  // Calculate absolute expire time (to avoid data overflow)
  current_time_msec = present_time.tv_sec;
  current_time_msec *= 1000;  // convert to milli-seconds

  current_time_msec += (present_time.tv_usec + 500) / 1000;
  return current_time_msec;
}

// Returns monotonic time
long long get_time_monotonic_ms()
{
#ifndef IZAT_OFFLINE
  uint64_t current_time_msec = 0;
  struct timespec tp = {};

  clock_gettime(CLOCK_MONOTONIC, &tp);

  current_time_msec = tp.tv_sec;
  current_time_msec *= 1000;  // convert to milli-seconds

  current_time_msec += ((tp.tv_nsec + 500000) / 1000000);
  return current_time_msec;
#else
  return get_time_rtc_ms();
#endif
}

// Check if rtc time is valid or not
bool is_time_rtc_ms_valid(long long rtc_time_ms)
{
  bool is_valid = false;

  // Only if the timestamp is big enough, we consider it as valid
  if (rtc_time_ms > TIME_RTC_MS_AS_OF_12_1_2014)
  {
    is_valid = true;
  }
  return is_valid;
}

// Returns monotonic time
long long get_time_boot_ms()
{
#ifdef CLOCK_BOOTTIME
  uint64_t current_time_msec = 0;
  struct timespec tp = {};

  clock_gettime(CLOCK_BOOTTIME, &tp);

  current_time_msec = tp.tv_sec;
  current_time_msec *= 1000;  // convert to milli-seconds

  current_time_msec += ((tp.tv_nsec + 500000) / 1000000);
  return current_time_msec;
#else
  return get_time_rtc_ms();
#endif
}

} // namespace qc_loc_fw

