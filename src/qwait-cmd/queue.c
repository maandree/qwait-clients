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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>



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
  int saved_errno;
  size_t i, j, n;
  int reverse = 0;
  
  /* Parse command line for interresting flags. */
  for (i = 1, n = (size_t)argc; i < n; i++)
    if (!strcmp(argv[i], "--reverse"))  reverse = 1;
  
  /* Acquire queue. */
  if ((libqwaitclient_qwait_get_queue(sock, &queue, queue_name)) < 0)  goto fail;
  
  /* Print the queue. (It is already sorted) */
  for (i = 0, n = queue.position_count; i < n; i++)
    {
      j = reverse ? (n - i - 1) : i;
      libqwaitclient_qwait_position_dump(queue.positions + j, stderr);
    }
  
  errno = 0;
 fail:
  saved_errno = errno;
  libqwaitclient_qwait_queue_destroy(&queue);
  return errno = saved_errno, errno ? -1 : 0;
}

