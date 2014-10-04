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
#include "queue.h"

#include "globals.h"

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <sys/time.h>

#include <libqwaitclient/macros.h>


/**
 * Return the character length of a NUL-terminated UTF-8 string
 * 
 * @param   string  The string
 * @return          The length of the string in characters rather than bytes
 */
static size_t ustrlen(const char* string)
{
  size_t i, n, rc = 0;
  for (i = 0, n = strlen(string); i < n; i++)
    if ((string[i] & 0xC0) != 0x80)
      rc++;
  return rc;
}


/**
 * Get a colour for a location
 * 
 * @param   location  The location
 * @return            A colour for the location, `NULL` if the computer room is unknown
 */
static const char* get_location_colour(const char* location)
{
  int computer_room = libqwaitclient_computers_get_room(location);
  if (computer_room == LIBQWAITCLIENT_COMPUTERS_UKNOWN)
    return NULL;
  return libqwaitclient_computers_get_terminal_colour(computer_room, 0);
}


/**
 * Print a position
 * 
 * @param   position            The position
 * @param   is_help             Whether the position is a request for help
 * @param   show_id             Whether to show the user ID
 * @param   show_time           Whether to show the entry time as a wall-clock time
 *                              rather than difference, 2 for Unix time
 * @param   show_detailed_time  Whether to display detailed time
 * @param   max_real_name       The length of longest real name
 * @param   max_location        The length of longest location string
 * @param   max_comment         The length of longest comment
 * @param   now                 The current time
 * @return                      Zero on succes, -1 on error
 */
static int print_position(libqwaitclient_qwait_position_t* restrict position, int is_help,
                          int show_id, int show_time, int show_detailed_time,
                          size_t max_real_name, size_t max_location, size_t max_comment,
                          struct timespec* restrict now)
{
  libqwaitclient_qwait_position_time_t time;
  char* str_time;
  const char* loc_colour;
  int r;
  
  /* Get time string. */
  if (show_time != 2)
    {
      if (show_time)  r = libqwaitclient_qwait_position_parse_time(position, &time, 1);
      else            r = libqwaitclient_qwait_position_diff_time(position, &time, now);
      if (r < 0)
	return -1;
      str_time = libqwaitclient_qwait_position_string_time(&time, show_detailed_time);
      if (str_time == NULL)
	return -1;
    }
  else
    {
      if (xmalloc(str_time, 3 * sizeof(time_t) + 6, char))
	return -1;
      sprintf(str_time, "%ji.%03i",
	      (intmax_t)(position->enter_time_seconds),
	      position->enter_time_mseconds);
    }
  
#define S(X)  position->X, (int)(max_##X - ustrlen(position->X)), ""
  
  /* Print entry. */
  loc_colour = get_location_colour(position->location);
  printf("%s%*.s%s%s%s    \033[00;%s%sm%s%*.s\033[00m    \033[%sm%s%*.s\033[00m    %s\n",
	 S(real_name), show_id ? " (" : "", show_id ? position->user_id : "", show_id ? ")" : "",
	 loc_colour == NULL ? "00" : loc_colour, loc_colour == NULL ? "" : ";01", S(location),
	 is_help ? "01" : "00", S(comment),
	 str_time);
  
#undef S
  
  free(str_time);
  return 0;
}


/**
 * Print entries of a queue
 * 
 * @param   sock        A socket that is connected to the qwait server
 * @param   queue_name  The name of the queue
 * @return              Zero on success, -1 on error
 */
