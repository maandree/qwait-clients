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
#include "json.h"

#include "macros.h"

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>


/**
 * Whitespace characters allowed by JSON
 */
#define JSON_WHITESPACE  " \t\n\r"


#if defined(DEBUG) && defined(__GNUC__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wformat-extra-args"
# define D(format, ...)  fprintf(stderr, "%s:%i: " format "\n", __FILE__, __LINE__, __VA_ARGS__ +0)
#else
# define D(...)  ((void) 0)
#endif


#define  _this_  libqwaitclient_json_t* restrict this


/**
 * Parse a part of a JSON structure
 * 
 * @param   this    The JSON structure to fill in
 * @param   code    The serialised JSON structure from where we should begin
 * @param   length  The length of `code` (what is remaining of the original code)
 * @return          The number of read char:s, zero on error
 */
static size_t libqwaitclient_json_subparse(_this_, const char* restrict code, size_t length);


/**
 * Release all resources in a JSON structure
 * 
 * @param  this  The JSON structure
 */
void libqwaitclient_json_destroy(_this_)
{
  size_t i, n = this->length;
  
  if (this->type == LIBQWAITCLIENTS_JSON_TYPE_LARGE_INTEGER)
    free(this->data.large_integer), this->data.large_integer = NULL;
  else if (this->type == LIBQWAITCLIENTS_JSON_TYPE_STRING)
    free(this->data.string), this->data.string = NULL;
  else if (this->type == LIBQWAITCLIENTS_JSON_TYPE_ARRAY)
    {
      for (i = 0; i < n; i++)
	libqwaitclient_json_destroy(this->data.array + i);
      free(this->data.array), this->data.array = NULL;
    }
  else if (this->type == LIBQWAITCLIENTS_JSON_TYPE_OBJECT)
    {
      for (i = 0; i < n; i++)
	{
	  free(this->data.object[i].name);
	  libqwaitclient_json_destroy(&(this->data.object[i].value));
	}
      free(this->data.object), this->data.object = NULL;
    }
}


/**
 * Parse a part of a JSON structure that is a string
 * 
 * @param   this    The JSON structure to fill in
 * @param   code    The serialised JSON structure from where we should begin
 * @param   length  The length of `code` (what is remaining of the original code)
 * @return          The number of read char:s, zero on error
 */
