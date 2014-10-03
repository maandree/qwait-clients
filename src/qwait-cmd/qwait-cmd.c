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


/**
 * Command line client for qwait
 * 
 * @param   argc_  The number of elements in `argv_`
 * @param   argv_  Command line arguments, including the command name
 * @return         Zero on and only on success
 */
int main(int argc_, char** argv_)
{
  libqwaitclient_http_socket_t sock;
  int r = 0, rc = 0;
  size_t i, j, n;
  char* nonopts[10];
  int action_list_queues = 0;
  int action_print_queue = 0;
  int action_find_in_queue = 0;
  int action_list_owned = 0;
  int action_list_moderated = 0;
  
  /* Globalise the command line arguments. */
  argc = argc_;
  argv = argv_;
  
  /* Get arguments that this function cares about. */
  for (i = 1, j = 0, n = (size_t)argc; i < n; i++)
    {
      if (argv[i][0] == '-')
	continue;
      
      if (j == sizeof(nonopts) / sizeof(*nonopts))
	goto invalid_command;
      
      nonopts[j++] = argv[i];
    }
  
#define argeq(a, b)            (!strcmp(nonopts[a], b))
#define argeq1(A, c)           ((c == j) && argeq(0, A))
#define argeq2(A, B, c)        ((c == j) && argeq(0, A) && argeq(1, B))
#define argeq4(A, B, C, D, c)  ((c == j) && argeq(0, A) && argeq(1, B) && argeq(2, C) && argeq(3, D))
  
  /* Parse filtered command line arguments. */
  if      (argeq2("list", "queues", 2) || argeq1("queues", 1))         action_list_queues = 1;
  else if (argeq2("print", "queue", 3) || argeq2("view", "queue", 3))  action_print_queue = 1;
  else if ((j == 4) && argeq(0, "find") && argeq(2, "in"))             action_find_in_queue = 1;
  else if (argeq4("list", "queues", "owned", "by", 5))                 action_list_owned = 1;
  else if (argeq4("list", "queues", "moderated", "by", 5))             action_list_moderated = 1;
  else
    goto invalid_command;
  
#undef argeq4
#undef argeq2
#undef argeq1
#undef argeq
  
  /* Connect to the server. */
  t (libqwaitclient_http_socket_initialise(&sock, QWAIT_SERVER_HOST, QWAIT_SERVER_PORT));
  t (libqwaitclient_http_socket_connect(&sock));
  
#define ta(cond, fun, ...)  t ((cond) && (r = fun(__VA_ARGS__), r < 0))
  
  /* Take action! */
  ta (action_list_queues,    print_queues,           &sock);
  ta (action_print_queue,    print_queue,            &sock, nonopts[2]);
  ta (action_find_in_queue,  print_queue_position,   &sock, nonopts[3], nonopts[1]);
  ta (action_list_owned,     print_owned_queues,     &sock, nonopts[4]);
  ta (action_list_moderated, print_moderated_queues, &sock, nonopts[4]);
  if (r >= 0)
    rc = r;
  
#undef ta
  
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

