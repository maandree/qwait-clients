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
#include "login-information.h"

#include "macros.h"
#include "json.h"

#include <string.h>
#include <errno.h>
#include <stdlib.h>



#define _this_  libqwaitclient_login_information_t* restrict this
#define _json_  libqwaitclient_json_t* restrict json



/**
 * Find the beginning of the login information in a message
 * 
 * @param   message         The message content, not NUL-terminated,
 *                          this string's content will be modified
 * @param   message_length  The length of `message`
 * @return                  A position inside `message` where the informaion starts,
 *                          this data will _not_ be NUL-terminated, `NULL` on error
 */
static char* libqwaitclient_login_information_grep(char* restrict message, size_t message_length)
{
  char* substr;
  
  /* NUL-terminate the message so we can use `strstr` on it. The last character in the
     message is removed, this does not matter because it cannot be a port of the information
     we are interested in. Namely, we a want a JavaScript and in the worst case of a valid
     message the last character is the '>' in the "</html>" that ends the message, in the
     best case (that is, otherwise) it is insignificant whitespace after then end of the
     message. Thus removing the last character cannot impact us. */
  message[message_length - 1] = '\0';
  
  /* We are interested in in some JavaScript, so just to be on the safe side lets seek to
     the first and only CDATA so we do not accidentally get the wrong text on our next
     search incase the file's content changes in the future. */
  substr = strstr(message, "<![CDATA[");
  if (substr == NULL)
    return errno = EBADMSG, NULL;
  
  /* We are interested in what the only function returns. */
  substr = strstr(substr, "return");
  if (substr == NULL)
    return errno = EBADMSG, NULL;
  
  /* That function returns a map, so we seek past the return keyword and any whitespace. */
  substr = strchr(substr, '{');
  if (substr == NULL)
    return errno = EBADMSG, NULL;
  
  /* What we are interested in ends at a semi-colon, however, we cannot be sure that a text
     inside that data is does not contain a semi-colon so we will not look for it. */
  
  return substr;
}


/**
 * NUL-terminate the data found by `libqwaitclient_login_information_grep`
 * 
 * @param   data  The data found by `libqwaitclient_login_information_grep`,
 *                it will be cut of with another NUL-termination
 * @return        Zero on success, -1 on error
 */
static int libqwaitclient_login_information_nul_terminate(char* restrict data)
{
  size_t i;
  int brackets = 0, escape = 0;
  char quote = '\0';
  
  for (i = 0;;)
    {
      char c = data[i++];
      if (c == '\0')
	/* `libqwaitclient_login_information_grep` NUL-terminated the message
	   but something is very wrong if we got all the way there. */
	return errno = EBADMSG, -1;
      
      if (quote != '\0')
	{
	  if      (escape)         escape = 0;
	  else if (c == '\\')      escape = 1;
	  else if (c == quote)     quote = '\0';
	}
      else if (strchr("\'\"", c))  quote = c;
      else if (strchr("{[", c))    brackets++;
      else if (strchr("]}", c))
	if (--brackets == 0)
	  break;
    }
  
  data[i] = '\0';
  return 0;
}


/**
 * Convert a strings to JSON-style strings, that is surrounded by
 * "-quotes and not '-quotes
 * 
 * @param   data  The data whose strings should be JSON:ified, this string
 *                must use the same allocation as the `message` parameter
 *                in `libqwaitclient_login_information_grep` because we want
 *                to use the allocated space behide the data to avoid
 *                unnecessarily creating a new allocation.
 * @return        Zero on success, -1 on error
 */
static int libqwaitclient_login_information_jsonify_strings(char* restrict data)
{
  size_t i, j, n = strlen(data);
  int escape = 0;
  char quote = '\0';
  
  /* Make sure a strings are started and ended with "-quotes
     rather than '-quotes and that all "-quotes inside strings
     are escaped. */
  for (i = 0, j = n + 1; i < n; i++)
    {
      char c = data[i];
      
      if (quote != '\0')
	{
	  if      (escape)         escape = 0;
	  else if (c == '\\')      escape = 1;
	  else if (c == quote)     quote = '\0';
	  else if (c == '"')
	    {
	      if (data[j] == '\0')
		/* `libqwaitclient_login_information_grep` NUL-terminated the message
		   but something is very wrong if we got all the way there. */
		return errno = EBADMSG, -1;
	      data[j++] = '\\';
	    }
	}
      else if (strchr("\'\"", c))
	quote = c, c = '\"';
      
      if (data[j] == '\0')
	return errno = EBADMSG, -1; /* Same problem as earlier. */
      data[j++] = c;
    }
  
  /* Move new string to where the old string was. */
  memmove(data, data + n + 1, (j - n) * sizeof(char));
  /* NUL-termiante the data. */
  data[j - n] = '\0';
  
  /* Because we know that the string cannot have shrunk, only
     stayed the same size or growed a little bit, there will
     be no NUL-terminations beyond this string except one
     character before the original message ended, as inserted
     by `libqwaitclient_login_information_grep` */
  
  return 0;
}


