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
#include "macros.h"
#include "config.h"
#include "http-socket.h"
#include "qwait-protocol.h"

#include <stdio.h>
#include <stdlib.h>


#define t(expression)   if (expression)  goto fail


int main(int argc, char** argv)
{
  libqwaitclient_http_socket_t sock;
  libqwaitclient_qwait_queue_t* queues = NULL;
  size_t i, n;
  int rc = 0;
  
  (void) argc;
  
  t (libqwaitclient_http_socket_initialise(&sock, QWAIT_SERVER_HOST, QWAIT_SERVER_PORT));
  t (libqwaitclient_http_socket_connect(&sock));
  
  t ((queues = libqwaitclient_qwait_get_queues(&sock, &n)) == NULL);
  for (i = 0; i < n; i++)
    {
      printf(i ? "\n" : "");
      libqwaitclient_qwait_queue_dump(queues + i, stdout);
    }
  
 done:
  libqwaitclient_http_socket_disconnect(&sock);
  libqwaitclient_http_socket_destroy(&sock);
  for (i = 0; i < n; i++)
      libqwaitclient_qwait_queue_destroy(queues + i);
  free(queues);
  return rc;
  
 fail:
  perror(*argv);
  rc = 1;
  goto done;
}

