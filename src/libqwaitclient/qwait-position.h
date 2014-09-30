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
#ifndef LIBQWAITCLIENTS_QWAIT_POSITION_H
#define LIBQWAITCLIENTS_QWAIT_POSITION_H


#include "json.h"

#include <time.h>
#include <stdio.h>


/**
 * An entry in a queue
 */
typedef struct libqwaitclients_qwait_position
{
  /**
   * Where the student is sitting
   */
  char* restrict position;
  
  /**
   * Comment left by the student, such as presentation
   * or request for help and which exercise it entry
   * concerns
   */
  char* restrict comment;
  
  /**
   * The user ID, that unreadable 8-character [0-9a-z]
   * string starting with "u1"
   */
  char* restrict user_id;
  
  /**
   * The student's real name
   */
  char* restrict real_name;
  
  /**
   * The wall-clock time the student entered the queue,
   * in POSIX time (whole seconds)
   */
  time_t start_time_seconds;
  
  /**
   * Millisecond counterpart to `start_time_seconds`
   */
  int start_time_mseconds;
  
} libqwaitclients_qwait_position_t;



#define  _this_  libqwaitclient_qwait_position_t* restrict this


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
int libqwaitclient_qwait_position_parse(_this_, const libqwaitclients_json_t* restrict data);

/**
 * Compares the time of entry for two queue entries
 * 
 * @param   a  -1 is returned if this entry entered the queue before `b`
 * @param   b  1 is returned if this entry entered the queue before `a`
 * @return     See `a` and `b`, and refer to `qsort(3)`, `strcmp(3)`, et cetera; earlier entries first
 */
int libqwaitclient_qwait_position_compare_by_time(const void* a, const void* b);

/**
 * Print a queue entry to a file for debugging
 * 
 * @param  this    The queue entry
 * @param  output  The output sink
 */
void libqwaitclient_qwait_position_dump(_this_, FILE* output);


#undef __this__


#endif

