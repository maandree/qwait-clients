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
#ifndef QWAIT_CMD_MISCELLANEOUS_H
#define QWAIT_CMD_MISCELLANEOUS_H


#include <libqwaitclient.h>


#define _sock_  libqwaitclient_http_socket_t* restrict sock


/**
 * Print the user's reverse DNS address
 * 
 * @param   sock  A socket that is connected to the qwait server
 * @return        Zero on success, -1 on error
 */
int print_user_hostname(_sock_);

/**
 * Print the remote server's product name and version
 * 
 * @param   sock  A socket that is connected to the qwait server
 * @return        Zero on success, -1 on error
 */
int print_qwait_version(_sock_);

/**
 * Print user login information
 * 
 * @param   sock  A socket that is connected to the qwait server
 * @return        Zero on success, -1 on error
 */
int print_user_login(_sock_);


#undef _sock_

#endif

