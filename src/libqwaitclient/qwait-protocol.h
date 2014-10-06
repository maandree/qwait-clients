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
#include "login-information.h"
#include "authentication.h"

#define _GNU_SOURCE
#include <stddef.h>


#define _sock_   libqwaitclient_http_socket_t*       restrict sock
#define _auth_   libqwaitclient_authentication_t*    restrict auth
#define _queue_  libqwaitclient_qwait_queue_t*       restrict queue
#define _user_   libqwaitclient_qwait_user_t*        restrict user
#define _login_  libqwaitclient_login_information_t* restrict login


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
 * Get complete information on all QWait administrators
 * 
 * @param   sock        The socket used to remote communication
 * @param   auth        User authentication
 * @param   user_count  Output parameter for the number of returned users
 * @return              Information on all administrators, `NULL` on error
 */
libqwaitclient_qwait_user_t* libqwaitclient_qwait_get_admins(_sock_, const _auth_, size_t* restrict user_count);

/**
 * Get complete information on all QWait users
 * 
 * @param   sock        The socket used to remote communication
 * @param   auth        User authentication
 * @param   user_count  Output parameter for the number of returned users
 * @return              Information on all users, `NULL` on error
 */
libqwaitclient_qwait_user_t* libqwaitclient_qwait_get_users(_sock_, const _auth_, size_t* restrict user_count);

/**
 * Find users by their real name
 * 
 * @param   sock          The socket used to remote communication
 * @param   auth          User authentication
 * @param   partial_name  The all returned user's real name should contain this string
 * @param   user_count    Output parameter for the number of returned users
 * @return                Information on all found users, `NULL` on error
 */
libqwaitclient_qwait_user_t* libqwaitclient_qwait_find_user(_sock_, const _auth_, const char* partial_name,
							    size_t* restrict user_count);

/**
 * Get complete information about a user
 * 
 * @param   sock     The socket used to remote communication
 * @param   user     Output parameter for the user information
 * @param   user_id  The user's ID
 * @return           Zero on success, -1 on error
 */
int libqwaitclient_qwait_get_user(_sock_, _user_, const char* restrict user_id);

/**
 * Hide or unhide a queue
 * 
 * @param   sock        The socket used to remote communication
 * @param   auth        User authentication
 * @param   queue_name  The name of the queue
 * @param   hidden      Whether the queue should be hidden
 * @return              Zero on success, -1 on error
 */
int libqwaitclient_qwait_set_queue_hidden(_sock_, const _auth_, const char* restrict queue_name, int hidden);

/**
 * Lock or unlock a queue
 * 
 * @param   sock        The socket used to remote communication
 * @param   auth        User authentication
 * @param   queue_name  The name of the queue
 * @param   locked      Whether the queue should be locked
 * @return              Zero on success, -1 on error
 */
int libqwaitclient_qwait_set_queue_locked(_sock_, const _auth_, const char* restrict queue_name, int locked);

/**
 * Remove all entries in a queue
 * 
 * @param   sock        The socket used to remote communication
 * @param   auth        User authentication
 * @param   queue_name  The name of the queue
 * @return              Zero on success, -1 on error
 */
int libqwaitclient_qwait_clear_queue(_sock_, const _auth_, const char* restrict queue_name);

/**
 * Delete a queue
 * 
 * @param   sock        The socket used to remote communication
 * @param   auth        User authentication
 * @param   queue_name  The name of the queue
 * @return              Zero on success, -1 on error
 */
int libqwaitclient_qwait_delete_queue(_sock_, const _auth_, const char* restrict queue_name);

/**
 * Create a new queue
 * 
 * @param   sock         The socket used to remote communication
 * @param   auth         User authentication
 * @param   queue_title  The title of the new queue
 * @return               Zero on success, -1 on error
 */
int libqwaitclient_qwait_create_queue(_sock_, const _auth_, const char* restrict queue_title);

/**
 * Join or leave a queue
 * 
 * @param   sock        The socket used to remote communication
 * @param   auth        User authentication
 * @param   queue_name  The name of the queue
 * @param   user_id     The user ID of the user that should join or leave the queue
 * @param   wait        Whether the user should join the queue
 * @return              Zero on success, -1 on error
 */
int libqwaitclient_qwait_set_queue_wait(_sock_, const _auth_, const char* restrict queue_name,
					const char* restrict user_id, int wait);

/**
 * Set or change the user's comment in a queue
 * 
 * @param   sock        The socket used to remote communication
 * @param   auth        User authentication
 * @param   queue_name  The name of the queue
 * @param   user_id     The user ID of the affected user
 * @param   comment     The comment for the entry
 * @return              Zero on success, -1 on error
 */
int libqwaitclient_qwait_set_queue_wait_comment(_sock_, const _auth_, const char* restrict queue_name,
						const char* restrict user_id, const char* restrict comment);

/**
 * Set or change the user's announced location in a queue
 * 
 * @param   sock        The socket used to remote communication
 * @param   auth        User authentication
 * @param   queue_name  The name of the queue
 * @param   user_id     The user ID of the affected user
 * @param   location    The announced location for the entry
 * @return              Zero on success, -1 on error
 */
int libqwaitclient_qwait_set_queue_wait_location(_sock_, const _auth_, const char* restrict queue_name,
						 const char* restrict user_id, const char* restrict location);

/**
 * Add a user as a moderator of a queue or remove said status
 * 
 * @param   sock        The socket used to remote communication
 * @param   auth        User authentication
 * @param   queue_name  The name of the queue
 * @param   user_id     The user ID of the affected user
 * @param   moderator   Whether the user should be a moderator of the queue
 * @return              Zero on success, -1 on error
 */
int libqwaitclient_qwait_set_queue_moderator(_sock_, const _auth_, const char* restrict queue_name,
					     const char* restrict user_id, int moderator);

/**
 * Add a user as an owner of a queue or remove said status
 * 
 * @param   sock        The socket used to remote communication
 * @param   auth        User authentication
 * @param   queue_name  The name of the queue
 * @param   user_id     The user ID of the affected user
 * @param   owner       Whether the user should be an owner of the queue
 * @return              Zero on success, -1 on error
 */
int libqwaitclient_qwait_set_queue_owner(_sock_, const _auth_, const char* restrict queue_name,
					 const char* restrict user_id, int owner);

/**
 * Add a user as a QWait administrator or remove said status
 * 
 * @param   sock        The socket used to remote communication
 * @param   auth        User authentication
 * @param   user_id     The user ID of the affected user
 * @param   admin       Whether the user should be a QWait administrator
 * @return              Zero on success, -1 on error
 */
int libqwaitclient_qwait_set_admin(_sock_, const _auth_, const char* restrict user_id, int admin);

/**
 * Get login information
 * 
 * @param   sock   The socket used to remote communication
 * @param   auth   User authentication, may be `NULL`
 * @param   login  Output paramter for the login information
 * @return         Zero on success, -1 on error
 */
int libqwaitclient_qwait_get_login_information(_sock_, const _auth_, _login_);


#undef _login_
#undef _user_
#undef _queue_
#undef _auth_
#undef _sock_


#endif

