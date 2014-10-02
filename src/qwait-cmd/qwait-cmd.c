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

#include <libqwaitclient.h>

#include <stdio.h>
#include <string.h>


#define  t(expression)   if (expression)  goto fail


int main(int argc_, char** argv_)
{
  libqwaitclient_http_socket_t sock;
  int rc = 0;
  size_t i, j, n;
  char* nonopts[5];
  int list_queues = 0;
  
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
  
#define argeq(a, b)   ((a < j) && !strcmp(nonopts[a], b))
#define argeq1(A)     (argeq(0, A) && (j == 1))
#define argeq2(A, B)  (argeq(0, A) && argeq(1, B) && (j == 2))
  
  /* Parse filterd command line arguments. */
  if (argeq2("list", "queues") || argeq1("queues"))
    list_queues = 1;
  else
    goto invalid_command;
  
#undef argeq2
#undef argeq1
#undef argeq
  
  /* Connect to the server. */
  t (libqwaitclient_http_socket_initialise(&sock, QWAIT_SERVER_HOST, QWAIT_SERVER_PORT));
  t (libqwaitclient_http_socket_connect(&sock));
  
  t (list_queues && print_queues(&sock));
  
  /* Aced it! */
 done:
  /* Disconnect and destroy. */
  libqwaitclient_http_socket_disconnect(&sock);
  libqwaitclient_http_socket_destroy(&sock);
  return rc;
  
  /* I just don't know want when wrong! */
 fail:
  perror(*argv);
  rc = 1;
  goto done;
  
  /* The user did not specify a valid action. */
 invalid_command:
  fprintf(stderr, "What are you trying to do?\n");
  return 2;
}


#undef t

