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


#define _sock_   libqwaitclient_http_socket_t* restrict sock
#define _queue_  libqwaitclient_qwait_queue_t* restrict queue
#define _user_   libqwaitclient_qwait_user_t*  restrict user


#define  t(expression)   if (expression)  goto fail


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
  int saved_errno;
  size_t i, n;
  
  memset(&json, 0, sizeof(libqwaitclient_json_t));
  
  libqwaitclient_http_message_zero_initialise(&mesg);
  t (libqwaitclient_http_message_extend_headers(&mesg, mesg.header_count = 1) < 0);
  t (xmalloc(mesg.headers[0], 7 + strlen(sock->host), char));
  sprintf(mesg.headers[0], "Host: %s", sock->host);
  t ((mesg.top = strdup("GET /api/queues HTTP/1.1")) == NULL);
  
  t (libqwaitclient_http_socket_send(sock, &mesg));
  t (libqwaitclient_http_socket_receive(sock));
  t (libqwaitclient_json_parse(&json, sock->message.content, sock->message.content_size));
  
  if (json.type != LIBQWAITCLIENT_JSON_TYPE_ARRAY)
    {
      errno = EBADMSG;
      goto fail;
    }
  n = json.length;
  t (xcalloc(rc, max(n, 1), libqwaitclient_qwait_queue_t)); /* `max(n, 1)`: do not return `NULL`. */
  for (i = 0; i < n; i++)
    t (libqwaitclient_qwait_queue_parse(rc + i, json.data.array + i));
  
  libqwaitclient_json_destroy(&json);
  libqwaitclient_http_message_destroy(&mesg);
  return *queue_count = n, rc;
  
 fail:
  saved_errno = errno;
#ifdef DEBUG
  fprintf(stderr, "=============================================\n");
  fprintf(stderr, "RECIEVED MESSAGE:\n");
  fprintf(stderr, "---------------------------------------------\n");
  libqwaitclient_http_message_dump(&mesg, stderr, 0);
  fprintf(stderr, "---------------------------------------------\n");
  libqwaitclient_json_dump(&json, stderr);
  fprintf(stderr, "=============================================\n");
#endif
  libqwaitclient_json_destroy(&json);
  libqwaitclient_http_message_destroy(&mesg);
  if (saved_errno == EINVAL)
    saved_errno = EBADMSG;
  return *queue_count = 0, errno = saved_errno, NULL;
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
  int saved_errno;
  
  memset(&json, 0, sizeof(libqwaitclient_json_t));
  libqwaitclient_qwait_queue_initialise(queue);
  
  libqwaitclient_http_message_zero_initialise(&mesg);
  t (libqwaitclient_http_message_extend_headers(&mesg, mesg.header_count = 1) < 0);
  t (xmalloc(mesg.headers[0], strlen("Host: %s") + strlen(sock->host), char));
  sprintf(mesg.headers[0], "Host: %s", sock->host);
  t (xmalloc(mesg.top, strlen("GET /api/queue/%s HTTP/1.1") + strlen(queue_name), char));
  sprintf(mesg.top, "GET /api/queue/%s HTTP/1.1", queue_name);
  
  t (libqwaitclient_http_socket_send(sock, &mesg));
  t (libqwaitclient_http_socket_receive(sock));
  t (libqwaitclient_json_parse(&json, sock->message.content, sock->message.content_size));
  t (libqwaitclient_qwait_queue_parse(queue, &json));
  
  libqwaitclient_json_destroy(&json);
  libqwaitclient_http_message_destroy(&mesg);
  return 0;
  
 fail:
  saved_errno = errno;
#ifdef DEBUG
  fprintf(stderr, "=============================================\n");
  fprintf(stderr, "RECIEVED MESSAGE:\n");
  fprintf(stderr, "---------------------------------------------\n");
  libqwaitclient_http_message_dump(&mesg, stderr, 0);
  fprintf(stderr, "---------------------------------------------\n");
  libqwaitclient_json_dump(&json, stderr);
  fprintf(stderr, "=============================================\n");
