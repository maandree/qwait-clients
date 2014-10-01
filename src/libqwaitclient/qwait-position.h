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
#ifndef LIBQWAITCLIENT_QWAIT_POSITION_H
#define LIBQWAITCLIENT_QWAIT_POSITION_H


#include "json.h"

#define _GNU_SOURCE
#include <time.h>
#include <stdio.h>


/**
 * An entry in a queue
 */
typedef struct libqwaitclient_qwait_position
{
  /**
   * Where the student is sitting
   */
  char* location;
  
  /**
   * Comment left by the student, such as presentation
   * or request for help and which exercise it entry
   * concerns
   */
  char* comment;
  
  /**
   * The user ID, that unreadable 8-character [0-9a-z]
   * string starting with "u1"
   */
  char* user_id;
  
  /**
   * The student's real name
   */
  char* real_name;
  
  /**
   * The wall-clock time the student entered the queue,
   * in POSIX time (whole seconds)
   */
  time_t enter_time_seconds;
  
  /**
   * Millisecond counterpart to `enter_time_seconds`
   */
  int enter_time_mseconds;
  
} libqwaitclient_qwait_position_t;


/**
 * Parsed enter-time time for a queue entry
 */
typedef struct libqwaitclient_qwait_position_time
{
  /**
   * Non-zero if parsed as a time difference,
   * zero if parsed as a wall-clock time
   */
  int is_difference;
  
  /**
   * The timezone acronym,
   * undefined if parsed as a time difference
   */
  char timezone[11];
  
  /**
   * If parsed as wall-clock time:
   *   -1 if the timezone is negative,
   *   1 if the timezone is positive,
   *   0 if the timezone is UTC
   * 
   * If parsed as time difference:
   *   -1 if the timestamp is in the future,
   *   1 if the timestamp is in the past (expected),
   *   0 if the timestamp is right now
   */
  signed sign;
  
  /**
   * The timezone offset hours,
   * undefined if parsed as a time difference
   */
  unsigned timezone_h;
  
  /**
   * The timezone offset minutes,
   * undefined if parsed as a time difference
   */
  unsigned timezone_m;
  
  /**
   * The day of the week,
   * undefined if parsed as a time difference
   * 
   * 0 = Monday, 6 = Sunday
   */
  unsigned wday;
  
  /**
   * The year (exactly, however hypothetically, x + 1, if x < 0,
   * where x is the year), undefined if parsed as a time difference
   */
  signed year;
  /* Overflows the year 32768 if `signed` is 16 bits which is its minimum. */
  
  /**
   * The month, 1 based,
   * undefined if parsed as a time difference
   */
  unsigned month;
  
  /**
   * The day, 1 based, if parsed as a wall-clock time
   * 
   * The number of days, 0 based and unbounded,
   * if parsed as a time difference
   */
  unsigned day;
  /* Overflows for time difference if the  time difference is at least 180 days,
     assuming `unsigned` is 16 bits which is its minimum. */
  
  /**
   * The hour or number of hours modulo 24
   */
  unsigned hour;
  
  /**
   * The minute or number of minute modulo 60
   */
  unsigned min;
  
  /**
   * The second (softly bounded to 60, however the current
   * qwait protocol only permit to 59) or number of seconds
   * modulo 60
   */
  unsigned sec;
  
  /**
   * The millisecond or number of milliseconds modulo 1000
   */
  unsigned msec;
  
} libqwaitclient_qwait_position_time_t;



#define  _this_  libqwaitclient_qwait_position_t* restrict this
#define  _time_  libqwaitclient_qwait_position_time_t* restrict time


/**
 * Initialises a queue entry
 * 
 * @param  this  The queue entry
 */
void libqwaitclient_qwait_position_initialise(_this_);

/**
 * Releases all resources a queue entry, but not the entry itself
 * 
 * @param  this  The queue entry
 */
void libqwaitclient_qwait_position_destroy(_this_);

/**
 * Contextually parses parsed JSON data into a queue entry
 * 
 * @param   this  The queue entry to fill in
 * @param   data  The data to parse
 * @return        Zero on success, -1 on error
 */
int libqwaitclient_qwait_position_parse(_this_, const libqwaitclient_json_t* restrict data);

/**
 * Compares the time of entry for two queue entries
 * 
 * @param   a  -1 is returned if this entry entered the queue before `b`
 * @param   b  1 is returned if this entry entered the queue before `a`
 * @return     See `a` and `b`, and refer to `qsort(3)`, `strcmp(3)`, et cetera; earlier entries first
 */
int libqwaitclient_qwait_position_compare_by_time(const void* a, const void* b) __attribute__((pure));

/**
 * Print a queue entry to a file for debugging
 * 
 * @param  this    The queue entry
 * @param  output  The output sink
 */
void libqwaitclient_qwait_position_dump(const _this_, FILE* output);

/**
 * Get the time a entry was added to its queue
 * 
 * @param   this   The queue entry
 * @param   time   Output parameter for when the entry was added to the queue
 * @param   local  Whether to return in local time rather than UTC
 * @return         Zero on success, -1 on error
 */
int libqwaitclient_qwait_position_parse_time(const _this_, _time_, int local);

/**
 * Calculate how long ago a entry was added to its queue
 * 
 * @param   this  The queue entry
 * @param   time  Output parameter for how long ago the entry was added to the queue
 * @param   now   Please set to `&t` where `t` is set by `clock_gettime(CLOCK_REALTIME, &t)`
 * @return        Zero on success, -1 on error
 */
int libqwaitclient_qwait_position_diff_time(const _this_, _time_, const struct timespec* restrict now);

/**
 * Make a human-readable string of the time created by
 * `libqwaitclient_qwait_position_parse_time` or `libqwaitclient_qwait_position_diff_time`
 * 
 * @param   time  The time the entry was added to the queue or how long ago that was
 * @return        The time information as a free:able string, `NULL` on error
 */
char* libqwaitclient_qwait_position_string_time(const _time_);


#undef _time_
#undef _this_


#endif

