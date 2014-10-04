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
#include "computers.h"

#include <stdlib.h>
#include <string.h>



/**
 * Presice [0, 255] sRGB colours for the computer rooms,
 * (-1, -1, -1) for unknown
 */
static int computer_colours[][3] =
  {
    [LIBQWAITCLIENT_COMPUTERS_UKNOWN]    = {  -1,  -1,  -1 },
    [LIBQWAITCLIENT_COMPUTERS_CERISE]    = { 255, 192, 203 },
    [LIBQWAITCLIENT_COMPUTERS_BLUE]      = {   0,   0, 255 },
    [LIBQWAITCLIENT_COMPUTERS_RED]       = { 255,   0,   0 },
    [LIBQWAITCLIENT_COMPUTERS_ORANGE]    = { 255, 127,   0 },
    [LIBQWAITCLIENT_COMPUTERS_YELLOW]    = { 255, 255,   0 },
    [LIBQWAITCLIENT_COMPUTERS_GREEN]     = {   0, 128,   0 },
    [LIBQWAITCLIENT_COMPUTERS_BROWN]     = { 127,  63,  31 },
    [LIBQWAITCLIENT_COMPUTERS_GREY]      = { 128, 128, 128 },
    [LIBQWAITCLIENT_COMPUTERS_CRIMSON]   = { 217,  21,  54 },
    [LIBQWAITCLIENT_COMPUTERS_WHITE]     = { 255, 255, 255 },
    [LIBQWAITCLIENT_COMPUTERS_MAGENTA]   = { 255,   0, 255 },
    [LIBQWAITCLIENT_COMPUTERS_VIOLET]    = { 172,   0, 230 },
    [LIBQWAITCLIENT_COMPUTERS_TURQUOISE] = {  64, 224, 208 },
    [LIBQWAITCLIENT_COMPUTERS_SPEL]      = { 230, 173, 173 },
    [LIBQWAITCLIENT_COMPUTERS_SPORT]     = { 173, 173, 230 },
    [LIBQWAITCLIENT_COMPUTERS_MUSIK]     = { 173, 231, 173 },
    [LIBQWAITCLIENT_COMPUTERS_KONST]     = { 232, 231, 175 },
    [LIBQWAITCLIENT_COMPUTERS_MAT]       = { 232, 201, 175 }
  };


/**
 * With inner array index 0:
 *   Gross foreground colours suitable for 8- and 16-colour
 *   terminals
 * 
 * With inner array index 1:
 *   Rather accurate foreground colours suitable for
 *   256-colour terminals
 * 
 * With inner array index 2:
 *   Accurate foreground colours suitable for
 *   terminals capable for displaying 24-bit colours
 *   encoded with the specification used in KDE's Konsole
 */
