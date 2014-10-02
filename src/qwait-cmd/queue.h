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
#ifndef QWAIT_CMD_QUEUE_H
#define QWAIT_CMD_QUEUE_H


#include <libqwaitclient.h>


/**
 * Print entries of a queue
 * 
 * @param   sock        A socket that is connected to the qwait server
 * @param   queue_name  The name of the queue
 * @return              Zero on success, -1 on error
 */
int print_queue(libqwaitclient_http_socket_t* restrict sock, const char* restrict queue_name);


/**
 * Find the 0-based position in a queue for a student,
 * that is, the number of students before that student
 * 
 * @param   sock        A socket that is connected to the qwait server
 * @param   queue_name  The name of the queue
 * @param   user_id     The user's ID
 * @return              Zero on success, 1 if not found, -1 on error
 */
int print_queue_position(libqwaitclient_http_socket_t* restrict sock,
			 const char* restrict queue_name, const char* restrict user_id);


#endif

