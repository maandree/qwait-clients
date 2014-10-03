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
#include "authentication.h"

#include <libqwaitclient.h>

#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>



/**
 * Get the pathname of the file where authentication should stored
 * 
 * @return  The pathname of the authentication information file,
 *          `NULL` on error with `errno` set to zero if the error
 *          has already been printed
 */
static char* get_auth_file(void)
{
  struct passwd* userdata;
  const char* home = NULL;
  size_t n;
  char* rc;
  
  /* Get user information. */
  errno = 0;
  userdata = getpwuid(getuid());
  if (userdata == NULL)
    {
      if (errno == 0)
	fprintf(stderr, "You do not exist.\n"), errno = 0;
      return NULL;
    }
  
  /* Get the user's home directory. */
  home = userdata->pw_dir;
  if (home == NULL)
    return fprintf(stderr, "You are homeless.\n"), errno = 0, NULL;
  
  /* Allocated pathname string. */
  n = strlen(home) + strlen("/.qwait-auth-token");
  rc = malloc((n + 1) * sizeof(char));
  if (rc == NULL)
    return NULL;
  
  /* Write and return pathname string. */
  sprintf(rc, "%s%s", home, "/.qwait-auth-token");
  return rc;
}


/**
 * Get the username of the user
 * 
 * @return  The username of the user, `NULL` on error with `errno`
 *          set to zero if the error has already been printed
 */
static const char* get_username(void)
{
  struct passwd* userdata;
  
  /* Get user information. */
  errno = 0;
  userdata = getpwuid(getuid());
  if (userdata == NULL)
    {
      if (errno == 0)
	fprintf(stderr, "You do not exist.\n"), errno = 0;
      return NULL;
    }
  
  /* Verify that the user has a username and return it. */
  if (userdata->pw_name == NULL)
    return fprintf(stderr, "You do not have a name.\n"), errno = 0, NULL;
  return userdata->pw_name;
}


/**
 * Request a server-side logout
 * 
 * @param  pathname  The pathname of the file with login data
 */
static void log_out(const char* restrict pathname)
{
  struct stat attr;
  void* address;
  size_t len;
  int fd;
  
  /* Stat auth file for its size. */
  if (stat(pathname, &attr) < 0)
    return;
  len = (size_t)(attr.st_size);
  
  /* Open auth file. */
  if (fd = open(pathname, O_RDONLY | O_CLOEXEC), fd < 0)
    return;
  
  /* Memory map auth file. */
  address = mmap(0, len, PROT_READ, MAP_PRIVATE, fd, 0);
  if (address == MAP_FAILED)
    {
      close(fd);
      return;
    }
  
  /* Perform logout. */
  libqwaitclient_auth_log_out(address, len);
  
  /* Release resources. */
  free(address);
  close(fd);
}


/**
 * Log in or log out
 * 
 * @param   username  Your username, `NULL` to log out, an empty string can be
 *                    used for your username on the computer you are using
 * @return            Zero on success, -1 on error, 1 if authentication failed
 */
int authenticate(const char* restrict username)
{
  char* pathname = NULL;
  int saved_errno;
  
  /* Get use name if "" is used. */
  if (username && (*username == '\0'))
    if (username = get_username(), username == NULL)
      return -1;
  
  /* Get pathname of authentication file. */
  if (pathname = get_auth_file(), pathname == NULL)
    return -1;
  
  /* Log out, if requested. */
  if (username == NULL)
    if (log_out(pathname), unlink(pathname) < 0)
      goto fail;
  
  /* TODO login */
  
  free(pathname);
  return 0;
  
 fail:
  saved_errno = errno;
  free(pathname);
  return errno = saved_errno, -1;
}

