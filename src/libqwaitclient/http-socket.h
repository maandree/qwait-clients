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
#ifndef LIBQWAITCLIENT_HTTP_SOCKET_H
#define LIBQWAITCLIENT_HTTP_SOCKET_H


#include "http-message.h"

#include <stdint.h>


/**
 * Wrapper around INET TCP client socket with basic HTTP facilities
 */
typedef struct libqwaitclient_http_socket
{
  /**
   * The DNS address or other identification of the server,
   * IDN is not necessarily supported
   */
  const char* host;
  
  /**
   * The socket port the server is listening on
   */
  uint16_t port;
  
  /**
   * Set to the INET family the socket is created with,
   * if it changes when resolving as host, a new socket
   * will be needed
   */
  int inet_family;
  
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
  libqwaitclient_http_message_t message;
  
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
  
} libqwaitclient_http_socket_t;



#define  _this_  libqwaitclient_http_socket_t* restrict this



/**
 * Initialise an HTTP socket
 * 
 * @param   this  The HTTP socket
 * @param   host  The DNS address or other identification of the server,
 *                IDN is not necessarily supported
 * @param   port  The socket port the server is listening on
 * @return        Zero on success, -1 on error with `errno` set accordingly
 */
int libqwaitclient_http_socket_initialise(_this_, const char* host, uint16_t port);

/**
 * Release all resources of an HTTP socket 
 * 
 * @param  this  The HTTP socket
 */
void libqwaitclient_http_socket_destroy(_this_);

/**
 * Connect an HTTP socket to its server
 * 
 * @param   this  The HTTP socket
 * @return        Zero on success, -1 on error with `errno` set accordingly
 */
int libqwaitclient_http_socket_connect(_this_);

/**
 * Disconnect an HTTP socket from its server
 * 
 * @param  this  The HTTP socket
 */
void libqwaitclient_http_socket_disconnect(_this_);

/**
 * Send a message over an HTTP socket
 * 
 * @param   this     The HTTP socket
 * @param   message  The message to send, `NULL` to continue with an already started message
 * @return           Zero on success, -1 on error with `errno` set accordingly
 */
int libqwaitclient_http_socket_send(_this_, const libqwaitclient_http_message_t* restrict message);

/**
 * Receive message over an HTTP socket
 * 
 * The receive message will be stored to `this->message`
 * 
 * @param   this  The HTTP socket
 * @return        Non-zero on error or interruption, errno will be
 *                set accordingly. Destroy the message on error,
 *                be aware that the reading could have been
 *                interrupted by a signal rather than canonical error.
 *                If -2 is returned errno will not have been set,
 *                -2 indicates that the message is malformated,
 *                which is a state that cannot be recovered from.
 */
int libqwaitclient_http_socket_receive(_this_);



#undef _this_

#endif

