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
#include "miscellaneous.h"

#include "authentication.h"

#include <errno.h>
#include <stdio.h>


#define _sock_  libqwaitclient_http_socket_t* restrict sock


/**
 * Print the user's reverse DNS address
 * 
 * @param   sock  A socket that is connected to the qwait server
 * @return        Zero on success, -1 on error
 */
int print_user_hostname(_sock_)
{
  libqwaitclient_login_information_t login;
  int r, saved_errno;
  
  libqwaitclient_login_information_initialise(&login);
  if (r = libqwaitclient_qwait_get_login_information(sock, NULL, &login), r)
    goto fail;
  
  printf("%s\n", login.hostname);
  
 fail:
  saved_errno = errno;
  libqwaitclient_login_information_destroy(&login);
  return errno = saved_errno, r;
}


/**
 * Print the remote server's product name and version
 * 
 * @param   sock  A socket that is connected to the qwait server
 * @return        Zero on success, -1 on error
 */
int print_qwait_version(_sock_)
{
  libqwaitclient_login_information_t login;
  int r, saved_errno;
  
  libqwaitclient_login_information_initialise(&login);
  if (r = libqwaitclient_qwait_get_login_information(sock, NULL, &login), r)
    goto fail;
  
  printf("%s %s\n", login.product.name, login.product.version);
  
 fail:
  saved_errno = errno;
  libqwaitclient_login_information_destroy(&login);
  return errno = saved_errno, r;
}


/**
 * Print user login information
 * 
 * @param   sock  A socket that is connected to the qwait server
 * @return        Zero on success, -1 on error
 */
int print_user_login(_sock_)
{
  libqwaitclient_authentication_t* auth_ = NULL;
  libqwaitclient_authentication_t auth;
  libqwaitclient_login_information_t login;
  size_t i, n;
  int r, saved_errno;
  
  /* Acquire authentication information. */
  r = get_authentication(&auth);
  if      (r < 0)   goto fail;
  else if (r == 0)  auth_ = &auth;
  
  libqwaitclient_login_information_initialise(&login);
  if (r = libqwaitclient_qwait_get_login_information(sock, auth_, &login), r)
    goto fail;
  
  if (login.current_user.anonymous)
    printf("anonymous\n");
  else
    {
      printf("%s (%s)\n", login.current_user.real_name, login.current_user.user_id);
      printf("%s\n",      login.current_user.admin     ? "\033[01;31madmin\033[00m"     : "not admin");
      printf(login.current_user.role_count ? "roles" : "no roles");
      for (i = 0, n = login.current_user.role_count; i < n; i++)
	printf("%s%s", i ? ", " : ": ", login.current_user.roles[i]);
      printf("\n");
    }
  
 fail:
  saved_errno = errno;
  if (auth_)
    libqwaitclient_authentication_destroy(auth_);
  libqwaitclient_login_information_destroy(&login);
  return errno = saved_errno, r;
}


#undef _sock_

