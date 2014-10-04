/**
 * qwait-clients — Non-web clients for QWait
 * Copyright © 2014  Mattias Andrée (maandree@member.fsf.org)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "qwait-position.h"

#include "macros.h"
#include "json.h"

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>


#define  _this_  libqwaitclient_qwait_position_t* restrict this
#define  _time_  libqwaitclient_qwait_position_time_t* restrict time


/**
 * Three-letter month names
 */
static const char* months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
				"Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

/**
 * Three-letter names of days of the week
 */
static const char* wdays[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Say", "Sun" };


/**
 * Initialises a queue entry
 * 
 * @param  this  The queue entry
 */
void libqwaitclient_qwait_position_initialise(_this_)
{
  memset(this, 0, sizeof(libqwaitclient_qwait_position_t));
}


/**
 * Releases all resources a queue entry, but not the entry itself
 * 
 * @param  this  The queue entry
 */
void libqwaitclient_qwait_position_destroy(_this_)
{
  free(this->location);
  free(this->comment);
  free(this->user_id);
  free(this->real_name);
  memset(this, 0, sizeof(libqwaitclient_qwait_position_t));
}


/**
 * Contextually parses parsed JSON data into a queue entry
 * 
 * @param   this  The queue entry to fill in
 * @param   data  The data to parse
 * @return        Zero on success, -1 on error
 */
int libqwaitclient_qwait_position_parse(_this_, const libqwaitclient_json_t* restrict data)
{
  const libqwaitclient_json_t* restrict data_location   = NULL;
  const libqwaitclient_json_t* restrict data_comment    = NULL;
  const libqwaitclient_json_t* restrict data_user_id    = NULL;
  const libqwaitclient_json_t* restrict data_real_name  = NULL;
  const libqwaitclient_json_t* restrict data_enter_time = NULL;
  size_t i, n = data->length;
  int saved_errno;
  
  if (data->type != LIBQWAITCLIENT_JSON_TYPE_OBJECT)
    return errno = EINVAL, -1;
  
#define test(want)      ((strlen(want) == len) && !memcmp(name, want, len * sizeof(char)))
#define str(var, have)  ((have->type == LIBQWAITCLIENT_JSON_TYPE_NULL) ?	\
			  (var = NULL, 0) :					\
			  (var = libqwaitclient_json_to_zstr(have), var == NULL))
  
  /* Read information. */
  for (i = 0; i < n; i++)
    {
      const libqwaitclient_json_t* restrict value = &(data->data.object[i].value);
      char* name = data->data.object[i].name;
      size_t len = data->data.object[i].name_length;
      
      if      (test("location"))      data_location   = value;
      else if (test("comment"))       data_comment    = value;
      else if (test("userName"))      data_user_id    = value;
      else if (test("readableName"))  data_real_name  = value;
      else if (test("startTime"))     data_enter_time = value;
      else
	goto einval;
    }
  
  /* Check that everything was found. */
  if (data_location   == NULL)  goto einval;
  if (data_comment    == NULL)  goto einval;
  if (data_user_id    == NULL)  goto einval;
  if (data_real_name  == NULL)  goto einval;
  if (data_enter_time == NULL)  goto einval;
  
  /* Evaluate data. */
  if (str(this->location,  data_location))    goto fail;
  if (str(this->comment,   data_comment))     goto fail;
  if (str(this->user_id,   data_user_id))     goto fail;
  if (str(this->real_name, data_real_name))   goto fail;
  if (data_enter_time->type != LIBQWAITCLIENT_JSON_TYPE_INTEGER)
    goto einval;
  this->enter_time_seconds = (time_t)(data_enter_time->data.integer / 1000);
  this->enter_time_mseconds   = (int)(data_enter_time->data.integer % 1000);
  
#undef str
#undef test
  
  return 0;
  
 einval:
  errno = EINVAL;
 fail:
  saved_errno = errno;
  libqwaitclient_qwait_position_destroy(this);
  return errno = saved_errno, -1;
}


/**
 * Compares the time of entry for two queue entries
 * 
 * @param   a  -1 is returned if this entry entered the queue before `b`
 * @param   b  1 is returned if this entry entered the queue before `a`
 * @return     See `a` and `b`, and refer to `qsort(3)`, `strcmp(3)`, et cetera; earlier entries first
 */