static size_t libqwaitclient_json_subparse_string(_this_, const char* restrict code, size_t length)
{
  /* “JSON permits including the null character U+0000 <control-0000>
   * in a string as long as it is escaped (with "\u0000").”
   * 
   * We will permit it without any escape, for simplicity.
   * We cannot use NUL-termination. */
  
  /* “JSON documents may be, and often are, encoded in UTF-8, which
   * supports the full Unicode character set. This allows even characters
   * outside the BMP (U+10000 to U+10FFFF). However, if escaped those
   * characters must be written using surrogate pairs, a detail missed
   * by some JSON parsers. This is similar to how the UTF-16 encoding works.”
   * 
   * I hate JSON. */
  
  int32_t* restrict utf32 = NULL;
  int32_t escaped = 0;
  size_t i, j, n, read_length;
  int escape, saved_errno;
  char* new;
  
  
  /* Find the length of the string. */
  code++;
  for (i = 0, escape = 0; i < length; i++)
    {
      char c = code[i];
      if      (escape)     escape = 0;
      else if (c == '\\')  escape = 1;
      else if (c == '"')   break;
    }
  if (i == length)
    return D("premature end of string",), errno = EINVAL, 0;
  length = i;
  read_length = i + 2;
  
  
  /* Second, we will encode to UTF-32 to surrogate support easier,
     and to mitigate UTF-8 encoding length attacks. For this we
     need a UTF-32 string of at most the same length. */
  if (xmalloc(utf32, length, int32_t))
    return 0;
  
  /* Convert UTF-8 to UTF-32. Allow suboptimal encoding. */
  for (i = j = 0, n = 0; i < length; i++)
    {
      unsigned char c = (unsigned char)(code[i]);
      
      if (n)
	{
	  /* Check that a new character do not start here. */
	  if ((c & 0xC0) != 0x80)
	    return D("new character during multibyte character",), free(utf32), errno = EINVAL, 0;
	  
	  /* Continued reading of multibyte-character. */
	  n--;
	  utf32[j] <<= 6;
	  utf32[j] |= (int32_t)(c & 0x7F);
	  
	  /* Did the multibyte-charcter end here? */
	  if (!n)
	    j++;
	}
      else if ((c & 0x80) == 0)
	/* Single byte-character. */
	utf32[j++] = (int32_t)c;
      else if ((c & 0xC0) == 0x80)
	/* Non-first byte in multibyte-character at being of character. */
	return D("multibyte-character without start",), free(utf32), errno = EINVAL, 0;
      else
	{
	  /* First byte in multibyte-character,
	     get length and most significan bits. */
	  while ((c & 0x80))
	    c <<= 1, n++;
	  utf32[j] = (int32_t)(c >> n);
	  n--;
	}
    }
  /* Check that multibyte-character has ended, or
     that we ended with a single byte-character. */
  if (n)
    return D("string ended during multibyte-character",), free(utf32), errno = EINVAL, 0;
  
  /* Update the length. UTF-32 int:s rather than UTF-8 bytes. */
  length = j;
  
  
  /* Resolve escapes. (Don't bother with what JSON allows, do all of them.) */
  
  for (i = j = 0, escape = 0; i < length; i++)
    {
      char c = (char)(utf32[i]);
      
      /* Non-ASCII is not used in escapes. */
      if (utf32[i] >> 7)
	{
	  if (escape)
	    return D("non-ASCII used in escape",), free(utf32), errno = EINVAL, 0;
	  utf32[j++] = utf32[i];
	}
      else if (escape == -1)
	/* Initial character after escape symbol. */
	switch (c)
	  {
	  case 'e':  utf32[j++] = (int32_t)'\033', escape = 0;  break;
	  case 'r':  utf32[j++] = (int32_t)'\r',   escape = 0;  break;
	  case 't':  utf32[j++] = (int32_t)'\t',   escape = 0;  break;
	  case 'a':  utf32[j++] = (int32_t)'\a',   escape = 0;  break;
	  case 'f':  utf32[j++] = (int32_t)'\f',   escape = 0;  break;
	  case 'v':  utf32[j++] = (int32_t)'\v',   escape = 0;  break;
	  case 'b':  utf32[j++] = (int32_t)'\b',   escape = 0;  break;
	  case 'n':  utf32[j++] = (int32_t)'\n',   escape = 0;  break;
	  case 'x':
	  case 'X':  escape = 2;  break;
	  case 'u':  escape = 4;  break;
	  case 'U':  escape = 6;  break;
	  default:
	    if (('0' <= c) && (c <= '7'))
	      escape = -2;
	    else
	      utf32[j++] = utf32[i], escape = 0;
	    break;
	  }
      else if (escape == -2)
	{
	  /* Octals */
	  if (('0' <= c) && (c <= '7'))
	    escaped = (escaped << 3) | (c & 7);
	  else
	    utf32[j++] = escaped, escaped = 0, i--;
	}
      else if (escape)
	{
	  /* \x (\X), \u and \U */
	  if      (('0' <= c) && (c <= '9'))  escaped = (escaped << 4) | (c & 15);
	  else if (('a' <= c) && (c <= 'f'))  escaped = (escaped << 4) | (c - 'a' + 1);
	  else if (('A' <= c) && (c <= 'F'))  escaped = (escaped << 4) | (c - 'A' + 1);
	  else
	    return D("non-hexadecimal digit in hexadecimal escape",), free(utf32), errno = EINVAL, 0;
	  if (--escape == 0)
	    utf32[j++] = escaped, escaped = 0;
	}
      else if (c == '\\')
	escape = -1;
      else
	utf32[j++] = utf32[i];
    }
  if (escape == -2)
    utf32[j++] = escaped;
  else if (escape)
    return D("string ended during escape",), free(utf32), errno = EINVAL, 0;
  
  /* Update the length. */
  length = j;
  
  /* And now those surrogates. */
  for (i = j = 0, escaped = 0; i < length; i++)
    {
      int32_t a, b, c = utf32[i];
      
      if (escaped)
	{
	  if ((c < 0xD800) || (0xDFFF < c))
	    return D("surrogate without its partner",), free(utf32), errno = EINVAL, 0;
	  
	  /* Lowest surrogate is in lead. */
	  a = escaped, b = c, escaped = 0;
	  if (a > b)
	    a ^= b, b ^= a, a ^= b;
	  
	  /* The lead must not be in [0xDC00, 0xDFFF] */
	  if (a >= 0xDC00)
	    return D("both surrogates in a pair are lead",), free(utf32), errno = EINVAL, 0;
	  
	  /* The trail must be in [0xDC00, 0xDFFF] */
	  if (b < 0xDC00)
	    return D("both surrogates in a pair are trail",), free(utf32), errno = EINVAL, 0;
	  
	  a &= 0x03FF, b &= 0x03FF;
	  utf32[j++] = (a << 10) | b;
	}
      else if ((0xD800 <= c) && (c <= 0xDFFF))
	escaped = c;
      else
	utf32[j++] = c;
    }
  if (escaped)
    return D("string with unpaired surrogate",), free(utf32), errno = EINVAL, 0;
  
  /* Update the length. */
  length = j;
  
  
  /* Re-encode into UTF-8. */
  
  /* Allocated the string, UTF-8 characters are at most 6 bytes,
     but we know that that our result cannot be longer then the
     original encoding. */
  if (xmalloc(this->data.string, read_length, char))
    return saved_errno = errno, free(utf32), errno = saved_errno, 0;
  
  /* Re-encode. */
#define utf8  (this->data.string)
  for (i = length, j = read_length; i-- > 0;) /* Backwards makes UTF-32 to UTF-8 easier. */
    {
      uint32_t c = (uint32_t)(utf32[i]);
      unsigned char prefix = 0x80;
      
      /* Check that escape resolving did not resolve in any invalid
         characters (don't bother if the unsigned version overflowed.) */
      if (c & (1LL << 31))
	return D("character uses the 32:th bit",), free(utf8), errno = EINVAL, 0;
      
      /* Trivial case. (Needs special attention because the 7:th bit can be used.) */
      if (c < 128)
	{
	  utf8[--j] = (char)c;
	  continue;
	}
      
      /* Less trivial case. */
      do
	utf8[--j] = (c & 0x3F) | 0x80, c >>= 6, prefix |= prefix >> 1;
      while (c || (utf8[j] & (prefix ^ 0x80)));
      utf8[j] |= (prefix << 1) & 255;
    }
  
  /* Free UTF-32 encode, calculate UTF-8 encoding length, and 
     move the UTF-8 string to the beginning of its encoding. */
  free(utf32);
  this->length = read_length - j;
  memmove(utf8, utf8 + j, this->length);
#undef utf8
  
  /* Shrink the string's allocation so it is not unnecessarily large. */
  new = this->data.string;
  if (xrealloc(new, this->length, char) && (this->length))
    return 0;
  this->data.string = new;
  
  return read_length;
}


