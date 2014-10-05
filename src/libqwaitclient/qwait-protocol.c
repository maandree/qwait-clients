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
#include "qwait-protocol.h"

#include "macros.h"
#include "json.h"

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#define _sock_   libqwaitclient_http_socket_t*    restrict sock
#define _mesg_   libqwaitclient_http_message_t*   restrict mesg
#define _auth_   libqwaitclient_authentication_t* restrict auth
#define _json_   libqwaitclient_json_t*           restrict json
#define _queue_  libqwaitclient_qwait_queue_t*    restrict queue
#define _user_   libqwaitclient_qwait_user_t*     restrict user


#define t(expression)     if (expression)  goto fail
#define mkstr_(buf, ...)  asprintf(&(buf), __VA_ARGS__)
#define mkstr(buf, ...)   (mkstr_(buf, __VA_ARGS__) < 0)



/**
 * Initialise data used protocol functions
 * 
 * @param  mesg  The allocation for the received message
 * @param  json  The allocation for the parsed data, `NULL` if the response is not parsed as JSON
 */
static void initialise(_mesg_, _json_)
{
  if (json != NULL)
    memset(json, 0, sizeof(libqwaitclient_json_t));
  libqwaitclient_http_message_zero_initialise(mesg);
}


/**
 * Release data used protocol functions
 * 
 * @param  mesg  The allocation for the received message
 * @param  json  The allocation for the parsed data, `NULL` if the response is not parsed as JSON
 */
static void destroy(_mesg_, _json_)
{
  if (json != NULL)
    libqwaitclient_json_destroy(json);
  libqwaitclient_http_message_destroy(mesg);
}


/**
 * Common failure procedure for protocol functions
 * 
 * This function will not modify `errno`
 * 
 * @param  sock  The socket used to remote communication
 * @param  mesg  The received message
 * @param  json  The parsed data, `NULL` if the response is not parsed as JSON
 */
static void protocol_failure(const _sock_, _mesg_, _json_)
{
  int saved_errno = errno;
  
#ifdef DEBUG
  /* Dump received message. */
  fprintf(stderr, "=============================================\n");
  fprintf(stderr, "RECEIVED MESSAGE:\n");
  fprintf(stderr, "---------------------------------------------\n");
  libqwaitclient_http_message_dump(&(sock->message), stderr, 0);
  if (json != NULL)
    {
      fprintf(stderr, "---------------------------------------------\n");
      libqwaitclient_json_dump(json, stderr);
    }
  fprintf(stderr, "=============================================\n");
#else
  (void) sock;
#endif
  
  /* Release resources. */
  if (json != NULL)
    libqwaitclient_json_destroy(json);
  libqwaitclient_http_message_destroy(mesg);
  
  /* If we got EINVAL, it should really be EBADMSG. */
  if (saved_errno == EINVAL)
    saved_errno = EBADMSG;
  
  errno = saved_errno;
}


/**
 * Send a query to the server and wait for a response
 * 
 * @param   sock     The socket used to remote communication
 * @param   mesg     Message to send, it will be filled with the standard headers
 *                   and the content, but `mesg->top` must have been set and
 *                   authentication headers must have been added if authentication
 *                   is needed
 * @param   json     Output parameter for the response, `NULL` if it should not be parsed as JSON
 * @param   content  Content to add to the message, `NULL` if none:
 * @return           Zero on success, -1 on error
 */
static int protocol_query(_sock_, _mesg_, _json_, const libqwaitclient_json_t* restrict content)
{
  /* Allocate space for additional headers. */
  t (libqwaitclient_http_message_extend_headers(mesg, content == NULL ? 1 : 3) < 0);
  
  /* Add header: Host */
  t (mkstr(mesg->headers[mesg->header_count++], "Host: %s", sock->host));
  
  /* Add headers: Content-Type */
  if (content != NULL)
    t (mkstr(mesg->headers[mesg->header_count], "Content-Type: application/json"));
  
  /* Add content. */
  if (content != NULL)
    {
      char* content_data = NULL;
      size_t content_length = 0;
      t (libqwaitclient_json_compose(content, &content_data, &content_length));
      mesg->content = content_data;
      mesg->content_size = content_length;
      mesg->content_ptr = content_length;
    }
  
  /* Add headers: Content-Length */
  if (content != NULL)
    t (mkstr(mesg->headers[mesg->header_count++], "Content-Length: %zu", mesg->content_size));
  
  /* Send request, receive response, and parse response. */
  t (libqwaitclient_http_socket_send(sock, mesg));
  t (libqwaitclient_http_socket_receive(sock));
  if (json != NULL)
    t (libqwaitclient_json_parse(json, sock->message.content, sock->message.content_size));
  
  return 0;
  
 fail:
  return -1;
}


