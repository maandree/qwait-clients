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
#ifndef LIBQWAITCLIENT_AUTHENTICATION_H
#define LIBQWAITCLIENT_AUTHENTICATION_H


#include "http-message.h"

#define _GNU_SOURCE
#include <stddef.h>
#include <stdio.h>



/**
 * Message authentication data
 */
typedef struct libqwaitclient_authentication {
  
  /**
   * Header to include in the message
   */
  char** headers;
  
  /**
   * The number of elements in `header`
   */
  size_t header_count;
  
} libqwaitclient_authentication_t;


#define _this_  libqwaitclient_authentication_t* restrict this


/**
 * Initialises authentication data
 * 
 * @param  this  The authentication data
 */
void libqwaitclient_authentication_initialise(_this_);

/**
 * Releases all resources authentication data, but not the structure itself
 * 
 * @param  this  The authentication data
 */
void libqwaitclient_authentication_destroy(_this_);

/**
 * Get parsed authentication data for messages
 * 
 * @param   this         Output parameter for the parsed authentication data
 * @param   data         The authentication data
 * @param   data_length  The length of `data`
 * @return               Zero on possible success, -1 on definite error
 */
int libqwaitclient_authentication_get(_this_, const char* restrict data, size_t data_length);

/**
 * Print authentication data to a file for debugging
 * 
 * @param  this    The authentication data
 * @param  output  The output sink
 */
void libqwaitclient_authentication_dump(const _this_, FILE* output);

/**
 * Add authentication tokens to a message
 * 
 * @param   this  The authentication data, may be `NULL`
 * @param   mesg  The message to which to add authentication
 * @return        Zero on success, -1 on error (assuming success of `libqwaitclient_authentication_get`)
 */
int libqwaitclient_auth_sign(const _this_, libqwaitclient_http_message_t* restrict mesg);

/**
 * Perform a login
 * 
 * @param   username     The user's username
 * @param   password     The user's password
 * @param   data         Output parameter for authentication data, free even on failure
 * @param   data_length  Output parameter for the length of `*data`
 * @return               Zero on success, -1 on error, 1 if login failed
 */
int libqwaitclient_auth_log_in(const char* restrict username, const char* restrict password,
			       char** restrict data, size_t* restrict data_length);

/**
 * Request a server-side logout
 * 
 * @param   data         The authentication data
 * @param   data_length  The length of `data`
 * @return               Zero on possible success, -1 on definite error
 */
int libqwaitclient_auth_log_out(const char* restrict data, size_t data_length);

/**
 * Get the user's ID
 * 
 * @param   user_id  Output parameter for the user ID
 * @return           Zero on success, -1 on error, 1 if the user does not
 *                   have a passwd entry, 2 if the user does not have a
 *                   home directory in the passwd entry, 3 if the user
 *                   is not using a university computer
 */
int libqwaitclient_auth_user_id(char** restrict user_id);


#undef _this_

#endif