/**
 * Skip whitespaces in a JSON code
 * 
 * @scope  parsed:size_t           The number of read characters, will be updated
 * @scope  length:const size_t     The length of the code
 * @scope  code:const char* const  The code
 */
#define SKIP_JSON_WHITESPACE								\
  while ((parsed < length) && code[parsed] && strchr(JSON_WHITESPACE, code[parsed]))	\
    parsed++


/**
 * Parse a part of a JSON structure that is an array
 * 
 * @param   this    The JSON structure to fill in
 * @param   code    The serialised JSON structure from where we should begin
 * @param   length  The length of `code` (what is remaining of the original code)
 * @return          The number of read char:s, zero on error
 */
static size_t libqwaitclient_json_subparse_array(_this_, const char* restrict code, size_t length)
{
  size_t allocated = 16;
  size_t parsed = 1;
  size_t subparsed;
  
  SKIP_JSON_WHITESPACE;
  
  /* It is an error if the code ends without a ']'. */
  if (parsed == length)
    return D("opened array without elements or closer",), errno = EINVAL, 0;
  
  /* Check for empty array. (Edge case) */
  if (code[parsed] == ']')
    return parsed + 1;
  
  if (xmalloc(this->data.array, allocated, libqwaitclient_json_t))
    return 0;
  
  do
    {
      /* Make sure another element can be added. */
      if (this->length == allocated)
	{
	  libqwaitclient_json_t* new = this->data.array;
	  if (xrealloc(new, allocated <<= 1, libqwaitclient_json_t))
	    return 0;
	  this->data.array = new;
	}
      
      SKIP_JSON_WHITESPACE;
      
      /* Parse next element. */
      subparsed = libqwaitclient_json_subparse(this->data.array + this->length, code + parsed, length - parsed);
      this->length++;
      if (subparsed == 0)
	return 0;
      parsed += subparsed;
      
      SKIP_JSON_WHITESPACE;
      
      /* It is an error if the code ends without a ']'. */
      if (parsed == length)
	return D("premature end of array",), errno = EINVAL, 0;
      
    } while (code[parsed++] == ',');
  
  /* It is an error if the code ends without a ']'. */
  if (code[parsed - 1] != ']') /* The `while();` increased `parsed`. */
    return D("array ended but not with the correct symbol",), errno = EINVAL, 0;
  
  /* Shrink the allocated to fit the elements exactly. */
  if (allocated > this->length)
    {
      libqwaitclient_json_t* new = this->data.array;
      if (xrealloc(new, this->length, libqwaitclient_json_t) && (this->length))
	return 0;
      this->data.array = new;
    }
  
  return parsed;
}


