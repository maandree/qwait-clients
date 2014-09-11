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
#ifndef LIBQWAITCLIENTS_MACROS_H
#define LIBQWAITCLIENTS_MACROS_H



/**
 * Return the maximum value of two values
 * 
 * @param   a  One of the values
 * @param   b  The other one of the values
 * @return     The maximum value
 */
#define max(a, b)  \
  (a < b ? b : a)


/**
 * Return the minimum value of two values
 * 
 * @param   a  One of the values
 * @param   b  The other one of the values
 * @return     The minimum value
 */
#define min(a, b)  \
  (a < b ? a : b)


/**
 * Cast a buffer to another type and get the slot for an element
 * 
 * @param   buffer:char*  The buffer
 * @param   type          The data type of the elements for the data type to cast the buffer to
 * @param   index:size_t  The index of the element to address
 * @return  [type]        A slot that can be set or get
 */
#define buf_cast(buffer, type, index)  \
  ((type*)(buffer))[index]


/**
 * Set the value of an element a buffer that is being cast
 * 
 * @param   buffer:char*   The buffer
 * @param   type           The data type of the elements for the data type to cast the buffer to
 * @param   index:size_t   The index of the element to address
 * @param   variable:type  The new value of the element
 * @return  variable:      The new value of the element
 */
#define buf_set(buffer, type, index, variable)	\
  ((type*)(buffer))[index] = (variable)


/**
 * Get the value of an element a buffer that is being cast
 * 
 * @param   buffer:const char*  The buffer
 * @param   type                The data type of the elements for the data type to cast the buffer to
 * @param   index:size_t        The index of the element to address
 * @param   variable:type       Slot to set with the value of the element
 * @return  variable:           The value of the element
 */
#define buf_get(buffer, type, index, variable)	\
  variable = ((const type*)(buffer))[index]


/**
 * Increase the pointer of a buffer
 * 
 * @param   buffer:char*  The buffer
 * @param   type          A data type
 * @param   count:size_t  The number elements of the data type `type` to increase the pointer with
 * @retrun  buffer:       The buffer
 */
#define buf_next(buffer, type, count)  \
  buffer += (count) * sizeof(type) / sizeof(char)


/**
 * Decrease the pointer of a buffer
 * 
 * @param   buffer:char*  The buffer
 * @param   type          A data type
 * @param   count:size_t  The number elements of the data type `type` to decrease the pointer with
 * @retrun  buffer:       The buffer
 */
#define buf_prev(buffer, type, count)  \
  buffer -= (count) * sizeof(type) / sizeof(char)


/**
 * This macro combines `buf_set` with `buf_next`, it sets
 * element zero and increase the pointer by one element
 * 
 * @param   buffer:char*   The buffer
 * @param   type           The data type of the elements for the data type to cast the buffer to
 * @param   variable:type  The new value of the element
 * @return  variable:      The new value of the element
 */
#define buf_set_next(buffer, type, variable)  \
  buf_set(buffer, type, 0, variable),         \
  buf_next(buffer, type, 1)


/**
 * This macro combines `buf_set` with `buf_next`, it sets
 * element zero and increase the pointer by one element
 * 
 * @param   buffer:char*   The buffer
 * @param   type           The data type of the elements for the data type to cast the buffer to
 * @param   variable:type  Slot to set with the value of the element
 * @return  variable:      The value of the element
 */
#define buf_get_next(buffer, type, variable)  \
  buf_get(buffer, type, 0, variable),         \
  buf_next(buffer, type, 1)


/**
 * Check whether two strings are equal
 * 
 * @param   a:const char*  One of the strings
 * @param   b:const char*  The other of the strings
 * @return  :int           Whether the strings are equal
 */
#define strequals(a, b)  \
  (strcmp(a, b) == 0)


/**
 * Check whether a string starts with another string
 * 
 * @param   haystack:const char*  The string to inspect
 * @param   needle:const char*    The string `haystack` should start with
 * @return  :int                  Whether `haystack` starts with `needle`
 */
#define startswith(haystack, needle)  \
  (strstr(haystack, needle) == haystack)


/**
 * Free an array and all elements in an array
 * 
 * @param  array:void**     The array to free
 * @param  elements:size_t  The number of elements, in the array, to free
 * @scope  i:size_t         The variable `i` must be declared as `size_t` and available for use
 */
#define xfree(array, elements)    \
  for (i = 0; i < elements; i++)  \
    free((array)[i]);		  \
  free(array)


/**
 * `malloc` wrapper that returns whether the allocation was not successful
 *  
 * @param   var:type*        The variable to which to assign the allocation
 * @param   elements:size_t  The number of elements to allocate
 * @param   type             The data type of the elements for which to create an allocation
 * @return  :int             Evaluates to true if an only if the allocation failed
 */
#define xmalloc(var, elements, type)  \
  ((var = malloc((elements) * sizeof(type))) == NULL)


/**
 * `calloc` wrapper that returns whether the allocation was not successful
 *  
 * @param   var:type*        The variable to which to assign the allocation
 * @param   elements:size_t  The number of elements to allocate
 * @param   type             The data type of the elements for which to create an allocation
 * @return  :int             Evaluates to true if an only if the allocation failed
 */
#define xcalloc(var, elements, type)  \
  ((var = calloc(elements, sizeof(type))) == NULL)


/**
 * `remalloc` wrapper that returns whether the allocation was not successful
 *  
 * @param   var:type*        The variable to which to assign the reallocation
 * @param   elements:size_t  The number of elements to allocate
 * @param   type             The data type of the elements for which to create an allocation
 * @return  :int             Evaluates to true if an only if the allocation failed
 */
#define xrealloc(var, elements, type)  \
  ((var = realloc(var, (elements) * sizeof(type))) == NULL)


/**
 * Double to the size of an allocation on the heap
 * 
 * @param   old:type*        Variable in which to store the old value temporarily
 * @param   var:type*        The variable to which to assign the reallocation
 * @param   elements:size_t  The number of elements to allocate
 * @param   type             The data type of the elements for which to create an allocation
 * @return  :int             Evaluates to true if an only if the allocation failed
 */
#define growalloc(old, var, elements, type)  \
  (old = var, xrealloc(var, (elements) <<= 1, type) ? (var = old, (elements) >>= 1, perror(*argv), 1) : 0)


/**
 * Return the return of an expression if its return is negative
 * 
 * @param  expression:int  The expression to test
 * @scope  r:int           The variable `r` must be declared as `r` and available for use
 */
#define try(condition)   if ((r = (condition)) < 0)  return r


#endif

