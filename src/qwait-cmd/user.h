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
#ifndef QWAIT_CMD_USER_H
#define QWAIT_CMD_USER_H


#include <libqwaitclient.h>


#define _sock_  libqwaitclient_http_socket_t* restrict sock


/**
 * Print information about a user
 * 
 * @param   sock     A socket that is connected to the qwait server
 * @param   user_id  The ID of the user
 * @return           Zero on success, -1 on error
 */
int print_user_information(_sock_, const char* restrict user_id);

/**
 * Add or remove QWait administrator status for a user
 * 
 * @param   sock     A socket that is connected to the qwait server
 * @param   user_id  The ID of the user
 * @param   admin    Whether the user should be an administrator
 * @return           Zero on success, -1 on error
 */
int user_set_admin(_sock_, const char* restrict user_id, int admin);

/**
 * Add or remove moderator status for a user over a queue
 * 
 * @param   sock        A socket that is connected to the qwait server
 * @param   user_id     The ID of the user
 * @parma   queue_name  The name of queue
 * @param   moderator   Whether the user should be a moderator
 * @return              Zero on success, -1 on error
 */
int user_set_moderator(_sock_, const char* restrict user_id, const char* restrict queue_name, int moderator);

/**
 * Add or remove owner status for a user over a queue
 * 
 * @param   sock        A socket that is connected to the qwait server
 * @param   user_id     The ID of the user
 * @parma   queue_name  The name of queue
 * @param   owner       Whether the user should be an owner
 * @return              Zero on success, -1 on error
 */
int user_set_owner(_sock_, const char* restrict user_id, const char* restrict queue_name, int owner);

/**
 * Make a user join or leave a queue
 * 
 * @param   sock        A socket that is connected to the qwait server
 * @param   user_id     The ID of the user
 * @parma   queue_name  The name of queue
 * @param   wait        Whether the user should be in the queue
 * @return              Zero on success, -1 on error
 */
int user_set_wait(_sock_, const char* restrict user_id, const char* restrict queue_name, int wait);

/**
 * Change the comment for a entry in a queue
 * 
 * @param   sock        A socket that is connected to the qwait server
 * @param   user_id     The ID of the user
 * @parma   queue_name  The name of queue
 * @param   comment     The comment for the user's entry in the queue
 * @return              Zero on success, -1 on error
 */
int user_set_comment(_sock_, const char* restrict user_id, const char* restrict queue_name,
		     const char* restrict comment);

/**
 * Change the announced location for a entry in a queue
 * 
 * @param   sock        A socket that is connected to the qwait server
 * @param   user_id     The ID of the user
 * @parma   queue_name  The name of queue
 * @param   comment     The student's physical location
 * @return              Zero on success, -1 on error
 */
int user_set_location(_sock_, const char* restrict user_id, const char* restrict queue_name,
		     const char* restrict location);


#undef _sock_

#endif

