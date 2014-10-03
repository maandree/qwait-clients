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
#include "qwait-user.h"

#include "macros.h"

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>



#define  _this_  libqwaitclient_qwait_user_t* restrict this


/**
 * Initialises a user
 * 
 * @param  this  The user
 */
void libqwaitclient_qwait_user_initialise(_this_)
{
  memset(this, 0, sizeof(libqwaitclient_qwait_user_t));
}


/**
 * Destroy a user (release its resources but do not free it)
 * 
 * @param  this  The user
 */
void libqwaitclient_qwait_user_destroy(_this_)
{
  size_t i, n;
  free(this->user_id);
  free(this->real_name);
  for (i = 0, n = this->role_count; i < n; i++)
    free(this->roles[i]);
  free(this->roles);
  for (i = 0, n = this->owned_queue_count; i < n; i++)
    free(this->owned_queues[i]);
  free(this->owned_queues);
  for (i = 0, n = this->moderated_queue_count; i < n; i++)
    free(this->moderated_queues[i]);
  free(this->moderated_queues);
  for (i = 0, n = this->queue_count; i < n; i++)
    {
      free(this->positions[i].location);
      free(this->positions[i].comment);
      /* `user_id` and `real_name` are duplicates of the
	 `libqwaitclient_qwait_user_t` corresponding members. */
      free(this->queues[i]);
    }
  free(this->positions);
  free(this->queues);
  memset(this, 0, sizeof(libqwaitclient_qwait_user_t));
}


/**
 * Contextually parses parsed JSON data into a user
 * 
 * @param   this  The user to fill in
 * @param   data  The data to parse
 * @return        Zero on success, -1 on error
 */
int libqwaitclient_qwait_user_parse(_this_, const libqwaitclient_json_t* restrict data)
{
  const libqwaitclient_json_t* restrict data_user_id    = NULL;
  const libqwaitclient_json_t* restrict data_real_name  = NULL;
  const libqwaitclient_json_t* restrict data_admin      = NULL;
  const libqwaitclient_json_t* restrict data_anonymous  = NULL;
  const libqwaitclient_json_t* restrict data_roles      = NULL;
  const libqwaitclient_json_t* restrict data_queues     = NULL;
  const libqwaitclient_json_t* restrict data_owned      = NULL;
  const libqwaitclient_json_t* restrict data_moderated  = NULL;
  size_t i, n = data->length;
  int saved_errno;
  
  if (data->type != LIBQWAITCLIENT_JSON_TYPE_OBJECT)
    return errno = EINVAL, -1;
  
#define test(want)        ((strlen(want) == len) && !memcmp(name, want, len * sizeof(char)))
#define str(var, have)    ((have->type == LIBQWAITCLIENT_JSON_TYPE_NULL) ?	\
			    (var = NULL, 0) :					\
			    (var = libqwaitclient_json_to_zstr(have), var == NULL))
#define bool(var, have)   ((have->type != LIBQWAITCLIENT_JSON_TYPE_BOOLEAN) ?	\
			   (var = 0, errno = EINVAL, 1) :			\
			   (var = have->data.boolean, 0))
