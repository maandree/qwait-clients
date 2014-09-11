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
#define _GNU_SOURCE
#include "http-socket.h"

#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>


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
int libqwaitclient_http_socket_initialise(_this_, const char* host, uint16_t port)
{
  this->host = host;
  this->port = port;
  this->inet_family = AF_INET;
  this->socket_fd = 0;
  this->connected = 0;
  
  this->socket_fd = socket(this->inet_family, SOCK_STREAM, IPPROTO_TCP);
  if (this->socket_fd < 0)
    return this->socket_fd = 0, -1;
  
  return 0;
}


/**
 * Release all resources of an HTTP socket 
 * 
 * @param  this  The HTTP socket
 */
void libqwaitclient_http_socket_destroy(_this_)
{
  libqwaitclient_http_socket_disconnect(this);
  if (this->socket_fd > 0)
    close(this->socket_fd), this->socket_fd = 0;
}


/**
 * Connect the HTTP socket to its server
 * 
 * @param   this  The HTTP socket
 * @return        Zero on success, -1 on error with `errno` set accordingly
 */
int libqwaitclient_http_socket_connect(_this_)
{
  char port[6];
  struct addrinfo hints;
  struct addrinfo* hosts;
  struct addrinfo* host;
  int saved_errno;
  int r;
  
  if (this->connected)
    return 0;
  
  /* Resolve hostname and create socket address structure. */
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  sprintf(port, "%u", (unsigned int)(this->port));
  r = getaddrinfo(this->host, port, &hints, &hosts);
  if (r)
    {
      /* Not really an excellent translation, but for some
       * reason these are POSIX exceptions that do not
       * appear in errno.h. And not translating then would
       * complicate the client's code and the API. */
      switch (r)
	{
	case EAI_ADDRFAMILY: return errno = EHOSTUNREACH, -1;
	case EAI_AGAIN:      return errno = EAGAIN, -1;
	case EAI_BADFLAGS:   return errno = EINVAL, -1;
	case EAI_FAIL:       return errno = EPROTO, -1;
	case EAI_FAMILY:     return errno = EAFNOSUPPORT, -1;
	case EAI_MEMORY:     return errno = ENOMEM, -1;
	case EAI_NODATA:     return errno = EDESTADDRREQ, -1;
	case EAI_NONAME:     return errno = EADDRNOTAVAIL, -1;
	case EAI_SERVICE:    return errno = EPROTOTYPE, -1;
	case EAI_SOCKTYPE:   return errno = ENOTSUP, -1;
	case EAI_SYSTEM:     return -1;
	}
    }
  
  /* Test all found resolutions. */
  errno = EHOSTUNREACH, r = -1;
  for (host = hosts; host != NULL; host = host->ai_next)
    {
      /* We have precreated the socket so we only need the
       * recreate it if the INET family changes. */
      if (host->ai_family != this->inet_family)
	{
	  this->inet_family = host->ai_family;
	  close(this->socket_fd);
	  this->socket_fd = socket(this->inet_family, SOCK_STREAM, IPPROTO_TCP);
	  if (this->socket_fd < 0)
	    continue;
	}
      
      /* Try to connect to the server. */
      r = connect(this->socket_fd, host->ai_addr, host->ai_addrlen);
      if (r < 0)
	continue;
      break;
    }
  
  if (r < 0)
    goto fail;
  
  this->connected = 1;
  freeaddrinfo(hosts);
  return 0;
  
 fail:
  saved_errno = errno;
  if (this->socket_fd < 0)
    this->socket_fd = 0;
  freeaddrinfo(hosts);
  errno = saved_errno;
  return -1;
}


/**
 * Disconnect the HTTP socket from its server
 * 
 * @param  this  The HTTP socket
 */
void libqwaitclient_http_socket_disconnect(_this_)
{
  if (this->connected == 0)
    return;
  
  shutdown(this->socket_fd, SHUT_RDWR);
  this->connected = 0;
}



#undef _this_

