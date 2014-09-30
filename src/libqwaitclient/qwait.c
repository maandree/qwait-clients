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
#include "qwait.h"

#include "macros.h"
#include "config.h"
#include "http-socket.h"
#include "http-message.h"
#include "json.h"

#include <string.h>
#include <stdio.h>


#define  t(expression)   if (expression)  goto fail


int main(int argc, char** argv)
{
  libqwaitclient_http_socket_t sock;
  libqwaitclient_http_message_t msg;
  libqwaitclient_json_t json;
  
  (void) argc;
  
  memset(&json, 0, sizeof(libqwaitclient_json_t));
  libqwaitclient_http_message_zero_initialise(&msg);
  t (libqwaitclient_http_socket_initialise(&sock, QWAIT_SERVER_HOST, QWAIT_SERVER_PORT));
  
  t (libqwaitclient_http_message_extend_headers(&msg, 1));
  t ((msg.headers[msg.header_count++] = strdup("Host: " QWAIT_SERVER_HOST)) == NULL);
  t ((msg.top = strdup("GET /api/queues HTTP/1.1")) == NULL);
  
  t (libqwaitclient_http_socket_connect(&sock));
  t (libqwaitclient_http_socket_send(&sock, &msg));
  t (libqwaitclient_http_socket_receive(&sock));
  
  printf("%.*s\n\n", (int)(sock.message.content_size), sock.message.content);
  
  t (libqwaitclient_json_parse(&json, sock.message.content, sock.message.content_size));
  
  libqwaitclient_json_dump(&json, stdout);
  
  libqwaitclient_http_socket_disconnect(&sock);
  libqwaitclient_http_socket_destroy(&sock);
  libqwaitclient_http_message_destroy(&msg);
  libqwaitclient_json_destroy(&json);
  return 0;
  
 fail:
  perror(*argv);
  libqwaitclient_http_socket_disconnect(&sock);
  libqwaitclient_http_socket_destroy(&sock);
  libqwaitclient_http_message_destroy(&msg);
  libqwaitclient_json_destroy(&json);
  return 1;
}

