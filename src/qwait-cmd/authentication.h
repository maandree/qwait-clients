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
#ifndef QWAIT_CMD_AUTHENTICATION_H
#define QWAIT_CMD_AUTHENTICATION_H


#include <libqwaitclient.h>


/**
 * Log in or log out
 * 
 * @param   username  Your username, `NULL` to log out, an empty string can be
 *                    used for your username on the computer you are using
 * @return            Zero on success, -1 on error, 1 if authentication failed
 */
int authenticate(const char* restrict username);

/**
 * Get authentication data
 * 
 * @param   auth  Output parameter for the authentication data
 * @return        Zero on success, -1 on error
 */
int get_authentication(libqwaitclient_authentication_t* restrict auth);

/**
 * Print the user's ID
 * 
 * @return  Zero on success, -1 on error
 */
int print_user_id(void);


#endif

