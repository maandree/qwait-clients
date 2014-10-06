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
#ifndef QWAIT_CMD_USERS_H
#define QWAIT_CMD_USERS_H


#include <libqwaitclient.h>


/**
 * Print all users
 */
#define QWAIT_CMD_USERS_ALL  0

/**
 * Print only QWait administrators
 */
#define QWAIT_CMD_USERS_ADMINS  1


/**
 * Print information about users on the system
 * 
 * @param   sock  A socket that is connected to the qwait server
 * @param   role  Either of: `QWAIT_CMD_USERS_ALL`, `QWAIT_CMD_USERS_ADMINS`
 * @return        Zero on success, -1 on error, 1 if not logged in
 */
int print_users(libqwaitclient_http_socket_t* restrict sock, int role);

/**
 * Print information about users on the system by finding them by their name
 * 
 * @param   sock       A socket that is connected to the qwait server
 * @param   real_name  A string that the must exist in the real name of the printed users
 * @return             Zero on success, -1 on error, 1 if not logged in
 */
int print_users_by_name(libqwaitclient_http_socket_t* restrict sock, const char* restrict real_name);


#endif

