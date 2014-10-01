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
#include "http-message.h"

#include "macros.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>


#define _this_ libqwaitclient_http_message_t* restrict this


/**
 * Initialise a message slot so that it can
 * be used by `libqwaitclient_http_message_read`
 * 
 * @param   this  Memory slot in which to store the new message
 * @return        Non-zero on error, `errno` will be set accordingly
 */
int libqwaitclient_http_message_initialise(_this_)
{
  this->top = NULL;
  this->headers = NULL;
  this->header_count = 0;
  this->content = NULL;
  this->content_size = 0;
  this->content_ptr = 0;
  this->buffer_size = 128;
  this->buffer_ptr = 0;
  this->transfer_encoding = KNOWN_LENGTH;
  this->stage = 0;
  if (xmalloc(this->buffer, this->buffer_size, char))
    {
      int saved_errno = errno;
      libqwaitclient_http_message_destroy(this);
      errno = saved_errno;
      return -1;
    }
  return 0;
}


/**
 * Zero initialise a message slot
 * 
 * @param  this  Memory slot in which to store the new message
 */
void libqwaitclient_http_message_zero_initialise(_this_)
{
  this->top = NULL;
  this->headers = NULL;
  this->header_count = 0;
  this->content = NULL;
  this->content_size = 0;
  this->content_ptr = 0;
  this->buffer = NULL;
  this->buffer_size = 0;
  this->buffer_ptr = 0;
  this->transfer_encoding = KNOWN_LENGTH;
  this->stage = 0;
}


/**
 * Release all resources in a message, should
 * be done even if initialisation fails
 * 
 * @param  this  The message
 */
void libqwaitclient_http_message_destroy(_this_)
{
  free(this->top), this->top = NULL;
  if (this->headers != NULL)
    {
      size_t i;
      xfree(this->headers, this->header_count);
      this->headers = NULL;
    }
  free(this->content), this->content = NULL;
  free(this->buffer), this->buffer = NULL;
}


/**
 * Extend the header list's allocation
 * 
 * @param   this    The message
 * @param   extent  The number of additional entries
 * @return          Zero on success, -1 on error
 */
int libqwaitclient_http_message_extend_headers(_this_, size_t extent)
{
  char** new_headers = this->headers;
  if (xrealloc(new_headers, this->header_count + extent, char*))
    return -1;
  this->headers = new_headers;
  return 0;
}


/**
 * Extend the read buffer by way of doubling
 * 
 * @param   this  The message
 * @return        Zero on success, -1 on error
 */
static int libqwaitclient_http_message_extend_buffer(_this_)
{
  char* new_buf = this->buffer;
  if (xrealloc(new_buf, this->buffer_size << 1, char))
      return -1;
  this->buffer = new_buf;
  this->buffer_size <<= 1;
  return 0;
}


/**
 * Reset the header list and the content
 * 
 * @param  this  The message
 */
static void reset_message(_this_)
{
  free(this->top);
  this->top = NULL;
  
  if (this->headers != NULL)
    {
      size_t i;
      xfree(this->headers, this->header_count);
      this->headers = NULL;
    }
  this->header_count = 0;
  
  free(this->content);
  this->content = NULL;
  this->content_size = 0;
  this->content_ptr = 0;
  this->transfer_encoding = KNOWN_LENGTH;
}


/**
 * Read the headers the message and determine, and store, its content's length
 * 
 * @param   this  The message
 * @return        Zero on success, -2 on error (malformated message: unrecoverable state)
 */
static int get_content_length(_this_)
{
  char* header;
  size_t i;
  
  for (i = 0; i < this->header_count; i++)
    if (startswith(this->headers[i], "Content-Length: "))
      {
	/* We know how long the content should be. */
	this->transfer_encoding = KNOWN_LENGTH;

	/* Store the message length. */
	header = this->headers[i] + strlen("Content-Length: ");
	this->content_size = (size_t)atol(header);
	
	/* Do not except a length that is not correctly formated. */
	for (; *header; header++)
	  if ((*header < '0') || ('9' < *header))
	    return -2; /* Malformated value, enters unrecoverable state. */
	
	/* Stop searching for headers, we have found one. */
	break;
      }
    else if (strequals(this->headers[i], "Transfer-Encoding: chunked"))
      {
	/* We will receive the content in chunkes. */
	this->transfer_encoding = CHUNKED_TRANSFER;
      }
  
  return 0;
}


