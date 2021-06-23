#define INTRO "\n"                                                                             \
              "////////////////////////////////////////////////////////////////////////////\n" \
              "//                    Signal Registry                                     //\n" \
              "//      Instrument Client for Anritsu MS20XXC Series                      //\n" \
              "//             https://instr.signalregistry.net                           //\n" \
              "//                 Author: Huseyin YIGIT                                  //\n" \
              "//                    Version: 1.0.0                                      //\n" \
              "////////////////////////////////////////////////////////////////////////////\n"

#define DEBUG_LOG

#define LOCALHOST "localhost"
#define LOCALPORT 3001
#define HOST "instr.signalregistry.net"
#define PORT 80
#define STREAM_INTERVAL 1000 // milliseconds

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#pragma warning(disable : 4996)

////////////////////////////////////////////////////////////////////////////////
// Libraries
////////////////////////////////////////////////////////////////////////////////
#include "civetweb.h"
#include "jansson.h"

////////////////////////////////////////////////////////////////////////////////
// Instrument
////////////////////////////////////////////////////////////////////////////////
#include "instr.h"

////////////////////////////////////////////////////////////////////////////////
// Logging
////////////////////////////////////////////////////////////////////////////////
static inline void LOGGER(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
  fflush(stdout);
}

////////////////////////////////////////////////////////////////////////////////
// Helper arguments and functions
////////////////////////////////////////////////////////////////////////////////
static int CLOSENOW = 0;
static int EXITNOW = 0;
static int RETRY_COUNT = 50;

enum CLIENT_MSG_TYPE
{
  CLIENT_MSG_BLANK,
  CLIENT_DEV_CONF,
};

/* Helper function to get a printable name for websocket opcodes */
static const char *
msgtypename(int flags)
{
  unsigned f = (unsigned)flags & 0xFu;
  switch (f)
  {
  case MG_WEBSOCKET_OPCODE_CONTINUATION:
    return "continuation";
  case MG_WEBSOCKET_OPCODE_TEXT:
    return "text";
  case MG_WEBSOCKET_OPCODE_BINARY:
    return "binary";
  case MG_WEBSOCKET_OPCODE_CONNECTION_CLOSE:
    return "connection close";
  case MG_WEBSOCKET_OPCODE_PING:
    return "PING";
  case MG_WEBSOCKET_OPCODE_PONG:
    return "PONG";
  }
  return "unknown";
}

/* Callback for handling data received from the server */
static int
websocket_client_data_handler(struct mg_connection *conn,
                              int flags,
                              char *data,
                              size_t data_len,
                              void *user_data)
{
  time_t now = time(NULL);

  /* We may get some different message types (websocket opcodes).
	 * We will handle these messages differently. */
  int is_text = ((flags & 0xf) == MG_WEBSOCKET_OPCODE_TEXT);
  int is_bin = ((flags & 0xf) == MG_WEBSOCKET_OPCODE_BINARY);
  int is_ping = ((flags & 0xf) == MG_WEBSOCKET_OPCODE_PING);
  int is_pong = ((flags & 0xf) == MG_WEBSOCKET_OPCODE_PONG);
  int is_close = ((flags & 0xf) == MG_WEBSOCKET_OPCODE_CONNECTION_CLOSE);

  /* Check if we got a websocket PING request */
  if (is_ping)
  {
    /* PING requests are to check if the connection is broken.
		 * They should be replied with a PONG with the same data.
		 */
    mg_websocket_client_write(conn,
                              MG_WEBSOCKET_OPCODE_PONG,
                              data,
                              data_len);
    return 1;
  }

  /* It we got a websocket TEXT message, handle it ... */
  if (is_text)
  {
    char *buff = 0;
    json_error_t *err_buff = NULL;
    char *msg = (char *)malloc(sizeof(char) * (data_len + 1));
    snprintf(msg, data_len + 1, "%s", data);
    json_t *obj = json_loads(msg, 0, err_buff);
    LOGGER("[DEBUG] Message : %s\n", msg);
    if (json_object_get(obj, "server") != NULL)
    {
      json_object_del(obj, "server");
      json_decref(obj);
    }
    else if (json_object_get(obj, "client") != NULL)
    {
      if (json_integer_value(json_object_get(obj, "client")) == CLIENT_DEV_CONF)
      {
        json_object_del(obj, "client");
        // instr_add_query(obj);
      }
    }
    free(msg);
  }

  /* It could be a CLOSE message as well. */
  if (is_close)
  {
    LOGGER("Got close signal\n");
    return 0;
  }

  /* Return 1 to keep the connection open */
  return 1;
}

/* Callback for handling a close message received from the server */
static void
websocket_client_close_handler(const struct mg_connection *conn,
                               void *user_data)
{
  CLOSENOW = 1;
  LOGGER("[INFO] Disconnected.\n");
}