int libqwaitclient_qwait_position_compare_by_time(const void* a, const void* b)
{
  const libqwaitclient_qwait_position_t* a_ = a;
  const libqwaitclient_qwait_position_t* b_ = b;
  
#define cmp(p, q)  ((p) < (q) ? -1 : (p) > (q) ? 1 : 0)
  
  if (a_->enter_time_seconds == b_->enter_time_seconds)
    return cmp(a_->enter_time_mseconds, b_->enter_time_mseconds);
  return cmp(a_->enter_time_seconds, b_->enter_time_seconds);
  
#undef cmp
}


/**
 * Print a queue entry to a file for debugging
 * 
 * @param  this    The queue entry
 * @param  output  The output sink
 */
void libqwaitclient_qwait_position_dump(const _this_, FILE* output)
{
  libqwaitclient_qwait_position_time_t enter_time;
  libqwaitclient_qwait_position_time_t enter_diff;
  char* str_time = NULL;
  char* str_diff = NULL;
  
  if (!libqwaitclient_qwait_position_parse_time(this, &enter_time, 1))
    str_time = libqwaitclient_qwait_position_string_time(&enter_time, 0);
  if (!libqwaitclient_qwait_position_diff_time(this, &enter_diff, NULL))
    str_diff = libqwaitclient_qwait_position_string_time(&enter_diff, 0);
  
  fprintf(output, "\"%s\"(%s) @ %s: %s, entered %ji.%03i (%s; %s)\n",
	  this->real_name, this->user_id, this->location, this->comment,
	  (intmax_t)(this->enter_time_seconds), this->enter_time_mseconds,
	  str_time, str_diff);
  
  free(str_time);
  free(str_diff);
}


/**
 * Get the time a entry was added to its queue
 * 
 * @param   this   The queue entry
 * @param   time   Output parameter for when the entry was added to the queue
 * @param   local  Whether to return in local time rather than UTC
 * @return         Zero on success, -1 on error
 */