/**
 * Verify that a header is correctly formated
 * 
 * @param   header  The header, must be NUL-terminated
 * @param   length  The length of the header
 * @return          Zero if valid, -2 if invalid (malformated message: unrecoverable state)
 */
static int __attribute__((pure)) validate_header(const char* header, size_t length)
{
  char* p = memchr(header, ':', length * sizeof(char));
  
  if ((p == NULL) || /* Buck you, rawmemchr should not segfault the program. */
      (p[1] != ' ')) /* Also an invalid format. ' ' is mandated after the ':'. */
    return -2;
  
  return 0;
}


/**
 * Remove the beginning of the read buffer
 * 
 * @param  this        The message
 * @param  length      The number of characters to remove  
 * @param  update_ptr  Whether to update the buffer pointer
 */
static void unbuffer_beginning(_this_, size_t length, int update_ptr)
{
  memmove(this->buffer, this->buffer + length, (this->buffer_ptr - length) * sizeof(char));
  if (update_ptr)
    this->buffer_ptr -= length;
}


/**
 * Remove the header–content delimiter from the buffer,
 * get the content's size and allocate the content
 * 
 * @param   this  The message
 * @return        The return value follows the rules of `libqwaitclient_http_message_read`
 */
static int initialise_content(_this_)
{
  /* Remove the \n (end of empty line) we found from the buffer. */
  unbuffer_beginning(this, 2, 1);
  
  /* Get the length of the content. */
  if (get_content_length(this) < 0)
    return -2; /* Malformated value, enters unrecoverable state. */
  
  /* Allocate the content buffer. */
  if (this->content_size > 0)
    if (xmalloc(this->content, this->content_size, char))
      return -1;
  
  return 0;
}


/**
 * Create a header from the buffer and store it
 * 
 * @param   this    The message
 * @param   length  The length of the header, including NUL-termination
 * @return          The return value follows the rules of `libqwaitclient_http_message_read`
 */
static int store_header(_this_, size_t length)
{
  char* header;
  
  /* Allocate the header. */
  if (xmalloc(header, length, char))
    return -1;
  /* Copy the header data into the allocated header, */
  memcpy(header, this->buffer, length * sizeof(char));
  /* and NUL-terminate it. */
  header[length - 1] = '\0';
  
  /* Remove the header data from the read buffer. */
  unbuffer_beginning(this, length + 1, 1);
  
  /* Make sure the the header syntax is correct so that
     the program does not need to care about it. */
  if (validate_header(header, length))
    {
      free(header);
      return -2;
    }
  
  /* Store the header in the header list. */
  this->headers[this->header_count++] = header;
  
  return 0;
}


/**
 * Create a header from the buffer and store it
 * 
 * @param   this    The message
 * @param   length  The length of the header, including NUL-termination
 * @return          The return value follows the rules of `libqwaitclient_http_message_read`
 */
static int store_top(_this_, size_t length)
{
  /* Allocate the top. */
  if (xmalloc(this->top, length, char))
    return -1;
  /* Copy the header data into the allocated header, */
  memcpy(this->top, this->buffer, length * sizeof(char));
  /* and NUL-terminate it. */
  this->top[length - 1] = '\0';
  
  /* Remove the top data from the read buffer. */
  unbuffer_beginning(this, length + 1, 1);
  
  return 0;
}


