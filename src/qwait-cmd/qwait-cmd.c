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
#include "authentication.h"
#include "user.h"
#include "users.h"

#include <libqwaitclient.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>


#define  t(expression)   if (expression)  goto fail
#define  abs(value)      (value < 0 ? -value : value)


/**
 * Check whether the command line arguments, excluding dash-options, matches a pattern
 * 
 * @param   verbs               The command line arguments, excluding dash-options
 * @param   verb_count          The number of elements in `verbs`
 * @param   ...:const char*...  `NULL`-terminated pattern, "" for wildcards
 * @return                      Whether the pattern matched
 */
static int test_verbs(char* const* verbs, size_t verb_count, ...)
{
  char* p;
  va_list ap;
  size_t i = 0;
  
  va_start(ap, verb_count);
  
  for (;; i++)
    {
      p = va_arg(ap, char*);
      if (p == NULL)
	break;
      if (i == verb_count)
	{
	  va_end(ap);
	  return 0;
	}
      if (*p == '\0')
	continue;
      if (strcmp(verbs[i], p))
	break;
    }
  
  va_end(ap);
  return i == verb_count;
}


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
  int r = 0, rc = 0, have_sock = 0;
  size_t i, j, n;
  char* nonopts[10];
  int action_list_queues = 0;
  int action_print_queue = 0;
  int action_find_in_queue = 0;
  int action_list_owned = 0;
  int action_list_moderated = 0;
  int action_log_in = 0;
  int action_log_out = 0;
  int action_stat_user = 0;
  int action_who_am_i = 0;
  int action_list_admins = 0;
  int action_list_users = 0;
  int action_find_user = 0;
  int action_lock_queue = 0;
  int action_hide_queue = 0;
  int action_clear_queue = 0;
  int action_delete_queue = 0;
  int action_create_queue = 0;
  int action_set_admin = 0;
  int action_set_moderator = 0;
  int action_set_owner = 0;
  int action_set_wait = 0;
  int action_set_comment = 0;
  int action_set_location = 0;
  
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
#define argeq3(A, B, C, c)     ((c == j) && argeq(0, A) && argeq(1, B) && argeq(2, C))
#define argeq4(A, B, C, D, c)  ((c == j) && argeq(0, A) && argeq(1, B) && argeq(2, C) && argeq(3, D))
#define argeqn(...)            (test_verbs(nonopts, j, __VA_ARGS__))
#define is_user_id(i)          (strstr(nonopts[i], "u1") == nonopts[i])
#define x                      ""   /* Wildcard in argeqn */
#define e                      NULL /* End-of-arguments sentinel in argeqn */
  
  /* Parse filtered command line arguments. */
  if      (argeq2("list", "queues", 2) || argeq1("queues", 1))           action_list_queues = 1;
  else if (argeq2("print", "queue", 3) || argeq2("view", "queue", 3))    action_print_queue = 1;
  else if (argeqn("find", x, "in", x, e))                                action_find_in_queue = 1;
  else if (argeq4("list", "queues", "owned", "by", 5))                   action_list_owned = 1;
  else if (argeq4("list", "queues", "moderated", "by", 5))               action_list_moderated = 1;
  else if (argeq3("log", "in", "as", 4))                                 action_log_in = 3;
  else if (argeq2("login", "as", 3))                                     action_log_in = 2;
  else if (argeq2("log", "in", 2) || argeq1("login", 1))                 action_log_in = -1;
  else if (argeq2("log", "out", 2) || argeq1("logout", 1))               action_log_out = 1;
  else if (argeq2("stat", "user", 3))                                    action_stat_user = 2;
  else if (argeq1("stat", 2) && is_user_id(1))                           action_stat_user = 1;
  else if (argeq3("who", "am", "I", 3) || argeq3("who", "am", "i", 3))   action_who_am_i = 1;
  else if (argeq2("list", "admins", 2))                                  action_list_admins = 1;
  else if (argeq2("list", "administrators", 2))                          action_list_admins = 1;
  else if (argeq2("list", "users", 2))                                   action_list_users = 1;
  else if (argeq2("find", "user", 3))                                    action_find_user = 1;
  else if (argeq1("lock", 2))                                            action_lock_queue = 1; /* XXX test */
  else if (argeq1("unlock", 2))                                          action_lock_queue = -1; /* XXX test */
  else if (argeq1("hide", 2))                                            action_hide_queue = 1; /* XXX test */
  else if (argeq1("unhide", 2))                                          action_hide_queue = -1; /* XXX test */
  else if (argeq1("clear", 2))                                           action_clear_queue = 1;
  else if (argeq1("delete", 2))                                          action_delete_queue = 1; /* XXX test */
  else if (argeq1("create", 2))                                          action_create_queue = 1; /* XXX test */
  else if (argeqn("add",    x, "as",       "admin",         e))          action_set_admin = 1; /* XXX test */
  else if (argeqn("add",    x, "as",       "administrator", e))          action_set_admin = 1;
  else if (argeqn("add",    x, "as", "an", "admin",         e))          action_set_admin = 1;
  else if (argeqn("add",    x, "as", "an", "administrator", e))          action_set_admin = 1;
  else if (argeqn("remove", x, "as",       "admin",         e))          action_set_admin = -1; /* XXX test */
  else if (argeqn("remove", x, "as",       "administrator", e))          action_set_admin = -1;
  else if (argeqn("remove", x, "as", "an", "admin",         e))          action_set_admin = -1;
  else if (argeqn("remove", x, "as", "an", "administrator", e))          action_set_admin = -1;
  else if (argeqn("add",    x, "as",       "moderator", "of", x, e))     action_set_moderator = 5; /* XXX test */
  else if (argeqn("add",    x, "as", "a",  "moderator", "of", x, e))     action_set_moderator = 6;
  else if (argeqn("remove", x, "as",       "moderator", "of", x, e))     action_set_moderator = -5; /* XXX test */
  else if (argeqn("remove", x, "as", "a",  "moderator", "of", x, e))     action_set_moderator = -6;
  else if (argeqn("add",    x, "as",       "owner",     "of", x, e))     action_set_owner = 5; /* XXX test */
  else if (argeqn("add",    x, "as", "a",  "owner",     "of", x, e))     action_set_owner = 6;
  else if (argeqn("remove", x, "as",       "owner",     "of", x, e))     action_set_owner = -5; /* XXX test */
  else if (argeqn("remove", x, "as", "an", "owner",     "of", x, e))     action_set_owner = -6;
  else if (argeqn("add",    x, "to",   x, e))                            action_set_wait = 3;
  else if (argeqn("remove", x, "from", x, e))                            action_set_wait = -3;
  else if (argeqn("set",    "comment",  "for", x, "in", x, "to", x, e))  action_set_comment = 1;
  else if (argeqn("change", "comment",  "for", x, "in", x, "to", x, e))  action_set_comment = 1;
  else if (argeqn("set",    "location", "for", x, "in", x, "to", x, e))  action_set_location = 1;
  else if (argeqn("change", "location", "for", x, "in", x, "to", x, e))  action_set_location = 1;
  else
    goto invalid_command;
  /* TODO test action_set_wait, action_set_comment and action_set_location upon others. */
  
