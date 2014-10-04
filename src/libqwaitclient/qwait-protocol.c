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


#define _sock_   libqwaitclient_http_socket_t*  restrict sock
#define _mesg_   libqwaitclient_http_message_t* restrict mesg
#define _json_   libqwaitclient_json_t*         restrict json
#define _queue_  libqwaitclient_qwait_queue_t*  restrict queue
#define _user_   libqwaitclient_qwait_user_t*   restrict user


#define t(expression)    if (expression)  goto fail
#define mkstr(buf, ...)  asprintf(&(buf), __VA_ARGS__)



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



#undef mkstr
#undef t


#undef _user_
#undef _queue_
#undef _json_
#undef _mesg_
#undef _sock_


/*  Other protocol parts:
    
     join queue:                PUT /api/queue/<queue.name>/position/<user.user_id>
    leave queue:             DELETE /api/queue/<queue.name>/position/<user.user_id>
    change comment:             PUT /api/queue/<queue.name>/position/<user.user_id>/comment
                                    {"comment":<comment>}
    change location:            PUT /api/queue/<queue.name>/position/<user.user_id>/location
                                    {"location":<location>}
    
    delete queue:            DELETE /api/queue/<queue.name>
     clear queue:              POST /api/queue/<queue.name>/clear
    create queue:               PUT /api/queue/<queue.name>
                                    {"title":<queue.title>}
      hide queue:               PUT /api/queue/<queue.name>/hidden
                                    true
    unhide queue:               PUT /api/queue/<queue.name>/hidden
                                    false
      lock queue:               PUT /api/queue/<queue.name>/locked
                                    true
    unlock queue:               PUT /api/queue/<queue.name>/locked
                                    false
   
       add queue moderator:     PUT /api/queue/<queue.name>/moderator/<user.user_id>
    remove queue moderator:  DELETE /api/queue/<queue.name>/moderator/<user.user_id>
       add queue owner:         PUT /api/queue/<queue.name>/owner/<user.user_id>
    remove queue owner:      DELETE /api/queue/<queue.name>/owner/<user.user_id>

queue.title="$(echo "${queue.name,,}" | sed -r -e 's:[ \f\n\r\t\v\u00a0\u1680\u180e\u2000\u2001\u2002\u2003\u2004\u2005\u2006\u2007\u2008\u2009\u200a\u2028\u2029\u202f\u205f\u3000/]+:-:g')"
 */

