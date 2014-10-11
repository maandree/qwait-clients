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
#include <stddef.h>
#include <signal.h>
#include <stdio.h>



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


/**
 * Flush the standard output stream to apply
 * changes to the terminals settings
 * 
 * @return  See return value for `fflush`
 */
#define flush()  fflush(stdout)

/**
 * Tell the terminal to hide the text cursor
 * 
 * @return  See return value for `printf`
 */
#define hide_cursor()  printf("\033[?25l")

/**
 * Tell the terminal to show the text cursor
 * 
 * @return  See return value for `printf`
 */
#define show_cursor()  printf("\033[?25h")

/**
 * Initialise a subterminal
 * 
 * @return  See return value for `printf`
 */
#define initialise_terminal()  printf("\033[?1049h")

/**
 * Terminate the subterminal
 * 
 * @return  See return value for `printf`
 */
#define terminate_terminal()  printf("\033[?1049l")

/**
 * Set the title on the terminal, but in the
 * decoration and in the taskbar
 * 
 * @param   title  The title of the terminal
 * @return         See return value for `printf`
 */
#define set_title(title)  printf("\033]0;%s\a", title)

#define bold(text)  "\033[01m" text "\033[21m"
#define reverse_video(text)  "\033[07m" text "\033[27m"
#define clear() "\033[2J"
#define home() "\033[H"
#define row(num) "\033[" #num ";1H"
#define insert_spaces(num) "\033[" #num "@"

#endif

