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
#ifndef LIBQWAITCLIENT_WEBMESSAGE_H
#define LIBQWAITCLIENT_WEBMESSAGE_H


#define _GNU_SOURCE
#include <stddef.h>


/**
 * Message passed between the server and the client over a websocket
 */
typedef struct libqwaitclient_webmessage
{
  /**
   * The content of the message, `NULL` if none (of zero-length)
   */
  char* content;
  
  /**
   * The size of the content
   */
  size_t content_size;
  
  /**
   * How much of the content that has been stored (internal data)
   */
  size_t content_ptr;
  
  /**
   * Internal buffer for the reading function (internal data)
   */
  char* buffer;
  
  /**
   * The size allocated to `buffer` (internal data)
   */
  size_t buffer_size;
  
  /**
   * The number of bytes used in `buffer` (internal data)
   */
  size_t buffer_ptr;
  
} libqwaitclient_webmessage_t;


#define _this_ libqwaitclient_webmessage_t* restrict this


/**
 * Zero initialise a message slot
 * 
 * @param  this  Memory slot in which to store the new message
 */
void libqwaitclient_webmessage_zero_initialise(_this_);

/**
 * Release all resources in a message, should
 * be done even if initialisation fails
 * 
 * @param  this  The message
 */
void libqwaitclient_webmessage_destroy(_this_);

/**
 * Read the next message from a file descriptor
 * 
 * @param   this  Memory slot in which to store the new message
 * @param   fd    The file descriptor
 * @return        Non-zero on error or interruption, errno will be
 *                set accordingly. Destroy the message on error,
 *                be aware that the reading could have been
 *                interrupted by a signal rather than canonical error.
 *                If -2 is returned errno will not have been set,
 *                -2 indicates that the message is malformated,
 *                which is a state that cannot be recovered from.
 */
int libqwaitclient_webmessage_read(_this_, int fd);

/**
 * Get the required allocation size for `data` of the
 * function `libqwaitclient_webmessage_compose`
 * 
 * @param   this  The message
 * @return        The size of the message when marshalled
 */
size_t libqwaitclient_webmessage_compose_size(const _this_) __attribute__((pure));

/**
 * Marshal a message for communication
 * 
 * @param  this  The message
 * @param  data  Output buffer for the marshalled data
 */
void libqwaitclient_webmessage_compose(const _this_, char* restrict data);


#undef _this_

#endif