/**
 * Parse a part of a JSON structure that is an object
 * 
 * @param   this    The JSON structure to fill in
 * @param   code    The serialised JSON structure from where we should begin
 * @param   length  The length of `code` (what is remaining of the original code)
 * @return          The number of read char:s, zero on error
 */
static size_t libqwaitclient_json_subparse_object(_this_, const char* restrict code, size_t length)
{
  size_t allocated = 16;
  size_t parsed = 1;
  size_t subparsed;
  
  SKIP_JSON_WHITESPACE;
  
  /* It is an error if the code ends without a '}'. */
  if (parsed == length)
    return D("opened array without elements or closer",), errno = EINVAL, 0;
  
  /* Check for empty object. (Edge case) */
  if (code[parsed] == '}')
    return parsed + 1;
  
  if (xmalloc(this->data.object, allocated, libqwaitclient_json_t))
    return 0;
  
  do
    {
      #define NEXT  (this->data.object[this->length])
      
      /* Make sure another member can be added. */
      if (this->length == allocated)
	{
	  libqwaitclient_json_association_t* new = this->data.object;
	  if (xrealloc(new, allocated <<= 1, libqwaitclient_json_t))
	    return 0;
	  this->data.object = new;
	}
      
      SKIP_JSON_WHITESPACE;
      
      /* Parse next member's name. */
      NEXT.name = NULL;
      subparsed = libqwaitclient_json_subparse(&(NEXT.value), code + parsed, length - parsed);
      if (subparsed == 0)
	return 0;
      if (NEXT.value.type != LIBQWAITCLIENTS_JSON_TYPE_STRING)
	return D("object key was not a string",), this->length++, errno = EINVAL, 0;
      NEXT.name = NEXT.value.data.string;
      NEXT.name_length = NEXT.value.length;
      memset(&(NEXT.value), 0, sizeof(libqwaitclient_json_t));
      parsed += subparsed;
      
      SKIP_JSON_WHITESPACE;
      
      /* ':' delimits a member's name and its value. */
      if ((parsed == length) || (code[parsed++] != ':'))
	return D("invalid delimiter between object key and value",), free(NEXT.name), errno = EINVAL, 0;
      
      SKIP_JSON_WHITESPACE;
      
      /* Parse next member's value. */
      subparsed = libqwaitclient_json_subparse(&(NEXT.value), code + parsed, length - parsed);
      this->length++;
      if (subparsed == 0)
	return 0;
      parsed += subparsed;
      
      SKIP_JSON_WHITESPACE;
      
      /* It is an error if the code ends without a '}'. */
      if (parsed == length)
	return D("premature end of object",), errno = EINVAL, 0;
      
      #undef NEXT
    } while (code[parsed++] == ',');
  
  /* It is an error if the code ends without a '}'. */
  if (code[parsed - 1] != '}') /* The `while();` increased `parsed`. */
    return D("array ended but not with the correct symbol",), errno = EINVAL, 0;
  
  /* Shrink the allocated to fit the members exactly. */
  if (allocated > this->length)
    {
      libqwaitclient_json_association_t* new = this->data.object;
      if (xrealloc(new, this->length, libqwaitclient_json_association_t) && (this->length))
	return 0;
      this->data.object = new;
    }
  
  return parsed;
}


