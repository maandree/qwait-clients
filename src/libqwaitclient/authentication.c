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

#include "macros.h"

#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>



/**
 * Perform a login
 * 
 * @param   username     The user's username
 * @param   password     The user's password
 * @param   data         Output parameter for authentication data, free even on failure
 * @param   data_length  Output parameter for the length of `*data`
 * @return               Zero on success, -1 on error, 1 if login failed
 */
int libqwaitclient_auth_log_in(const char* restrict username, const char* restrict password,
			       char** restrict data, size_t* restrict data_length)
{
  int pipe_rw[2] = { -1, -1 };
  pid_t pid = -1, reaped = -1;
  int saved_errno, status;
  size_t written = 0, allocated = 0;
  ssize_t got;
  char* old;
  
  *data = NULL;
  *data_length = 0;
  
  /* Create pipe and fork. */
  if (pipe(pipe_rw) < 0)        goto fail;
  if (pid = fork(), pid == -1)  goto fail;
  
  /* Select procedure. */
  if (pid)
    goto parent;
  
  
  /* CHILD PROCESS. */
  setenv("username", username, 1);
  setenv("password", password, 1);
  close(pipe_rw[0]); /* Close pipe's read-end. */
  if (pipe_rw[1] != STDOUT_FILENO)
    {
      /* Set pipe write-end to stdout. */
      close(STDOUT_FILENO);
      if (dup2(pipe_rw[1], STDOUT_FILENO) < 0)
	return perror("qwait-login"), 2;
      close(pipe_rw[1]);
    }
  execl(LIBEXECDIR "/qwait-login", "qwait-login", NULL);
  return perror("qwait-login"), exit(2), -1;
  
  
  /* PARENT PROCESS. */
 parent:
  
  close(pipe_rw[1]), pipe_rw[1] = -1; /* Close pipe's write-end. */
  /* Read output */
  for (;;)
    {
      if (written == allocated)
	{
	  char* new = *data;
	  if (xrealloc(new, allocated += 128, char))
	    goto fail;
	  *data = new;
	}
      if (got = read(pipe_rw[0], *data + written, allocated - written), got < 0)
	{
	  if (errno == EINTR)
	    continue;
	  goto fail;
	}
      if (got == 0)
	break;
      written += (size_t)got;
    }
  *data_length = written;
  /* Shrink allocation. */
  old = *data;
  if (xrealloc(*data, written, char) && written)
    *data = old;
  /* Wait for `qwait-logout` to exit. */
  while (reaped != pid)
    {
      reaped = waitpid(pid, &status, 0);
      if (reaped == -1)
	{
	  if (errno == EINTR)
	    continue;
	  goto fail;
	}
    }
  pid = -1;
  
  /* Return 0 on success and 1 on login failure. */
  return (status || (*data_length < 2)) ? 1 : 0;
  
  /* Parent process failure. */
 fail:
  saved_errno = errno;
  if (pipe_rw[0] >= 0)  close(pipe_rw[0]);
  if (pipe_rw[1] >= 0)  close(pipe_rw[1]);
  if (pid > 0)          kill(pid, SIGTERM);
  return errno = saved_errno, -1;
}


/**
 * Request a server-side logout
 * 
 * @param   data         The authentication data
 * @param   data_length  The length of `data`
 * @return               Zero on possible success, -1 on definite error
 */
int libqwaitclient_auth_log_out(const char* restrict data, size_t data_length)
{
  int pipe_rw[2] = { -1, -1 };
  pid_t pid = -1, reaped = -1;
  int saved_errno, status;
  ssize_t wrote;
  size_t written;
  
  /* Create pipe and fork. */
  if (pipe(pipe_rw) < 0)        goto fail;
  if (pid = fork(), pid == -1)  goto fail;
  
  /* Select procedure. */
  if (pid)
    goto parent;
  
  
  /* CHILD PROCESS. */
  
  close(pipe_rw[1]); /* Close pipe's write-end. */
  if (pipe_rw[0] != STDIN_FILENO)
    {
      /* Set pipe read-end to stdin. */
      close(STDIN_FILENO);
      if (dup2(pipe_rw[0], STDIN_FILENO) < 0)
	return perror("qwait-logout"), 2;
      close(pipe_rw[0]);
    }
  execl(LIBEXECDIR "/qwait-logout", "qwait-logout", NULL);
  return perror("qwait-logout"), exit(2), -1;
  
  
  /* PARENT PROCESS. */
 parent:
  
  close(pipe_rw[0]), pipe_rw[0] = -1; /* Close pipe's read-end. */
  /* Write authentication information to `qwait-logout`'s stdin. */
  written = 0;
  while (written < data_length)
    {
      wrote = write(pipe_rw[1], data, data_length - written);
      if (wrote < 0)
	{
	  if (errno == EINTR)
	    continue;
	  goto fail;
	}
      written += (size_t)wrote;
    }
  close(pipe_rw[1]), pipe_rw[1] = -1; /* Close pipe's write-end. */
  /* Wait for `qwait-logout` to exit. */
  while (reaped != pid)
    {
      reaped = waitpid(pid, &status, 0);
      if (reaped == -1)
	{
	  if (errno == EINTR)
	    continue;
	  goto fail;
	}
    }
  pid = -1;
  
  /* Return as `qwait-logout` exited or died. */
  return WIFEXITED(status) ? WEXITSTATUS(status) : WIFSIGNALED(status) ? WTERMSIG(status) : 1;
  
  /* Parent process failure. */
 fail:
  saved_errno = errno;
  if (pipe_rw[0] >= 0)  close(pipe_rw[0]);
  if (pipe_rw[1] >= 0)  close(pipe_rw[1]);
  if (pid > 0)          kill(pid, SIGTERM);
  return errno = saved_errno, -1;
}

