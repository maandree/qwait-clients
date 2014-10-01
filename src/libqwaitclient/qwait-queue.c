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
#include "qwait-queue.h"

#include "macros.h"

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <setjmp.h>


#define  _this_  libqwaitclient_qwait_queue_t* restrict this


/**
 * Initialises a queue
 * 
 * @param  this  The queue
 */
void libqwaitclient_qwait_queue_initialise(_this_)
{
  memset(this, 0, sizeof(libqwaitclient_qwait_queue_t));
}


/**
 * Releases all resources a queue, but not the queue itself
 * 
 * @param  this  The queue
 */
void libqwaitclient_qwait_queue_destroy(_this_)
{
  size_t i, n;
  free(this->name);
  free(this->title);
  for (i = 0, n = this->owner_count;     i < n; i++)  free(this->owners[i]);
  for (i = 0, n = this->moderator_count; i < n; i++)  free(this->moderators[i]);
  for (i = 0, n = this->position_count;  i < n; i++)
    libqwaitclient_qwait_position_destroy(this->positions + i);
  free(this->owners);
  free(this->moderators);
  free(this->positions);
  memset(this, 0, sizeof(libqwaitclient_qwait_queue_t));
}


/**
 * Contextually parses parsed JSON data into a queue
 * 
 * @param   this  The queue to fill in
 * @param   data  The data to parse
 * @return        Zero on success, -1 on error
 */
int libqwaitclient_qwait_queue_parse(_this_, const libqwaitclient_json_t* restrict data)
{
  const libqwaitclient_json_t* restrict data_name       = NULL;
  const libqwaitclient_json_t* restrict data_title      = NULL;
  const libqwaitclient_json_t* restrict data_hidden     = NULL;
  const libqwaitclient_json_t* restrict data_locked     = NULL;
  const libqwaitclient_json_t* restrict data_owners     = NULL;
  const libqwaitclient_json_t* restrict data_moderators = NULL;
  const libqwaitclient_json_t* restrict data_positions  = NULL;
  size_t i, n = data->length;
  int saved_errno;
  
  if (data->type != LIBQWAITCLIENT_JSON_TYPE_OBJECT)
    return errno = EINVAL, -1;
  
#define test(want)  ((strlen(want) == len) && !memcmp(name, want, len * sizeof(char)))
  
  /* Read information. */
  for (i = 0; i < n; i++)
    {
      const libqwaitclient_json_t* restrict value = &(data->data.object[i].value);
      char* name = data->data.object[i].name;
      size_t len = data->data.object[i].name_length;
      
      if      (test("name"))        data_name       = value;
      else if (test("title"))       data_title      = value;
      else if (test("hidden"))      data_hidden     = value;
      else if (test("locked"))      data_locked     = value;
      else if (test("owners"))      data_owners     = value;
      else if (test("moderators"))  data_moderators = value;
      else if (test("positions"))   data_positions  = value;
      else
	goto einval;
    }
  
  /* Check that everything was found. */
  if (data_name       == NULL)  goto einval;
  if (data_title      == NULL)  goto einval;
  if (data_hidden     == NULL)  goto einval;
  if (data_locked     == NULL)  goto einval;
  if (data_owners     == NULL)  goto einval;
  if (data_moderators == NULL)  goto einval;
  if (data_positions  == NULL)  goto einval;
  
  /* Evaluate data. */
  if ((this->name       = libqwaitclient_json_to_zstr(data_name))    == NULL)  goto fail;
  if ((this->title      = libqwaitclient_json_to_zstr(data_title))   == NULL)  goto fail;
  if ((this->hidden     = libqwaitclient_json_to_bool(data_hidden))  < 0)      goto fail;
  if ((this->locked     = libqwaitclient_json_to_bool(data_locked))  < 0)      goto fail;
  if ((this->owners     = libqwaitclient_json_to_zstrs(data_owners)) == NULL)
    if (errno)
      goto fail;
  this->owner_count = data_owners->length;
  if ((this->moderators = libqwaitclient_json_to_zstrs(data_moderators)) == NULL)
    if (errno)
      goto fail;
  this->moderator_count = data_moderators->length;
  if (data_positions->type != LIBQWAITCLIENT_JSON_TYPE_ARRAY)
    goto einval;
  n = data_positions->length;
  if (xcalloc(this->positions, n, libqwaitclient_qwait_position_t))  goto fail;
  for (i = 0; i < n; i++, this->position_count++)
    if (libqwaitclient_qwait_position_parse(this->positions + i, data_positions->data.array + i) < 0)
      goto fail;
  
  /* Order positions by time. */
  qsort(this->positions, this->position_count,
	sizeof(libqwaitclient_qwait_position_t),
	libqwaitclient_qwait_position_compare_by_time);
  
  return 0;
  
 einval:
  errno = EINVAL;
 fail:
  saved_errno = errno;
  libqwaitclient_qwait_queue_destroy(this);
  return errno = saved_errno, -1;
  
#undef test
}


/**
 * Compares the title of queues
 * 
 * @param   a  -1 is returned if this queue is an alphabetically lower title than `b`
 * @param   b  1 is returned if this queue is an alphabetically lower title than `a`
 * @return     See `a` and `b`, and refer to `qsort(3)`, `strcmp(3)`, et cetera; ascending order
 */
int libqwaitclient_qwait_queue_compare_by_title(const void* a, const void* b)
{
  const libqwaitclient_qwait_queue_t* a_ = a;
  const libqwaitclient_qwait_queue_t* b_ = b;
  return strcmp(a_->title, b_->title);
}


/**
 * Print a queue to a file for debugging
 * 
 * @param  this    The queue entry
 * @param  output  The output sink
 */
void libqwaitclient_qwait_queue_dump(const _this_, FILE* output)
{
  size_t i, n;
  
  fprintf(output, "queue \"%s\" (%s)", this->title, this->name);
  
  fprintf(output, this->owner_count ? "\n  owners" : "\n  no owners");
  for (i = 0, n = this->owner_count; i < n; i++)
    fprintf(output, "%s%s", i ? ", " : ": ", this->owners[i]);
  
  fprintf(output, this->moderator_count ? "\n  moderators" : "\n  no moderators");
  for (i = 0, n = this->moderator_count; i < n; i++)
    fprintf(output, "%s%s", i ? ", " : ": ", this->moderators[i]);
  
  fprintf(output, this->position_count ? "\n  entries\n" : "\n  no entries\n");
  for (i = 0, n = this->position_count; i < n; i++)
    {
      fprintf(output, "    ");
      libqwaitclient_qwait_position_dump(this->positions + i, output);
    }
}


#undef _this_