/* Websocket client test function */
int run_websocket_client(const char *host,
                         int port,
                         int secure,
                         const char *path,
                         const char *greetings)
{
  struct mg_connection *conn = NULL;
  char *buff = NULL, err_buf[100] = {0};
  json_t *obj = json_object();

  /* INSTR_CONN_READY */
  /* First seek for local installed instrument server */
  LOGGER("[INFO] Searching for local server ... ");
  // conn = mg_connect_websocket_client(LOCALHOST,
  //                                    LOCALPORT,
  //                                    secure,
  //                                    err_buf,
  //                                    sizeof(err_buf),
  //                                    path,
  //                                    NULL,
  //                                    websocket_client_data_handler,
  //                                    websocket_client_close_handler,
  //                                    NULL);
  if (conn == NULL)
  {
    LOGGER("Not found.\n");
    /* Connect to the given WS or WSS (WS secure) server */
    LOGGER("[INFO] Connecting to remote server %s ... ", host);
    conn = mg_connect_websocket_client(host,
                                       port,
                                       secure,
                                       err_buf,
                                       sizeof(err_buf),
                                       path,
                                       NULL,
                                       websocket_client_data_handler,
                                       websocket_client_close_handler,
                                       NULL);
  }
  if (conn == NULL)
  {
    LOGGER("\n[ERROR] Connection could not be established. (%s)\n", err_buf);
    CLOSENOW = 1;
    return 0;
  }
  LOGGER("OKAY.\n");
  CLOSENOW = 0;
  json_object_set(obj, "instr", json_integer(INSTR_CONN_READY));
  buff = json_dumps(obj, JSON_COMPACT);
  mg_websocket_client_write(conn, MG_WEBSOCKET_OPCODE_TEXT, buff, strlen(buff) + 1);
  json_object_clear(obj);
  free(buff);

  /* INSTR_DEV_READY */
  if (!instr_connect(obj))
  {
    LOGGER("[ERROR] %s\n", json_string_value(json_object_get(obj, "err")));
    EXITNOW = CLOSENOW = 1;
    return 0;
  }
  buff = json_dumps(obj, JSON_COMPACT);
  mg_websocket_client_write(conn, MG_WEBSOCKET_OPCODE_TEXT, buff, strlen(buff) + 1);
  json_object_clear(obj);
  free(buff);

  /* INSTR_DEV_INFO */
  if (!instr_info(obj))
  {
    LOGGER("[ERROR] %s\n", json_string_value(json_object_get(obj, "err")));
    EXITNOW = CLOSENOW = 1;
    return 0;
  }
  buff = json_dumps(obj, JSON_COMPACT);
  mg_websocket_client_write(conn, MG_WEBSOCKET_OPCODE_TEXT, buff, strlen(buff) + 1);
  json_object_clear(obj);
  free(buff);

  LOGGER("[INFO] Waiting for UI initialization ...");
  Sleep(1000);
  LOGGER("OKAY.\n");

  LOGGER("[INFO] Started streaming.\n");
  int counter = 0;
  while (CLOSENOW < 1)
  {
    /* INSTR_DEV_CONF */
    if (!instr_conf(obj))
    {
      LOGGER("[ERROR] %s\n", json_string_value(json_object_get(obj, "err")));
      CLOSENOW = 1;
      return 0;
    }
    buff = json_dumps(obj, JSON_COMPACT);
    mg_websocket_client_write(conn, MG_WEBSOCKET_OPCODE_TEXT, buff, strlen(buff) + 1);
    json_object_clear(obj);
    free(buff);

    // /* INSTR_DEV_DATA */
    // if (!instr_data(obj))
    // {
    //   LOGGER("[ERROR] %s\n", json_string_value(json_object_get(obj, "err")));
    //   CLOSENOW = 1;
    //   return 0;
    // }
    // buff = json_dumps(obj, JSON_COMPACT);
    // mg_websocket_client_write(conn, MG_WEBSOCKET_OPCODE_TEXT, buff, strlen(buff) + 1);
    // json_object_clear(obj);
    // free(buff);

    // LOGGER("[INFO] Stream count: %d", ++counter);
    Sleep(STREAM_INTERVAL);
  }

  // instr_disconnect();
  json_decref(obj);
  Sleep(3000);
  CLOSENOW = 1;
  mg_close_connection(conn);
  return 1;
}

/* main will initialize the CivetWeb library
 * and start the WebSocket client test function */
int main(int argc, char *argv[])
{
  char path[100] = {'\0'};
  char cid[10] = {'\0'};

  LOGGER(INTRO);

  LOGGER("[INFO] Initializing websocket framework ... ");
  if (mg_init_library(0))
  {
    LOGGER("\n[ERROR] Cannot start websocket framework.\n");
    return EXIT_FAILURE;
  }
  LOGGER("OKAY.\n");

  int counter = 0;
  while (!EXITNOW)
  {
    if (counter % RETRY_COUNT == 0)
    {
      LOGGER("Enter cid code: ");
      scanf("%s", &cid);
      sprintf(path, "/websocket?_signalregistry=%s&side=2", cid);
    }

    run_websocket_client(HOST, PORT, 0, path, NULL);
    while (!CLOSENOW)
      Sleep(1000);

    counter++;
    if (counter % RETRY_COUNT != 0)
      LOGGER("[INFO] Retry connecting ...\n");
    Sleep(500);
  }
}