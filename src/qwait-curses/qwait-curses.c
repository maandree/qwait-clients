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

#include <stdio.h>
#include <unistd.h>



int main(int argc, char** argv)
{
  size_t terminal_width = 80;
  size_t terminal_height = 24;
  
  (void) argc;
  
  if (update_terminal_size(&terminal_width, &terminal_height) < 0)
    return perror(*argv), 1;
  
  printf("%zu, %zu\n", terminal_width, terminal_height);
  
  if (catch_terminal_resize_signal() < 0)
    return perror(*argv), 1;
  
  for (;;)
    {
      pause();
      
      if (terminal_resized == 0)
	continue;
      terminal_resized = 0;
      
      if (update_terminal_size(&terminal_width, &terminal_height) < 0)
	return perror(*argv), 1;
      
      printf("%zu, %zu\n", terminal_width, terminal_height);
    }
}

