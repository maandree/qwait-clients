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
#ifndef LIBQWAITCLIENT_QWAIT_USER_H
#define LIBQWAITCLIENT_QWAIT_USER_H


#include "json.h"
#include "qwait-position.h"

#define _GNU_SOURCE
#include <stddef.h>


/**
 * A user
 */
typedef struct libqwaitclient_qwait_user
{
  /**
   * The user's ID
   */
  char* user_id;
  
  /**
   * The user's name
   */
  char* real_name;
  
  /**
   * Whether the user is an administrator
   */
  int admin;
  
  /**
   * Whether the user is anonymous
   */
  int anonymous;
  
  /**
   * List of roles the user has
   * 
   * Possible roles:
   * - user:   Included for everyone(?)
   * - admin:  Included for and only for those with `admin` set
   */
  char** roles;
  
  /**
   * The number of elements in `roles`
   */
  size_t role_count;
  
  /**
   * List of queues that the user owns
   */
  char** owned_queues;
  
  /**
   * The number of elements in `owned_queues`
   */
  size_t owned_queue_count;
  
  /**
   * List of queues that the user moderates
   */
  char** moderated_queues;
  
  /**
   * The number of elements in `moderated_queues`
   */
  size_t moderated_queue_count;
  
  /**
   * Entries in the queues that the user holds
   * 
   * Do not destroy these yourself
   */
  libqwaitclient_qwait_position_t* positions;
  
  /**
   * Corresponding queue names for elements in `positions`
   */
  char** queues;
  
  /**
   * The number of elements in `positions` and `queues`
   */
  size_t queue_count;
  
} libqwaitclient_qwait_user_t;



#define  _this_  libqwaitclient_qwait_user_t* restrict this


/**
 * Initialises a user
 * 
 * @param  this  The user
 */
void libqwaitclient_qwait_user_initialise(_this_);

/**
 * Destroy a user (release its resources but do not free it)
 * 
 * @param  this  The user
 */
void libqwaitclient_qwait_user_destroy(_this_);

/**
 * Contextually parses parsed JSON data into a user
 * 
 * @param   this  The user to fill in
 * @param   data  The data to parse
 * @return        Zero on success, -1 on error
 */
int libqwaitclient_qwait_user_parse(_this_, const libqwaitclient_json_t* restrict data);

/**
 * Print a user to a file for debugging
 * 
 * @param  this    The user
 * @param  output  The output sink
 */
void libqwaitclient_qwait_user_dump(const _this_, FILE* output);


#undef _this_


#endif

