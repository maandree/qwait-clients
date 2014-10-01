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
#define _GNU_SOURCE
#include "qwait-protocol.h"

#include "macros.h"
#include "json.h"

#include <errno.h>
#include <string.h>
#include <stdio.h>


#define _sock_   libqwaitclient_http_socket_t*  restrict sock
#define _queue_  libqwaitclient_qwait_queue_t*  restrict queue


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
  
  libqwaitclient_http_message_zero_initialise(&mesg);
  t (libqwaitclient_http_message_extend_headers(&mesg, mesg.header_count = 1) < 0);
  t (xmalloc(mesg.headers[0], strlen("Host: %s") + strlen(sock->host), char));
  sprintf(mesg.headers[0], "Host: %s", sock->host);
  t (xmalloc(mesg.top, strlen("GET /api/queue/%s HTTP/1.1") + strlen(queue_name), char));
  sprintf(mesg.top, "GET /api/queue/%s HTTP/1.1", queue_name);
  
  t (libqwaitclient_http_socket_send(sock, &mesg));
  t (libqwaitclient_http_socket_receive(sock));
  t (libqwaitclient_json_parse(&json, sock->message.content, sock->message.content_size));
  libqwaitclient_qwait_queue_initialise(queue);
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


#undef _queue_
#undef _sock_