#undef SKIP_JSON_WHITESPACE


/**
 * Parse a part of a JSON structure that is an integer
 * 
 * @param   this    The JSON structure to fill in
 * @param   code    The serialised JSON structure from where we should begin
 * @param   length  The length of the encoding of the value (cannot more)
 * @return          Zero on success, -1 on error
 */
static int libqwaitclient_json_subparse_integer(_this_, const char* restrict code, size_t length)
{
  #define STR_INT64_END  "9223372036854775808"  /* 2⁶³ = INT64_MAX + 1 */
  
  int r, pos, neg;
  char buf[20];
  char* end = NULL;
  int twoscomp = (-INT64_MAX) != INT64_MIN;
  /* C allows three diffent signaled integer representations:
     
       - Two's complement:   INT64_MIN = -INT64_MAX - 1, or, -x == ~x + 1
       - Ones' complement:   INT64_MIN = -INT64_MAX and -x == ~x
       - Sign and magnitude: INT64_MIN = -INT64_MAX and (x << 1) == (-x << 1)
     
     GCC always uses two's complement, but this is not true for C in general.
  */
  
  /* Read sign. */
  pos = *code == '+';
  neg = *code == '-';
  if (pos | neg)
    code++, length--;
  pos = !neg;
  
  /* We most have a number, and we have already read the sign. */
  if ((length == 0) || (*code == '+') || (*code == '-'))
    return D("integer must start with at most one +/- and must otherwise have content",), errno = EINVAL, -1;
  
  /* If the length is more than 19, we know that it is larger than `int64_t`. */
  if (length > 19)
    goto large_integer;
  
  /* Copy the code into a NUL-terminated buffer so we
     can compare against the `int64_t` and parse its
     value with `strtoll`. */
  memcpy(buf, code, length * sizeof(char));
  buf[length] = '\0';
  
  /* Is the integer larger than `int64_t`? We are restricted to `INT64_MAX`. */
  if (!twoscomp || pos)
    if (strcmp(buf, STR_INT64_END) >= 0)
      goto large_integer;
  
  /* Is the integer larger than `int64_t`? We are restricted to `INT64_MAX + 1`. */
  if (twoscomp && neg)
    {
      r = strcmp(buf, STR_INT64_END);
      if (r == 0)
	return this->data.integer = INT64_MIN, 0; /* Edge case. */
      else if (r > 0)
	goto large_integer;
    }
  
  /* Parse the integer. */
  this->data.integer = (int64_t)strtoll(buf, &end, 10);
  if (end == NULL)
    return -1;
  if (*end != '\0')
    return D("integer was malformated",), errno = EINVAL, -1;
  /* Apply sign. */
  if (neg)
    this->data.integer = -(this->data.integer);
  
  return 0;
  
  /* Large integer! */
 large_integer:
  this->type = LIBQWAITCLIENTS_JSON_TYPE_LARGE_INTEGER;
  if (xmalloc(this->data.large_integer, neg + length + 1, char))
    return -1;
  memcpy(this->data.large_integer + neg, code, length * sizeof(char));
  if (neg)
    this->data.large_integer[0] = '-';
  this->data.large_integer[neg + length] = '\0';
  this->length = neg + length;
  return 0;
}


