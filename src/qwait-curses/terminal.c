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
#include "terminal.h"

#include <sys/ioctl.h>
#include <unistd.h>


/* _GNU_SOURCE (#define:d in "terminal.h") is need
 * for `signal` to have permanent effect, but since
 * we need to use _GNU_SOURCE we might as well
 * upgrade to `sigaction`.
 * 
 * An alternative method would be to always call
 * `signal(signo, sig_winch)` in `sig_winch`, however
 * that complicates error handling for the `signal`
 * function. */



/**
 * This varible is set to non-zero when the terminal
 * ha been resized, once you detect this you should
 * reset it to zero
 * 
 * This variable will only be updated if
 * `catch_terminal_resize_signal` has retured
 * successfully
 */
volatile sig_atomic_t terminal_resized = 0;



/**
 * This function is caleld when the terminal is
 * resized if `catch_terminal_resize_signal` has
 * returned successfully
 */
#include <stdio.h>
static void sig_winch(int signo)
{
  (void) signo;
  terminal_resized = 1;
}



/**
 * Get the new size of the terminal
 * 
 * @param   terminal_width   Output parameter for the number of columns in the terminal
 * @param   terminal_height  Output parameter for the number of rows in the terminal  
 * @return                   Zero on success, -1 on error
 */
int update_terminal_size(size_t* restrict terminal_width, size_t* restrict terminal_height)
{
  struct winsize terminal_size;
  if (ioctl(STDIN_FILENO, TIOCGWINSZ, &terminal_size) < 0)
    return -1;
  *terminal_width  = (size_t)(terminal_size.ws_col);
  *terminal_height = (size_t)(terminal_size.ws_row);
  return 0;
}


/**
 * Configure blocking functions to get interrupted
 * when the terminal is resized, and for `terminal_resized`
 * to be set on said event
 * 
 * @return  Zero on success, -1 on error
 */
int catch_terminal_resize_signal(void)
{
  struct sigaction action;
  sigset_t sigset;
  
  sigemptyset(&sigset);
  action.sa_handler = sig_winch;
  action.sa_mask = sigset;
  action.sa_flags = 0;
  
  return sigaction(SIGWINCH, &action, NULL);
}