int print_queue(libqwaitclient_http_socket_t* restrict sock, const char* restrict queue_name)
{
  libqwaitclient_qwait_queue_t queue;
  struct timespec now;
  int saved_errno;
  size_t i, j, k, n, m;
  size_t max_real_name = 0, max_location = 0, max_comment = 0;
  int show_id = 0;
  int show_time = 0;
  int show_detailed_time = 0;
  int show_presentation = 1;
  int show_help = 1;
  
  /* Parse command line for interresting flags. */
  for (i = 1, n = (size_t)argc; i < n; i++)
    if      (!strcmp(argv[i], "--id"))             show_id = 1;
    else if (!strcmp(argv[i], "--time"))           show_time = 1;
    else if (!strcmp(argv[i], "--unix"))           show_time = 2;
    else if (!strcmp(argv[i], "--posix"))          show_time = 2;
    else if (!strcmp(argv[i], "--unix-time"))      show_time = 2;
    else if (!strcmp(argv[i], "--posix-time"))     show_time = 2;
    else if (!strcmp(argv[i], "--detailed-time"))  show_detailed_time = 1;
    else if (!strcmp(argv[i], "--help-only"))      show_presentation = 0;
    else if (!strcmp(argv[i], "--presentations"))  show_help = 0;
  
  /* Acquire queue. */
  if ((libqwaitclient_qwait_get_queue(sock, &queue, queue_name)) < 0)  goto fail;
  
  /* Get coloumn sizes. */
  for (i = 0, n = queue.position_count; i < n; i++)
    {
#define S(X)  len = ustrlen(position.X), max_##X = len < max_##X ? max_##X : len
      
      libqwaitclient_qwait_position_t position = queue.positions[i];
      size_t len;
      S(real_name); S(location); S(comment);
      
#undef S
    }
  
  /* Get time. */
  if (show_time == 0)
    if (clock_gettime(CLOCK_REALTIME, &now) < 0)
      goto fail;
  
  /* Print the queue. (It is already sorted.) */
  for (i = 0, n = queue.position_count; i < n; i++)
    {
      char comment_[7];
      libqwaitclient_qwait_position_t position = queue.positions[i];
      int is_help = 0;
      
      /* Is this a request for help. */
      m = strlen(position.comment);
      for (j = k = 0; (k < 6) && (j < m); j++)
	if ((k == 0) || (comment_[k - 1] != position.comment[j]))
	  comment_[k++] = position.comment[j];
      if (j < m)
	comment_[k++] = 'x';
      comment_[k] = '\0';
      is_help  = strcasestr(comment_, "hjälp") != NULL;
      is_help |= strcasestr(comment_, "hjÄlp") != NULL;
      is_help |= strcasestr(comment_, "help")  != NULL;
      
      /* Filter queue. */
      if (!show_presentation && !is_help)  continue;
      if (!show_help         &&  is_help)  continue;
      
      /* Print position. */
      if (print_position(&position, is_help,
			 show_id, show_time, show_detailed_time,
			 max_real_name, max_location, max_comment,
			 &now) < 0)
	goto fail;
    }
  
  errno = 0;
 fail:
  saved_errno = errno;
  libqwaitclient_qwait_queue_destroy(&queue);
  return errno = saved_errno, errno ? -1 : 0;
}


/**
 * Find and print the 0-based position in a queue for a student,
 * that is, the number of students before that student
 * 
 * @param   sock        A socket that is connected to the qwait server
 * @param   queue_name  The name of the queue
 * @param   user_id     The user's ID
 * @return              Zero on success, 1 if not found, -1 on error
 */
int print_queue_position(libqwaitclient_http_socket_t* restrict sock,
			 const char* restrict queue_name, const char* restrict user_id)
{
  libqwaitclient_qwait_queue_t queue;
  size_t i, n;
  int saved_errno, rc = 0;
  
  /* Acquire queue. */
  if ((libqwaitclient_qwait_get_queue(sock, &queue, queue_name)) < 0)  goto fail;
  
  /* Find the student's position. */
  for (i = 0, n = queue.position_count; i < n; i++)
    if (!strcmp(queue.positions[i].user_id, user_id))
      break;
  
  /* Print the position. */
  if (i == n)  printf("Not found\n"), rc = 1;
  else         printf("%zu\n", i);
  
  errno = 0;
 fail:
  saved_errno = errno;
  libqwaitclient_qwait_queue_destroy(&queue);
  return errno = saved_errno, errno ? -1 : rc;
}

