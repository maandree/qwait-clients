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


#define _this_ libqwaitclient_webmessage_t* restrict this



/**
 * Zero initialise a message slot
 * 
 * @param  this  Memory slot in which to store the new message
 */
void libqwaitclient_webmessage_zero_initialise(_this_)
{
  this->content = NULL;
  this->content_size = 0;
  this->content_ptr = 0;
  this->buffer = NULL;
  this->buffer_size = 0;
  this->buffer_ptr = 0;
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
int libqwaitclient_webmessage_read(_this_, int fd)
{
  /* TODO libqwaitclient_webmessage_read */
  (void) this;
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

