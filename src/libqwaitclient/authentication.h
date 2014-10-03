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
 * Add authentication tokens to a message
 * 
 * @param   mesg         The message to which to add authentication
 * @param   data         The authentication data
 * @param   data_length  The length of `data`
 * @return               Zero on possible success, -1 on definite error
 */
int libqwaitclient_auth_sign(libqwaitclient_http_message_t* restrict mesg,
			     const char* restrict data, size_t data_length);


#endif