int libqwaitclient_qwait_position_parse_time(const _this_, _time_, int local)
{
  /* We assume that the server is correct and that the time is positive,
   * and that the year is at least 2001. */
  
  time_t s = this->enter_time_seconds;
  int is_leap;
  
  time->is_difference = 0;
  
  /* Zero initialise timezone. */
  memset(time->timezone, 0, sizeof(time->timezone));
  time->sign = 0;
  time->timezone_h = 0;
  time->timezone_m = 0;
  
  if (!local)
    sprintf(time->timezone, "UTC");
  else
    {
      struct timeval _time; /* First argument of gettimeofday must not be `NULL`. */
      struct timezone tz_;
      int tz;
      
      if (gettimeofday(&_time, &tz_) < 0)
	return -1;
      tz = -(tz_.tz_minuteswest);
      tzset();
      sprintf(time->timezone, "%s", tzname[((-timezone) / 60 == tz) ? 0 : 1]);
      /* Someone choose to invert it, probably because it is faster
         to do TIME - TZ than TIME + TZ to get the local time.
         (Or because they are America-centric, just like when having
	 Sunday be the first day of the week.) */
      
      /* Timezone offset */
      tz *= time->sign = tz < 0 ? -1 : tz > 0 ? 1 : 0;
      time->timezone_h = (unsigned)tz / 60;
      time->timezone_m = (unsigned)tz % 60;
      
      /* Add the offset to the time, we want it to be local. */
      s += time->sign * (signed)(time->timezone_m) * 60;
      s += time->sign * (signed)(time->timezone_h) * 60 * 60;
    }
  
  /* The time of the day. */
  time->msec = (unsigned)(this->enter_time_mseconds);
  time->sec  = (unsigned)(s % 60), s /= 60;
  time->min  = (unsigned)(s % 60), s /= 60;
  time->hour = (unsigned)(s % 24), s /= 24;
  
  /* Epoch translation, from 1970-01-01 to 2001-01-01. */
  s -= 978307200L / (24L * 60L * 60L);
  time->year = 2001;
  
  /* 2001-01-01 is an awesome epoch: trivial day of the week calculation. */
  time->wday = (unsigned)(s % 7);
  /* Side note: 0001-01-01 was a Monday, we know this because 2001-01-01
                was a Monday and 365,2425 ⋅ 2000 ≡ 0 (Mod 7). */
  
  /* And simple fast year calculation. */
  time->year += (signed)(s / 146097) * 400, s %= 146097;
  time->year += (signed)(s /  36524) * 100, s %=  36524;
  time->year += (signed)(s /   1461) *   4, s %=   1461;
  time->year += (signed)(s /    365) *   1, s %=    365;
  
  /* If we are on a leapyear, February is one day longer. */
  is_leap = ((time->year % 4) == 0) && ((time->year % 100) != 0);
  is_leap |= (time->year % 400) == 0;
  is_leap = is_leap ? 1 : 0;
  
  /* Calculate month and day by checking the day of the year. */
#define M(m)  (time_t)(								\
               (m > 0 ? 31 : 0) + (m > 1 ? 28 : 0) + (m > 2 ? 31 : 0) +		\
               (m > 3 ? 30 : 0) + (m > 4 ? 31 : 0) + (m > 5 ? 30 : 0) +		\
               (m > 6 ? 31 : 0) + (m > 7 ? 31 : 0) + (m > 8 ? 30 : 0) +		\
	       (m > 9 ? 31 : 0) + (m > 10 ? 30 : 0) + (m > 1 ? is_leap : 0))
  if      (s <  M(1))  time->month =  1, time->day = (unsigned)(s -  M(0) + 1);
  else if (s <  M(2))  time->month =  2, time->day = (unsigned)(s -  M(1) + 1);
  else if (s <  M(3))  time->month =  3, time->day = (unsigned)(s -  M(2) + 1);
  else if (s <  M(4))  time->month =  4, time->day = (unsigned)(s -  M(3) + 1);
  else if (s <  M(5))  time->month =  5, time->day = (unsigned)(s -  M(4) + 1);
  else if (s <  M(6))  time->month =  6, time->day = (unsigned)(s -  M(5) + 1);
  else if (s <  M(7))  time->month =  7, time->day = (unsigned)(s -  M(6) + 1);
  else if (s <  M(8))  time->month =  8, time->day = (unsigned)(s -  M(7) + 1);
  else if (s <  M(9))  time->month =  9, time->day = (unsigned)(s -  M(8) + 1);
  else if (s < M(10))  time->month = 10, time->day = (unsigned)(s -  M(9) + 1);
  else if (s < M(11))  time->month = 11, time->day = (unsigned)(s - M(10) + 1);
  else                 time->month = 12, time->day = (unsigned)(s - M(11) + 1);
#undef M
  
  return 0;
}


/**
 * Calculate how long ago a entry was added to its queue
 * 
 * @param   this  The queue entry
 * @param   time  Output parameter for how long ago the entry was added to the queue
 * @param   now   Please set to `&t` where `t` is set by `clock_gettime(CLOCK_REALTIME, &t)`
 * @return        Zero on success, -1 on error
 */
int libqwaitclient_qwait_position_diff_time(const _this_, _time_, const struct timespec* restrict now)
{
  struct timespec now_;
  time_t s;
  int ms;
  
  if (now == NULL)
    {
      if (clock_gettime(CLOCK_REALTIME, &now_) < 0)
	return -1;
      now = &now_;
    }
  
  s = now->tv_sec - this->enter_time_seconds;
  ms = (int)((now->tv_nsec + 500000L) / 1000000L) - this->enter_time_mseconds;
  
  time->is_difference = 1,  time->sign =  s < 0 ? -1 :  s > 0 ? 1 : 0;
  if (time->sign == 0)      time->sign = ms < 0 ? -1 : ms > 0 ? 1 : 0;
  if (time->sign < 0)
    s = -s, ms = -ms;
  
  if (ms > 0)  s += 1, ms -= 1000;
  if (ms < 0)  s -= 1, ms += 1000;
  
  time->msec = (unsigned)ms;
  time->sec  = (unsigned)(s % 60), s /= 60;
  time->min  = (unsigned)(s % 60), s /= 60;
  time->hour = (unsigned)(s % 24), s /= 24;
  time->day  = (unsigned)s;
  
  return 0;
}


