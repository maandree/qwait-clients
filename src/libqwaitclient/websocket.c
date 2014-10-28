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
#include "websocket.h"

#include "macros.h"

#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <errno.h>


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
int libqwaitclient_websocket_handshake(_http_socket_, const char* restrict bus)
{
  /* TODO libqwaitclient_websocket_handshake */
  /*
    GET /bus/client/292/ulpxgl50/websocket HTTP/1.1\r\n -- /292/ulpxgl50 seems random
    Upgrade: websocket\r\n
    Connection: Upgrade\r\n
    Host: qwait.csc.kth.se\r\n
    Origin: http://qwait.csc.kth.se\r\n
    Pragma: no-cache\r\n
    Cache-Control: no-cache\r\n
    Sec-WebSocket-Key: ????????????????????????\r\n -- base64, unconstrained
    Sec-WebSocket-Version: 13\r\n
    \r\n
  *//*
    HTTP/1.1 101 Switching Protocols\r\n
    Upgrade: WebSocket\r\n -- sic!
    Connection: Upgrade\r\n
    Sec-WebSocket-Accept: ????????????????????????????\r\n -- base64, ignorable
    \r\n
   */
  (void) http_socket;
  (void) bus;
  return 0;
}


/**
 * Upgrade an HTTP socket to a websocket,
 * this does to include the handshake procedure
 * 
 * @param  this         The websocket
 * @param  http_socket  The HTTP socket, no need to destroy it if this function is successful
 */
void libqwaitclient_websocket_upgrade(_this_, _http_socket_)
{
  /* Copy trivial data. */
  this->socket_fd         = http_socket->socket_fd;
  this->connected         = http_socket->connected;
  this->send_buffer       = http_socket->send_buffer;
  this->send_buffer_alloc = http_socket->send_buffer_alloc;
  this->send_buffer_size  = http_socket->send_buffer_size;
  this->send_buffer_ptr   = http_socket->send_buffer_ptr;
  
  /* Move message into new structure. */
  libqwaitclient_webmessage_zero_initialise(&(this->message));
  this->message.content      = http_socket->message.content;
  this->message.content_size = http_socket->message.content_size;
  this->message.buffer       = http_socket->message.buffer;
  this->message.buffer_size  = http_socket->message.buffer_size;
  this->message.buffer_ptr   = http_socket->message.buffer_ptr;
  
  /* Destroy HTTP socket without closing the connection or deleting moved data. */
  http_socket->host = NULL;
  http_socket->socket_fd = -1;
  http_socket->connected = 0;
  http_socket->send_buffer = NULL;
  libqwaitclient_http_message_zero_initialise(&(http_socket->message));
  libqwaitclient_http_socket_destroy(http_socket);
}


/**
 * Release all resources of a websocket 
 * 
 * @param  this  The websocket
 */
void libqwaitclient_websocket_destroy(_this_)
{
  libqwaitclient_websocket_disconnect(this);
  if (this->socket_fd >= 0)
    close(this->socket_fd), this->socket_fd = -1;
  free(this->send_buffer), this->send_buffer = NULL;
  this->send_buffer_ptr = this->send_buffer_size = this->send_buffer_alloc = 0;
  libqwaitclient_webmessage_destroy(&(this->message));
}


/**
 * Disconnect a websocket from its server
 * 
 * @param  this  The websocket
 */
void libqwaitclient_websocket_disconnect(_this_)
{
  if (this->connected == 0)
    return;
  
  shutdown(this->socket_fd, SHUT_RDWR);
  this->connected = 0;
}


/**
 * Send a message over a websocket
 * 
 * @param   this     The websocket
 * @param   message  The message to send, `NULL` to continue with an already started message
 * @return           Zero on success, -1 on error with `errno` set accordingly
 */
int libqwaitclient_websocket_send(_this_, const libqwaitclient_webmessage_t* restrict message)
{
#define length  (this->send_buffer_size - this->send_buffer_ptr)

  size_t block_size;
  
  /* You may only send one message at a time, and you need to send a message. */
  if ((message != NULL) && (length != 0))  return errno = EINPROGRESS, -1;
  if ((message == NULL) && (length == 0))  return errno = ENODATA, -1;
  
  /* Starting on a new message? */
  if (message != NULL)
    {
      /* Get the length of the message to send.  */
      size_t size = libqwaitclient_webmessage_compose_size(message);
      
      /* Save the length of the message, and temporarly mark the
         message as finished in case something goes wrong. */
      this->send_buffer_ptr = this->send_buffer_size = size;
      
      /* Reallocate the send buffer if it is too small. */
      if (size > this->send_buffer_alloc)
	{
	  char* buffer = this->send_buffer;
	  if (xrealloc(buffer, size, char))
	    return -1;
	  this->send_buffer = buffer;
	  this->send_buffer_alloc = size;
	}
      
      /* Start from the beginning. */
      this->send_buffer_ptr = 0;
      
      /* Compose the message. */
      libqwaitclient_webmessage_compose(message, this->send_buffer);
    }
  
  /* Send as much of the message as possible. */
  block_size = length;
  errno = 0;
  while (length > 0)
    {
      /* Send. */
      ssize_t just_sent = send(this->socket_fd,
			       this->send_buffer + this->send_buffer_ptr,
			       min(block_size, length),
			       MSG_NOSIGNAL);
      
      /* How did it go? */
      if (just_sent < 0)
	{
	  /* On error or interruption, cancel, unless the size of the
	     message block was too large, in which case, first try to
	     make the block smaller. */
	  if ((errno != EMSGSIZE) || (block_size >>= 1, block_size == 0))
	    return -1;
	}
      else
	/* Wind the message. */
	this->send_buffer_ptr += (size_t)just_sent;
    }
  
  return 0;
  
#undef length
}


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
int libqwaitclient_websocket_receive(_this_)
{
  int r, saved_errno;
  r = libqwaitclient_webmessage_read(&(this->message), this->socket_fd);
  if ((r == -1) && (errno == ECONNRESET))
    {
      saved_errno = errno;
      libqwaitclient_websocket_disconnect(this);
      errno = saved_errno;
    }
  return r;
}



#undef _http_socket_
#undef _this_

