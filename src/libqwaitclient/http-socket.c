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
#include "http-socket.h"

#include "macros.h"

#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#define _this_  libqwaitclient_http_socket_t* restrict this



#ifdef VERBOSE_DEBUG
/**
 * Escape a string and print it to buffer
 * 
 * @param   output  Output sink for the string that should be escaped
 * @param   input   The string to escape and write to `output`
 * @param   length  The length of `input`
 * @return          The number of written bytes, at most four times
 *                  `length`, not NUL-terminated
 */
static size_t dump_string(char* restrict output, const char* restrict input, size_t length)
{
  size_t i, j;
  for (i = j = 0; i < length; i++)
    {
      unsigned char c = (unsigned char)(input[i]);
      if    (c == '\033')   output[j++] = '\\', output[j++] = 'e';
      else if (c == '\r')   output[j++] = '\\', output[j++] = 'r';
      else if (c == '\t')   output[j++] = '\\', output[j++] = 't';
      else if (c == '\a')   output[j++] = '\\', output[j++] = 'a';
      else if (c == '\f')   output[j++] = '\\', output[j++] = 'f';
      else if (c == '\v')   output[j++] = '\\', output[j++] = 'v';
      else if (c == '\b')   output[j++] = '\\', output[j++] = 'b';
      else if (c == '\n')   output[j++] = '\\', output[j++] = 'n', output[j++] = '\n';
      else if (c == '\\')   output[j++] = '\\', output[j++] = '\\';
      else if ((c >= 127) || (c < ' '))
	{
	  output[j++] = '\\';
	  output[j++] = 'x';
	  output[j++] = "0123456789abcdef"[(c >> 4) & 15];
	  output[j++] = "0123456789abcdef"[(c >> 0) & 15];
	}
      else
	output[j++] = (char)c;
    }
  return j;
}


/**
 * Dump the send buffer to stderr
 * 
 * @param  this  The HTTP socket
 */
static void dump_send_buffer(const _this_)
{
  size_t offset = 0;
  char* str = malloc((this->send_buffer_size * 4 + 1) * sizeof(char));
  if (str == NULL)
    {
      perror("\033[01;31mskipping transmission output");
      fprintf(stderr, "\033[00m");
      fflush(stderr);
      return;
    }
  offset += dump_string(str + offset, this->send_buffer, this->send_buffer_size);
  str[offset++] = '\0';
  fprintf(stderr,
	  "\033[00;01;35m(start of transmission on next line)\n"
	  "\033[00;35m%s"
	  "\033[00;01;35m(end of transmission)\033[00m\n",
	  str);
  free(str);
}


/**
 * Dump the receive message to stderr
 * 
 * @param  this  The HTTP socket
 */
static void dump_message(const _this_)
{
  size_t offset = 0, i, n;
  char* str;
  n = strlen(this->message.top) + 2;
  for (i = 0; i < this->message.header_count; i++)
    n += strlen(this->message.headers[i]) + 2;
  n += 2 + this->message.content_size;
  str = malloc((n * 4 + 1) * sizeof(char));
  if (str == NULL)
    {
      perror("\033[01;31mskipping received message output");
      fprintf(stderr, "\033[00m");
      fflush(stderr);
      return;
    }
  offset += dump_string(str + offset, this->message.top, strlen(this->message.top));
  sprintf(str + offset, "\\r\\n\n"), offset += 5;
  for (i = 0; i < this->message.header_count; i++)
    {
      offset += dump_string(str + offset, this->message.headers[i], strlen(this->message.headers[i]));
      sprintf(str + offset, "\\r\\n\n"), offset += 5;
    }
  sprintf(str + offset, "\\r\\n\n"), offset += 5;
  offset += dump_string(str + offset, this->message.content, this->message.content_size);
  str[offset++] = '\0';
  fprintf(stderr,
	  "\033[00;01;32m(start of received message on next line)\n"
	  "\033[00;32m%s"
	  "\033[00;01;32m(end of received message)\033[00m\n",
	  str);
  free(str);
}
#endif


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
  int saved_errno;
  
  this->host = host;
  this->port = port;
  this->inet_family = AF_INET;
  this->socket_fd = 0;
  this->connected = 0;
  this->send_buffer = NULL;
  this->send_buffer_alloc = 0;
  this->send_buffer_size = 0;
  this->send_buffer_ptr = 0;
  
  if (libqwaitclient_http_message_initialise(&(this->message)) < 0)
    goto fail;
  
  this->socket_fd = socket(this->inet_family, SOCK_STREAM, IPPROTO_TCP);
  if (this->socket_fd < 0)
    goto fail;
  
  return 0;
  
 fail:
  saved_errno = errno;
  this->socket_fd = 0;
  libqwaitclient_http_message_destroy(&(this->message));
  errno = saved_errno;
  return -1;
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
  free(this->send_buffer), this->send_buffer = NULL;
  this->send_buffer_ptr = this->send_buffer_size = this->send_buffer_alloc = 0;
  libqwaitclient_http_message_destroy(&(this->message));
}


/**
 * Connect an HTTP socket to its server
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
	default:
	  return errno = EREMOTEIO, -1;
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
  
  if (host == NULL)
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
 * Disconnect an HTTP socket from its server
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


/**
 * Send a message over an HTTP socket
 * 
 * @param   this     The HTTP socket
 * @param   message  The message to send, `NULL` to continue with an already started message
 * @return           Zero on success, -1 on error with `errno` set accordingly
 */
int libqwaitclient_http_socket_send(_this_, const libqwaitclient_http_message_t* restrict message)
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
      size_t size = libqwaitclient_http_message_compose_size(message);
      
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
      libqwaitclient_http_message_compose(message, this->send_buffer);
#ifdef VERBOSE_DEBUG
      dump_send_buffer(this);
#endif
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
int libqwaitclient_http_socket_receive(_this_)
{
  int r, saved_errno;
  r = libqwaitclient_http_message_read(&(this->message), this->socket_fd);
  if ((r == -1) && (errno == ECONNRESET))
    {
      saved_errno = errno;
      libqwaitclient_http_socket_disconnect(this);
      errno = saved_errno;
    }
#ifdef VERBOSE_DEBUG
  if (r == 0)
    {
      saved_errno = errno;
      dump_message(this);
      errno = saved_errno;
    }
#endif
  return r;
}



#undef _this_

