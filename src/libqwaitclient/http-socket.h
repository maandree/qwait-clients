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
#ifndef LIBQWAITCLIENTS_HTTP_SOCKET_H
#define LIBQWAITCLIENTS_HTTP_SOCKET_H


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
 * Connect the HTTP socket to its server
 * 
 * @param   this  The HTTP socket
 * @return        Zero on success, -1 on error with `errno` set accordingly
 */
int libqwaitclient_http_socket_connect(_this_);

/**
 * Disconnect the HTTP socket from its server
 * 
 * @param  this  The HTTP socket
 */
void libqwaitclient_http_socket_disconnect(_this_);



#undef _this_

#endif

