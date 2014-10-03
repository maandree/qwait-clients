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
#include "user.h"

#include "globals.h"

#include <libqwaitclient/macros.h>

#include <string.h>
#include <stdlib.h>
#include <errno.h>


/**
 * Print information about a user
 * 
 * @param   sock     A socket that is connected to the qwait server
 * @param   user_id  The Id of the user
 * @return           Zero on success, -1 on error
 */
int print_user_information(libqwaitclient_http_socket_t* restrict sock, const char* restrict user_id)
{
  size_t max_queue = 0, max_location = 0, max_comment = 0;
  libqwaitclient_qwait_user_t user;
  size_t i, n;
  int saved_errno;
  struct timespec now;
  char* str_time = NULL;
  int show_time = 0;
  int show_detailed_time = 0;
  
  /* Parse command line for interresting flags. */
  for (i = 1, n = (size_t)argc; i < n; i++)
    if      (!strcmp(argv[i], "--time"))           show_time = 1;
    else if (!strcmp(argv[i], "--unix"))           show_time = 2;
    else if (!strcmp(argv[i], "--posix"))          show_time = 2;
    else if (!strcmp(argv[i], "--unix-time"))      show_time = 2;
    else if (!strcmp(argv[i], "--posix-time"))     show_time = 2;
    else if (!strcmp(argv[i], "--detailed-time"))  show_detailed_time = 1;
  
  /* Acquire user information. */
  if ((libqwaitclient_qwait_get_user(sock, &user, user_id)) < 0)  goto fail;
  
  /* Print user information. */
  printf("%s (%s)\n", user.real_name, user.user_id);
  printf("%s\n",      user.admin     ? "\033[01;31madmin\033[00m"     : "not admin");
  printf("%s",        user.anonymous ? "\033[01;35manonymous\033[00m" : "not anonymous");
  printf(user.role_count ? "\nroles" : "\nno roles");
  for (i = 0, n = user.role_count; i < n; i++)
    printf("%s%s", i ? ", " : ": ", user.roles[i]);
  printf(user.owned_queue_count ? "\nowned queues" : "\nno owned queues");
  for (i = 0, n = user.owned_queue_count; i < n; i++)
    printf("%s%s", i ? ", " : ": ", user.owned_queues[i]);
  printf(user.moderated_queue_count ? "\nmoderated queues" : "\nno moderated queues");
  for (i = 0, n = user.moderated_queue_count; i < n; i++)
    printf("%s%s", i ? ", " : ": ", user.moderated_queues[i]);
  
  /* Get time. */
  if (show_time == 0)
    if (clock_gettime(CLOCK_REALTIME, &now) < 0)
      goto fail;
  
  /* Print entered queues and the user's position in those queues. */
  printf(user.queue_count ? "\n\nqueue entries:\n" : "\nno queue entries\n");
  if (user.queue_count == 0)
    goto done;
  for (i = 0, n = user.queue_count; i < n; i++)
    {
      max_queue    = max(strlen(user.queues[i]),             max_queue);
      max_location = max(strlen(user.positions[i].location), max_location);
      max_comment  = max(strlen(user.positions[i].comment),  max_comment);
    }
  for (i = 0, n = user.queue_count; i < n; i++)
    {
      libqwaitclient_qwait_position_t pos = user.positions[i];
      libqwaitclient_qwait_position_time_t time;
      int r;
      
      /* Get time string. */
      free(str_time);
      if (show_time != 2)
	{
	  if (show_time)  r = libqwaitclient_qwait_position_parse_time(&pos, &time, 1);
	  else            r = libqwaitclient_qwait_position_diff_time(&pos, &time, &now);
	  if (r < 0)
	    goto fail;
	  str_time = libqwaitclient_qwait_position_string_time(&time, show_detailed_time);
	  if (str_time == NULL)
	    goto fail;
	}
      else
	{
	  if (xmalloc(str_time, 3 * sizeof(time_t) + 6, char))
	    goto fail;
	  sprintf(str_time, "%ji.%03i",
		  (intmax_t)(pos.enter_time_seconds),
		  pos.enter_time_mseconds);
	}
      
      /* Print entry. */
      printf("%s:%*.s    %s%*.s    %s%*.s    %s\n",
	     user.queues[i], max_queue    - strlen(user.queues[i]), "",
	     pos.location,   max_location - strlen(pos.location),   "",
	     pos.comment,    max_comment  - strlen(pos.comment),    "",
	     str_time);
    }
  
 done:
  errno = 0;
 fail:
  saved_errno = errno;
  free(str_time);
  libqwaitclient_qwait_user_destroy(&user);
  return errno = saved_errno, errno ? -1 : 0;
}