static const char* computer_colours_term[][3] =
  {
    [LIBQWAITCLIENT_COMPUTERS_UKNOWN]    = { "00", "00",       "00"               },
    [LIBQWAITCLIENT_COMPUTERS_CERISE]    = { "35", "38;5;217", "38;2;255;192;203" },
    [LIBQWAITCLIENT_COMPUTERS_BLUE]      = { "34", "38;5;21",  "38;2;0;0;255"     },
    [LIBQWAITCLIENT_COMPUTERS_RED]       = { "31", "38;5;196", "38;2;255;0;0"     },
    [LIBQWAITCLIENT_COMPUTERS_ORANGE]    = { "33", "38;5;208", "38;2;255;127;0"   },
    [LIBQWAITCLIENT_COMPUTERS_YELLOW]    = { "33", "38;5;226", "38;2;255;255;0"   },
    [LIBQWAITCLIENT_COMPUTERS_GREEN]     = { "32", "38;5;28",  "38;2;0;128;0"     },
    [LIBQWAITCLIENT_COMPUTERS_BROWN]     = { "33", "38;5;95",  "38;2;127;63;31"   },
    [LIBQWAITCLIENT_COMPUTERS_GREY]      = { "00", "38;5;244", "38;2;128;128;128" },
    [LIBQWAITCLIENT_COMPUTERS_CRIMSON]   = { "31", "38;5;167", "38;2;217;21;54"   },
    [LIBQWAITCLIENT_COMPUTERS_WHITE]     = { "00", "38;5;231", "38;2;255;255;255" },
    [LIBQWAITCLIENT_COMPUTERS_MAGENTA]   = { "35", "38;5;201", "38;2;255;0;255"   },
    [LIBQWAITCLIENT_COMPUTERS_VIOLET]    = { "35", "38;5;128", "38;2;172;0;230"   },
    [LIBQWAITCLIENT_COMPUTERS_TURQUOISE] = { "36", "38;5;44",  "38;2;64;224;208"  },
    [LIBQWAITCLIENT_COMPUTERS_SPEL]      = { "31", "38;5;181", "38;2;230;173;173" },
    [LIBQWAITCLIENT_COMPUTERS_SPORT]     = { "34", "38;5;146", "38;2;173;173;230" },
    [LIBQWAITCLIENT_COMPUTERS_MUSIK]     = { "32", "38;5;151", "38;2;173;231;173" },
    [LIBQWAITCLIENT_COMPUTERS_KONST]     = { "33", "38;5;187", "38;2;232;231;175" },
    [LIBQWAITCLIENT_COMPUTERS_MAT]       = { "33", "38;5;223", "38;2;232;201;175" }
  };



/**
 * Figure out which computer room a student is sitting in by her location string
 * 
 * @param   location  The student's location
 * @return            The computer room the student is sitting in, zero if unknown
 */
int libqwaitclient_computers_get_room(const char* restrict location)
{
#define test1(A)           (strcasestr(location, A) != NULL)
#define test2(A, B)        (test1(A) && test1(B))
#define test3(A, B, C)     (test1(A) && test1(B) && test1(C))
#define test4(A, B, C, D)  (test1(A) && test1(B) && test1(C) && test1(D))
    
  if (location == NULL)
    return LIBQWAITCLIENT_COMPUTERS_UKNOWN;
  
  if (test1("cerise"))                      return LIBQWAITCLIENT_COMPUTERS_CERISE;
  if (test3("blå", "blÅ", "blue"))          return LIBQWAITCLIENT_COMPUTERS_BLUE;
  if (test3("röd", "rÖd", "red"))           return LIBQWAITCLIENT_COMPUTERS_RED;
  if (test1("orange"))                      return LIBQWAITCLIENT_COMPUTERS_ORANGE;
  if (test2("gul", "yellow"))               return LIBQWAITCLIENT_COMPUTERS_YELLOW;
  if (test3("grön", "grÖn", "green"))       return LIBQWAITCLIENT_COMPUTERS_GREEN;
  if (test2("brun", "brown"))               return LIBQWAITCLIENT_COMPUTERS_BROWN;
  if (test4("grå", "grÅ", "grey", "gray"))  return LIBQWAITCLIENT_COMPUTERS_GREY;
  if (test2("karmosin", "crimson"))         return LIBQWAITCLIENT_COMPUTERS_CRIMSON;
  if (test2("vit", "white"))                return LIBQWAITCLIENT_COMPUTERS_WHITE;
  if (test1("magenta"))                     return LIBQWAITCLIENT_COMPUTERS_MAGENTA;
  if (test1("violet"))                      return LIBQWAITCLIENT_COMPUTERS_VIOLET;
  if (test2("turkos", "turquoise"))         return LIBQWAITCLIENT_COMPUTERS_TURQUOISE;
  if (test1("spel"))                        return LIBQWAITCLIENT_COMPUTERS_SPEL;
  if (test1("sport"))                       return LIBQWAITCLIENT_COMPUTERS_SPORT;
  if (test1("musik"))                       return LIBQWAITCLIENT_COMPUTERS_MUSIK;
  if (test1("konst"))                       return LIBQWAITCLIENT_COMPUTERS_KONST;
  if (test1("mat"))                         return LIBQWAITCLIENT_COMPUTERS_MAT;
  
  return LIBQWAITCLIENT_COMPUTERS_UKNOWN;
  
#undef test4
#undef test3
#undef test2
#undef test1
}


