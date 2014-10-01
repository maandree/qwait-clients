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
#ifndef LIBQWAITCLIENT_QWAIT_QUEUE_H
#define LIBQWAITCLIENT_QWAIT_QUEUE_H


#include "json.h"
#include "qwait-position.h"

#include <stddef.h>


/**
 * A queue for a class
 */
typedef struct libqwaitclient_qwait_queue
{
  /**
   * The queue's ID
   */
  char* restrict name;
  
  /**
   * The queue's name
   */
  char* restrict title;
  
  /**
   * Whether the queue is hidden
   */
  int hidden;
  
  /**
   * Whether the queue is locked
   */
  int locked;
  
  /**
   * List of queue owners (user ID:s)
   */
  char* restrict* restrict owners;
  
  /**
   * The number of elements in `owners`
   */
  size_t owner_count;
  
  /**
   * List of queue moderators (user ID:s)
   */
  char* restrict* restrict moderators;
  
  /**
   * The number of elements in `moderators`
   */
  size_t moderator_count;
  
  /**
   * Entries in the queue
   */
  libqwaitclient_qwait_position_t* restrict positions;
  
  /**
   * The number of elements in `positions`
   */
  size_t position_count;
  
} libqwaitclient_qwait_queue_t;



#define  _this_  libqwaitclient_qwait_queue_t* restrict this


/**
 * Initialises a queue
 * 
 * @param  this  The queue
 */
void libqwaitclient_qwait_queue_initialise(_this_);

/**
 * Releases all resources a queue, but not the queue itself
 * 
 * @param  this  The queue
 */
void libqwaitclient_qwait_queue_destroy(_this_);

/**
 * Contextually parses parsed JSON data into a queue
 * 
 * @param   this  The queue to fill in
 * @param   data  The data to parse
 * @return        Zero on success, -1 on error
 */
int libqwaitclient_qwait_queue_parse(_this_, const libqwaitclient_json_t* restrict data);

/**
 * Compares the title of queues
 * 
 * @param   a  -1 is returned if this queue is an alphabetically lower title than `b`
 * @param   b  1 is returned if this queue is an alphabetically lower title than `a`
 * @return     See `a` and `b`, and refer to `qsort(3)`, `strcmp(3)`, et cetera; ascending order
 */
int libqwaitclient_qwait_queue_compare_by_title(const void* a, const void* b);

/**
 * Print a queue to a file for debugging
 * 
 * @param  this    The queue entry
 * @param  output  The output sink
 */
void libqwaitclient_qwait_queue_dump(const _this_, FILE* output);


#undef _this_


#endif

