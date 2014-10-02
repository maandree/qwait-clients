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
#include "queues.h"

#include "globals.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>



/**
 * Print basic information about a queue, that is: name,
 * title if different than name, queue size or locked,
 * and whether it is hidden
 * 
 * @return  queue  The queue
 */
static void print_queue_info(const libqwaitclient_qwait_queue_t* restrict queue)
{
  /* Locked? Otherwise queue size. */
  if (queue->locked)
    printf("\033[01;31mlocked\033[00m  ");
  else if (queue->position_count)
    printf("\033[01;32m%6zu\033[00m  ", queue->position_count);
  else
    printf("\033[00;00m     0\033[00m  ");
  
  /* Queue name, and title if different than the name. */
  if (strcmp(queue->name, queue->title))
    printf("%s (\"%s\")", queue->name, queue->title);
  else
    printf("%s", queue->name);
  
  /* Hidden? */
  if (queue->hidden)
    printf(" (hidden)");
  
  /* That was all a one-liners, lets end the line. */
  printf("\n");
}


/**
 * Print detailed information about a queue, this include
 * everything except the actual queue entries
 * 
 * @return  queue  The queue
 */
static void print_detailed_queue_info(const libqwaitclient_qwait_queue_t* restrict queue)
{
  size_t i, n;
  
  /* Queue name and title, even if identical. */
  printf("name: \033[01;34m%s\033[00m\n", queue->name);
  printf("title: %s\n", queue->title);
  
  /* Queue owners by user ID (not username). */
  printf("%s", queue->owner_count ? "owners" : "no owners");
  for (i = 0, n = queue->owner_count; i < n; i++)
    printf("%s%s", i ? ", " : ": ", queue->owners[i]);
  printf("\n");
  
  /* Queue moderators by user ID (not username). */
  printf("%s", queue->moderator_count ? "moderators" : "no moderators");
  for (i = 0, n = queue->moderator_count; i < n; i++)
    printf("%s%s", i ? ", " : ": ", queue->moderators[i]);
  printf("\n");
  
  /* Hidden? Locked? */
  printf(queue->hidden ?    "\033[31mhidden\033[00m\n" : "visible\n");
  printf(queue->locked ? "\033[01;31mlocked\033[00m\n" : "unlocked\n");
  
  /* Queue size. */
  if (queue->position_count)
    printf("queue size: \033[01;32m%zu\033[00m\n", queue->position_count);
  else
    printf("queue size: \033[00;00m0\033[00m\n");
}


/**
 * Print information about all queues (that have not been filtered)
 * 
 * @param   sock  A socket that is connected to the qwait server
 * @return        Zero on success, -1 on error
 */
int print_queues(libqwaitclient_http_socket_t* restrict sock)
{
  libqwaitclient_qwait_queue_t* restrict queues = NULL;
  size_t i, n;
  int first, saved_errno;
  int show_hidden  = 0;
  int show_locked  = 1;
  int show_empty   = 1;
  int show_details = 0;
  
  /* Parse command line for interresting flags. */
  for (i = 1, n = (size_t)argc; i < n; i++)
    if      (!strcmp(argv[i],      "--hidden"))  show_hidden  = 1;
    else if (!strcmp(argv[i],   "--no-hidden"))  show_hidden  = 0;
    else if (!strcmp(argv[i], "--only-hidden"))  show_hidden  = 2;
    else if (!strcmp(argv[i],      "--locked"))  show_locked  = 1;
    else if (!strcmp(argv[i],   "--no-locked"))  show_locked  = 0;
    else if (!strcmp(argv[i], "--only-locked"))  show_locked  = 2;
    else if (!strcmp(argv[i],       "--empty"))  show_empty   = 1;
    else if (!strcmp(argv[i],    "--no-empty"))  show_empty   = 0;
    else if (!strcmp(argv[i],  "--only-empty"))  show_empty   = 2;
    else if (!strcmp(argv[i], "--details"))      show_details = 1;
  
  /* Acquire queue. */
  if ((queues = libqwaitclient_qwait_get_queues(sock, &n)) == NULL)  goto fail;
  /* Sort queue by title. */
  qsort(queues, n, sizeof(libqwaitclient_qwait_queue_t),
	libqwaitclient_qwait_queue_compare_by_title);
  
  /* It is not possible to sort by anything else because it not
   * necessary to implement it in the program. For example, if
   * you want the queues sorted by queue size but have locked
   * queues at the bottom you simple run
   * 
   *     qwait-cmd --no-locked | sort -n | tac ; qwait-cmd --only-locked
   * 
   * Much easier than adding flags for specify every singly detail
   * for how to sort the queues and then implment that.
   */
  
  /* Print all queues. */
  for (i = 0, first = 1; i < n; i++)
    {
      const libqwaitclient_qwait_queue_t* queue = queues + i;
      
      /* Test filtering. */
      if ((show_hidden == 0) &&  queue->hidden)          continue;
      if ((show_locked == 0) &&  queue->locked)          continue;
      if ((show_empty  == 0) && !queue->position_count)  continue;
      if ((show_hidden == 2) && !queue->hidden)          continue;
      if ((show_locked == 2) && !queue->locked)          continue;
      if ((show_empty  == 2) &&  queue->position_count)  continue;
      
      /* Print queue. */
      if (show_details)
	{
	  if (!first)  printf("\n");
	  else         first = 0;
	  print_detailed_queue_info(queue);
	}
      else
	print_queue_info(queue);
    }
  
  errno = 0;
 fail:
  saved_errno = errno;
  for (i = 0; i < n; i++)
      libqwaitclient_qwait_queue_destroy(queues + i);
  free(queues);
  return errno = saved_errno, errno ? -1 : 0;
}