#endif
  libqwaitclient_json_destroy(&json);
  libqwaitclient_http_message_destroy(&mesg);
  if (saved_errno == EINVAL)
    saved_errno = EBADMSG;
  return errno = saved_errno, -1;
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
  int saved_errno;
  
  memset(&json, 0, sizeof(libqwaitclient_json_t));
  libqwaitclient_qwait_user_initialise(user);
  
  libqwaitclient_http_message_zero_initialise(&mesg);
  t (libqwaitclient_http_message_extend_headers(&mesg, mesg.header_count = 1) < 0);
  t (xmalloc(mesg.headers[0], strlen("Host: %s") + strlen(sock->host), char));
  sprintf(mesg.headers[0], "Host: %s", sock->host);
  t (xmalloc(mesg.top, strlen("GET /api/user/%s HTTP/1.1") + strlen(user_id), char));
  sprintf(mesg.top, "GET /api/user/%s HTTP/1.1", user_id);
  
  t (libqwaitclient_http_socket_send(sock, &mesg));
  t (libqwaitclient_http_socket_receive(sock));
  t (libqwaitclient_json_parse(&json, sock->message.content, sock->message.content_size));
  t (libqwaitclient_qwait_user_parse(user, &json));
  
  libqwaitclient_json_destroy(&json);
  libqwaitclient_http_message_destroy(&mesg);
  return 0;
  
 fail:
  saved_errno = errno;
#ifdef DEBUG
  fprintf(stderr, "=============================================\n");
  fprintf(stderr, "RECIEVED MESSAGE:\n");
  fprintf(stderr, "---------------------------------------------\n");
  libqwaitclient_http_message_dump(&mesg, stderr, 0);
  fprintf(stderr, "---------------------------------------------\n");
  libqwaitclient_json_dump(&json, stderr);
  fprintf(stderr, "=============================================\n");
#endif
  libqwaitclient_json_destroy(&json);
  libqwaitclient_http_message_destroy(&mesg);
  if (saved_errno == EINVAL)
    saved_errno = EBADMSG;
  return errno = saved_errno, -1;
}


#undef t


#undef _user_
#undef _queue_
#undef _sock_


/*  Other protocol parts:
    
     join queue:                PUT /api/queue/<queue.name>/position/<user.user_id>
    leave queue:             DELETE /api/queue/<queue.name>/position/<user.user_id>
    change comment:             PUT /api/queue/<queue.name>/position/<user.user_id>/comment
                                    Content-Type: application/json
                                    {"comment":<comment>}
    change location:            PUT /api/queue/<queue.name>/position/<user.user_id>/location
                                    Content-Type: application/json
                                    {"location":<location>}
    
    delete queue:            DELETE /api/queue/<queue.name>
     clear queue:              POST /api/queue/<queue.name>/clear
    create queue:               PUT /api/queue/<queue.name>
                                    {"title":<queue.title>}
      hide queue:               PUT /api/queue/<queue.name>/hidden
                                    Content-Type: application/json
                                    true
    unhide queue:               PUT /api/queue/<queue.name>/hidden
                                    Content-Type: application/json
                                    false
      lock queue:               PUT /api/queue/<queue.name>/locked
                                    Content-Type: application/json
                                    true
    unlock queue:               PUT /api/queue/<queue.name>/locked
                                    Content-Type: application/json
                                    false
   
       add queue moderator:     PUT /api/queue/<queue.name>/moderator/<user.user_id>
    remove queue moderator:  DELETE /api/queue/<queue.name>/moderator/<user.user_id>
       add queue owner:         PUT /api/queue/<queue.name>/owner/<user.user_id>
    remove queue owner:      DELETE /api/queue/<queue.name>/owner/<user.user_id>

queue.title="$(echo "${queue.name,,}" | sed -r -e 's:[ \f\n\r\t\v\u00a0\u1680\u180e\u2000\u2001\u2002\u2003\u2004\u2005\u2006\u2007\u2008\u2009\u200a\u2028\u2029\u202f\u205f\u3000/]+:-:g')"
 */