/**
 * Parse a part of a JSON structure that is a floating-point number
 * 
 * @param   this    The JSON structure to fill in
 * @param   code    The serialised JSON structure from where we should begin
 * @param   length  The length of the encoding of the value (cannot more)
 * @return          Zero on success, -1 on error
 */
static int libqwaitclient_json_subparse_floating(_this_, const char* restrict code, size_t length)
{
  char* buf = NULL;
  char* end = NULL;
  int saved_errno;
  
  /* We need NUL-termination for the next step. */
  if (xmalloc(buf, length + 1, char))
    return -1;
  memcpy(buf, code, length * sizeof(char));
  buf[length] = '\0';
  
  /* `strtod` uses a superset of JSON's floating-point syntax; that is OK. */
  this->data.floating = strtod(buf, &end);
  if (end == NULL)
    return saved_errno = errno, free(buf), errno = saved_errno, -1;
  if (*end != '\0')
    return D("floating-point number was malformatted",), free(buf), errno = EINVAL, -1;
  
  free(buf);
  return 0;
}


/**
 * Parse a part of a JSON structure that is a number
 * 
 * @param   this    The JSON structure to fill in
 * @param   code    The serialised JSON structure from where we should begin
 * @param   length  The length of `code` (what is remaining of the original code)
 * @return          The number of read char:s, zero on error
 */
static size_t libqwaitclient_json_subparse_number(_this_, const char* restrict code, size_t length)
{
  size_t part_length = 0;
  int r, is_float = 0;
  
  /* Measure the length of the value's encoding and identify whether it is floating-point. */
  while (part_length < length)
    if (code[part_length] && strchr("+-0123456789", code[part_length]))
      part_length++;
    else if (code[part_length] && strchr(".eE", code[part_length]))
      {
	is_float = 1;
	part_length++;
      }
    else
      break;
  
  /* Set preliminary type. (Is final if floating.) */
  this->type = is_float ? LIBQWAITCLIENTS_JSON_TYPE_FLOATING : LIBQWAITCLIENTS_JSON_TYPE_INTEGER;
  
  /* Parse the value. */
  if (is_float)
    r = libqwaitclient_json_subparse_floating(this, code, part_length);
  else
    r = libqwaitclient_json_subparse_integer(this, code, part_length);
  
  /* The parsers used above use the zero–negative-convention. */
  return r < 0 ? 0 : part_length;
}


/**
 * Parse a part of a JSON structure
 * 
 * @param   this    The JSON structure to fill in
 * @param   code    The serialised JSON structure from where we should begin
 * @param   length  The length of `code` (what is remaining of the original code)
 * @return          The number of read char:s, zero on error
 */