/**
 * Create a URI-safe version of a string
 * 
 * @param   string  The unescaped string
 * @return          Corresponding URI-escaped string without
 *                  unnecessarily escaped characters, `NULL` on error
 */
static char* uri_encode(const char* restrict string)
{
  char* rc;
  size_t i, j, n;
  
  n = strlen(string);
  t (xmalloc(rc, n * 3 + 1, char));
  
  for (i = j = 0; i < n; i++)
    {
      char c = string[i];
      if      (('a' <= c) && (c <= 'z'))     rc[j++] = c;
      else if (('A' <= c) && (c <= 'Z'))     rc[j++] = c;
      else if (strchr("0123456789-_.~", c))  rc[j++] = c;
      else
	{
	  rc[j++] = '%';
	  rc[j++] = "0123456789ABCDEF"[(c >> 4) & 15];
	  rc[j++] = "0123456789ABCDEF"[(c >> 0) & 15];
	}
    }
  
  rc[j] = '\n';
  return rc;
  
 fail:
  return NULL;
}



/**
 * Get complete information on all queues
 * 
 * @param   sock         The socket used to remote communication
 * @param   queue_count  Output parameter for the number of returned queues
 * @return               Information for all queues, `NULL` on error
 */
libqwaitclient_qwait_queue_t* libqwaitclient_qwait_get_queues(_sock_, size_t* restrict queue_count)
{
  libqwaitclient_qwait_queue_t* restrict rc;
  libqwaitclient_http_message_t mesg;
  libqwaitclient_json_t json;
  size_t i, n;
  
  initialise(&mesg, &json);
  
  t (mkstr(mesg.top, "GET /api/queues HTTP/1.1"));
  t (protocol_query(sock, &mesg, &json, NULL));
  
  if (json.type != LIBQWAITCLIENT_JSON_TYPE_ARRAY)
    {
      errno = EBADMSG;
      goto fail;
    }
  n = json.length;
  t (xcalloc(rc, max(n, 1), libqwaitclient_qwait_queue_t)); /* `max(n, 1)`: do not return `NULL`. */
  for (i = 0; i < n; i++)
    t (libqwaitclient_qwait_queue_parse(rc + i, json.data.array + i));
  
  return destroy(&mesg, &json), *queue_count = n, rc;
 fail:
  return protocol_failure(sock, &mesg, &json), *queue_count = 0, NULL;
}


/**
 * Get complete information on a queue
 * 
 * @param   sock        The socket used to remote communication
 * @param   queue       Output parameter for the queue
 * @param   queue_name  The ID of the queue
 * @return              Zero on success, -1 on error
 */
int libqwaitclient_qwait_get_queue(_sock_, _queue_, const char* restrict queue_name)
{
  libqwaitclient_http_message_t mesg;
  libqwaitclient_json_t json;
  
  initialise(&mesg, &json);
  libqwaitclient_qwait_queue_initialise(queue);
  
  t (mkstr(mesg.top, "GET /api/queue/%s HTTP/1.1", queue_name));
  t (protocol_query(sock, &mesg, &json, NULL));
  t (libqwaitclient_qwait_queue_parse(queue, &json));
  
  return destroy(&mesg, &json), 0;
 fail:
  return protocol_failure(sock, &mesg, &json), -1;
}


/**
 * Get complete information on all QWait administrators
 * 
 * @param   sock        The socket used to remote communication
 * @param   auth        User authentication
 * @param   user_count  Output parameter for the number of returned users
 * @return              Information on all administrators, `NULL` on error
 */