#define strs(var_, have)  ((this->var_##_count = have->length,					\
			    (this->var_##s = libqwaitclient_json_to_zstrs(have)) == NULL)	\
			   ? (errno ? (this->var_##_count = 0, 1) : 0) : 0)
  
  /* Read information. */
  for (i = 0; i < n; i++)
    {
      const libqwaitclient_json_t* restrict value = &(data->data.object[i].value);
      char* name = data->data.object[i].name;
      size_t len = data->data.object[i].name_length;
      
      if      (test("name"))             data_user_id   = value;
      else if (test("readableName"))     data_real_name = value;
      else if (test("admin"))            data_admin     = value;
      else if (test("anonymous"))        data_anonymous = value;
      else if (test("roles"))            data_roles     = value;
      else if (test("queuePositions"))   data_queues    = value;
      else if (test("ownedQueues"))      data_owned     = value;
      else if (test("moderatedQueues"))  data_moderated = value;
      else
	goto einval;
    }
  
  /* Check that everything was found. */
  if (data_user_id   == NULL)  goto einval;
  if (data_real_name == NULL)  goto einval;
  if (data_admin     == NULL)  goto einval;
  if (data_anonymous == NULL)  goto einval;
  if (data_roles     == NULL)  goto einval;
  if (data_queues    == NULL)  goto einval;
  if (data_owned     == NULL)  goto einval;
  if (data_moderated == NULL)  goto einval;
  
  /* Evaluate data. */
  if ( str(this->user_id,         data_user_id))    goto fail;
  if ( str(this->real_name,       data_real_name))  goto fail;
  if (bool(this->admin,           data_admin))      goto fail;
  if (bool(this->anonymous,       data_anonymous))  goto fail;
  if (strs(      role,            data_roles))      goto fail;
  if (strs(      owned_queue,     data_owned))      goto fail;
  if (strs(      moderated_queue, data_moderated))  goto fail;
  
  /* Evaluate data for queue positions. */
  if (data_queues->type != LIBQWAITCLIENT_JSON_TYPE_ARRAY)
    goto einval;
  if (xcalloc(this->positions, data_queues->length, libqwaitclient_qwait_position_t))  goto fail;
  if (xcalloc(this->queues,    data_queues->length, char*))                            goto fail;
  this->queue_count = n = data_queues->length;
  for (i = 0; i < n; i++)
    {
      const libqwaitclient_json_t* restrict data_location = NULL;
      const libqwaitclient_json_t* restrict data_comment  = NULL;
      const libqwaitclient_json_t* restrict data_queue    = NULL;
      const libqwaitclient_json_t* restrict data_time     = NULL;
      libqwaitclient_qwait_position_t* restrict pos = this->positions + i;
      libqwaitclient_json_association_t* restrict pos_data;
      size_t j, m;
      
      if (data_queues->data.array[i].type != LIBQWAITCLIENT_JSON_TYPE_OBJECT)
	goto einval;
      pos_data = data_queues->data.array[i].data.object;
      m        = data_queues->data.array[i].length;
      
      pos->user_id   = this->user_id;
      pos->real_name = this->real_name;
      
      /* Read information. */
      for (j = 0; j < m; j++)
	{
	  const libqwaitclient_json_t* restrict value = &(pos_data[j].value);
	  char* name = pos_data[j].name;
	  size_t len = pos_data[j].name_length;
	  
	  if      (test("location"))   data_location = value;
	  else if (test("comment"))    data_comment  = value;
	  else if (test("queueName"))  data_queue    = value;
	  else if (test("startTime"))  data_time     = value;
	  else
	    goto einval;
	}
      
      /* Check that everything was found. */
      if (data_location == NULL)  goto einval;
      if (data_comment  == NULL)  goto einval;
      if (data_queue    == NULL)  goto einval;
      if (data_time     == NULL)  goto einval;
      
      /* Evaluate data. */
      if (str(pos->location,   data_location)) goto fail;
      if (str(pos->comment,    data_comment))  goto fail;
      if (str(this->queues[i], data_queue))    goto fail;
      if (data_time->type != LIBQWAITCLIENT_JSON_TYPE_INTEGER)
	goto einval;
      pos->enter_time_seconds = (time_t)(data_time->data.integer / 1000);
      pos->enter_time_mseconds   = (int)(data_time->data.integer % 1000);
    }
  
#undef strs
#undef bool
#undef str
#undef test
  
  return 0;
  
 einval:
  errno = EINVAL;
 fail:
  saved_errno = errno;
  libqwaitclient_qwait_user_destroy(this);
  return errno = saved_errno, -1;
}


/**
 * Print a user to a file for debugging
 * 
 * @param  this    The user
 * @param  output  The output sink
 */
void libqwaitclient_qwait_user_dump(const _this_, FILE* output)
{
  size_t i, n;
  
  fprintf(output, "%s (%s)\n", this->real_name, this->user_id);
  fprintf(output, "  admin: %s\n",   this->admin     ? "yes" : "no");
  fprintf(output, "  anonymous: %s", this->anonymous ? "yes" : "no");
  
  fprintf(output, this->role_count ? "\n  roles" : "\n  no roles");
  for (i = 0, n = this->role_count; i < n; i++)
    fprintf(output, "%s%s", i ? ", " : ": ", this->roles[i]);
  
  fprintf(output, this->owned_queue_count ? "\n  owned queues" : "\n  no owned queues");
  for (i = 0, n = this->owned_queue_count; i < n; i++)
    fprintf(output, "%s%s", i ? ", " : ": ", this->owned_queues[i]);
  
  fprintf(output, this->moderated_queue_count ? "\n  moderated queues" : "\n  no moderated queues");
  for (i = 0, n = this->moderated_queue_count; i < n; i++)
    fprintf(output, "%s%s", i ? ", " : ": ", this->moderated_queues[i]);
  
  fprintf(output, this->queue_count ? "\n  queue entries:\n" : "\n  no queue entries\n");
  for (i = 0, n = this->queue_count; i < n; i++)
    {
      libqwaitclient_qwait_position_t pos = this->positions[i];
      
      fprintf(output, "    %s @ %s, %s, entered %ji.%03i\n",
	      this->queues[i], pos.location, pos.comment,
	      (intmax_t)(pos.enter_time_seconds), pos.enter_time_mseconds);
    }
}


#undef _this_