static size_t libqwaitclient_json_subparse(_this_, const char* restrict code, size_t length)
{
  /* The data types are not equality large, so we
   * initalise everything to avoid runtime warnings. */
  memset(this, 0, sizeof(libqwaitclient_json_t));
  /* Pleasant side effect: this->length = 0 and arrays are `NULL` */
  
  /* That would be invalid, and our code below assumes `length` >= 1. */
  if (length == 0)
    return D("have nothing to parse",), errno = EINVAL, 0;
  
  /* String, array and object are easy to identify, delegate it, they are complex. */
  if (*code == '"')
    {
      this->type = LIBQWAITCLIENTS_JSON_TYPE_STRING;
      return libqwaitclient_json_subparse_string(this, code, length);
    }
  else if (*code == '[')
    {
      this->type = LIBQWAITCLIENTS_JSON_TYPE_ARRAY;
      return libqwaitclient_json_subparse_array(this, code, length);
    }
  else if (*code == '{')
    {
      this->type = LIBQWAITCLIENTS_JSON_TYPE_OBJECT;
      return libqwaitclient_json_subparse_object(this, code, length);
    }
  
  /* Null, true, and false are keyword that are easily distinguishable
   * and there no other alternatives. We parse the them exactly, a caller
   * will fail of there was something more behind them. */
  if ((length >= 4) && !memcmp(code, "null", 4 * sizeof(char)))
    return this->type = LIBQWAITCLIENTS_JSON_TYPE_NULL, 4;
  else if ((length >= 4) && !memcmp(code, "true", 4 * sizeof(char)))
    return this->type = LIBQWAITCLIENTS_JSON_TYPE_BOOLEAN, this->data.boolean = 1, 4;
  else if ((length >= 5) && !memcmp(code, "false", 5 * sizeof(char)))
    return this->type = LIBQWAITCLIENTS_JSON_TYPE_BOOLEAN, this->data.boolean = 0, 5;
  
  /* Numbers are a bit more complex, delegate it. */
  return libqwaitclient_json_subparse_number(this, code, length);
}


/**
 * Parse a JSON structure
 * 
 * @param   this    The JSON structure to fill in
 * @param   code    The serialised JSON structure
 * @param   length  The length of `code`
 * @return          Zero on success, -1 on error
 */
int libqwaitclient_json_parse(_this_, const char* restrict code, size_t length)
{
  size_t parsed;
  int saved_errno;
  
  /* Ignoring leading and trailing whitespace. */
  while (length && *code && strchr(JSON_WHITESPACE, *code))
    code++, length--;
  while (length && code[length - 1] && strchr(JSON_WHITESPACE, code[length - 1]))
    length--;
  
  /* Parse the code. */
  parsed = libqwaitclient_json_subparse(this, code, length);
  if (parsed == 0)
    {
      saved_errno = errno;
      libqwaitclient_json_destroy(this);
      return errno = saved_errno, -1;
    }
  
  /* Require that everything was part of the JSON code. */
  if (parsed < length)
    return D("parsed data did not make up all data",), libqwaitclient_json_destroy(this), errno = EINVAL, -1;
  
  return 0;
}


/**
 * Print a string as part of a JSON structure in debug format, exclude surrounding quotes
 * 
 * @param  f       The output sink
 * @param  string  The string to print
 * @parma  n       The `char`-length of `string`
 */
static void libqwaitclient_json_subdump_string(FILE* f, const char* string, size_t n)
{
  size_t i, j, size = min(n, 16);
  char* buf;
  
  if (xmalloc(buf, size, char))
    perror("libqwaitclient_json_subdump_string"), abort();
  
  for (i = j = 0; i < n; i++)
    {
      char c = string[i];
      
      if (j + 10 > size)
	if (xrealloc(buf, size <<= 1, char))
	  perror("libqwaitclient_json_subdump_string"), abort();
      
      if (strchr("\\\"", c))
	buf[j++] = '\\', buf[j++] = c;
      else if ((0 <= c) && (c < ' '))
	{
	  buf[j++] = '\\';
	  switch (c)
	    {
	    case '\033':  buf[j++] = 'e';  break;
	    case '\r':    buf[j++] = 'r';  break;
	    case '\t':    buf[j++] = 't';  break;
	    case '\a':    buf[j++] = 'a';  break;
	    case '\f':    buf[j++] = 'f';  break;
	    case '\v':    buf[j++] = 'v';  break;
	    case '\b':    buf[j++] = 'b';  break;
	    case '\n':    buf[j++] = 'n';  break;
	    default:
	      sprintf(buf + j, "%o", (int)c);
	      j += strlen(buf + j);
	      break;
	    }
	}
      else
	buf[j++] = c;
    }
  
  buf[j] = '\0';
  fprintf(f, "%s", buf);
  free(buf);
}