libqwaitclient_qwait_user_t* libqwaitclient_qwait_get_admins(_sock_, const _auth_, size_t* restrict user_count)
{
  libqwaitclient_qwait_user_t* restrict rc;
  libqwaitclient_http_message_t mesg;
  libqwaitclient_json_t json;
  size_t i, n;
  
  initialise(&mesg, &json);
  
  t (libqwaitclient_auth_sign(auth, &mesg));
  t (mkstr(mesg.top, "GET /api/users?role=admin HTTP/1.1"));
  t (protocol_query(sock, &mesg, &json, NULL));
  
  if (json.type != LIBQWAITCLIENT_JSON_TYPE_ARRAY)
    {
      errno = EBADMSG;
      goto fail;
    }
  n = json.length;
  t (xcalloc(rc, max(n, 1), libqwaitclient_qwait_user_t)); /* `max(n, 1)`: do not return `NULL`. */
  for (i = 0; i < n; i++)
    t (libqwaitclient_qwait_user_parse(rc + i, json.data.array + i));
  
  return destroy(&mesg, &json), *user_count = n, rc;
 fail:
  return protocol_failure(sock, &mesg, &json), *user_count = 0, NULL;
}


/**
 * Get complete information on all QWait users
 * 
 * @param   sock        The socket used to remote communication
 * @param   auth        User authentication
 * @param   user_count  Output parameter for the number of returned users
 * @return              Information on all users, `NULL` on error
 */
libqwaitclient_qwait_user_t* libqwaitclient_qwait_get_users(_sock_, const _auth_, size_t* restrict user_count)
{
  libqwaitclient_qwait_user_t* restrict rc;
  libqwaitclient_http_message_t mesg;
  libqwaitclient_json_t json;
  size_t i, n;
  
  initialise(&mesg, &json);
  
  t (libqwaitclient_auth_sign(auth, &mesg));
  t (mkstr(mesg.top, "GET /api/users HTTP/1.1"));
  t (protocol_query(sock, &mesg, &json, NULL));
  
  if (json.type != LIBQWAITCLIENT_JSON_TYPE_ARRAY)
    {
      errno = EBADMSG;
      goto fail;
    }
  n = json.length;
  t (xcalloc(rc, max(n, 1), libqwaitclient_qwait_user_t)); /* `max(n, 1)`: do not return `NULL`. */
  for (i = 0; i < n; i++)
    t (libqwaitclient_qwait_user_parse(rc + i, json.data.array + i));
  
  return destroy(&mesg, &json), *user_count = n, rc;
 fail:
  return protocol_failure(sock, &mesg, &json), *user_count = 0, NULL;
}


/**
 * Find users by their real name
 * 
 * @param   sock           The socket used to remote communication
 * @param   auth          User authentication
 * @param   partial_name  The all returned user's real name should contain this string
 * @param   user_count    Output parameter for the number of returned users
 * @return                Information on all found users, `NULL` on error
 */
libqwaitclient_qwait_user_t* libqwaitclient_qwait_find_user(_sock_, const _auth_, const char* partial_name,
							    size_t* restrict user_count)
{
  libqwaitclient_qwait_user_t* restrict rc;
  libqwaitclient_http_message_t mesg;
  libqwaitclient_json_t json;
  char* partial_name_uri = NULL;
  size_t i, n;
  
  initialise(&mesg, &json);
  
  t (libqwaitclient_auth_sign(auth, &mesg));
  t (!(partial_name_uri = uri_encode(partial_name)));
  t (mkstr(mesg.top, "GET /api/users?query=%s HTTP/1.1", partial_name_uri));
  t (protocol_query(sock, &mesg, &json, NULL));
  
  if (json.type != LIBQWAITCLIENT_JSON_TYPE_ARRAY)
    {
      errno = EBADMSG;
      goto fail;
    }
  n = json.length;
  t (xcalloc(rc, max(n, 1), libqwaitclient_qwait_user_t)); /* `max(n, 1)`: do not return `NULL`. */
  for (i = 0; i < n; i++)
    t (libqwaitclient_qwait_user_parse(rc + i, json.data.array + i));
  
  return destroy(&mesg, &json), free(partial_name_uri), *user_count = n, rc;
 fail:
  return protocol_failure(sock, &mesg, &json), free(partial_name_uri), *user_count = 0, NULL;
}


