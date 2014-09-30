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
#ifndef LIBQWAITCLIENTS_JSON_H
#define LIBQWAITCLIENTS_JSON_H


#include <stddef.h>
#include <stdint.h>
#include <stdio.h>


/**
 * The data is an integer that fits `int64_t`
 */
#define LIBQWAITCLIENTS_JSON_TYPE_INTEGER  0

/**
 * The data is an integer larger that what would fit `int64_t`
 */
#define LIBQWAITCLIENTS_JSON_TYPE_LARGE_INTEGER  1

/**
 * The data is a double-precision floating-point
 */
#define LIBQWAITCLIENTS_JSON_TYPE_FLOATING  2

/**
 * The data is a string
 */
#define LIBQWAITCLIENTS_JSON_TYPE_STRING  3

/**
 * The data is a boolean
 */
#define LIBQWAITCLIENTS_JSON_TYPE_BOOLEAN  4

/**
 * The data is an array
 */
#define LIBQWAITCLIENTS_JSON_TYPE_ARRAY  5

/**
 * The data is an object
 */
#define LIBQWAITCLIENTS_JSON_TYPE_OBJECT  6

/**
 * The data is null
 */
#define LIBQWAITCLIENTS_JSON_TYPE_NULL  7



/**
 * JavaScript Object Notation
 */
struct libqwaitclient_json;

/**
 * Key–value-pair for a JavaScript Object Notation object (associative array)
 */
struct libqwaitclient_json_association;



/**
 * JavaScript Object Notation
 */
typedef struct libqwaitclient_json
{
  /**
   * The data type
   */
  int type;
  
  /**
   * The length for the members `large_integer`, `string`, `array` and `object`
   */
  size_t length;
  
  /**
   * The data
   */
  union
  {
    /**
     * Used if `type` is `LIBQWAITCLIENTS_JSON_TYPE_INTEGER`
     */
    int64_t integer;
    
    /**
     * Used if `type` is `LIBQWAITCLIENTS_JSON_TYPE_LARGE_INTEGER`
     * 
     * The integer is stored as a NUL-terminated string without
     * a '+' prefix
     * 
     * The number of bytes/characters are determined by `length`,
     * this excludes NUL-termination
     */
    char* large_integer;
    
    /**
     * Used if `type` is `LIBQWAITCLIENTS_JSON_TYPE_FLOATING`
     */
    double floating;
    
    /**
     * Used if `type` is `LIBQWAITCLIENTS_JSON_TYPE_STRING`
     * 
     * UTF-8 string that may contain NUL, and it may contain
     * characters up to and surrogate pair are resolved
     * 
     * UTF-8 encoding length attacks are mitigated
     * 
     * The number of bytes are determined by `length`
     */
    char* string;
    
    /**
     * Used if `type` is `LIBQWAITCLIENTS_JSON_TYPE_BOOLEAN`
     * 
     * 0 for false, 1 for true
     */
    int boolean;
    
    /**
     * Used if `type` is `LIBQWAITCLIENTS_JSON_TYPE_ARRAY`
     * 
     * The number of elements are determined by `length`
     */
    struct libqwaitclient_json* array;
    
    /**
     * Used if `type` is `LIBQWAITCLIENTS_JSON_TYPE_OBJECT`
     * 
     * The number of pairs are determined by `length`
     */
    struct libqwaitclient_json_association* object;
    
  } data;
  
} libqwaitclient_json_t;


/**
 * Key–value-pair for a JavaScript Object Notation object (associative array)
 */
typedef struct libqwaitclient_json_association
{
  /**
   * The key (the name)
   * 
   * UTF-8 string that may contain NUL, and it may contain
   * characters up to and surrogate pair are resolved
   * 
   * UTF-8 encoding length attacks are mitigated
   * 
   * The number of bytes are determined by `name_length`
   */
  char* name;
  
  /**
   * The number of bytes in `name`
   */
  size_t name_length;
  
  /**
   * The value associated with the key `name`
   */
  struct libqwaitclient_json value;
  
} libqwaitclient_json_association_t;



#define  _this_  libqwaitclient_json_t* restrict this


/**
 * Release all resources in a JSON structure
 * 
 * @param  this  The JSON structure
 */
void libqwaitclient_json_destroy(_this_);

/**
 * Convert a JSON boolean to a booleanic `int`
 * 
 * @param   this  The JSON boolean
 * @return        1 if true, 0 if false, -1 on error
 */
int libqwaitclient_json_to_bool(const _this_) __attribute__((pure));

/**
 * Convert a JSON string to a NUL-terminated string
 * 
 * @param   this  The JSON string
 * @return        The string in NUL-terminated format, `NULL` on error
 */
char* libqwaitclient_json_to_zstr(const _this_);

/**
 * Convert a JSON string array to an array of NUL-terminated strings
 * 
 * @param   this  The JSON string
 * @return        The array of NUL-termianted strings, `NULL` on error
 */
char* restrict* libqwaitclient_json_to_zstrs(const _this_);

/**
 * Parse a JSON structure
 * 
 * @param   this    The JSON structure to fill in
 * @param   code    The serialised JSON structure
 * @param   length  The length of `code`
 * @return          Zero on success, -1 on error
 */
int libqwaitclient_json_parse(_this_, const char* restrict code, size_t length);

/**
 * Print a JSON structure in debug format, this
 * is not a serialisation for sending data to
 * other machines, it is simply a debugging tool
 * 
 * @param  this    The JSON structure
 * @param  output  The output sink
 */
void libqwaitclient_json_dump(_this_, FILE* output);


#undef _this_

#endif