/**
 * Print a part of a JSON structure in debug format
 * 
 * @param  this    The JSON structure
 * @param  f       The output sink
 * @parma  indent  The size of the indent
 */
static void libqwaitclient_json_subdump(_this_, FILE* f, int indent)
{
#define PRIindent  "*.s"
  size_t i, n = this->length;
  
  switch (this->type)
    {
    case LIBQWAITCLIENTS_JSON_TYPE_INTEGER:
      fprintf(f, "%" PRIi64, this->data.integer);
      break;
      
    case LIBQWAITCLIENTS_JSON_TYPE_LARGE_INTEGER:
      fprintf(f, "%s(L)", this->data.large_integer);
      break;
      
    case LIBQWAITCLIENTS_JSON_TYPE_FLOATING:
      fprintf(f, "%lf(F)", this->data.floating);
      break;
      
    case LIBQWAITCLIENTS_JSON_TYPE_STRING:
      fprintf(f, "\"");
      libqwaitclient_json_subdump_string(f, this->data.string, n);
      fprintf(f, "\"(%zu)", this->length);
      break;
      
    case LIBQWAITCLIENTS_JSON_TYPE_BOOLEAN:
      fprintf(f, this->data.boolean ? "true" : "false");
      break;
      
    case LIBQWAITCLIENTS_JSON_TYPE_ARRAY:
      if (n == 0)
	{
	  fprintf(f, "[]");
	  break;
	}
      if (n == 1)
	{
	  fprintf(f, "[");
	  libqwaitclient_json_subdump(this->data.array, f, indent);
	  fprintf(f, "]");
	  break;
	}
      fprintf(f, "[\n%" PRIindent, indent + 2, "");
      for (i = 0; i < n; i++)
	{
	  fprintf(f, "%s%" PRIindent "", i ? ",\n" : "", i ? indent + 2 : 0, "");
	  libqwaitclient_json_subdump(this->data.array + i, f, indent + 2);
	}
      fprintf(f, "\n%" PRIindent "]", indent, "");
      break;
      
    case LIBQWAITCLIENTS_JSON_TYPE_OBJECT:
      if (n == 0)
	{
	  fprintf(f, "{ }");
	  break;
	}
      if (n == 1)
	{
	  fprintf(f, "{ \"");
	  libqwaitclient_json_subdump_string(f, this->data.object->name, this->data.object->name_length);
	  fprintf(f, "\"(%zu) = ", this->data.object->name_length);
	  libqwaitclient_json_subdump(&(this->data.object->value), f, indent);
	  fprintf(f, " }");
	  break;
	}
      fprintf(f, "{\n%" PRIindent, indent + 2, "");
      for (i = 0; i < n; i++)
	{
	  fprintf(f, "%s%" PRIindent "\"", i ? ",\n" : "", i ? indent + 2 : 0, "");
	  libqwaitclient_json_subdump_string(f, this->data.object[i].name, this->data.object[i].name_length);
	  fprintf(f, "\"(%zu) = ", this->data.object[i].name_length);
	  libqwaitclient_json_subdump(&(this->data.object[i].value), f, indent + 2);
	}
      fprintf(f, "\n%" PRIindent "}", indent, "");
      break;
      
    case LIBQWAITCLIENTS_JSON_TYPE_NULL:
      fprintf(f, "null");
      break;
      
    default:
      abort();
      break;
    }
  
#undef PRIindent
}


/**
 * Print a JSON structure in debug format, this
 * is not a serialisation for sending data to
 * other machines, it is simply a debugging tool
 * 
 * @param  this    The JSON structure
 * @param  output  The output sink
 */
void libqwaitclient_json_dump(_this_, FILE* output)
{
  libqwaitclient_json_subdump(this, output, 0);
  fprintf(output, "\n");
}


#undef _this_
#undef D
#if defined(DEBUG) && defined(__GNUC__)
# pragma GCC diagnostic pop
#endif