/**
 * Get complete information about a user
 * 
 * @param   sock     The socket used to remote communication
 * @param   user     Output parameter for the user information
 * @param   user_id  The user's ID
 * @return           Zero on success, -1 on error
 */
int libqwaitclient_qwait_get_user(_sock_, _user_, const char* restrict user_id)
{
  libqwaitclient_http_message_t mesg;
  libqwaitclient_json_t json;
  
  initialise(&mesg, &json);
  libqwaitclient_qwait_user_initialise(user);
  
  t (mkstr(mesg.top, "GET /api/user/%s HTTP/1.1", user_id));
  t (protocol_query(sock, &mesg, &json, NULL));
  t (libqwaitclient_qwait_user_parse(user, &json));
  
  return destroy(&mesg, &json), 0;
 fail:
  return protocol_failure(sock, &mesg, &json), -1;
}


/**
 * Send a command that does not except a response with content
 * 
 * @param   sock  The socket used to remote communication
 * @param   auth  User authentication
 * @param   json  JSON message to include, may be `NULL`
 * @param   head  Head of the message to send
 * @return          Zero on success, -1 on error
 */
static int send_command(_sock_, const _auth_, const _json_, char* restrict head)
{
  libqwaitclient_http_message_t mesg;
  
  if (head == NULL)
    return -1; /* The functions beneath does not need to care about error handling. */
  
  initialise(&mesg, NULL);
  mesg.top = head;
  
  t (libqwaitclient_auth_sign(auth, &mesg));
  t (protocol_query(sock, &mesg, NULL, json));
  
  return destroy(&mesg, NULL), 0;
 fail:
  return protocol_failure(sock, &mesg, NULL), -1;
}


static char* make_queue_name(const char* restrict queue_title) /* TODO */
{
  (void) queue_title;
  return NULL;

/*
queue.title="$(echo "${queue.name,,}" | sed -r -e 's:[ \f\n\r\t\v\u00a0\u1680\u180e\u2000\u2001\u2002\u2003\u2004\u2005\u2006\u2007\u2008\u2009\u200a\u2028\u2029\u202f\u205f\u3000/]+:-:g')"
*/
}

static int make_json_object(_json_, const char* restrict name, const char* restrict value) /* TODO */
{
  (void) json;
  (void) name;
  (void) value;
  return -1;
}


/**
 * Hide or unhide a queue
 * 
 * @param   sock        The socket used to remote communication
 * @param   auth        User authentication
 * @param   queue_name  The name of the queue
 * @param   hidden      Whether the queue should be hidden
 * @return              Zero on success, -1 on error
 */
int libqwaitclient_qwait_set_queue_hidden(_sock_, const _auth_, const char* restrict queue_name, int hidden)
{
  libqwaitclient_json_t json;
  char* head;
  json.length = 0;
  json.type = LIBQWAITCLIENT_JSON_TYPE_BOOLEAN;
  json.data.boolean = hidden;
  mkstr_(head, "PUT /api/queue/%s/hidden HTTP/1.1", queue_name);
  return send_command(sock, auth, &json, head);
}


/**
 * Lock or unlock a queue
 * 
 * @param   sock        The socket used to remote communication
 * @param   auth        User authentication
 * @param   queue_name  The name of the queue
 * @param   hidden      Whether the queue should be locked
 * @return              Zero on success, -1 on error
 */
int libqwaitclient_qwait_set_queue_locked(_sock_, const _auth_, const char* restrict queue_name, int locked)
{
  libqwaitclient_json_t json;
  char* head;
  json.length = 0;
  json.type = LIBQWAITCLIENT_JSON_TYPE_BOOLEAN;
  json.data.boolean = locked;
  mkstr_(head, "PUT /api/queue/%s/locked HTTP/1.1", queue_name);
  return send_command(sock, auth, &json, head);
}


