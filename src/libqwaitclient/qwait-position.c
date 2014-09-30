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
#include "qwait-position.h"

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>


#define  _this_  libqwaitclient_qwait_position_t* restrict this


/**
 * Initialises a queue entry
 * 
 * @param  this  The queue entry
 */
void libqwaitclient_qwait_position_initialise(_this_)
{
  memset(this, 0, sizeof(libqwaitclient_qwait_position_t));
}


/**
 * Releases all resources a queue entry, but not the entry itself
 * 
 * @param  this  The queue entry
 */
void libqwaitclient_qwait_position_destroy(_this_)
{
  free(this->location);
  free(this->comment);
  free(this->user_id);
  free(this->real_name);
  memset(this, 0, sizeof(libqwaitclient_qwait_position_t));
}


/**
 * Contextually parses parsed JSON data into a queue entry
 * 
 * @param   this  The queue entry to fill in
 * @param   data  The data to parse
 * @return        Zero on success, -1 on error
 */
int libqwaitclient_qwait_position_parse(_this_, const libqwaitclient_json_t* restrict data)
{
  const libqwaitclient_json_t* restrict data_location   = NULL;
  const libqwaitclient_json_t* restrict data_comment    = NULL;
  const libqwaitclient_json_t* restrict data_user_id    = NULL;
  const libqwaitclient_json_t* restrict data_real_name  = NULL;
  const libqwaitclient_json_t* restrict data_enter_time = NULL;
  size_t i, n = data->length;
  int saved_errno;
  
  if (data->type != LIBQWAITCLIENTS_JSON_TYPE_OBJECT)
    return errno = EINVAL, -1;
  
#define test(want)      ((strlen(want) == len) && !memcmp(name, want, len * sizeof(char)))
#define str(var, have)  ((have->type == LIBQWAITCLIENTS_JSON_TYPE_NULL) ?	\
			  (var = NULL, 0) :					\
			  (var = libqwaitclient_json_to_zstr(have), var == NULL))
  
  /* Read information. */
  for (i = 0; i < n; i++)
    {
      const libqwaitclient_json_t* restrict value = &(data->data.object[i].value);
      char* name = data->data.object[i].name;
      size_t len = data->data.object[i].name_length;
      
      if      (test("location"))      data_location   = value;
      else if (test("comment"))       data_comment    = value;
      else if (test("userName"))      data_user_id    = value;
      else if (test("readableName"))  data_real_name  = value;
      else if (test("startTime"))     data_enter_time = value;
      else
	goto einval;
    }
  
  /* Check that everything was found. */
  if (data_location   == NULL)  goto einval;
  if (data_comment    == NULL)  goto einval;
  if (data_user_id    == NULL)  goto einval;
  if (data_real_name  == NULL)  goto einval;
  if (data_enter_time == NULL)  goto einval;
  
  /* Evaluate data. */
  if (str(this->location,  data_location))    goto fail;
  if (str(this->comment,   data_comment))     goto fail;
  if (str(this->user_id,   data_user_id))     goto fail;
  if (str(this->real_name, data_real_name))   goto fail;
  if (data_enter_time->type != LIBQWAITCLIENTS_JSON_TYPE_INTEGER)
    goto einval;
  this->enter_time_seconds = (time_t)(data_enter_time->data.integer / 1000);
  this->enter_time_mseconds   = (int)(data_enter_time->data.integer % 1000);
  
#undef str
#undef test
  
  return 0;
  
 einval:
  errno = EINVAL;
 fail:
  saved_errno = errno;
  libqwaitclient_qwait_position_destroy(this);
  return errno = saved_errno, -1;
}


/**
 * Compares the time of entry for two queue entries
 * 
 * @param   a  -1 is returned if this entry entered the queue before `b`
 * @param   b  1 is returned if this entry entered the queue before `a`
 * @return     See `a` and `b`, and refer to `qsort(3)`, `strcmp(3)`, et cetera; earlier entries first
 */
int libqwaitclient_qwait_position_compare_by_time(const void* a, const void* b)
{
  const libqwaitclient_qwait_position_t* a_ = a;
  const libqwaitclient_qwait_position_t* b_ = b;
  
#define cmp(p, q)  ((q) < (p) ? -1 : (q) > (p) ? 1 : 0)
  
  if (a_->enter_time_seconds == b_->enter_time_seconds)
    return cmp(a_->enter_time_mseconds, b_->enter_time_mseconds);
  return cmp(a_->enter_time_seconds, b_->enter_time_seconds);
  
#undef cmp
}


/**
 * Print a queue entry to a file for debugging
 * 
 * @param  this    The queue entry
 * @param  output  The output sink
 */
void libqwaitclient_qwait_position_dump(_this_, FILE* output)
{
  fprintf(output, "\"%s\"(%s) @ %s: %s, entered %ji.%03i\n",
	  this->real_name, this->user_id, this->location, this->comment,
	  (intmax_t)(this->enter_time_seconds), this->enter_time_mseconds);
}


#undef _this_

