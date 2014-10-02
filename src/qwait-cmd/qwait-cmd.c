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
#include "globals.h"
#include "queues.h"
#include "queue.h"

#include <libqwaitclient.h>

#include <stdio.h>
#include <string.h>


#define  t(expression)   if (expression)  goto fail


int main(int argc_, char** argv_)
{
  libqwaitclient_http_socket_t sock;
  int r = 0, rc = 0;
  size_t i, j, n;
  char* nonopts[5];
  int action_list_queues = 0;
  int action_print_queue = 0;
  int action_find_in_queue = 0;
  
  /* Globalise the command line arguments. */
  argc = argc_;
  argv = argv_;
  
  /* Get arguments that these function cares about. */
  for (i = 1, j = 0, n = (size_t)argc; i < n; i++)
    {
      if (argv[i][0] == '-')
	continue;
      
      if (j == sizeof(nonopts) / sizeof(*nonopts))
	goto invalid_command;
      
      nonopts[j++] = argv[i];
    }
  
#define argeq(a, b)      ((a < j) && !strcmp(nonopts[a], b))
#define argeq1(A, c)     (argeq(0, A) && (c == j))
#define argeq2(A, B, c)  (argeq(0, A) && argeq(1, B) && (c == j))
  
  /* Parse filterd command line arguments. */
  if      (argeq2("list", "queues", 2) || argeq1("queues", 1))         action_list_queues = 1;
  else if (argeq2("print", "queue", 3) || argeq2("view", "queue", 3))  action_print_queue = 1;
  else if (argeq(0, "find") && argeq(2, "in") && (j == 4))             action_find_in_queue = 1;
  else
    goto invalid_command;
  
#undef argeq2
#undef argeq1
#undef argeq
  
  /* Connect to the server. */
  t (libqwaitclient_http_socket_initialise(&sock, QWAIT_SERVER_HOST, QWAIT_SERVER_PORT));
  t (libqwaitclient_http_socket_connect(&sock));
  
  /* Take action! */
  t (action_list_queues   && (r = print_queues(&sock), r < 0));
  t (action_print_queue   && (r = print_queue(&sock, nonopts[2]), r < 0));
  t (action_find_in_queue && (r = print_queue_position(&sock, nonopts[3], nonopts[1]), r < 0));
  if (r >= 0)
    rc = r;
  
  /* Aced it! */
 done:
  /* Disconnect and destroy. */
  libqwaitclient_http_socket_disconnect(&sock);
  libqwaitclient_http_socket_destroy(&sock);
  return rc;
  
  /* I just don't know want when wrong! */
 fail:
  perror(*argv);
  rc = 2;
  goto done;
  
  /* The user did not specify a valid action. */
 invalid_command:
  fprintf(stderr, "What are you trying to do?\n");
  return 3;
}


#undef t