/**
 * Remove all entries in a queue
 * 
 * @param   sock        The socket used to remote communication
 * @param   auth        User authentication
 * @param   queue_name  The name of the queue
 * @return              Zero on success, -1 on error
 */
int libqwaitclient_qwait_clear_queue(_sock_, const _auth_, const char* restrict queue_name)
{
  char* head;
  mkstr_(head, "POST /api/queue/%s/clear HTTP/1.1", queue_name);
  return send_command(sock, auth, NULL, head);
}


/**
 * Delete a queue
 * 
 * @param   sock        The socket used to remote communication
 * @param   auth        User authentication
 * @param   queue_name  The name of the queue
 * @return              Zero on success, -1 on error
 */
int libqwaitclient_qwait_delete_queue(_sock_, const _auth_, const char* restrict queue_name)
{
  char* head;
  mkstr_(head, "DELETE /api/queue/%s HTTP/1.1", queue_name);
  return send_command(sock, auth, NULL, head);
}


/**
 * Create a new queue
 * 
 * @param   sock         The socket used to remote communication
 * @param   auth         User authentication
 * @param   queue_title  The title of the new queue
 * @return               Zero on success, -1 on error
 */
int libqwaitclient_qwait_create_queue(_sock_, const _auth_, const char* restrict queue_title)
{
  libqwaitclient_json_t json;
  char* queue_name = NULL;
  int saved_errno, r = -1;
  char* head;
  t (make_json_object(&json, "title", queue_title));
  t (!(queue_name = make_queue_name(queue_title)));
  mkstr_(head, "PUT /api/queue/%s HTTP/1.1", queue_name);
  r = send_command(sock, auth, &json, head);
 fail:
  saved_errno = errno;
  free(queue_name);
  libqwaitclient_json_destroy(&json);
  return errno = saved_errno, r;
}


/**
 * Join or leave a queue
 * 
 * @param   sock        The socket used to remote communication
 * @param   auth        User authentication
 * @param   queue_name  The name of the queue
 * @param   user_id     The user ID of the user that should join or leave the queue
 * @param   wait        Whether the user should join the queue
 * @return              Zero on success, -1 on error
 */
int libqwaitclient_qwait_set_queue_wait(_sock_, const _auth_, const char* restrict queue_name,
					const char* restrict user_id, int wait)
{
  char* head;
  mkstr_(head, "%s /api/queue/%s/position/%s HTTP/1.1", wait ? "PUT" : "DELETE", queue_name, user_id);
  return send_command(sock, auth, NULL, head);
}


/**
 * Set or change the user's comment in a queue
 * 
 * @param   sock        The socket used to remote communication
 * @param   auth        User authentication
 * @param   queue_name  The name of the queue
 * @param   user_id     The user ID of the affected user
 * @param   comment     The comment for the entry
 * @return              Zero on success, -1 on error
 */
int libqwaitclient_qwait_set_queue_wait_comment(_sock_, const _auth_, const char* restrict queue_name,
						const char* restrict user_id, const char* restrict comment)
{
  char* head;
  libqwaitclient_json_t json;
  int saved_errno, r = -1;
  t (make_json_object(&json, "comment", comment));
  mkstr_(head, "PUT /api/queue/%s/position/%s/comment HTTP/1.1", queue_name, user_id);
  r = send_command(sock, auth, &json, head);
 fail:
  saved_errno = errno;
  libqwaitclient_json_destroy(&json);
  return errno = saved_errno, r;
}


/**
 * Set or change the user's announced location in a queue
 * 
 * @param   sock        The socket used to remote communication
 * @param   auth        User authentication
 * @param   queue_name  The name of the queue
 * @param   user_id     The user ID of the affected user
 * @param   location    The announced location for the entry
 * @return              Zero on success, -1 on error
 */