/**
 * Make a coarse human-readable string of the time created by
 * `libqwaitclient_qwait_position_parse_time` or `libqwaitclient_qwait_position_diff_time`
 * 
 * @param   time      The time the entry was added to the queue or how long ago that was
 * @return            The time information as a free:able string, `NULL` on error
 */
static char* libqwaitclient_qwait_position_coarse_string_time(const _time_)
{
  char* buf;
  if (xmalloc(buf, 16 + 3 * sizeof(unsigned), char)) /* a bit excessive */
    return NULL;
  
  if (time->is_difference == 0)
    {
      sprintf(buf, "%u %s %02u:%02u", time->day, months[time->month - 1], time->hour, time->min);
      return buf;
    }
  else if (time->day  == 1)  sprintf(buf,  "1 day");
  else if (time->day  >= 2)  sprintf(buf, "%u days", time->day);
  else if (time->hour == 1)  sprintf(buf,  "1 hour");
  else if (time->hour >= 2)  sprintf(buf, "%u hours", time->hour);
  else if (time->min  == 1)  sprintf(buf,  "1 minute");
  else if (time->min  >= 2)  sprintf(buf, "%u minutes", time->min);
  else if (time->sec  >= 5)  sprintf(buf, "%u seconds", time->sec);
  else
    {
      sprintf(buf, "Now");
      return buf;
    }
  
  if (time->sign < 0)
    {
      memmove(buf + 3, buf, (strlen(buf) + 1) * sizeof(char));
      memcpy(buf, "In ", strlen("In ") * sizeof(char));
    }
  
  return buf;
}


/**
 * Make a detailed human-readable string of the time created by
 * `libqwaitclient_qwait_position_parse_time` or `libqwaitclient_qwait_position_diff_time`
 * 
 * @param   time      The time the entry was added to the queue or how long ago that was
 * @return            The time information as a free:able string, `NULL` on error
 */
static char* libqwaitclient_qwait_position_detailed_string_time(const _time_)
{
  char* buf;
  if (xmalloc(buf, 60 + 3 * sizeof(unsigned), char)) /* a bit excessive */
    return NULL;
  
  if (time->is_difference == 0)
    {
      sprintf(buf, "%i-(%02u)%s-%02u %02u:%02u:%02u.%03u %s (UTC%s%02u%02u), %s",
	      time->year, time->month, months[time->month - 1], time->day,
	      time->hour, time->min, time->sec, time->msec, time->timezone,
	      time->sign < 0 ? "-" : "+", time->timezone_h, time->timezone_m,
	      wdays[time->wday]);
      return buf;
    }
  
  if (time->sign == 0)
    return sprintf(buf, "Now"), buf;
  
  *buf = 0;
  
  if (time->sign < 0)
    sprintf(buf + strlen(buf), "In ");
  
  if (time->day == 1)  sprintf(buf + strlen(buf),  "1 day, ");
  if (time->day >= 2)  sprintf(buf + strlen(buf), "%u days, ", time->day);
  
#define z(x)  (time->x == 0)
#define o(x)  (time->x == 1)
  
  if (time->day || time->hour)
    sprintf(buf,  "%u:%02u:%02u.%03u %s",
	    time->hour, time->min, time->sec, time->msec,
	    (o(hour) && z(min) && z(sec) && z(msec)) ? "hour" : "hours");
  else if (time->min)
    sprintf(buf,  "%u:%02u.%03u %s",
	    time->min, time->sec, time->msec,
	    (o(min) && z(sec) && z(msec)) ? "minute" : "minutes");
  else
    sprintf(buf,  "%u.%03u %s",
	    time->sec, time->msec,
	    (o(sec) && z(msec)) ? "second" : "seconds");
  
#undef z
#undef o
  
  return buf;
}


/**
 * Make a human-readable string of the time created by
 * `libqwaitclient_qwait_position_parse_time` or `libqwaitclient_qwait_position_diff_time`
 * 
 * @param   time      The time the entry was added to the queue or how long ago that was
 * @param   detailed  Whether to result should be detailed
 * @return            The time information as a free:able string, `NULL` on error
 */
char* libqwaitclient_qwait_position_string_time(const _time_, int detailed)
{
  if (detailed)
    return libqwaitclient_qwait_position_detailed_string_time(time);
  else
    return libqwaitclient_qwait_position_coarse_string_time(time);
}


#undef _time_
#undef _this_

