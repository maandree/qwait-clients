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
#include "terminal.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/wait.h>


/**
 * The original TTY settings
 */
static struct termios saved_stty;


/**
 * Restore the terminals original settings
 */
static void restore_terminal(void)
{
  show_cursor();
  terminate_terminal();
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_stty);
}


/**
 * Fork the process and continue as the child process,
 * the parent process will wait for the child process
 * to die, than restore the terminal's original settings
 * by invoking `restore_terminal` and then attempt to
 * die with the same signal as the child process or
 * (if unsuccessful or not killed by a signal) die with
 * the same exit value by returing the appropriate value
 * and let the caller return the same value
 * 
 * @return  NB! -1 if the program should continue normally,
 *          non-negative if the program should exit with the
 *          returned value as the program's exit value
 */
static int guard_terminal_settings(void)
{
  int status, saved_errno, signo;
  pid_t pid, reaped;
  
  /* Fork or die. */
  if (pid = fork(), pid == -1)
    return perror(*argv), 1;
  
  /* The child process should continue normally. */
  if (pid == 0)
    return -1;
  
  /* Parent process: they as the child, but cleanup after it first. */
  for (;;)
    {
      /* Signals may not kill us, unless they are
         unsuppressible, we especially do not want
         to die of SIGINT.
	 We do this before each wait, because
	 `signal` does not have permanent effect
	 in all libc:s. */
      for (signo = 1; signo < _NSIG; signo++)
	if (signo != SIGCHLD) /* We need SIGCHLD to wait for it. */
	  signal(signo, SIG_IGN);
      
      /* Wait for the child process to die. */
      reaped = waitpid(pid, &status, 0);
      if ((reaped == pid) || ((reaped == -1) && (errno != EINTR)))
	{
	  /* Restore the terminal settings. */
	  saved_errno = errno;
	  restore_terminal();
	  flush();
	  errno = saved_errno;
	  
	  /* Error (not interruption), die. */
	  if (reaped == -1)
	    return perror(*argv), 1;
	  
	  /* Return with the same exit value as the child process. */
	  if (WIFEXITED(status))
	    return WEXITSTATUS(status);
	  
	  /* Signals may kill us again, we want to die
	     with the same signal as the child process. */
	  for (signo = 1; signo < _NSIG; signo++)
	    signal(signo, SIG_DFL);
	  
	  /* Die with the same signal as the child process,
	     or if not possible, with that signal's number
	     as our exit value. */
	  raise(WTERMSIG(status));
	  return WTERMSIG(status);
	}
    }
}


/**
 * "curses" client for qwait
 * 
 * @param   argc_  The number of elements in `argv_`
 * @param   argv_  Command line arguments, including the command name
 * @return         Zero on and only on success
 */
int main(int argc_, char** argv_)
{
  struct termios stty;
  size_t terminal_width;
  size_t terminal_height;
  int r;
  
  argc = argc_;
  argv = argv_;
  
  tcgetattr(STDIN_FILENO, &stty);
  saved_stty = stty;
  
  if (r = guard_terminal_settings(), r >= 0)
    return r;
  
  stty.c_lflag &= (tcflag_t)~(ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &stty);
  set_title("qwait-curses");
  initialise_terminal();
  hide_cursor();
  flush();
  
  if (update_terminal_size(&terminal_width, &terminal_height) < 0)
    return perror(*argv), 1;
  if (catch_terminal_resize_signal() < 0)
    return perror(*argv), 1;
  
  for (;;)
    {
      printf(home()clear()reverse_video(insert_spaces(%zu)bold("qwait-curses")),
	     terminal_width);
      
      printf(row(%zu)reverse_video(insert_spaces(%zu)),
	     terminal_height, terminal_width);
      
      flush();
      
      if (getchar() == 'q')
	return 0;
      
      if (terminal_resized == 0)
	continue;
      terminal_resized = 0;
      
      if (update_terminal_size(&terminal_width, &terminal_height) < 0)
	return perror(*argv), 1;
    }
}

