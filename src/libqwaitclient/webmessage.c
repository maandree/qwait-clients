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
#include "webmessage.h"

#include "macros.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>


#define _this_ libqwaitclient_webmessage_t* restrict this


/* http://tools.ietf.org/html/rfc6455#page-28 is a useful resource. */

/* This implement assumes CHAR_BIT = 8, which is always true for POSIX systems. */



/**
 * Zero initialise a message slot
 * 
 * @param  this  Memory slot in which to store the new message
 */
void libqwaitclient_webmessage_zero_initialise(_this_)
{
  memset(this, 0, sizeof(libqwaitclient_webmessage_t));
}


/**
 * Release all resources in a message, should
 * be done even if initialisation fails
 * 
 * @param  this  The message
 */
void libqwaitclient_webmessage_destroy(_this_)
{
  free(this->content);
  free(this->buffer);
  libqwaitclient_webmessage_zero_initialise(this);
}


/**
 * Extend the read buffer by way of doubling
 * 
 * @param   this  The message
 * @return        Zero on success, -1 on error
 */
static int libqwaitclient_webmessage_extend_buffer(_this_)
{
  char* new_buf = this->buffer;
  if (xrealloc(new_buf, this->buffer_size << 1, char))
      return -1;
  this->buffer = new_buf;
  this->buffer_size <<= 1;
  return 0;
}


/**
 * Continue reading from the socket into the buffer
 * 
 * @param   this  The message
 * @param   fd    The file descriptor of the socket
 * @return        The return value follows the rules of `libqwaitclient_webmessage_read`
 */
static int continue_read(_this_, int fd)
{
  size_t n;
  ssize_t got;
  int r;
  
  /* Figure out how much space we have left in the read buffer. */
  n = this->buffer_size - this->buffer_ptr;
  
  /* If we do not have too much left, */
  if (n < 128)
    {
      /* grow the buffer, */
      try (libqwaitclient_webmessage_extend_buffer(this));
      
      /* and recalculate how much space we have left. */
      n = this->buffer_size - this->buffer_ptr;
    }
  
  /* Then read from the socket. */
  errno = 0;
  got = recv(fd, this->buffer + this->buffer_ptr, n, 0);
  this->buffer_ptr += (size_t)(got < 0 ? 0 : got);
  if (errno)
    return -1;
  if (got == 0)
    {
      errno = ECONNRESET;
      return -1;
    }
  
  return 0;
}


/**
 * Reset the header list and the content
 * 
 * @param  this  The message
 */
static void reset_message(_this_)
{
  free(this->content);
  this->content = NULL;
  this->content_size = 0;
  this->content_ptr = 0;
}


/**
 * Receive a part of the payload
 * 
 * @param   this  Memory slot in which to store the new fragment
 * @return        Follows the rules of `libqwaitclient_webmessage_read`
 *                with one exception, if zero is returned the message
 *                has not been completely read, if 1 is returned the
 *                message has been completely read
 */
static int receive_known_length(_this_)
{
  if (this->content_size > 0)
    {
      /* How much of the content that has not yet been filled. */
      size_t need = this->content_size - this->content_ptr;
      /* How much we have of that what is needed. */
      size_t move = min(this->buffer_ptr, need);
      
      /* Copy what we have, and remove it from the the read buffer. */
      memcpy(this->content + this->content_ptr, this->buffer, move * sizeof(char));
      this->buffer_ptr -= move;
      memmove(this->buffer, this->buffer + move, this->buffer_ptr * sizeof(char));
      
      /* Keep track of how much we have read. */
      this->content_ptr += move;
    }
  if (this->content_ptr == this->content_size)
    {
      /* If we have filled the content (or there was no content),
	 mark the end of this stage, i.e. that the message is
	 complete, and return with success. */
      this->stage = 3;
      return 1;
    }
  return 0;
}


/**
 * Read the next fragment from a file descriptor
 * 
 * @param   this  Memory slot in which to store the new fragment
 * @param   fd    The file descriptor
 * @return        Non-zero on error or interruption, errno will be
 *                set accordingly. Destroy the message on error,
 *                be aware that the reading could have been
 *                interrupted by a signal rather than canonical error.
 *                If -2 is returned errno will not have been set,
 *                -2 indicates that the message is malformated,
 *                which is a state that cannot be recovered from.
 */
int libqwaitclient_webmessage_read(_this_, int fd)
{
  int r;
  
  /* If we are at stage 3, we are done and it is time to start over.
     This is important because the function could have been interrupted. */
  if (this->stage == 3)
    {
      reset_message(this);
      this->stage = 0;
    }
  
  /* Read from file descriptor until we have a full fragment. */
  for (;;)
    {
      /* Stage 0: Read fragment metadata. */
      if ((this->stage == 0) && (this->buffer_ptr >= 1))
	{
	  /* Parse the read byte. */
	  this->final  = ((this->buffer[0] & 0x80) == 0x80);
	  this->opcode = (this->buffer[0] & 0x0F);
	  
	  /* Remove the read byte from the buffer. */
	  memmove(this->buffer, this->buffer + 1, --(this->buffer_ptr) * sizeof(char));
	  
	  /* Mark end of stage, next stage is acquiring the payload length. */
	  this->stage++;
	}
      
      
      /* Stage 1: Acquire payload length. */
      if ((this->stage == 1) && (this->buffer_ptr > 0))
	{
	  /* How any bytes do we need to determine the payload length? */
	  size_t i, needed_bytes = 1;
	  if (this->buffer[0] == (char)126)  needed_bytes += 2;
	  if (this->buffer[0] == (char)127)  needed_bytes += 8;
	  
	  /* Do we have what it takes? */
	  if (this->buffer_ptr < needed_bytes)
	    goto need_more;
	  
	  /* How long is the payload? */
	  if (needed_bytes == 1)
	    this->content_size = (size_t)(this->buffer[0]);
	  else
	    for (this->content_size = 0, i = needed_bytes; i > 0; i--)
	      {
		this->content_size <<= 8;
		this->content_size |= (size_t)(unsigned char)(this->buffer[i - 1]);
	      }
	  
	  /* Allocate buffer for the payload. */
	  if (xmalloc(this->content, this->content_size, char))
	    return -1;
	  
	  /* Remove the read bytes from the buffer. */
	  this->buffer_ptr -= needed_bytes;
	  memmove(this->buffer, this->buffer + needed_bytes, this->buffer_ptr * sizeof(char));
	  
	  /* Mark end of stage, next stage is acquiring the payload. */
	  this->stage++;
	}
      
      
      /* Stage 2: Acquire the payload. */
      r = 0;
      if (this->stage == 2)
	r = receive_known_length(this);
      if (r)
	return r < 0 ? r : (r - 1);
      
      
      /* If we did not enter stage 3. */
    need_more:
      
      /* Continue reading from the socket into the buffer. */
      try (continue_read(this, fd));
    }
  
  (void) fd;
  return 0;
}


/**
 * Get the required allocation size for `data` of the
 * function `libqwaitclient_webmessage_compose`
 * 
 * @param   this  The message
 * @return        The size of the message when marshalled
 */
size_t libqwaitclient_webmessage_compose_size(const _this_)
{
  /* TODO libqwaitclient_webmessage_compose_size */
  (void) this;
  return 0;
}


/**
 * Marshal a message for communication
 * 
 * @param  this  The message
 * @param  data  Output buffer for the marshalled data
 */
void libqwaitclient_webmessage_compose(const _this_, char* restrict data)
{
  /* TODO libqwaitclient_webmessage_compose */
  (void) this;
  (void) data;
}



#undef _this_