/**
 * Convert a object member names (keys) to JSON-style strings rather
 * than JavaScript-style identifiers
 * 
 * @param   data  The data whose keys should be JSON:ified, this string
 *                must use the same allocation as the `message` parameter
 *                in `libqwaitclient_login_information_grep` because we want
 *                to use the allocated space behide the data to avoid
 *                unnecessarily creating a new allocation.
 * @return        Zero on success, -1 on error
 */
static int libqwaitclient_login_information_jsonify_keys(char* restrict data)
{
#define WHITESPACE  " \t\n\r"
  
  size_t i, j, n, end;
  int quote = 0, stage = 0;
  
  /* Find the end of tha data and the end of the original
     message, the latter is the end of the allocation we
     can use to store our JSON:ified version of the data. */
  n = strlen(data);
  end = strlen(data + n + 1) + n + 1;
  
  /* Make sure the allocation we can reuse is large enough,
     if it is not the original message must have been corrupt.
     We are satisfied with the reusable space being as large
     as the data, this is actually excessive as we expect to
     only add 20 characters and we will be able to reuse the
     space were the data is stored. */
  if (n + n > end)
    return errno = EBADMSG, -1;
  
  /* Surround all keys with quotes, we will read the data
     backwards to simplify the task of finding the keys. */
  for (i = n, j = end - 1;; j--, i--)
    {
      char c = data[i];
      
      if (stage == 1)
	{
	  if (strchr(WHITESPACE, c) == NULL)
	    data[j--] = '\"', stage = 2;
	}
      else if (stage == 2)
	{
	  if (strchr(WHITESPACE, c) != NULL)
	    data[j--] = c, stage = 0, c = '\"';
	}
      else if (quote)
	{
	  if ((c == '\"') && (data[i - 1] != '\\'))
	    quote = 0;
	}
      else if (c == '\"')
	quote = 1;
      else if (c == ':')
	stage = 1;
      
      data[j] = c;
      if (i == 0)
	break;
    }
  
  /* Move new string to where the old string was. */
  memmove(data, data + j, (end - j) * sizeof(char));
  /* NUL-termiante the data. */
  data[end - j] = '\0';
  
  /* Because we know that the string cannot have shrunk, only
     growed a little bit or hypothetically stayed the same size,
     there will be no NUL-terminations beyond this string except
     one character before the original message ended, as inserted
     by `libqwaitclient_login_information_grep` */
  
  return 0;
#undef WHITESPACE
}


/**
 * Get a JSON-representation of the login information stored in a message
 * 
 * @param   json            Output parameter for the JSON-parsed data
 * @param   message         The message content, not NUL-terminated,
 *                          this string's content will be modified
 * @param   message_length  The length of `message`
 * @return                  Zero on success, -1 on error
 */
static int libqwaitclient_login_information_get_json(_json_, char* message, size_t message_length)
{
  char* data = libqwaitclient_login_information_grep(message, message_length);
  if (data == NULL)
    return -1;
  
  if (libqwaitclient_login_information_nul_terminate  (data) < 0)  return -1;
  if (libqwaitclient_login_information_jsonify_strings(data) < 0)  return -1;
  if (libqwaitclient_login_information_jsonify_keys   (data) < 0)  return -1;
  
  return libqwaitclient_json_parse(json, data, strlen(data));
}


/**
 * Create a named zero-length array in a JSON object
 * 
 * @param   member  Where the array should be stored
 * @param   name    The name of the array
 * @return          Zero on success, -1 on error
 */
static int array0(libqwaitclient_json_association_t* restrict member, const char* restrict name)
{
  member->value.type = LIBQWAITCLIENT_JSON_TYPE_ARRAY;
  member->value.length = 0;
  member->value.data.array = NULL;
  member->name_length = strlen(name);
  member->name = strdup(name);
  return member->name == NULL ? -1 : 0;
}


/**
 * Parse JSON-data into login information
 * 
 * @param   this  Output parameter for the login information
 * @param   json  The JSON-data, its content by be modified
 * @return        Zero on success, -1 on error
 */