/**
 * Continue reading from the socket into the buffer
 * 
 * @param   this  The message
 * @param   fd    The file descriptor of the socket
 * @return        The return value follows the rules of `libqwaitclient_http_message_read`
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
      try (libqwaitclient_http_message_extend_buffer(this));
      
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
 * Receive a part of the content, assuming the content's length is known
 * 
 * @param   this  Memory slot in which to store the new message
 * @return        Follows the rules of `libqwaitclient_http_message_read`
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
      unbuffer_beginning(this, move, 1);
	      
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
 * Receive a part of the content, assuming the content is sent in chunks
 * 
 * @param   this  Memory slot in which to store the new message
 * @return        Follows the rules of `libqwaitclient_http_message_read`
 *                with one exception, if zero is returned the message
 *                has not been completely read, if 1 is returned the
 *                message has been completely read
 */
static int receive_chunked_transfer(_this_)
{
  size_t length, chunk_size, i;
  char* old_content;
  char* p;
  
 again:
  
  /* Wait for the line, that tells us how large the chunk is, has been received. */
  p = memchr(this->buffer, '\n', this->buffer_ptr * sizeof(char));
  if (p == NULL)
    return 0;
  
  /* Verify that the line is CRLF-terminated. */
  length = (size_t)(p - this->buffer);
  if ((length > 0) && (*(p - 1) == '\r'))
    length--;
  else
    return -2;
  
  /* Parse the size of the chunk and validate it. */
  chunk_size = 0;
  for (i = 0; i < length; i++)
    {
      char c = this->buffer[i];
      chunk_size <<= 4;
      if      (('0' <= c) && (c <= '9'))  chunk_size |= (size_t)(c - '0' + 0);
      else if (('a' <= c) && (c <= 'f'))  chunk_size |= (size_t)(c - 'a' + 10);
      else if (('A' <= c) && (c <= 'F'))  chunk_size |= (size_t)(c - 'A' + 10);
      else
	return -2; /* Malformated value, enters unrecoverable state. */
    }
  
  /* Wait for the chunk to be fully received. */
  if (this->buffer_ptr < length + 2 + chunk_size + 2)
    return 0;
  
  /* Verify that the chunk is CRLF-terminated. */
  if (this->buffer[length + 2 + chunk_size + 0] != '\r')  return -2;
  if (this->buffer[length + 2 + chunk_size + 1] != '\n')  return -2;
  
  /* Are we done yet? */
  if (chunk_size == 0)
    {
      /* Remove the chunk from the buffer. */
      unbuffer_beginning(this, length + 2 + chunk_size + 2, 1);
      /* If we have filled the content (or there was no content),
	 mark the end of this stage, i.e. that the message is
	 complete, and return with success. */
      this->stage = 3;
      return 1;
    }
  
  /* Copy the content from the buffer to the content storage. */
  this->content_size = this->content_ptr + chunk_size;
  old_content = this->content;
  if (xrealloc(this->content, this->content_size, char))
    {
      this->content = old_content;
      return -1;
    }
  memcpy(this->content + this->content_ptr, this->buffer + length + 2, chunk_size * sizeof(char));
  this->content_ptr = this->content_size;
  
  /* Remove the chunk from the buffer. */
  unbuffer_beginning(this, length + 2 + chunk_size + 2, 1);
  
  /* Do we have more to process? */
  if (this->buffer_ptr > 0)
    goto again;
  
  return 0;
}


/**
 * Read the next message from a file descriptor of the socket
 * 
 * @param   this  Memory slot in which to store the new message
 * @param   fd    The file descriptor of the socket
 * @return        Non-zero on error or interruption, errno will be
 *                set accordingly. Destroy the message on error,
 *                be aware that the reading could have been
 *                interrupted by a signal rather than canonical error.
 *                If -2 is returned errno will not have been set,
 *                -2 indicates that the message is malformated,
 *                which is a state that cannot be recovered from.
 */