/**
 * Get the official colour of a computer room, or a colour as close a possible
 * as libqwaitclient can determine that the used terminal can parse and display,
 * as foreground colour
 * 
 * `printf("\033[00;%s;01m", libqwaitclient_computers_get_terminal_colour(room, depth))`;
 * will switch the output colour on the terminal to a bold version of the colour
 * for the computer room. Note however that if the computer room is unknown
 * `libqwaitclient_computers_get_room` will return `LIBQWAITCLIENT_COMPUTERS_UKNOWN`,
 * in such case you do probably not want to use the bold formatting.
 * 
 * If you want use a background colour instead of a foreground colour you should
 * exchange the first letter and only the first letter for a '4' if and only if
 * it is a '3'. Note however, that you will need to make a copy of the returned
 * string to do this operations. The returned string cannot be longer than
 * 17 characters including its NUL-termination, but to be future proof you should
 * allow for up to 32 characters just to be safe.
 * 
 * @param   computer_room  The computer room
 * @param   colour_depth   How accurate colours the terminal support:
 *                         - 8:   8 or 16 colours
 *                         - 256: 256 colours
 *                         - 24:  24-bit colours
 *                         - 0:   Look at the enviroment to make a guess
 * @return                 A foreground colour for the computer room, "00" if the computer
 *                         room is unknown or does not an official colour, "00" may also
 *                         be returned if the computer room has an official colour,
 *                         it is the case if the colour has to be approximated and the
 *                         proper colour is on the greyscale, the returned value should
 *                         not be freed
 */
const char* libqwaitclient_computers_get_terminal_colour(int computer_room, int colour_depth)
{
  int term = 0;
  
  if      (colour_depth == 256)  term = 1;
  else if (colour_depth == 24)   term = 2;
  else if (colour_depth == 0)
    {
      char* env = getenv("TERM");
      if (env && !strcmp(env, "xterm-256color"))
	term = 1;
      
      env = getenv("QWAIT_TERM_COLOURS");
      if      (env && !strcmp(env, "8"))    term = 0;
      else if (env && !strcmp(env, "16"))   term = 0;
      else if (env && !strcmp(env, "256"))  term = 1;
      else if (env && !strcmp(env, "24"))   term = 2;
    }
  
  return computer_colours_term[computer_room][term];
}


/**
 * Get the official colour of a computer room
 * 
 * @param   computer_room  The computer room
 * @param   red            Output parameter for the red component of the colour, may be `NULL`
 * @param   green          Output parameter for the green component of the colour, may be `NULL`
 * @param   blue           Output parameter for the blue component of the colour, may be `NULL`
 * @return                 The official [0, 255] sRGB colour of the computer room encoded
 *                         `((*red << 16) | (*green << 8) | (*blue << 0))`, -1 is returned and
 *                         stored in `red`, `green` and `blue` if the computer room is unknown
 *                         or does not have an official colour
 */
int32_t libqwaitclient_computers_get_numerical_colour(int computer_room, int* red, int* green, int* blue)
{
  int r = computer_colours[computer_room][0];
  int g = computer_colours[computer_room][1];
  int b = computer_colours[computer_room][2];
  
  if (red   != NULL)  *red   = r;
  if (green != NULL)  *green = g;
  if (blue  != NULL)  *blue  = b;
  
  if (r < 0)
    return -1;
  
  return ((int32_t)r << 16) | ((int32_t)g << 8) | ((int32_t)b << 0);
}