static int libqwaitclient_login_information_parse_json(_this_, _json_)
{
  const libqwaitclient_json_t* restrict data_hostname     = NULL;
  libqwaitclient_json_t*       restrict data_current_user = NULL;
  libqwaitclient_json_t*       restrict data_product      = NULL;
  size_t i, n = json->length;
  int saved_errno;
  
  if (json->type != LIBQWAITCLIENT_JSON_TYPE_OBJECT)
    return errno = EINVAL, -1;
  
#define test(want)  ((strlen(want) == len) && !memcmp(name, want, len * sizeof(char)))
  
  /* Read information. */
  for (i = 0; i < n; i++)
    {
      libqwaitclient_json_t* restrict value = &(json->data.object[i].value);
      char* name = json->data.object[i].name;
      size_t len = json->data.object[i].name_length;
      
      if      (test("currentUser"))  data_current_user = value;
      else if (test("hostname"))     data_hostname     = value;
      else if (test("product"))      data_product      = value;
      else
	goto einval;
    }
  
#undef test
  
  /* Check that everything was found. */
  if (data_current_user == NULL)  goto einval;
  if (data_hostname     == NULL)  goto einval;
  if (data_product      == NULL)  goto einval;
  
#define str(var, have)  	 (((have)->type == LIBQWAITCLIENT_JSON_TYPE_NULL) ?	\
				  (var = NULL, 0) :					\
				  (var = libqwaitclient_json_to_zstr(have), var == NULL))
#define test_name(member, key)	 (((member).name_length == strlen(key)) && 		\
				  !memcmp((member).name, key, (member).name_length * sizeof(char)))
  
  /* Evaluate data. */
  /*  Parse hostname.  */
  if (str(this->hostname, data_hostname))                          goto fail;
  /*  Check data type for user and product, and member count for the latter.  */
  if (data_current_user->type != LIBQWAITCLIENT_JSON_TYPE_OBJECT)  goto einval;
  if (data_product->type      != LIBQWAITCLIENT_JSON_TYPE_OBJECT)  goto einval;
  if (data_product->length    != 2)                                goto einval;
  /*  Add empty data for missing information for the user so we can use qwait-user to parse that information.  */
  if (xmalloc(data_current_user->data.object,
	      data_current_user->length + 3,
	      libqwaitclient_json_association_t))
    goto fail;
  if (array0(data_current_user->data.object + data_current_user->length++, "queuePositions"))   goto fail;
  if (array0(data_current_user->data.object + data_current_user->length++, "ownedQueues"))      goto fail;
  if (array0(data_current_user->data.object + data_current_user->length++, "moderatedQueues"))  goto fail;
  /*  Parse current user.  */
  if (libqwaitclient_qwait_user_parse(&(this->current_user), data_current_user) < 0)            goto fail;
  /*  Make sure the product name comes before the product version, swap otherwise.  */
  if (data_product->data.object[0].name[1] == 'n')
    {
      libqwaitclient_json_association_t temp = data_product->data.object[0];
      data_product->data.object[0] = data_product->data.object[1];
      data_product->data.object[1] = temp;
    }
  /*  Check that the member of the product has the expected names.  */
  if (!test_name(data_product->data.object[0], "name"))     goto einval;
  if (!test_name(data_product->data.object[1], "version"))  goto einval;
  /*  Parse product.  */
  if (str(this->product.name,    &(data_product->data.object[0].value)))  goto fail;
  if (str(this->product.version, &(data_product->data.object[1].value)))  goto fail;
  
#undef test_name
#undef str
  
  return 0;
  
 einval:
  errno = EINVAL;
 fail:
  saved_errno = errno;
  libqwaitclient_login_information_destroy(this);
  return errno = saved_errno, -1;
}


/**
 * Initialise a login information structure
 * 
 * @param  this  The login information that shall be initialised
 */
void libqwaitclient_login_information_initialise(_this_)
{
  memset(this, 0, sizeof(libqwaitclient_login_information_t));
}


/**
 * Release information in a login information structure,
 * but do not free the structure itself
 * 
 * @param  this  The login information
 */
void libqwaitclient_login_information_destroy(_this_)
{
  libqwaitclient_qwait_user_destroy(&(this->current_user));
  free(this->hostname),        this->hostname        = NULL;
  free(this->product.name),    this->product.name    = NULL;
  free(this->product.version), this->product.version = NULL;
}


/**
 * Parse login information for a HTTP response
 * 
 * @param   this            Output parameter for the login information
 * @param   message         The message content, not NUL-terminated,
 *                          this string's content will be modified
 * @param   message_length  The length of `message`
 * @return                  Zero on success, -1 on error
 */
int libqwaitclient_login_information_parse(_this_, char* restrict message, size_t message_length)
{
  libqwaitclient_json_t json;
  int saved_errno;
  
  memset(&json, 0, sizeof(libqwaitclient_json_t));
  
  if (libqwaitclient_login_information_get_json(&json, message, message_length))  goto fail;
  if (libqwaitclient_login_information_parse_json(this, &json))                   goto fail;
  
 fail:
  saved_errno = errno;
  libqwaitclient_json_destroy(&json);
  return errno = saved_errno, (errno ? -1 : 0);
}



#undef _json_
#undef _this_