int libqwaitclient_http_message_read(_this_, int fd)
{
  size_t header_commit_buffer = 0;
  int r;
  
  /* If we are at stage 3, we are done and it is time to start over.
     This is important because the function could have been interrupted. */
  if (this->stage == 3)
    {
      reset_message(this);
      this->stage = 0;
    }
  
  /* Read from file descriptor until we have a full message. */
  for (;;)
    {
      char* p;
      size_t length;
      
      /* Stage 0: status/request line. */
      if (this->stage == 0)
	{
	  /* Check that the line has been fully received. */
	  p = memchr(this->buffer, '\n', this->buffer_ptr * sizeof(char));
	  if (p == NULL)
	    goto need_more;
	  
	  /* Verift that the line is CRLF-terminated. */
	  length = (size_t)(p - this->buffer);
	  if ((length > 0) && (*(p - 1) == '\r'))
	    length--;
	  else
	    return -2;
	  
	  /* Create and store top line. */
	  try (store_top(this, length + 1));
	  
	  /* Mark end of stage, next stage is getting the headers. */
	  this->stage = 1;
	}
      
      
      /* Stage 1: headers. */
      /* Read all headers that we have stored into the read buffer. */
      while ((this->stage == 1) &&
	     ((p = memchr(this->buffer, '\n', this->buffer_ptr * sizeof(char))) != NULL))
	{
	  /* Verift that the line is CRLF-terminated. */
	  length = (size_t)(p - this->buffer);
	  if ((length > 0) && (*(p - 1) == '\r'))
	    length--;
	  else
	    return -2;
	  
	  if (length > 0)
	    {
	      /* We have found a header. */
	      
	      /* On every eighth header found with this function call,
		 we prepare the header list for eight more headers so
		 that it does not need to be reallocated again and again. */
	      if (header_commit_buffer == 0)
		try (libqwaitclient_http_message_extend_headers(this, header_commit_buffer = 8));
	      
	      /* Create and store header. */
	      try (store_header(this, length + 1));
	      header_commit_buffer -= 1;
	    }
	  else
	    {
	      /* We have found an empty line, i.e. the end of the headers. */
	      
	      /* Remove the header–content delimiter from the buffer,
		 get the content's size and allocate the content. */
	      try (initialise_content(this));
	      
	      /* Mark end of stage, next stage is getting the content. */
	      this->stage = 2;
	    }
	}
      
      
      /* Stage 2: content. */
      r = 0;
      if ((this->stage == 2) && (this->transfer_encoding == KNOWN_LENGTH))
	r = receive_known_length(this);
      else if ((this->stage == 2) && (this->transfer_encoding == CHUNKED_TRANSFER))
	r = receive_chunked_transfer(this);
      if (r)
	return r < 0 ? r : (r - 1);
      
      
      /* If stage 2 was not completed. */
    need_more:
      
      /* Continue reading from the socket into the buffer. */
      try (continue_read(this, fd));
    }
}


/**
 * Get the required allocation size for `data` of the
 * function `libqwaitclient_http_message_compose`
 * 
 * @param   this  The message
 * @return        The size of the message when marshalled
 */
size_t libqwaitclient_http_message_compose_size(const _this_)
{
  size_t rc = strlen(this->top) + 2;
  size_t i;
  for (i = 0; i < this->header_count; i++)
    rc += strlen(this->headers[i]) + 2;
  rc += 2 + this->content_size;
  return rc * sizeof(char);
}


/**
 * Marshal a message for communication
 * 
 * @param  this  The message
 * @param  data  Output buffer for the marshalled data
 */
void libqwaitclient_http_message_compose(const _this_, char* restrict data)
{
  size_t i, n;
  
  n = strlen(this->top);
  memcpy(data, this->top, n * sizeof(char));
  data += n;
  buf_set_next(data, char, '\r');
  buf_set_next(data, char, '\n');
  
  for (i = 0; i < this->header_count; i++)
    {
      n = strlen(this->headers[i]);
      memcpy(data, this->headers[i], n * sizeof(char));
      data += n;
      buf_set_next(data, char, '\r');
      buf_set_next(data, char, '\n');
    }
  buf_set_next(data, char, '\r');
  buf_set_next(data, char, '\n');
  
  if (this->content_size > 0)
    memcpy(data, this->content, this->content_size * sizeof(char));
}


#undef _this_

