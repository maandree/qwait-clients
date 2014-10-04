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
#include <termios.h>

#ifdef USE_LIBPASSPHRASE
# include <passphrase.h>
#endif



/**
 * Reads the password from stdin
 * 
 * @return  The password, `NULL` on error
 */
static char* read_password(void)
{
#ifdef USE_LIBPASSPHRASE
  
  char* password;
  int saved_errno;
  passphrase_disable_echo();
  password = passphrase_read();
  saved_errno = errno;
  passphrase_reenable_echo();
  return errno = saved_errno, password;
  
#else
  
  struct termios stty, saved_stty;
  char* password = NULL;
  int saved_errno = 0;
  ssize_t length_;
  size_t i, j, length, n = 0;
  
  /* Disable echo. */
  tcgetattr(STDIN_FILENO, &stty);
  saved_stty = stty;
  stty.c_lflag &= (tcflag_t)~ECHO;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &stty);
  
  /* Read password. */
  length_ = getline(&password, &n, stdin);
  if (length_ < 0)
    saved_errno = errno, free(password), password = NULL;
  else
    {
      length = (size_t)length_;
      
      /* Remove NUL-bytes, as they can be caused by kernel errors. */
      for (i = j = 0; i < length; i++)
	if (password[i])
	  password[j++] = password[i];
      /* Remove LF-character. */
      password[j - 1] = '\0';
    }
  
  /* Reenable echo. */
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_stty);
  
  return errno = saved_errno, password;
  
#endif
}


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
  address = mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);
  if ((address == MAP_FAILED) || (address == NULL))
    {
      close(fd);
      return;
    }
  
  /* Perform logout. */
  libqwaitclient_auth_log_out(address, len);
  
  /* Release resources. */
  close(fd);
  munmap(address, len);
}


/**
 * Perform a login
 * 
 * @param   pathname  The pathname of the file for the login data
 * @param   username  The user's username
 * @return            Zero on success, -1 on error, 1 if authentication failed
 */
static int log_in(const char* restrict pathname, const char* restrict username)
{
  char* data = NULL;
  char* restrict password = NULL;
  size_t data_length, written = 0;
  int rc = -1, saved_errno, fd = -1;
  ssize_t wrote;
  
  /* Get password. */
  fprintf(stdout, "[%s] password: ", username);
  fflush(stdout);
  password = read_password();
  if (password == NULL)
    goto fail;
  
  /* Log in. */
  rc = libqwaitclient_auth_log_in(username, password, &data, &data_length);
  saved_errno = errno;
  
  /* Save authentication data. */
  if (rc == 0)
    {
      if (fd = open(pathname, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR), fd < 0)
	goto fail;
      while (written < data_length)
	{
	  wrote = write(fd, data + written, data_length - written);
	  if (wrote < 0)
	    {
	      if (errno == EINTR)
		continue;
	      goto fail;
	    }
	  written += (size_t)wrote;
	}
      close(fd);
    }
  
 done:
#ifdef USE_LIBPASSPHRASE
  passphrase_wipe(password, strlen(password));
#endif
  free(password);
  free(data);
  return errno = saved_errno, rc;
  
 fail:
  saved_errno = errno;
  if (fd >= 0)
    close(fd), fd = -1, unlink(pathname);
  goto done;
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
  int saved_errno, rc = 0;
  
  /* Get use name if "" is used. */
  if (username && (*username == '\0'))
    if (username = get_username(), username == NULL)
      return -1;
  
  /* Get pathname of authentication file. */
  if (pathname = get_auth_file(), pathname == NULL)
    return -1;
  
  /* Log out, if requested. */
  if (username == NULL)
    {
      if (log_out(pathname), unlink(pathname) < 0)
	goto fail;
      goto done;
    }
  
  /* Log in, if requested. */
  rc = log_in(pathname, username);
  if (rc < 0)
    goto fail;
  
 done:
  free(pathname);
  return rc;
  
 fail:
  saved_errno = errno;
  free(pathname);
  return errno = saved_errno, -1;
}



/**
 * Get authentication data
 * 
 * @param   auth  Output parameter for the authentication data
 * @return        Zero on success, -1 on error
 */
int get_authentication(libqwaitclient_authentication_t* restrict auth)
{
  struct stat attr;
  void* address;
  size_t len;
  int fd, r, saved_errno;
  char* pathname;
  
  if (pathname = get_auth_file(), pathname == NULL)
    return -1;
  
  /* Stat auth file for its size. */
  if (stat(pathname, &attr) < 0)
    return -1;
  len = (size_t)(attr.st_size);
  
  /* Open auth file. */
  if (fd = open(pathname, O_RDONLY | O_CLOEXEC), fd < 0)
    return -1;
  
  /* Memory map auth file. */
  address = mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);
  if ((address == MAP_FAILED) || (address == NULL))
    {
      close(fd);
      return -1;
    }
  
  /* Get authentication. */
  r = libqwaitclient_authentication_get(auth, address, len);
  saved_errno = errno;
  
  /* Release resources. */
  close(fd);
  munmap(address, len);
  
  return errno = saved_errno, r;
}


/**
 * Print the user's ID
 * 
 * @return  Zero on success, -1 on error
 */
int print_user_id(void)
{
  char* user_id = NULL;
  int r = libqwaitclient_auth_user_id(&user_id);
  
  if (r == 0)
    {
      int saved_errno;
      errno = 0;
      r = printf("%s\n", user_id);
      saved_errno = errno;
      free(user_id);
      errno = saved_errno;
      return r;
    }
  else if (r == 1)  fprintf(stderr, "You do not exist.\n");
  else if (r == 2)  fprintf(stderr, "You are homeless.\n");
  else if (r == 3)  fprintf(stderr, "Cannot determine.\n");
  
  return errno = (r > 0 ? 0 : errno), (r == 3 ? 1 : -1);
}

