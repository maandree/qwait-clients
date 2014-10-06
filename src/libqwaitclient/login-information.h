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
#ifndef LIBQWAITCLIENT_LOGIN_INFORMATION_H
#define LIBQWAITCLIENT_LOGIN_INFORMATION_H


#include "qwait-user.h"

#define _GNU_SOURCE
#include <stddef.h>



/**
 * User login information
 */
typedef struct libqwaitclient_login_information
{
  /**
   * The user information for the logged in used,
   * if the user is not logged in we expect the
   * user ID and the user's real name to be null,
   * administrator status set to false, anonymous
   * status set to true, and the role to be
   * "ROLE_ANONYMOUS"
   * 
   * `owned_queue_count`, `moderated_queue_count`
   * and `queue_count` will always be zero
   */
  libqwaitclient_qwait_user_t current_user;
  
  /**
   * The user's reverse DNS address
   */
  char* hostname;
  
  /**
   * The server-side product
   */
  struct
  {
    /**
     * The name of the program the server is running,
     * we expect this to be "QWait"
     */
    char* name;
    
    /**
     * The version of the server
     */
    char* version;
    
  } product;
  
} libqwaitclient_login_information_t;



#define _this_  libqwaitclient_login_information_t* restrict this


/**
 * Initialise a login information structure
 * 
 * @param  this  The login information that shall be initialised
 */
void libqwaitclient_login_information_initialise(_this_);

/**
 * Release information in a login information structure,
 * but do not free the structure itself
 * 
 * @param  this  The login information
 */
void libqwaitclient_login_information_destroy(_this_);

/**
 * Parse login information for a HTTP response
 * 
 * @param   this            Output parameter for the login information
 * @param   message         The message content, not NUL-terminated,
 *                          this string's content will be modified
 * @param   message_length  The length of `message`
 * @return                  Zero on success, -1 on error
 */
int libqwaitclient_login_information_parse(_this_, char* restrict message, size_t message_length);


#undef _this_

#endif