#undef e
#undef x
#undef is_user_id
#undef argeq4
#undef argeq3
#undef argeq2
#undef argeq1
#undef argeq
  
#define ta(cond, fun, ...)  t ((cond) && (r = fun(__VA_ARGS__), r < 0))
  
  /* Special commands that do not require a connection to the QWait server. */
  if (action_log_in || action_log_out || action_who_am_i)
    {
      ta (action_log_in,  authenticate, (action_log_in == -1) ? "" : nonopts[action_log_in]);
      ta (action_log_out, authenticate, NULL);
      t  (action_who_am_i && (r = print_user_id(), r < 0));
      if (r >= 0)
	rc = r;
      goto done;
    }
  
  /* Connect to the server. */
  have_sock = 1;
  t (libqwaitclient_http_socket_initialise(&sock, QWAIT_SERVER_HOST, QWAIT_SERVER_PORT));
  t (libqwaitclient_http_socket_connect(&sock));
  
  /* Take action! */
  ta (action_list_queues,    print_queues,           &sock);
  ta (action_print_queue,    print_queue,            &sock, nonopts[2]);
  ta (action_find_in_queue,  print_queue_position,   &sock, nonopts[3], nonopts[1]);
  ta (action_list_owned,     print_owned_queues,     &sock, nonopts[4]);
  ta (action_list_moderated, print_moderated_queues, &sock, nonopts[4]);
  ta (action_stat_user,      print_user_information, &sock, nonopts[action_stat_user]);
  ta (action_list_admins,    print_users,            &sock, QWAIT_CMD_USERS_ADMINS);
  ta (action_list_users,     print_users,            &sock, QWAIT_CMD_USERS_ALL);
  ta (action_find_user,      print_users_by_name,    &sock, nonopts[2]);
  ta (action_lock_queue,     queue_set_lock,         &sock, nonopts[1], action_lock_queue > 0);
  ta (action_hide_queue,     queue_set_hide,         &sock, nonopts[1], action_hide_queue > 0);
  ta (action_clear_queue,    queue_clear,            &sock, nonopts[1]);
  ta (action_delete_queue,   queue_delete,           &sock, nonopts[1]);
  ta (action_create_queue,   queue_create,           &sock, nonopts[1]);
  ta (action_set_admin,      user_set_admin,         &sock, nonopts[1], action_set_admin > 0);
  ta (action_set_moderator,  user_set_moderator,     &sock, nonopts[1],
      nonopts[abs(action_set_moderator)],            action_set_moderator > 0);
  ta (action_set_owner,      user_set_owner,         &sock, nonopts[1],
      nonopts[abs(action_set_owner)],                action_set_owner > 0);
  ta (action_set_wait,       user_set_wait,          &sock, nonopts[1],
      nonopts[abs(action_set_wait)],                 action_set_wait > 0);
  ta (action_set_comment,    user_set_comment,       &sock, nonopts[3], nonopts[5], nonopts[7]);
  ta (action_set_location,   user_set_location,      &sock, nonopts[3], nonopts[5], nonopts[7]);
  if (r >= 0)
    rc = r;
  
#undef ta
  
  /* Aced it! */
 done:
  /* Disconnect and destroy. */
  if (have_sock)  libqwaitclient_http_socket_disconnect(&sock);
  if (have_sock)  libqwaitclient_http_socket_destroy(&sock);
  return rc;
  
  /* I just don't know want when wrong! */
 fail:
  if (errno)
    perror(*argv);
  rc = 2;
  goto done;
  
  /* The user did not specify a valid action. */
 invalid_command:
  fprintf(stderr, "What are you trying to do?\n");
  return 3;
}


#undef abs
#undef t

