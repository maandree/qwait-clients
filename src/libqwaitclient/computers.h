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
#ifndef LIBQWAITCLIENT_COMPUTERS_H
#define LIBQWAITCLIENT_COMPUTERS_H


#define _GNU_SOURCE
#include <stdint.h>


/**
 * Unknown computer room
 */
#define LIBQWAITCLIENT_COMPUTERS_UKNOWN  0

/**
 * Computer room "Cerise" (Enlish: Cerise),
 * official colour: sRGB(255, 192, 203)
 */
#define LIBQWAITCLIENT_COMPUTERS_CERISE  1

/**
 * Computer room "Blå" (Enlish: Blue),
 * official colour: sRGB(0, 0, 255)
 */
#define LIBQWAITCLIENT_COMPUTERS_BLUE  2

/**
 * Computer room "Röd" (Enlish: Red),
 * official colour: sRGB(255, 0, 0)
 */
#define LIBQWAITCLIENT_COMPUTERS_RED  3

/**
 * Computer room "Orange" (Enlish: Orange),
 * official colour: sRGB(255, 127, 0)
 */
#define LIBQWAITCLIENT_COMPUTERS_ORANGE  4

/**
 * Computer room "Gul" (Enlish: Yellow),
 * official colour: sRGB(255, 255, 0)
 */
#define LIBQWAITCLIENT_COMPUTERS_YELLOW  5

/**
 * Computer room "Grön" (Enlish: Green),
 * official colour: sRGB(0, 128, 0)
 */
#define LIBQWAITCLIENT_COMPUTERS_GREEN  6

/**
 * Computer room "Brun" (Enlish: Brown),
 * official colour: sRGB(127, 63, 31)
 */
#define LIBQWAITCLIENT_COMPUTERS_BROWN  7

/**
 * Computer room "Grå" (Enlish: Grey ("Gray" is also acceptable)),
 * official colour: sRGB(128, 128, 128)
 */
#define LIBQWAITCLIENT_COMPUTERS_GREY  8

/**
 * Computer room "Karmosin" (Enlish: Crimson),
 * official colour: sRGB(217, 21, 54)
 */
#define LIBQWAITCLIENT_COMPUTERS_CRIMSON  9

/**
 * Computer room "Vit" (Enlish: White),
 * official colour: sRGB(255, 255, 255)
 */
#define LIBQWAITCLIENT_COMPUTERS_WHITE  10

/**
 * Computer room "Magenta" (Enlish: Magenta),
 * official colour: sRGB(255, 0, 255)
 */
#define LIBQWAITCLIENT_COMPUTERS_MAGENTA  11

/**
 * Computer room "Violett" (Enlish: Violet),
 * official colour: sRGB(172, 0, 230)
 */
#define LIBQWAITCLIENT_COMPUTERS_VIOLET  12

/**
 * Computer room "Turkos" (Enlish: Turquoise),
 * official colour: sRGB(64, 224, 208)
 */
#define LIBQWAITCLIENT_COMPUTERS_TURQUOISE  13

/* I don't believe these have English names, but they do have colours. */

/**
 * Computer room "Spel" (or "Spelhallen"),
 * official colour: sRGB(230, 173, 173)
 */
#define LIBQWAITCLIENT_COMPUTERS_SPEL  14

/**
 * Computer room "Sport" (or "Sporthallen"),
 * official colour: sRGB(173, 173, 230)
 */
#define LIBQWAITCLIENT_COMPUTERS_SPORT  15

/**
 * Computer room "Musik" (or "Musiksalen"),
 * official colour: sRGB(173, 231, 173)
 */
#define LIBQWAITCLIENT_COMPUTERS_MUSIK  16

/**
 * Computer room "Konst" (or "Konsthallen"),
 * official colour: sRGB(232, 231, 175)
 */
#define LIBQWAITCLIENT_COMPUTERS_KONST  17

/**
 * Computer room "Mat" (or "Matsalen"),
 * official colour: sRGB(232, 201, 175)
 */
#define LIBQWAITCLIENT_COMPUTERS_MAT  18



/**
 * Figure out which computer room a student is sitting in by her location string
 * 
 * @param   location  The student's location
 * @return            The computer room the student is sitting in, zero if unknown
 */
int libqwaitclient_computers_get_room(const char* restrict location) __attribute__((pure));

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
const char* libqwaitclient_computers_get_terminal_colour(int computer_room, int colour_depth);

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
int32_t libqwaitclient_computers_get_numerical_colour(int computer_room, int* red, int* green, int* blue);



#endif

