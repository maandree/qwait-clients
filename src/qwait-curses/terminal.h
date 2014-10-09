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
#ifndef QWAIT_CURSES_TERMINAL_H
#define QWAIT_CURSES_TERMINAL_H

#define _GNU_SOURCE
/* _GNU_SOURCE is need for `signal` to have permanent effect,
 * but since we need to use _GNU_SOURCE we might as well upgrade
 * to sigaction. */

#include <stddef.h>
#include <signal.h>



/**
 * This varible is set to non-zero when the terminal
 * ha been resized, once you detect this you should
 * reset it to zero
 * 
 * This variable will only be updated if
 * `catch_terminal_resize_signal` has retured
 * successfully
 */
extern volatile sig_atomic_t terminal_resized;


/**
 * Get the new size of the terminal
 * 
 * @param   terminal_width   Output parameter for the number of columns in the terminal
 * @param   terminal_height  Output parameter for the number of rows in the terminal  
 * @return                   Zero on success, -1 on error
 */
int update_terminal_size(size_t* restrict terminal_width, size_t* restrict terminal_height);

/**
 * Configure blocking functions to get interrupted
 * when the terminal is resized, and for `terminal_resized`
 * to be set on said event
 * 
 * @return  Zero on success, -1 on error
 */
int catch_terminal_resize_signal(void);


#endif

