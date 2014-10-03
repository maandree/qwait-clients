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
#ifndef LIBQWAITCLIENT_QWAIT_PROTOCOL_H
#define LIBQWAITCLIENT_QWAIT_PROTOCOL_H


#include "http-socket.h"
#include "qwait-queue.h"
#include "qwait-user.h"

#define _GNU_SOURCE
#include <stddef.h>


#define _sock_   libqwaitclient_http_socket_t* restrict sock
#define _queue_  libqwaitclient_qwait_queue_t* restrict queue
#define _user_   libqwaitclient_qwait_user_t*  restrict user


/**
 * Get complete information on all queues
 * 
 * @param   sock         The socket used to remote communication
 * @param   queue_count  Output parameter for the number of returned queues
 * @return               Information for all queues, `NULL` on error
 */
libqwaitclient_qwait_queue_t* libqwaitclient_qwait_get_queues(_sock_, size_t* restrict queue_count);

/**
 * Get complete information on a queue
 * 
 * @param   sock        The socket used to remote communication
 * @param   queue       Output parameter for the queue
 * @param   queue_name  The ID of the queue
 * @return              Zero on success, -1 on error
 */
int libqwaitclient_qwait_get_queue(_sock_, _queue_, const char* restrict queue_name);

/**
 * Get complete information about a user
 * 
 * @param   sock     The socket used to remote communication
 * @param   user     Output parameter for the user information
 * @param   user_id  The user's ID
 * @return           Zero on success, -1 on error
 */
int libqwaitclient_qwait_get_user(_sock_, _user_, const char* restrict user_id);


#undef _user_
#undef _queue_
#undef _sock_


#endif

