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
#include "users.h"

#include "globals.h"
#include "authentication.h"

#include <stdio.h>
#include <stdlib.h>



/**
 * Print information about users on the system
 * 
 * @param   sock  A socket that is connected to the qwait server
 * @param   role  Either of: `QWAIT_CMD_USERS_ALL`, `QWAIT_CMD_USERS_ADMINS`
 * @return        Zero on success, -1 on error, 1 if not logged in
 */
int print_users(libqwaitclient_http_socket_t* restrict sock, int role)
{
  /* TODO a better interface is needed */
  
  libqwaitclient_authentication_t auth;
  libqwaitclient_qwait_user_t* users = NULL;
  size_t i, user_count = 0;
  int r;
  
  /* Acquire authentication information. */
  r = get_authentication(&auth);
  if (r < 0)   goto fail;
  if (r == 1)  goto not_logged_in;
  
  /* Acquire user information. */
  switch (role)
    {
    case QWAIT_CMD_USERS_ALL:     users = libqwaitclient_qwait_get_users(sock, &auth, &user_count);   break;
    case QWAIT_CMD_USERS_ADMINS:  users = libqwaitclient_qwait_get_admins(sock, &auth, &user_count);  break;
    default:
      return abort(), -1;
    }
  if (users == NULL)
    goto fail;
  
  /* Print user information. */
  for (i = 0; i < user_count; i++)
    {
      libqwaitclient_qwait_user_dump(users + i, stdout);
      libqwaitclient_qwait_user_destroy(users + i);
      printf("\n");
    }
  
  free(users);
  libqwaitclient_authentication_destroy(&auth);
  return 0;
  
 fail:
  free(users);
  libqwaitclient_authentication_destroy(&auth);
  return -1;
  
 not_logged_in:
  fprintf(stderr, "You are not logged in.\n");
  return 1;
}


/**
 * Print information about users on the system by finding them by their name
 * 
 * @param   sock       A socket that is connected to the qwait server
 * @param   real_name  A string that the must exist in the real name of the printed users
 * @return             Zero on success, -1 on error, 1 if not logged in
 */
int print_users_by_name(libqwaitclient_http_socket_t* restrict sock, const char* restrict real_name)
{
  /* TODO a better interface is needed */
  
  libqwaitclient_authentication_t auth;
  libqwaitclient_qwait_user_t* users = NULL;
  size_t i, user_count = 0;
  int r;
  
  /* Acquire authentication information. */
  r = get_authentication(&auth);
  if (r < 0)   goto fail;
  if (r == 1)  goto not_logged_in;
  
  /* Acquire user information. */
  users = libqwaitclient_qwait_find_user(sock, &auth, real_name, &user_count);
  if (users == NULL)
    goto fail;
  
  /* Print user information. */
  for (i = 0; i < user_count; i++)
    {
      libqwaitclient_qwait_user_dump(users + i, stdout);
      libqwaitclient_qwait_user_destroy(users + i);
      printf("\n");
    }
  
  free(users);
  libqwaitclient_authentication_destroy(&auth);
  return 0;
  
 fail:
  free(users);
  libqwaitclient_authentication_destroy(&auth);
  return -1;
  
 not_logged_in:
  fprintf(stderr, "You are not logged in.\n");
  return 1;
}

