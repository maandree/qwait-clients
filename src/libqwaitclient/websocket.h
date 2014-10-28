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
#ifndef LIBQWAITCLIENT_WEBSOCKET_H
#define LIBQWAITCLIENT_WEBSOCKET_H


#include "http-socket.h"
#include "webmessage.h"


/**
 * Client implementation of a websocket
 */
typedef struct libqwaitclient_websocket
{
  /**
   * The file descriptor of the socket
   */
  int socket_fd;
  
  /**
   * Whether the client is connected
   */
  int connected;
  
  /**
   * The message receive buffer
   */
  libqwaitclient_webmessage_t message;
  
  /**
   * The message that is currently being sent
   */
  char* send_buffer;
  
  /**
   * The size of the allocation of `send_buffer`
   */
  size_t send_buffer_alloc;
  
  /**
   * The size of the message in `send_buffer`
   */
  size_t send_buffer_size;
  
  /**
   * How much of `send_buffer` that has already been sent
   */
  size_t send_buffer_ptr;
  
} libqwaitclient_websocket_t;



#define _this_         libqwaitclient_websocket_t* restrict this
#define _http_socket_  libqwaitclient_http_socket_t* restrict http_socket



/**
 * Perform a websocket handshake over an HTTP socket
 * so `libqwaitclient_websocket_upgrade` may be used
 * to create a websocket
 * 
 * @param   this  The HTTP socket
 * @param   bus   QWait uses "/bus/client" here
 * @return        Zero on success, -1 on error with `errno` set accordingly,
 *                -2 if an malformated message received, -3 if the handshake
 *                failed for some other reason
 */
int libqwaitclient_websocket_handshake(_http_socket_, const char* restrict bus);

/**
 * Upgrade an HTTP socket to a websocket,
 * this does to include the handshake procedure
 * 
 * @param  this         The websocket
 * @param  http_socket  The HTTP socket, no need to destroy it if this function is successful
 */
void libqwaitclient_websocket_upgrade(_this_, _http_socket_);

/**
 * Release all resources of a websocket 
 * 
 * @param  this  The websocket
 */
void libqwaitclient_websocket_destroy(_this_);

/**
 * Disconnect a websocket from its server
 * 
 * @param  this  The websocket
 */
void libqwaitclient_websocket_disconnect(_this_);

/**
 * Send a message over a websocket
 * 
 * @param   this     The websocket
 * @param   message  The message to send, `NULL` to continue with an already started message
 * @return           Zero on success, -1 on error with `errno` set accordingly
 */
int libqwaitclient_websocket_send(_this_, const libqwaitclient_webmessage_t* restrict message);

/**
 * Receive message over a websocket
 * 
 * The receive message will be stored to `this->message`
 * 
 * @param   this  The websocket
 * @return        Non-zero on error or interruption, `errno` will be
 *                set accordingly. Destroy the message on error,
 *                be aware that the reading could have been
 *                interrupted by a signal rather than canonical error.
 *                If -2 is returned `errno` will not have been set,
 *                -2 indicates that the message is malformated,
 *                which is a state that cannot be recovered from.
 */
int libqwaitclient_websocket_receive(_this_);



#undef _http_socket_
#undef _this_

#endif