int libqwaitclient_qwait_set_queue_wait_location(_sock_, const _auth_, const char* restrict queue_name,
						 const char* restrict user_id, const char* restrict location)
{
  char* head;
  libqwaitclient_json_t json;
  int saved_errno, r = -1;
  t (make_json_object(&json, "location", location));
  mkstr_(head, "PUT /api/queue/%s/position/%s/location HTTP/1.1", queue_name, user_id);
  r = send_command(sock, auth, &json, head);
 fail:
  saved_errno = errno;
  libqwaitclient_json_destroy(&json);
  return errno = saved_errno, r;
}


/**
 * Add a user as a moderator of a queue or remove said status
 * 
 * @param   sock        The socket used to remote communication
 * @param   auth        User authentication
 * @param   queue_name  The name of the queue
 * @param   user_id     The user ID of the affected user
 * @param   moderator   Whether the user should be a moderator of the queue
 * @return              Zero on success, -1 on error
 */
int libqwaitclient_qwait_set_queue_moderator(_sock_, const _auth_, const char* restrict queue_name,
					     const char* restrict user_id, int moderator)
{
  char* head;
  mkstr_(head, "%s /api/queue/%s/moderator/%s HTTP/1.1", moderator ? "PUT" : "DELETE", queue_name, user_id);
  return send_command(sock, auth, NULL, head);
}


/**
 * Add a user as an owner of a queue or remove said status
 * 
 * @param   sock        The socket used to remote communication
 * @param   auth        User authentication
 * @param   queue_name  The name of the queue
 * @param   user_id     The user ID of the affected user
 * @param   owner       Whether the user should be an owner of the queue
 * @return              Zero on success, -1 on error
 */
int libqwaitclient_qwait_set_queue_owner(_sock_, const _auth_, const char* restrict queue_name,
					 const char* restrict user_id, int owner)
{
  char* head;
  mkstr_(head, "%s /api/queue/%s/owner/%s HTTP/1.1", owner ? "PUT" : "DELETE", queue_name, user_id);
  return send_command(sock, auth, NULL, head);
}


/**
 * Add a user as a QWait administrator or remove said status
 * 
 * @param   sock        The socket used to remote communication
 * @param   auth        User authentication
 * @param   user_id     The user ID of the affected user
 * @param   admin       Whether the user should be a QWait administrator
 * @return              Zero on success, -1 on error
 */
int libqwaitclient_qwait_set_admin(_sock_, const _auth_, const char* restrict user_id, int admin)
{
  char* head;
  libqwaitclient_json_t json;
  json.length = 0;
  json.type = LIBQWAITCLIENT_JSON_TYPE_BOOLEAN;
  json.data.boolean = admin;
  mkstr_(head, "PUT /api/user/%s/role/admin HTTP/1.1", user_id);
  return send_command(sock, auth, &json, head);
}



#undef mkstr
#undef mkstr_
#undef t


#undef _user_
#undef _queue_
#undef _json_
#undef _auth_
#undef _mesg_
#undef _sock_


/*
    Login information (partial content):  GET /
    
    If anonymous:
    
    <script type="text/javascript">
    \/\* <![CDATA[ \*\/
    angular.module('request', []).factory('requestInfo', [function () {
      return {
        currentUser: {
	  name: null,
	  readableName: null,
	  admin: false,
	  roles: ['ROLE_ANONYMOUS'],
	  anonymous: true
	},
	hostname: <reverse_dns (string)>,
	product: {
	  name: 'QWait',
	  version: '1.1.16'
	}
      };
    }]);
    \/\* ]]> \*\/
    </script>
    
    If not anonymous:
    
    <script type="text/javascript">
    \/\* <![CDATA[ \*\/
    angular.module('request', []).factory('requestInfo', [function () {
      return {
        currentUser: {
	  name: <user_id (string)>,
	  readableName: <real_name (string)>,
	  admin: <admin (boolean)>,
	  roles: <roles (string array)>,
	  anonymous: false
	},
	hostname: <reverse_dns (string)>,
	product: {
	  name: 'QWait',
	  version: '1.1.16'
	}
      };
    }]);
    \/\* ]]> \*\/
    </script>
    
    (product is for version 1.1.16 of QWait)
    (NB! this is data is not encoded with JSON,
         but in a similar manner: ' is used
	 instead of ", names in {} are not
	 strings but identifiers.)
 */

