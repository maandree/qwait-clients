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

#include <stddef.h>


/**
 * Get the new size of the terminal
 * 
 * @param   terminal_width   Output parameter for the number of columns in the terminal
 * @param   terminal_height  Output parameter for the number of rows in the terminal  
 * @return                   Zero on success, -1 on error
 */
int update_terminal_size(size_t* restrict terminal_width, size_t* restrict terminal_height);


#endif

